#include "MikutterWindow.hpp"

#include <QHBoxLayout>

MikutterWindow::MikutterWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("mikutter");

    auto central = new QWidget(this);
    central->setLayout(new QHBoxLayout(central));

    setCentralWidget(central);
}
