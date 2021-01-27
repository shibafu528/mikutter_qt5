#include "mikutter.hpp"
#include <functional>

namespace mikutter {
  void free_cpp_proc_function(void *data);
}

const rb_data_type_t mikutter::CppProcFunction = {
  "CppFunction",
  {nullptr, mikutter::free_cpp_proc_function, nullptr},
  nullptr,
  nullptr,
  RUBY_TYPED_FREE_IMMEDIATELY,
};

void mikutter::free_cpp_proc_function(void *data)
{
  auto func = static_cast<mikutter::CppProcFn*>(data);
  delete func;
}
