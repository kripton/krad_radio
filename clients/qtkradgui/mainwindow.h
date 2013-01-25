#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "stationwidget.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = 0);
  
signals:
  
public slots:
  void launchStation();
//  void launchStation(QString);
private:
  QTabWidget *tabWidget;
};

#endif // MAINWINDOW_H
