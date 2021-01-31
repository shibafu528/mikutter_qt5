#ifndef MIKUTTER_QT5_EXT_UTILS_HPP
#define MIKUTTER_QT5_EXT_UTILS_HPP

#include <QApplication>
#include <cstdio>

template <typename... Args>
static inline void mqt_log(const char *format, Args... args) {
  fprintf(stderr, "[mikutter_qt5_ext] ");
  fprintf(stderr, format, args...);
  fprintf(stderr, "\n");
}

template <typename Func>
static void mqt_post_to_main_thread(int msec, Func function) {
  auto timer = new QTimer();
  timer->setSingleShot(true);
  timer->moveToThread(qApp->thread());
  QObject::connect(timer, &QTimer::timeout, function);
  QObject::connect(timer, &QTimer::timeout, timer, &QObject::deleteLater);
  QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, msec));
}

#endif  // MIKUTTER_QT5_EXT_UTILS_HPP
