#include "mainwindow.h"
#include "stationwidget.h"
#include "kradstation.h"
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
  this->setWindowTitle("KRAD PRO");

  tabWidget = new QTabWidget(this);

  QStringList sl = KradStation::getRunningStations();
  QStringListIterator sli(sl);
  QPushButton *addButton = new QPushButton("+");
  addButton->setFixedSize(26, 26);
  connect(addButton, SIGNAL(clicked()), this, SLOT(launchStation()));
  tabWidget->setCornerWidget(addButton, Qt::TopRightCorner);

  tabWidget->setTabsClosable(true);
  //QScrollArea *scrollArea = new QScrollArea(this);
  //scrollArea->setWidget(tabWidget);

 // tabWidget->setMinimumSize(2000, 500);
 // tabWidget->setMaximumSize(2000, 1000);
  while (sli.hasNext()) {
    QString i = sli.next();
    StationWidget* station = new StationWidget(i, this);

    connect(tabWidget, SIGNAL(tabCloseRequested(int)), station, SLOT(closeTabRequested(int)));
    tabWidget->addTab(station, i);
  }
//  StationWidget *sw = new StationWidget(tr("djsh"), this);
  setCentralWidget(tabWidget);

}

void MainWindow::launchStation()
{
  QString name;
  QInputDialog *qi = new QInputDialog();
  name = qi->getText(this, tr("Launch a New Station"), tr("New Station Name"));


  if (name.length() > 3) {
    krad_radio_launch (name.toAscii().data());
    sleep(1);
 }

  tabWidget->addTab(new StationWidget(name, this), name);

}
