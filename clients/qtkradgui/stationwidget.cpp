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

  broadcastThread->start();
  connect(kradStation, SIGNAL(portgroupAdded(kr_mixer_portgroup_t*)), this, SLOT(portgroupAdded(kr_mixer_portgroup_t*)));
  connect(kradStation, SIGNAL(cpuTimeUpdated(int)), cpuTime, SLOT(setValue(int)));}


void StationWidget::portgroupAdded(kr_mixer_portgroup_t *rep)
{
  LabelledSlider *ls = new LabelledSlider(tr(rep->sysname), this);
  ls->setValue(rep->volume[0]);
  sliders.append(ls);
  mixerLayout->addWidget(ls);

  connect(kradStation, SIGNAL(volumeUpdate(kr_mixer_portgroup_control_rep_t*)), ls, SLOT(updateVolume(kr_mixer_portgroup_control_rep_t*)));
  connect(ls, SIGNAL(valueChanged(QString, int)), kradStation, SLOT(setVolume(QString, int)));

  if (rep->crossfade_group_rep != NULL) {
    Crossfade *cf = new Crossfade(tr(rep->crossfade_group_rep->portgroup_rep[0]->sysname),tr("%1 - %2").arg(tr(rep->crossfade_group_rep->portgroup_rep[0]->sysname)).arg(tr(rep->crossfade_group_rep->portgroup_rep[1]->sysname)));
    cf->setValue(rep->crossfade_group_rep->fade);
    mixerLayout->addWidget(cf);
    //cf->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    connect(kradStation, SIGNAL(crossfadUpdated(kr_mixer_portgroup_control_rep_t*)), cf, SLOT(updateVolume(kr_mixer_portgroup_control_rep_t*)));
    connect(cf, SIGNAL(valueChanged(QString,int)), kradStation, SLOT(setCrossfade(QString,int)));

  }
}


