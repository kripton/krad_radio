#include "krad_radio.h"

#include "krad_radio_client.h"



//int krad_radio_broadcast_subunit_destroyed (krad_broadcaster_t *krad_broadcaster, int unit, int subunit, kr_subunit_address_t *address);
int krad_radio_broadcast_subunit_destroyed (krad_ipc_broadcaster_t *broadcaster, kr_address_t *address);
int krad_radio_handler ( void *output, int *output_len, void *ptr );
