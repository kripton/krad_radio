#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QHash>

#include "kradstation.h"
#include "broadcastthread.h"


class MainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
  
public slots:
  void updateVolume(kr_mixer_portgroup_control_rep_t *control_rep);
private:
  KradStation *kradStation;
  BroadcastThread *broadcastThread;
  QHash<QString, QSlider> *sliders;

  QSlider *volumeSlider;
};

#endif // MAINWINDOW_H
