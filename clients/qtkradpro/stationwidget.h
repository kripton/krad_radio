#ifndef STATIONWIDGET_H
#define STATIONWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QMap>

#include "kradstation.h"
#include "broadcastthread.h"
#include "portgroupslider.h"
#include "crossfade.h"
#include "eq.h"
#include "eqslider.h"
#include "tapetubewidget.h"
#include "filterwidget.h"
#include "compositorcontrol.h"

class StationWidget : public QWidget
{
  Q_OBJECT
public:
  explicit StationWidget(QString sysname, QWidget *parent = 0);
  bool isConnected();
signals:

public slots:
  void portgroupAdded(kr_mixer_portgroup_t *rep);
  void addEffects(QString portname);
  void removeEffects(QString portname);
  void closeTabRequested(int index);
  void closeEffectRequested(int index);
  void frameSizeUpdated(QRect geometry);
private:
  KradStation *kradStation;
  BroadcastThread *broadcastThread;
  QThread* broadcastWorkerThread;
  QList<PortgroupSlider *> sliders;
  QHBoxLayout *mixerLayout;
  QVBoxLayout *mainLayout;
  EqWidget *eqWidget;
  TapetubeWidget *tapetubeWidget;
  FilterWidget *filterWidget;
  QMap<QString, QTabWidget*> effectsWidgets;
  CompositorControl *compCtrl;
  QDialog *compCtrlDialog;
};

#endif // STATIONWIDGET_H
