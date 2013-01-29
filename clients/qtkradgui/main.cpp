#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  a.setObjectName(QString("KRAD GUI PRO"));
  return a.exec();
}
