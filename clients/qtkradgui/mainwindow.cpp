#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
  kradStation = new KradStation("djsh");
  broadcastThread = new BroadcastThread(kradStation);
  volumeSlider = new QSlider(parent);


  QWidget *main_w = new QWidget (this);
  setCentralWidget (main_w);


  QHBoxLayout *layout = new QHBoxLayout (main_w);
  layout->addWidget(volumeSlider);

  connect(kradStation, SIGNAL(volumeUpdate(kr_mixer_portgroup_control_rep_t*)), this, SLOT(updateVolume(kr_mixer_portgroup_control_rep_t*)));
  connect(volumeSlider, SIGNAL(valueChanged(int)), kradStation, SLOT(setVolume(int)));
  broadcastThread->start();
}

MainWindow::~MainWindow()
{
}

void MainWindow::updateVolume(kr_mixer_portgroup_control_rep_t *control_rep)
{
  volumeSlider->setValue(control_rep->value);
}

