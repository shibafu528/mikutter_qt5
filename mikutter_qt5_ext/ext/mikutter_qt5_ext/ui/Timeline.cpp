#include "Timeline.hpp"

static VALUE timeline_add_message_i(RB_BLOCK_CALL_FUNC_ARGLIST(message, timeline_ptr)) {
  auto timeline = reinterpret_cast<Timeline *>(timeline_ptr);

  VALUE user = rb_funcall3(message, rb_intern("user"), 0, nullptr);
  VALUE idname;
  if (RB_TEST(user)) {
    idname = rb_funcall3(user, rb_intern("idname"), 0, nullptr);
  } else {
    idname = rb_str_new_cstr("");
  }

  VALUE desc = rb_funcall3(message, rb_intern("description"), 0, nullptr);
  timeline->addItem(QString("@%1: %2").arg(StringValuePtr(idname), StringValuePtr(desc)));

  return Qnil;
}

Timeline::Timeline(VALUE imaginary, QWidget *parent) : QListWidget(parent), imaginary(imaginary) {}

void Timeline::add(VALUE messages) {
  if (rb_obj_is_kind_of(messages, rb_mEnumerable) != Qtrue) {
    messages = rb_ary_new4(1, &messages);
  }

  // TODO: use show_filter

  rb_block_call(messages, rb_intern("each"), 0, nullptr, timeline_add_message_i, reinterpret_cast<VALUE>(this));
}