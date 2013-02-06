#include "krad_radio.h"
#include "krad_radio_client.h"

#ifndef KRAD_RADIO_INTERFACE_H
#define KRAD_RADIO_INTERFACE_H

typedef union {
  krad_mixer_portgroup_t *portgroup;
} krad_subunit_ptr_t;

int krad_radio_broadcast_subunit_created ( krad_mixer_t *krad_mixer, kr_address_t *address, void *subunit_in );
int krad_radio_broadcast_subunit_control (krad_ipc_broadcaster_t *broadcaster, kr_address_t *address_in, int control, float value, void *client);
int krad_radio_broadcast_subunit_destroyed (krad_ipc_broadcaster_t *broadcaster, kr_address_t *address);
int krad_radio_handler ( void *output, int *output_len, void *ptr );

#endif
