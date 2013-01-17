#include "krad_mixer.h"
int krad_mixer_broadcast_portgroup_control (krad_mixer_t *krad_mixer, char *portgroupname, char *controlname, float value);
int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc );
