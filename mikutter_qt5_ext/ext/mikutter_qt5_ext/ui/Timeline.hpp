#ifndef MIKUTTER_QT5_EXT_TIMELINE_HPP
#define MIKUTTER_QT5_EXT_TIMELINE_HPP

#include <QListView>

#include "../mikutter.hpp"
#include "TimelineModel.hpp"

class Timeline : public QListView {
  Q_OBJECT

public:
  Timeline(VALUE imaginary, QWidget *parent = nullptr);

  void add(VALUE messages);

  /**
   * @return Array
   */
  VALUE getActiveMessages();

protected:
  void focusInEvent(QFocusEvent *event) override;

private:
  VALUE imaginary;
  TimelineModel model;
};

#endif  // MIKUTTER_QT5_EXT_TIMELINE_HPP
