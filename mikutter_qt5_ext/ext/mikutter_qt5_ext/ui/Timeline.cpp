#include "Timeline.hpp"

static VALUE timeline_add_message_i(RB_BLOCK_CALL_FUNC_ARGLIST(message, timeline_model_ptr)) {
  auto model = reinterpret_cast<TimelineModel *>(timeline_model_ptr);
  model->add(message);

  return Qnil;
}

Timeline::Timeline(VALUE imaginary, QWidget *parent) : QListView(parent), imaginary(imaginary) {
  connect(this, &QWidget::customContextMenuRequested, [=](const QPoint &pos) {
    VALUE mod_plugin = rb_const_get(rb_cObject, rb_intern("Plugin"));
    VALUE mod_gui = rb_const_get(mod_plugin, rb_intern("GUI"));
    VALUE mod_command = rb_const_get(mod_gui, rb_intern("Command"));
    rb_funcall3(mod_command, rb_intern("menu_pop"), 0, nullptr);
  });

  connect(this, &QWidget::destroyed, [=]() { rb_funcall3(this->imaginary, rb_intern("destroy"), 0, nullptr); });

  this->setContextMenuPolicy(Qt::CustomContextMenu);
  this->setModel(&this->model);
}

void Timeline::add(VALUE messages) {
  if (rb_obj_is_kind_of(messages, rb_mEnumerable) != Qtrue) {
    messages = rb_ary_new4(1, &messages);
  }

  // TODO: use show_filter

  rb_block_call(messages, rb_intern("each"), 0, nullptr, timeline_add_message_i, reinterpret_cast<VALUE>(&this->model));
}

VALUE Timeline::getActiveMessages() {
  VALUE messages = rb_ary_new();

  for (const auto &index : this->selectedIndexes()) {
    rb_ary_push(messages, this->model.get(index.row()));
  }

  return messages;
}

void Timeline::focusInEvent(QFocusEvent *event) {
  QListView::focusInEvent(event);

  VALUE args[] = {Qtrue, Qtrue};
  rb_funcall3(this->imaginary, rb_intern("active!"), 2, args);
}
