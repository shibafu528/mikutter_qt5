#include <QApplication>
#include <QEventLoop>
#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>
#include <type_traits>

#include "mikutter.hpp"
#include "ui/MikutterWindow.hpp"
#include "ui/Timeline.hpp"
#include "utils.hpp"

QApplication *app;
QMainWindow *main_window;

VALUE dispatch_queue;
VALUE dispatch_table;
uint64_t dispatch_table_next = 0;
VALUE widget_hash;

static int PSEUDO_ARGC = 1;
static char *PSEUDO_ARGV[1];

const rb_data_type_t QWidgetWrapper = {
    "QWidgetWrapper", {nullptr, nullptr, nullptr}, nullptr, nullptr, RUBY_TYPED_FREE_IMMEDIATELY,
};

static inline VALUE wrap_widget(QWidget *widget) { return TypedData_Wrap_Struct(rb_cData, &QWidgetWrapper, widget); }

template <typename T>
static T unwrap_widget(VALUE widget_wrapper) {
  if (RB_NIL_P(widget_wrapper)) {
    return nullptr;
  } else {
    T widget;
    TypedData_Get_Struct(widget_wrapper, std::remove_pointer_t<T>, &QWidgetWrapper, widget);
    return widget;
  }
}

template <typename T>
static T widget_hash_lookup(VALUE imaginary) {
  VALUE val = rb_hash_aref(widget_hash, imaginary);
  return unwrap_widget<T>(val);
}

static void widget_join_tab(VALUE i_tab, QWidget *widget) {
  auto tab = widget_hash_lookup<QWidget *>(i_tab);
  if (tab == nullptr) {
    return;
  }

  VALUE i_pane = rb_funcall3(i_tab, rb_intern("parent"), 0, nullptr);
  if (RB_NIL_P(i_pane)) {
    return;
  }

  auto pane = widget_hash_lookup<QTabWidget *>(i_pane);
  if (pane == nullptr) {
    return;
  }

  tab->layout()->addWidget(widget);

  for (int i = 0; i < pane->count(); i++) {
    if (pane->widget(i) == tab) {
      return;
    }
  }

  VALUE tab_name = rb_funcall3(i_tab, rb_intern("name"), 0, nullptr);
  pane->addTab(tab, StringValuePtr(tab_name));
}

static VALUE qt5_init(VALUE self, VALUE plugin) {
  VALUE plugin_slug = rb_funcall(plugin, rb_intern("name"), 0);
  plugin_slug = rb_funcall(plugin_slug, rb_intern("to_s"), 0);
  mqt_log("qt5_init() plugin slug=%s", StringValuePtr(plugin_slug));

  mikutter_plugin_add_event_listener(plugin, "window_created",
                                     [](RB_BLOCK_CALL_FUNC_ARGLIST(i_window, callback_arg)) -> VALUE {
                                       mqt_log("on_window_created");

                                       main_window = new MikutterWindow();

                                       auto name = rb_funcall3(i_window, rb_intern("name"), 0, nullptr);
                                       main_window->setWindowTitle(StringValuePtr(name));

                                       main_window->show();

                                       rb_hash_aset(widget_hash, i_window, wrap_widget(main_window));
                                       return Qnil;
                                     });

  mikutter_plugin_add_event_listener(plugin, "pane_created",
                                     [](RB_BLOCK_CALL_FUNC_ARGLIST(i_pane, callback_arg)) -> VALUE {
                                       mqt_log("on_pane_created");

                                       auto pane = new QTabWidget();
                                       rb_hash_aset(widget_hash, i_pane, wrap_widget(pane));
                                       return Qnil;
                                     });

  mikutter_plugin_add_event_listener(plugin, "tab_created",
                                     [](RB_BLOCK_CALL_FUNC_ARGLIST(i_tab, callback_arg)) -> VALUE {
                                       mqt_log("on_tab_created");

                                       VALUE tab_name = rb_funcall3(i_tab, rb_intern("name"), 0, nullptr);

                                       auto tab = new QWidget();
                                       tab->setObjectName(StringValuePtr(tab_name));

                                       auto layout = new QVBoxLayout(tab);
                                       tab->setLayout(layout);

                                       rb_hash_aset(widget_hash, i_tab, wrap_widget(tab));
                                       return Qnil;
                                     });

  mikutter_plugin_add_event_listener(plugin, "timeline_created",
                                     [](RB_BLOCK_CALL_FUNC_ARGLIST(i_timeline, callback_arg)) -> VALUE {
                                       mqt_log("on_timeline_created");

                                       VALUE tab_name = rb_funcall3(i_timeline, rb_intern("name"), 0, nullptr);

                                       auto timeline = new Timeline(i_timeline);
                                       timeline->setObjectName(StringValuePtr(tab_name));

                                       rb_hash_aset(widget_hash, i_timeline, wrap_widget(timeline));
                                       return Qnil;
                                     });

  mikutter_plugin_add_event_listener(plugin, "gui_pane_join_window",
                                     [](RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) -> VALUE {
                                       mqt_log("on_gui_pane_join_window");

                                       rb_check_arity(argc, 2, UNLIMITED_ARGUMENTS);
                                       VALUE i_pane = argv[0];
                                       VALUE i_window = argv[1];

                                       auto window = widget_hash_lookup<QMainWindow *>(i_window);
                                       auto pane = widget_hash_lookup<QTabWidget *>(i_pane);

                                       if (pane->parent() == nullptr) {
                                         // attach
                                         window->centralWidget()->layout()->addWidget(pane);
                                       } else {
                                         // reattach
                                         qobject_cast<QLayout *>(pane->parent())->removeWidget(pane);
                                         window->centralWidget()->layout()->addWidget(pane);
                                       }

                                       return Qnil;
                                     });

  mikutter_plugin_add_event_listener(
      plugin, "gui_tab_join_pane", [](RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) -> VALUE {
        mqt_log("on_gui_tab_join_pane");

        rb_check_arity(argc, 2, UNLIMITED_ARGUMENTS);
        VALUE i_tab = argv[0];
        VALUE i_pane = argv[1];

        VALUE i_widget =
            rb_funcall3(rb_funcall3(i_tab, rb_intern("children"), 0, nullptr), rb_intern("first"), 0, nullptr);
        if (RB_NIL_P(i_widget)) {
          return Qnil;
        }

        auto widget = widget_hash_lookup<QWidget *>(i_widget);
        if (widget == nullptr) {
          return Qnil;
        }

        auto tab = widget_hash_lookup<QWidget *>(i_tab);
        auto pane = widget_hash_lookup<QTabWidget *>(i_pane);

        auto old_pane = widget->parentWidget();
        while (old_pane != nullptr && qobject_cast<QTabWidget *>(old_pane) != nullptr) {
          old_pane = old_pane->parentWidget();
        }

        if (tab != nullptr && pane != nullptr && old_pane != nullptr && pane != old_pane) {
          // TODO: reparent
          //      if (tab->parentWidget() != nullptr) {
          //
          //      }
        }

        return Qnil;
      });

  mikutter_plugin_add_event_listener(plugin, "gui_timeline_join_tab",
                                     [](RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) -> VALUE {
                                       mqt_log("on_gui_timeline_join_tab");

                                       rb_check_arity(argc, 2, UNLIMITED_ARGUMENTS);
                                       VALUE i_timeline = argv[0];
                                       VALUE i_tab = argv[1];

                                       auto widget = widget_hash_lookup<QWidget *>(i_timeline);
                                       if (widget != nullptr) {
                                         widget_join_tab(i_tab, widget);
                                       }

                                       return Qnil;
                                     });

  mikutter_plugin_add_event_listener(
      plugin, "gui_timeline_add_messages", [](RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) -> VALUE {
                                       mqt_log("on_gui_timeline_add_messages");

                                       rb_check_arity(argc, 2, UNLIMITED_ARGUMENTS);
                                       VALUE i_timeline = argv[0];
                                       VALUE messages = argv[1];

                                       auto timeline = widget_hash_lookup<Timeline *>(i_timeline);
                                       if (timeline != nullptr) {
                                         timeline->add(messages);
                                       }

                                       return Qnil;
                                     });

  // TODO: ヤバすぎるので何とかする
  PSEUDO_ARGV[0] = "ruby";
  app = new QApplication(PSEUDO_ARGC, (char **)PSEUDO_ARGV);

  return Qnil;
}

