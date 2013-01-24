#include "mainwindow.h"
#include "stationwidget.h"
#include "kradstation.h"
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
  QTabWidget *tabWidget = new QTabWidget(this);

  QStringList sl = KradStation::getRunningStations();
  QStringListIterator sli(sl);
  QPushButton *addButton = new QPushButton("+");
  addButton->setFixedSize(26, 26);

  tabWidget->setCornerWidget(addButton, Qt::TopRightCorner);

  while (sli.hasNext()) {
    QString i = sli.next();
    tabWidget->addTab(new StationWidget(i, this), i);
  }
//  StationWidget *sw = new StationWidget(tr("djsh"), this);
  setCentralWidget(tabWidget);

}
