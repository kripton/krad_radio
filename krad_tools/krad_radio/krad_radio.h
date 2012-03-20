#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>

typedef struct krad_radio_St krad_radio_t;

struct krad_radio_St {

	char *name;
	char *callsign;
//	krad_ipc_server_t *ipc;

};



void krad_radio_destroy(krad_radio_t *radio);
krad_radio_t *krad_radio_create();
void krad_radio_daemonize ();

void krad_radio (char *station_or_config);
