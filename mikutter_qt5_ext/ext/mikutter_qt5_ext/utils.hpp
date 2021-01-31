#ifndef MIKUTTER_QT5_EXT_UTILS_HPP
#define MIKUTTER_QT5_EXT_UTILS_HPP

#include <cstdio>

template <typename... Args>
static inline void mqt_log(const char *format, Args... args) {
  fprintf(stderr, "[mikutter_qt5_ext] ");
  fprintf(stderr, format, args...);
  fprintf(stderr, "\n");
}

#endif  // MIKUTTER_QT5_EXT_UTILS_HPP
