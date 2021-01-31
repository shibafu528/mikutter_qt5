#ifndef MIKUTTER_QT5_EXT_MIKUTTERWINDOW_HPP
#define MIKUTTER_QT5_EXT_MIKUTTERWINDOW_HPP

#include <QMainWindow>

class MikutterWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MikutterWindow(QWidget *parent = nullptr);
};

#endif  // MIKUTTER_QT5_EXT_MIKUTTERWINDOW_HPP
