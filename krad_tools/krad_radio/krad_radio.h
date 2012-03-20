#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>

#include "krad_ipc_server.h"

typedef struct krad_radio_St krad_radio_t;

struct krad_radio_St {

	char *name;
	char *callsign;
	krad_ipc_server_t *ipc;

};



void krad_radio_destroy(krad_radio_t *krad_radio);
krad_radio_t *krad_radio_create(char *callsign_or_config);
void krad_radio_daemonize ();
void krad_radio_run (krad_radio_t *krad_radio);
void krad_radio (char *callsign_or_config);

int krad_radio_handler ( void *input, void *output, int *output_len, void *ptr );
