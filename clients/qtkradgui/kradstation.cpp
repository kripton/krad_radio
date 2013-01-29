

#include "kradstation.h"
#include <QDebug>
#include <QStringList>

KradStation::KradStation(QObject *parent) :
  QObject(parent)
{
}

KradStation::KradStation(QString sysname, QObject *parent) :
  QObject(parent)
{

  client = kr_client_create (sysname.toAscii().data());

  this->sysname = sysname;
  if (client == NULL) {
    qDebug() << tr("Could not create client");
    return;
  }

  while (!kr_connect (client, sysname.toAscii().data_ptr()->data)) {
    qDebug() << tr("Could not connect to %1 krad radio daemon").arg(sysname);
    kr_client_destroy (&client);
    sleep(1);
  }

 // connect(this, SIGNAL(portgroupUpdate(kr_mixer_portgroup_t*)), this, SLOT(handlePortgroupUpdate(kr_mixer_portgroup_t*)));
  qDebug() << tr("Connected to %1!").arg(sysname);
  qDebug() << tr("Subscribing to all broadcasts");
  kr_broadcast_subscribe (client, ALL_BROADCASTS);
  qDebug() << tr("Subscribed to all broadcasts");
  kr_mixer_portgroups_list (client);
  kr_tags (client, NULL);
//  kr_client_response_wait_print (client);
}



void KradStation::waitForBroadcasts()
{
  int ret;
  unsigned int timeout_ms;

  ret = 0;
  timeout_ms = 3000;

  qDebug() << tr("Waiting for broadcasts up to %2 each").arg(timeout_ms);

    if (client != NULL) {
      ret = kr_poll (client, timeout_ms);
    } else {
      ret = -1;
    }

    if (ret > 0) {
      handleResponse ();
    }

}

QStringList KradStation::getRunningStations()
{
  char *list;
  QStringList sl;
  list = krad_radio_running_stations ();
  if (list[0] == 0) return sl; // No station running
  QString qliststr = tr(list);
  sl = qliststr.split(tr("\n"));
  return sl;
}


void KradStation::handlePortgroupAdded(kr_mixer_portgroup_t *portgroup)
{
  qDebug() << tr("here!!!!");
  qDebug() << tr("it's a portgroup called %1 and the volume is %2").arg(portgroup->sysname).arg(portgroup->volume[0]);

}

void KradStation::setVolume(QString portname, int value)
{
  qDebug() << tr("trying to set volume of %1 to %2").arg(portname).arg(value);
  kr_mixer_set_control (client, portname.toAscii().data(), "volume", (float) value, 0);
}

void KradStation::setCrossfade(QString name, int value)
{
  qDebug() << tr("trying to set crossfade %1 to %2").arg(name).arg(value);
  kr_mixer_set_control (client, name.toAscii().data(), "crossfade", (float) value, 0);
}

void KradStation::addTapetube(QString portname)
{
  qDebug() << tr("trying to add tapetube to %1").arg(portname);
  kr_mixer_add_effect (client, portname.toAscii().data(), "tapetube");
}

void KradStation::setTapeTube(QString portname, QString control, int value)
{
  qDebug() << tr("trying to set tapetube %1 %2 to %3").arg(portname).arg(control).arg(value);
  kr_mixer_set_effect_control (client, portname.toAscii().data(), 1, control.toAscii().data(), 0, (float) value);
}

void KradStation::addEq(QString portname)
{
  qDebug() << tr("trying to add eq");
  kr_mixer_add_effect (client, portname.toAscii().data(), "eq");
}

void KradStation::rmEq(QString portname)
{
  kr_mixer_remove_effect(client, portname.toAscii().data(), 0);
}

void KradStation::setEq(QString portname, int bandId, int value)
{
  qDebug() << tr("trying to set eq %1 to %2").arg(bandId).arg(value);
  kr_mixer_set_effect_control (client, portname.toAscii().data(), 0, "db", bandId, (float) value);
}

