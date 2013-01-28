#ifndef STATIONWIDGET_H
#define STATIONWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QHash>

#include "kradstation.h"
#include "broadcastthread.h"
#include "labelledslider.h"
#include "crossfade.h"
#include "eq.h"
#include "eqslider.h"

class StationWidget : public QWidget
{
  Q_OBJECT
public:
  explicit StationWidget(QString sysname, QWidget *parent = 0);

signals:

public slots:
  void portgroupAdded(kr_mixer_portgroup_t *rep);
  void closeTabRequested(int index);
private:
  KradStation *kradStation;
  BroadcastThread *broadcastThread;
  QThread* broadcastWorkerThread;
  QList<LabelledSlider *> sliders;
  QHBoxLayout *mixerLayout;
  QVBoxLayout *mainLayout;
  EqWidget *eqWidget;
};

#endif // STATIONWIDGET_H
