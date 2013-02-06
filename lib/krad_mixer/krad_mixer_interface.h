#include "krad_mixer.h"
#include "krad_radio_interface.h"

kr_portgroup_t *krad_mixer_portgroup_to_rep (krad_mixer_portgroup_t *portgroup,
                                             kr_portgroup_t *portgroup_rep);

int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc );
