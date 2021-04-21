#ifndef MIKUTTER_H
#define MIKUTTER_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wregister"
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
#include <ruby.h>
#pragma clang diagnostic pop

#include <functional>
#include <utility>

namespace mikutter {
  /** @see https://social.mikutter.hachune.net/@toshi_a/105649126549745595 */
  static constexpr int HYDE = 156;

  extern const rb_data_type_t CppProcFunction;
  using CppProcFn = std::function<VALUE(VALUE, VALUE, int, const VALUE *, VALUE)>;

  static inline VALUE wrap_cpp_proc_function(const CppProcFn &fn) {
    return TypedData_Wrap_Struct(rb_cData, &CppProcFunction, new CppProcFn(fn));
  }

  static VALUE cpp_proc_function_trampoline(RB_BLOCK_CALL_FUNC_ARGLIST(yielded_arg, callback_arg)) {
    CppProcFn *fn;
    TypedData_Get_Struct(callback_arg, CppProcFn, &CppProcFunction, fn);
    return (*fn)(yielded_arg, callback_arg, argc, argv, blockarg);
  }
}  // namespace mikutter

template <typename... Args>
void mikutter_plugin_call(VALUE event_name, Args... args) {
  constexpr auto argc = sizeof...(Args);
  VALUE mod_plugin = rb_const_get(rb_cObject, rb_intern("Plugin"));
  rb_funcall(mod_plugin, rb_intern("call"), argc + 1, event_name, args...);
}

static inline VALUE mikutter_plugin_add_event_listener(VALUE plugin, const char *event_name, VALUE callback) {
  VALUE rb_event_name = rb_str_new2(event_name);
  return rb_funcall_with_block(plugin, rb_intern("add_event"), 1, &rb_event_name, callback);
}

static inline VALUE mikutter_plugin_add_event_listener(VALUE plugin, const char *event_name,
                                                       const mikutter::CppProcFn &callback) {
  VALUE rb_event_name = rb_str_new2(event_name);
  VALUE rb_cpp_func = mikutter::wrap_cpp_proc_function(callback);
  VALUE rb_callback = rb_proc_new(mikutter::cpp_proc_function_trampoline, rb_cpp_func);
  return rb_funcall_with_block(plugin, rb_intern("add_event"), 1, &rb_event_name, rb_callback);
}

static inline VALUE mikutter_plugin_add_event_filter(VALUE plugin, const char *event_name, VALUE callback) {
  VALUE rb_event_name = rb_str_new2(event_name);
  return rb_funcall_with_block(plugin, rb_intern("add_event_filter"), 1, &rb_event_name, callback);
}

static inline VALUE mikutter_plugin_add_event_filter(VALUE plugin, const char *event_name,
                                                     const mikutter::CppProcFn &callback) {
  VALUE rb_event_name = rb_str_new2(event_name);
  VALUE rb_cpp_func = mikutter::wrap_cpp_proc_function(callback);
  VALUE rb_callback = rb_proc_new(mikutter::cpp_proc_function_trampoline, rb_cpp_func);
  return rb_funcall_with_block(plugin, rb_intern("add_event_filter"), 1, &rb_event_name, rb_callback);
}

#endif
