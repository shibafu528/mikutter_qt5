#ifndef MIKUTTER_QT5_EXT_TIMELINE_HPP
#define MIKUTTER_QT5_EXT_TIMELINE_HPP

#include <QListWidget>

#include "../mikutter.hpp"

class Timeline : public QListWidget {
  Q_OBJECT

public:
  Timeline(VALUE imaginary, QWidget *parent = nullptr);

  void add(VALUE messages);

private:
  VALUE imaginary;
};

#endif  // MIKUTTER_QT5_EXT_TIMELINE_HPP
