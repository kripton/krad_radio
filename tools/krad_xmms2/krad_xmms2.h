#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <signal.h>

#include <xmmsclient/xmmsclient.h>

#include "krad_system.h"

typedef struct krad_xmms_St krad_xmms_t;

struct krad_xmms_St {

	char sysname[256];
	char ipc_path[256];
	xmmsc_connection_t *connection;
	int fd;
	int connected;
	
	

};

void krad_xmms_register_for_broadcasts (krad_xmms_t *krad_xmms);
void krad_xmms_unregister_for_broadcasts (krad_xmms_t *krad_xmms);

void krad_xmms_connect (krad_xmms_t *krad_xmms);
void krad_xmms_disconnect (krad_xmms_t *krad_xmms);

void krad_xmms_destroy (krad_xmms_t *krad_xmms);
krad_xmms_t *krad_xmms_create (char *name, char *ipc_path);

