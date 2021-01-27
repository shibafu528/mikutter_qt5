#include "mikutter.hpp"

#include <stdio.h>
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QEventLoop>

QApplication *app;
QMainWindow *main_window;

VALUE dispatch_queue;
VALUE dispatch_table;
uint64_t dispatch_table_next = 0;
VALUE widget_hash;

static int PSEUDO_ARGC = 1;
static char *PSEUDO_ARGV[1];

class MikutterWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MikutterWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
    setWindowTitle("mikutter");

    auto central = new QWidget(this);
    central->setLayout(new QHBoxLayout(central));

    setCentralWidget(central);
  }
};

template <typename T>
static T widget_hash_lookup(VALUE imaginary)
{
  VALUE val = rb_hash_aref(widget_hash, imaginary);
  if (RB_NIL_P(val)) {
    return nullptr;
  } else {
    return reinterpret_cast<T>(val);
  }
}

static void widget_join_tab(VALUE i_tab, QWidget *widget)
{
  auto tab = widget_hash_lookup<QWidget*>(i_tab);
  if (tab == nullptr) {
    return;
  }

  VALUE i_pane = rb_funcall3(i_tab, rb_intern("parent"), 0, nullptr);
  if (RB_NIL_P(i_pane)) {
    return;
  }

  auto pane = widget_hash_lookup<QTabWidget*>(i_pane);
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

VALUE timeline_add_message_i(RB_BLOCK_CALL_FUNC_ARGLIST(message, rb_timeline))
{
  auto timeline = reinterpret_cast<QListWidget*>(rb_timeline);

  VALUE desc = rb_funcall3(message, rb_intern("description"), 0, nullptr);
  timeline->addItem(StringValuePtr(desc));

  return Qnil;
}

static VALUE cplugin_init(VALUE self, VALUE plugin)
{
  VALUE plugin_slug = rb_funcall(plugin, rb_intern("name"), 0);
  plugin_slug = rb_funcall(plugin_slug, rb_intern("to_s"), 0);
  fprintf(stderr, "[mikutter_qt5_ext] cplugin_init() plugin slug=%s \n", StringValuePtr(plugin_slug));

  mikutter_plugin_add_event_listener(plugin, "window_created", [](RB_BLOCK_CALL_FUNC_ARGLIST(i_window, callback_arg)) -> VALUE {
    fprintf(stderr, "[mikutter_qt5_ext] on_window_created\n");

    main_window = new MikutterWindow();

    auto name = rb_funcall3(i_window, rb_intern("name"), 0, nullptr);
    main_window->setWindowTitle(StringValuePtr(name));

    main_window->show();

    rb_hash_aset(widget_hash, i_window, reinterpret_cast<VALUE>(main_window));
    return Qnil;
  });

  mikutter_plugin_add_event_listener(plugin, "pane_created", [](RB_BLOCK_CALL_FUNC_ARGLIST(i_pane, callback_arg)) -> VALUE {
    fprintf(stderr, "[mikutter_qt5_ext] on_pane_created\n");

    auto pane = new QTabWidget();
    rb_hash_aset(widget_hash, i_pane, reinterpret_cast<VALUE>(pane));
    return Qnil;
  });

  mikutter_plugin_add_event_listener(plugin, "tab_created", [](RB_BLOCK_CALL_FUNC_ARGLIST(i_tab, callback_arg)) -> VALUE {
    fprintf(stderr, "[mikutter_qt5_ext] on_tab_created\n");

    VALUE tab_name = rb_funcall3(i_tab, rb_intern("name"), 0, nullptr);

    auto tab = new QWidget();
    tab->setObjectName(StringValuePtr(tab_name));

    auto layout = new QVBoxLayout(tab);
    tab->setLayout(layout);

    rb_hash_aset(widget_hash, i_tab, reinterpret_cast<VALUE>(tab));
    return Qnil;
  });

  mikutter_plugin_add_event_listener(plugin, "timeline_created", [](RB_BLOCK_CALL_FUNC_ARGLIST(i_timeline, callback_arg)) -> VALUE {
    fprintf(stderr, "[mikutter_qt5_ext] on_timeline_created\n");

    VALUE tab_name = rb_funcall3(i_timeline, rb_intern("name"), 0, nullptr);

    auto list = new QListWidget();
    list->setObjectName(StringValuePtr(tab_name));

    rb_hash_aset(widget_hash, i_timeline, reinterpret_cast<VALUE>(list));
    return Qnil;
  });

  mikutter_plugin_add_event_listener(plugin, "gui_pane_join_window", [](RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) -> VALUE {
    fprintf(stderr, "[mikutter_qt5_ext] on_gui_pane_join_window\n");

    rb_check_arity(argc, 2, UNLIMITED_ARGUMENTS);
    VALUE i_pane = argv[0];
    VALUE i_window = argv[1];

    auto window = widget_hash_lookup<QMainWindow*>(i_window);
    auto pane = widget_hash_lookup<QTabWidget*>(i_pane);

    if (pane->parent() == nullptr) {
      // attach
      window->centralWidget()->layout()->addWidget(pane);
    } else {
      // reattach
      qobject_cast<QLayout*>(pane->parent())->removeWidget(pane);
      window->centralWidget()->layout()->addWidget(pane);
    }

    return Qnil;
  });

  mikutter_plugin_add_event_listener(plugin, "gui_tab_join_pane", [](RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) -> VALUE {
    fprintf(stderr, "[mikutter_qt5_ext] on_gui_tab_join_pane\n");

    rb_check_arity(argc, 2, UNLIMITED_ARGUMENTS);
    VALUE i_tab = argv[0];
    VALUE i_pane = argv[1];

    VALUE i_widget = rb_funcall3(rb_funcall3(i_tab, rb_intern("children"), 0, nullptr), rb_intern("first"), 0, nullptr);
    if (RB_NIL_P(i_widget)) {
      return Qnil;
    }

    auto widget = widget_hash_lookup<QWidget*>(i_widget);
    if (widget == nullptr) {
      return Qnil;
    }

    auto tab = widget_hash_lookup<QWidget*>(i_tab);
    auto pane = widget_hash_lookup<QTabWidget*>(i_pane);

    auto old_pane = widget->parentWidget();
    while (old_pane != nullptr && qobject_cast<QTabWidget*>(old_pane) != nullptr) {
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

  mikutter_plugin_add_event_listener(plugin, "gui_timeline_join_tab", [](RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) -> VALUE {
    fprintf(stderr, "[mikutter_qt5_ext] on_gui_timeline_join_tab\n");

    rb_check_arity(argc, 2, UNLIMITED_ARGUMENTS);
    VALUE i_timeline = argv[0];
    VALUE i_tab = argv[1];

    auto widget = widget_hash_lookup<QWidget*>(i_timeline);
    if (widget != nullptr) {
      widget_join_tab(i_tab, widget);
    }

    return Qnil;
  });

  mikutter_plugin_add_event_listener(plugin, "gui_timeline_add_messages", [](RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) -> VALUE {
    fprintf(stderr, "[mikutter_qt5_ext] on_gui_timeline_add_messages\n");

    rb_check_arity(argc, 2, UNLIMITED_ARGUMENTS);
    VALUE i_timeline = argv[0];
    VALUE messages = argv[1];

    auto timeline = widget_hash_lookup<QListWidget*>(i_timeline);
    if (timeline != nullptr) {
      if (rb_obj_is_kind_of(messages, rb_mEnumerable) != Qtrue) {
        messages = rb_ary_new4(1, &messages);
      }

      // TODO: use show_filter

      rb_block_call(messages, rb_intern("each"), 0, nullptr, timeline_add_message_i, reinterpret_cast<VALUE>(timeline));
    }

    return Qnil;
  });

  // TODO: ヤバすぎるので何とかする
  PSEUDO_ARGV[0] = "ruby";
  app = new QApplication(PSEUDO_ARGC, (char**) PSEUDO_ARGV);
  
  return Qnil;
}

static VALUE cplugin_mainloop(VALUE self)
{
  QTimer delayerKicker;
  delayerKicker.setInterval(1000);
  QObject::connect(&delayerKicker, &QTimer::timeout, []() {
    VALUE delayer = rb_const_get(rb_cObject, rb_intern("Delayer"));
    rb_funcall3(delayer, rb_intern("run_once"), 0, nullptr);
  });

  app->exec();
  return Qnil;
}

static VALUE cplugin_enqueue(VALUE self)
{
  if (!rb_block_given_p()) {
    rb_raise(rb_eArgError, "Expected block");
  }
  rb_ary_push(dispatch_queue, rb_block_proc());
  QTimer::singleShot(0, []() {
    VALUE proc = Qnil;
    while ((proc = rb_ary_shift(dispatch_queue)) != Qnil) {
      rb_funcall(proc, rb_intern("call"), 0);
    }
  });
  return Qnil;
}

static VALUE cplugin_enqueue_delayed(VALUE self, VALUE delay)
{
  if (!rb_block_given_p()) {
    rb_raise(rb_eArgError, "Expected block");
  }
  auto ldelay = FIX2LONG(rb_funcall3(delay, rb_intern("to_i"), 0, nullptr)); // TODO: size check
  auto table_index = dispatch_table_next++;
  fprintf(stderr, "[mikutter_qt5_ext] cplugin_enqueue_delayed delay=%ld, index=%lu\n", ldelay, table_index);
  rb_hash_aset(dispatch_table, ULONG2NUM(table_index), rb_block_proc());
  QTimer::singleShot(ldelay * 1000 + 1500, [table_index]() {
    fprintf(stderr, "[mikutter_qt5_ext] cplugin_enqueue_delayed run index=%lu\n", table_index);
    VALUE idx = ULONG2NUM(table_index);
    VALUE proc = rb_hash_aref(dispatch_table, idx);
    if (RB_TEST(proc)) {
      rb_funcall(proc, rb_intern("call"), 0);
      rb_hash_delete(proc, idx);
    }
  });
  return Qnil;
}

extern "C" void Init_mikutter_qt5_ext()
{
  fprintf(stderr, "[mikutter_qt5_ext] Init_mikutter_qt5_ext()\n");

  dispatch_queue = rb_ary_new();
  rb_global_variable(&dispatch_queue);

  dispatch_table = rb_hash_new();
  rb_global_variable(&dispatch_table);

  widget_hash = rb_hash_new();
  rb_global_variable(&widget_hash);

  VALUE mod_plugin = rb_const_get(rb_cObject, rb_intern("Plugin"));
  VALUE mod_cplugin = rb_define_module_under(mod_plugin, "CPlugin");
  rb_define_module_function(mod_cplugin, "init", RUBY_METHOD_FUNC(cplugin_init), 1);
  rb_define_module_function(mod_cplugin, "mainloop", RUBY_METHOD_FUNC(cplugin_mainloop), 0);
  rb_define_module_function(mod_cplugin, "enqueue", RUBY_METHOD_FUNC(cplugin_enqueue), 0);
  rb_define_module_function(mod_cplugin, "enqueue_delayed", RUBY_METHOD_FUNC(cplugin_enqueue_delayed), 1);
}

#include "init.moc"
