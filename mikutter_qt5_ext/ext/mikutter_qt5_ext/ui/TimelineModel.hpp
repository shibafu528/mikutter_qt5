#ifndef MIKUTTER_QT5_EXT_TIMELINEMODEL_HPP
#define MIKUTTER_QT5_EXT_TIMELINEMODEL_HPP

#include <QAbstractListModel>

#include "../mikutter.hpp"

class TimelineModel : public QAbstractListModel {
public:
  int rowCount(const QModelIndex &parent) const override;

  QVariant data(const QModelIndex &index, int role) const override;

  void add(VALUE message);

  VALUE get(int index);

private:
  // TODO: 一切のGC保護がないぞい
  QVector<VALUE> messages;
};

#endif  // MIKUTTER_QT5_EXT_TIMELINEMODEL_HPP