static VALUE qt5_mainloop(int argc, VALUE *argv, VALUE self) {
  VALUE deadline;
  rb_scan_args(argc, argv, "01", &deadline);

  if (RB_TEST(deadline)) {
    deadline = rb_to_int(deadline);
    auto c_deadline = NUM2LL(deadline);
    app->processEvents(QEventLoop::AllEvents, c_deadline);
  } else {
    // The interrupter
    QTimer allen;
    allen.setInterval(250);
    QObject::connect(&allen, &QTimer::timeout, []() { rb_thread_check_ints(); });
    allen.start();

    app->exec();
  }
  return Qnil;
}

static VALUE qt5_enqueue(VALUE self) {
  if (!rb_block_given_p()) {
    rb_raise(rb_eArgError, "Expected block");
  }
  rb_ary_push(dispatch_queue, rb_block_proc());
  mqt_post_to_main_thread(0, []() {
    VALUE proc = Qnil;
    while (RB_TEST(proc = rb_ary_shift(dispatch_queue))) {
      rb_funcall(proc, rb_intern("call"), 0);
    }
  });
  return Qnil;
}

static VALUE qt5_enqueue_delayed(VALUE self, VALUE delay) {
  if (!rb_block_given_p()) {
    rb_raise(rb_eArgError, "Expected block");
  }
  auto ldelay = FIX2INT(rb_funcall3(delay, rb_intern("to_i"), 0, nullptr));  // TODO: size check
  auto table_index = dispatch_table_next++;
  rb_hash_aset(dispatch_table, ULONG2NUM(table_index), rb_block_proc());
  mqt_post_to_main_thread(ldelay * 1000 + 1500, [table_index]() {
    VALUE idx = ULONG2NUM(table_index);
    VALUE proc = rb_hash_aref(dispatch_table, idx);
    if (RB_TEST(proc)) {
      rb_funcall(proc, rb_intern("call"), 0);
      rb_hash_delete(proc, idx);
    }
  });
  return Qnil;
}

extern "C" void Init_mikutter_qt5_ext() {
  mqt_log("Init_mikutter_qt5_ext()");

  dispatch_queue = rb_ary_new();
  rb_global_variable(&dispatch_queue);

  dispatch_table = rb_hash_new();
  rb_global_variable(&dispatch_table);

  widget_hash = rb_hash_new();
  rb_global_variable(&widget_hash);

  VALUE mod_plugin = rb_const_get(rb_cObject, rb_intern("Plugin"));
  VALUE mod_cplugin = rb_define_module_under(mod_plugin, "Qt5");
  rb_define_module_function(mod_cplugin, "init", RUBY_METHOD_FUNC(qt5_init), 1);
  rb_define_module_function(mod_cplugin, "mainloop", RUBY_METHOD_FUNC(qt5_mainloop), -1);
  rb_define_module_function(mod_cplugin, "enqueue", RUBY_METHOD_FUNC(qt5_enqueue), 0);
  rb_define_module_function(mod_cplugin, "enqueue_delayed", RUBY_METHOD_FUNC(qt5_enqueue_delayed), 1);
}
