

#include "kradstation.h"
#include <QDebug>
#include <QStringList>

#define DEFAULT_DURATION 240

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

  if (!kr_connect (client, sysname.toAscii().data_ptr()->data)) {
    qDebug() << tr("Could not connect to %1 krad radio daemon").arg(sysname);
    kr_client_destroy (&client);
    return;
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

bool KradStation::isConnected()
{
  if (kr_connected(client) == 1)
    return true;
  else
    return false;

}


void KradStation::handlePortgroupAdded(kr_mixer_portgroup_t *portgroup)
{
  qDebug() << tr("here!!!!");
  qDebug() << tr("it's a portgroup called %1 and the volume is %2").arg(portgroup->sysname).arg(portgroup->volume[0]);

}

void KradStation::setVolume(QString portname, float value)
{
  qDebug() << tr("trying to set volume of %1 to %2").arg(portname).arg(value);
  kr_mixer_set_control (client, portname.toAscii().data(), "volume", value, 240);
}

void KradStation::setCrossfade(QString name, float value)
{
  qDebug() << tr("trying to set crossfade %1 to %2").arg(name).arg(value);
  kr_mixer_set_control (client, name.toAscii().data(), "crossfade",  value, 240);
}

void KradStation::addTapetube(QString portname)
{
  qDebug() << tr("trying to add tapetube to %1").arg(portname);
//  kr_mixer_add_effect (client, portname.toAscii().data(), "tapetube");
//  kr_mixer_set_effect_control (client, portname.toAscii().data(), 3, control.toAscii().data(), 0, value, 240, EASEINOUTSINE);
}

void KradStation::removeTapeTube(QString portname)
{
 //  kr_mixer_remove_effect(client, portname.toAscii().data(), 1);
}

void KradStation::setTapeTube(QString portname, QString control, float value)
{
  qDebug() << tr("trying to set tapetube %1 %2 to %3").arg(portname).arg(control).arg(value);
  kr_mixer_set_effect_control (client, portname.toAscii().data(), 3, control.toAscii().data(), 0, value, 240, EASEINOUTSINE);
}

void KradStation::addHiPassFilter(QString portname)
{
//  kr_mixer_add_effect(client, portname.toAscii().data(), "pass");
}

void KradStation::addLoPassFilter(QString portname)
{
 // kr_mixer_add_effect(client, portname.toAscii().data(), "pass");
 // kr_mixer_set_effect_control(client, portname.toAscii().data(), 3, "type",0, 1, 240, EASEINOUTSINE);
}

void KradStation::setHiPassFilter(QString portname, QString control, float value)
{
  qDebug() << tr("trying to set hi pass filter %1 %2 %3").arg(portname).arg(control).arg(value);
//  krad_ease_t blah;
  kr_mixer_set_effect_control(client, portname.toAscii().data(), 2, control.toAscii().data(), 0, value, 240, EASEINOUTSINE);
}

void KradStation::setLoPassFilter(QString portname, QString control, float value)
{
  qDebug() << tr("trying to set lo pass filter %1 %2 %3").arg(portname).arg(control).arg(value);
  kr_mixer_set_effect_control(client, portname.toAscii().data(), 1, control.toAscii().data(), 0, value, 240, EASEINOUTSINE);
}
void KradStation::removePassFilter(QString portname)
{
//  kr_mixer_remove_effect(client, portname.toAscii().data(), 2);
 // kr_mixer_remove_effect(client, portname.toAscii().data(), 3);
}

void KradStation::addEq(QString portname)
{
  qDebug() << tr("trying to add eq");
  //kr_mixer_effect_control_t
  //kr_mixer_add_effect (client, portname.toAscii().data(), "eq");
}

void KradStation::rmEq(QString portname)
{
  //kr_mixer_remove_effect(client, portname.toAscii().data(), 0);
}

void KradStation::setEq(QString portname, int bandId, float value)
{
  qDebug() << tr("trying to set eq %1 to %2").arg(bandId).arg(value);
  kr_mixer_set_effect_control (client, portname.toAscii().data(), 0, "db", bandId, value, DEFAULT_DURATION, EASEINOUTSINE);
}

void KradStation::eqBandAdded(QString portname, int bandId, float freq)
{
  qDebug() << tr("trying to addband on eq 0, id %2, freq %3").arg(bandId).arg(freq);
  kr_mixer_set_effect_control (client, portname.toAscii().data_ptr()->data, 0, "hz", bandId, freq, 0, EASEINOUTSINE);
}

void KradStation::xmms2Play(QString portname)
{
  kr_mixer_portgroup_xmms2_cmd(client, portname.toAscii().data(), "play");
}

void KradStation::xmms2Pause(QString portname)
{
  kr_mixer_portgroup_xmms2_cmd(client, portname.toAscii().data(), "pause");
}

void KradStation::xmms2Stop(QString portname)
{
  kr_mixer_portgroup_xmms2_cmd(client, portname.toAscii().data(), "stop");
}

void KradStation::xmms2Next(QString portname)
{
  kr_mixer_portgroup_xmms2_cmd(client, portname.toAscii().data(), "next");

}

void KradStation::xmms2Prev(QString portname)
{
  kr_mixer_portgroup_xmms2_cmd(client, portname.toAscii().data(), "prev");
}

void KradStation::openDisplay()
{
  kr_compositor_open_display(client, 1280, 720);
}

void KradStation::closeDisplay()
{
  kr_compositor_close_display(client);
}

void KradStation::addSprite(QString filename, int x, int y, int z, int tickrate, float scale, float opacity, float rotation)
{
  kr_compositor_add_sprite(client, filename.toAscii().data(), x, y, z, tickrate, scale, opacity, rotation);
}

void KradStation::setSprite(int spriteNum, int x, int y, int z, int tickrate, float scale, float opacity, float rotation)
{
  kr_compositor_set_sprite(client, spriteNum, x, y, z, tickrate, scale, opacity, rotation);
}

void KradStation::kill()
{
    //client = NULL;
    //qDebug() << krad_radio_pid(sysname.toLocal8Bit().data());
    krad_radio_destroy(sysname.toLocal8Bit().data());
    //delete this;
}

QRect KradStation::getCompFrameSize()
{
    kr_compositor_get_frame_size(client);
    kr_response_t *response;
    char *string;
    int wait_time_ms;
    int length;

    string = NULL;
    response = NULL;
    wait_time_ms = 250;

    if (kr_poll (client, wait_time_ms)) {
      kr_client_response_get (client, &response);
      if (response != NULL) {
        length = kr_response_to_string (response, &string);
        if (length > 0) {
          printf ("%s\n", string);
          kr_response_free_string (&string);
        }
        kr_response_free (&response);
      }
    } else {
      printf ("No response after waiting %dms\n", wait_time_ms);
    }
    qDebug() << "Frame size:" << string;
}

void KradStation::handleResponse()
{

  kr_response_t *response;
  kr_address_t *address;
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
  wait_time_ms = 250;

  if (kr_poll (client, wait_time_ms)) {
    kr_client_response_get (client, &response);

    if (response != NULL) {
      kr_response_address (response, &address);
      kr_address_debug_print (address);
      if (kr_response_is_list (response)) {
        items = kr_response_list_length (response);
        qDebug() << tr("Response is a list with %1 items.").arg(items);
        for (i = 0; i < items; i++) {
          if (kr_response_listitem_to_rep(response, i, &rep)) {
            qDebug() << tr("Got rep \"%1\"").arg(i);
            emitRepType(rep);

          } else {
            qDebug() << tr("Did not get item %1").arg(i);
          }
        }
      } else {

        if (kr_response_to_rep(response, &rep)) {
          qDebug() << tr("response is a single rep");
          emitRepType(rep);
        }
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


void KradStation::emitRepType(kr_rep_t *rep)
{


    if (rep == NULL) return;

    if (rep->type == EBML_ID_KRAD_MIXER_PORTGROUP || rep->type == EBML_ID_KRAD_MIXER_PORTGROUP_CREATED) {
        emit portgroupAdded(rep->rep_ptr.mixer_portgroup);
        qDebug() << tr("signalling portgroup added");
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

   /* if (rep->type == EBML_ID_KRAD_MIXER_PORTGROUP_DESTROYED) {
        emit portgroupDestroyed(tr(rep->rep_ptr.mixer_portgroup->sysname));
    }
*/
}