void KradStation::eqBandAdded(QString portname, int bandId, float freq)
{
  qDebug() << tr("trying to addband on eq 0, id %2, freq %3").arg(bandId).arg(freq);
  kr_mixer_set_effect_control (client, portname.toAscii().data_ptr()->data, 0, "addband", 0, freq);
}

void KradStation::kill()
{
    //client = NULL;
    //qDebug() << krad_radio_pid(sysname.toLocal8Bit().data());
    krad_radio_destroy(sysname.toLocal8Bit().data());
    //delete this;
}

void KradStation::handleResponse()
{

  kr_response_t *response;

  kr_item_t *item;
  kr_rep_t *rep;

  char *string;
  int wait_time_ms;
  int length;
  int number;
  int i;
  int items;


  items = 0;
  i = 0;
  number = 0;
  string = NULL;
  response = NULL;
  rep = NULL;
  wait_time_ms = 250;

  if (kr_poll (client, wait_time_ms)) {
    kr_client_response_get (client, &response);

    if (response != NULL) {
      if (kr_response_is_list (response)) {
        items = kr_response_list_length (response);
        qDebug() << tr("Response is a list with %1 items.").arg(items);
        for (i = 0; i < items; i++) {
          if (kr_response_list_get_item (response, i, &item)) {
            qDebug() << tr("Got item \"%1\" type is \"%2\"").arg(i).arg(kr_item_get_type_string (item));
            emitItemType(item);
            if (kr_item_to_string (item, &string)) {
              qDebug() << tr("Item String: %1").arg(string);
              kr_response_free_string (&string);

            } else {
              qDebug() << tr("Did not get item %1").arg(i);
            }
          }
        }
      } else {

        kr_response_get_item (response, &item);
        emitItemType(item);

        length = kr_response_to_string (response, &string);
        qDebug() <<  tr("Response Length: %1").arg(length);
        if (length > 0) {
          qDebug() <<  tr("Response String: %1").arg(string);
          kr_response_free_string (&string);
        }
        if (kr_response_to_int (response, &number)) {
          emit cpuTimeUpdated(number);
          qDebug() << tr("Response Int: %1").arg(number);
        }
        kr_response_free (&response);
      }
    } else {
      qDebug() << tr("No response after waiting %1 ms").arg(wait_time_ms);
    }
  }
  // printf ("\n");
}

void KradStation::emitItemType(kr_item_t *item)
{
    kr_rep_t *rep;

    if (item == NULL) return;
    qDebug() << tr("Got item type is %1").arg(kr_item_get_type_string (item));

    rep = kr_item_to_rep(item);
    if (rep == NULL) return;
    if (rep->type == EBML_ID_KRAD_MIXER_PORTGROUP || rep->type == EBML_ID_KRAD_MIXER_PORTGROUP_CREATED) {
        emit portgroupAdded(rep->rep_ptr.mixer_portgroup);
        //kr_rep_free (&rep);
    }
    if (rep->type ==  EBML_ID_KRAD_MIXER_CONTROL) {
        if (strcmp(rep->rep_ptr.mixer_portgroup_control->control, "volume") == 0) {
            emit volumeUpdate(rep->rep_ptr.mixer_portgroup_control);
        }
        qDebug() << tr("testing for crossfade: %1").arg(rep->rep_ptr.mixer_portgroup_control->control);
        if (strcmp(rep->rep_ptr.mixer_portgroup_control->control, "fade") == 0) {
            qDebug() << tr("emitting crossfade");
            emit crossfadUpdated(rep->rep_ptr.mixer_portgroup_control);
        }
    }

    if (rep->type == EBML_ID_KRAD_MIXER_PORTGROUP_DESTROYED) {
        emit portgroupDestroyed(tr(rep->rep_ptr.mixer_portgroup->sysname));
    }

}



