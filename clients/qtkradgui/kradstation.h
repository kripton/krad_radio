#ifndef KRADSTATION_H
#define KRADSTATION_H

#include <QObject>
extern "C" {
#include "kr_client.h"
#include "krad_radio_client.h"
#include "krad_mixer_client.h"
}

class KradStation : public QObject
{
  Q_OBJECT
public:
  explicit KradStation(QObject *parent = 0);
  KradStation(QString sysname, QObject *parent = 0);
  void waitForBroadcasts();
  static QStringList getRunningStations();
signals:
  void portgroupDestroyed(QString name);
  void portgroupAdded(kr_mixer_portgroup_t *portgroup);
  void volumeUpdate(kr_mixer_portgroup_control_rep_t *control_rep);
  void crossfadUpdated(kr_mixer_portgroup_control_rep_t *control_rep);
  void cpuTimeUpdated(int value);
public slots:
  void handlePortgroupAdded(kr_mixer_portgroup_t *portgroup);
  void setVolume(QString portname, int value);
  void setCrossfade(QString name, int value);
private:
  kr_client_t *client;
  QString sysname;

  void handleResponse();
  void emitItemType(kr_item_t *item);
  
};

#endif // KRADSTATION_H
