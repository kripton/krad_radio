#include "stationwidget.h"

StationWidget::StationWidget(QString sysname, QWidget *parent) :
  QWidget(parent)
{

  kradStation = new KradStation(sysname);
  broadcastThread = new BroadcastThread(kradStation);

  QProgressBar *cpuTime = new QProgressBar ();
  cpuTime->setOrientation(Qt::Horizontal);


  mainLayout = new QVBoxLayout (this);
  mixerLayout = new QHBoxLayout ();


  mainLayout->addLayout(mixerLayout);
  mainLayout->addWidget(cpuTime);

  broadcastWorkerThread = new QThread();
  broadcastThread->moveToThread(broadcastWorkerThread);
  connect(broadcastWorkerThread, SIGNAL(started()), broadcastThread, SLOT(process()));
  broadcastWorkerThread->start();

  connect(kradStation, SIGNAL(portgroupAdded(kr_mixer_portgroup_t*)), this, SLOT(portgroupAdded(kr_mixer_portgroup_t*)));
  connect(kradStation, SIGNAL(cpuTimeUpdated(int)), cpuTime, SLOT(setValue(int)));

}


void StationWidget::portgroupAdded(kr_mixer_portgroup_t *rep)
{
  // Check if the slider exists. do nothing if so
  foreach (PortgroupSlider* slider, sliders) {
    if (slider->getLabel() == rep->sysname) return;
  }

  PortgroupSlider *ps = new PortgroupSlider(tr(rep->sysname), this);
  ps->setValue(rep->volume[0]);
  sliders.append(ps);
  mixerLayout->addWidget(ps);

  connect(kradStation, SIGNAL(volumeUpdate(kr_mixer_portgroup_control_rep_t*)), ps, SLOT(updateVolume(kr_mixer_portgroup_control_rep_t*)));
  connect(ps, SIGNAL(valueChanged(QString, int)), kradStation, SLOT(setVolume(QString, int)));

  if (rep->crossfade_group_rep != NULL) {
    Crossfade *cf = new Crossfade(tr(rep->crossfade_group_rep->portgroup_rep[0]->sysname),tr("%1 - %2").arg(tr(rep->crossfade_group_rep->portgroup_rep[0]->sysname)).arg(tr(rep->crossfade_group_rep->portgroup_rep[1]->sysname)));
    cf->setValue(rep->crossfade_group_rep->fade);
    mixerLayout->addWidget(cf);
    //cf->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    connect(kradStation, SIGNAL(crossfadUpdated(kr_mixer_portgroup_control_rep_t*)), cf, SLOT(updateVolume(kr_mixer_portgroup_control_rep_t*)));
    connect(cf, SIGNAL(valueChanged(QString,int)), kradStation, SLOT(setCrossfade(QString,int)));

  }

  connect(ps, SIGNAL(addEffects(QString)), this, SLOT(addEffects(QString)));
  connect(ps, SIGNAL(removeEffects(QString)), this, SLOT(removeEffects(QString)));
}

void StationWidget::addEffects(QString portname)
{
  QTabWidget *tabWidget = new QTabWidget(this);
  eqWidget = new EqWidget(portname);
  tapetubeWidget = new TapetubeWidget(portname);
  filterWidget = new FilterWidget(portname);
  //tabWidget->setTabsClosable(true);
  tabWidget->addTab(eqWidget, tr("%1 EQ").arg(portname));
  tabWidget->addTab(tapetubeWidget, tr("%1 Tapetube").arg(portname));
  tabWidget->addTab(filterWidget, tr("%1 Filters").arg(portname));
  kradStation->addEq(portname);
  kradStation->addTapetube(portname);
  mainLayout->addWidget(tabWidget);
  connect(eqWidget, SIGNAL(bandAdded(QString,int,float)), kradStation, SLOT(eqBandAdded(QString, int,float)));
  //connect(eqWidget, SIGNAL(eqDestroyed(QString)), kradStation, SLOT(rmEq(QString)));

  eqWidget->addBand(20.0f);
  eqWidget->addBand(40.0f);
  eqWidget->addBand(70.0f);
  eqWidget->addBand(140.0f);
  eqWidget->addBand(300.0f);
  eqWidget->addBand(600.0f);
  eqWidget->addBand(1200.0f);
  eqWidget->addBand(2400.0f);
  eqWidget->addBand(5000.0f);
  eqWidget->addBand(10000.0f);
  eqWidget->addBand(20000.0f);


  connect(eqWidget, SIGNAL(bandDbChanged(QString,int,int)), kradStation, SLOT(setEq(QString,int,int)));
  connect(tapetubeWidget, SIGNAL(tapetubeUpdated(QString,QString,int)), kradStation, SLOT(setTapeTube(QString,QString,int)));

}

void StationWidget::removeEffects(QString portname)
{


  kradStation->rmEq(portname);
  kradStation->removeTapeTube(portname);

  eqWidget->close();
  tapetubeWidget->close();
  tabWidget->close();
}

void StationWidget::closeTabRequested(int index)
{
    Q_UNUSED(index);
    qDebug() << "tab close requested";
    broadcastThread->stopprocessing();
    broadcastWorkerThread->quit();
    broadcastWorkerThread->wait();
    broadcastWorkerThread->deleteLater();
    kradStation->kill();
    //delete this; // Maybe this is a little rude but it works :)
}

void StationWidget::closeEffectRequested(int index)
{

}


