#ifndef STATIONWIDGET_H
#define STATIONWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QHash>

#include "kradstation.h"
#include "broadcastthread.h"
#include "labelledslider.h"
#include "crossfade.h"

class StationWidget : public QWidget
{
  Q_OBJECT
public:
  explicit StationWidget(QString sysname, QWidget *parent = 0);
  
signals:
  
public slots:
  void portgroupAdded(kr_mixer_portgroup_t *rep);

private:
  KradStation *kradStation;
  BroadcastThread *broadcastThread;
  QList<LabelledSlider *> sliders;
  QHBoxLayout *mixerLayout;
  QVBoxLayout *mainLayout;
};

#endif // STATIONWIDGET_H
