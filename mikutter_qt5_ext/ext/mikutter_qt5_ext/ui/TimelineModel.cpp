#include "TimelineModel.hpp"

int TimelineModel::rowCount(const QModelIndex &parent) const { return this->messages.size(); }

QVariant TimelineModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    VALUE message = this->messages.at(index.row());
    VALUE user = rb_funcall3(message, rb_intern("user"), 0, nullptr);
    VALUE idname;
    if (RB_TEST(user)) {
      idname = rb_funcall3(user, rb_intern("idname"), 0, nullptr);
    } else {
      idname = rb_str_new_cstr("");
    }

    VALUE desc = rb_funcall3(message, rb_intern("description"), 0, nullptr);
    return QString("@%1: %2").arg(StringValuePtr(idname), StringValuePtr(desc));
  } else {
    return QVariant();
  }
}

void TimelineModel::add(VALUE message) {
  this->beginInsertRows(QModelIndex(), this->messages.size(), this->messages.size());
  this->messages.append(message);
  this->endInsertRows();
}

VALUE TimelineModel::get(int index) {
  return this->messages.at(index);
}
