

#include "kradstation.h"
#include <QDebug>

KradStation::KradStation(QObject *parent) :
  QObject(parent)
{
}

KradStation::KradStation(QString sysname, QObject *parent) :
  QObject(parent)
{

  client = kr_client_create ("krad qt test");

  this->sysname = sysname;
  if (client == NULL) {
    qDebug() << tr("Could not create client\n");
    return;
  }

  if (!kr_connect (client, sysname.toAscii().data_ptr()->data)) {
    qDebug() << tr("Could not connect to %1 krad radio daemon\n").arg(sysname);
    kr_client_destroy (&client);
    return;
  }

  connect(this, SIGNAL(portgroupUpdate(kr_mixer_portgroup_t*)), this, SLOT(handlePortgroupUpdate(kr_mixer_portgroup_t*)));
  qDebug() << tr("Connected to %1!\n").arg(sysname);
  qDebug() << tr("Subscribing to all broadcasts\n");
  kr_broadcast_subscribe (client, ALL_BROADCASTS);
  qDebug() << tr("Subscribed to all broadcasts\n");

}



void KradStation::waitForBroadcasts()
{

  int b;
  int ret;
  uint64_t max;
  unsigned int timeout_ms;

  ret = 0;
  b = 0;
  max = 10000000;
  timeout_ms = 3000;

  qDebug() << tr("Waiting for up to %1 broadcasts up to %2 each\n").arg(max).arg(timeout_ms);

  while (b < max) {

    ret = kr_poll (client, timeout_ms);

    if (ret > 0) {
      handleResponse ();
    }
    b++;
  }

}


void KradStation::handlePortgroupAdded(kr_mixer_portgroup_t *portgroup)
{
  qDebug() << tr("here!!!!");
  qDebug() << tr("it's a portgroup called %1 and the volume is %2\n").arg(portgroup->sysname).arg(portgroup->volume[0]);

}

void KradStation::setVolume(int value)
{
  qDebug() << tr("trying to set volume to %1\n").arg(value);
  kr_mixer_set_control (client, "XMMS2", "volume", (float) value, 0);
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
        qDebug() << tr("Response is a list with %1 items.\n").arg(items);
        for (i = 0; i < items; i++) {
          if (kr_response_list_get_item (response, i, &item)) {
            qDebug() << tr("Got item %1 type is %2\n").arg(i).arg(kr_item_get_type_string (item));
            if (kr_item_to_string (item, &string)) {
              qDebug() << tr("Item String: %1\n").arg(string);
              kr_response_free_string (&string);

            } else {
              qDebug() << tr("Did not get item %1\n").arg(i);
            }
          }
        }
      }

      kr_response_get_item (response, &item);
      if (item != NULL) {
        qDebug() << tr("Got item type is %1\n").arg(kr_item_get_type_string (item));

        rep = kr_item_to_rep(item);
        if (rep != NULL) {
          if (rep->type == EBML_ID_KRAD_MIXER_PORTGROUP) {

            emit portgroupAdded(rep->rep_ptr.mixer_portgroup);
            //kr_rep_free (&rep);
          }
          if (rep->type ==  EBML_ID_KRAD_MIXER_CONTROL) {
            emit volumeUpdate(rep->rep_ptr.mixer_portgroup_control);
          }

        }
      }

      length = kr_response_to_string (response, &string);
      qDebug() <<  tr("Response Length: %1\n").arg(length);
      if (length > 0) {
        qDebug() <<  tr("Response String: %1\n").arg(string);
        kr_response_free_string (&string);
      }
      if (kr_response_to_int (response, &number)) {
          qDebug() << tr("Response Int: %1\n").arg(number);
      }
      kr_response_free (&response);
    }
  } else {
    qDebug() << tr("No response after waiting %1 ms\n").arg(wait_time_ms);
  }

    // printf ("\n");
}



