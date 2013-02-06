#include "krad_radio.h"
#include "krad_radio_client.h"

int krad_radio_broadcast_subunit_control (krad_ipc_broadcaster_t *broadcaster, kr_address_t *address_in, int control, float value, void *client);
int krad_radio_broadcast_subunit_destroyed (krad_ipc_broadcaster_t *broadcaster, kr_address_t *address);
int krad_radio_handler ( void *output, int *output_len, void *ptr );
