#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/un.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <pthread.h>

#include "krad_ipc_client.h"


typedef struct {

	krad_ipc_client_t *krad_ipc_client;
	char ipc_path[320];

} krad_mixer_ipc_client_t;

krad_mixer_ipc_client_t *krad_mixer_connect ();

int krad_mixer_create_portgroup (krad_mixer_ipc_client_t *client, char *name);
int krad_mixer_destroy_portgroup (krad_mixer_ipc_client_t *client, char *name);
int krad_mixer_wait (krad_mixer_ipc_client_t *client, char *buffer, int size);
int krad_mixer_recv (krad_mixer_ipc_client_t *client, char *buffer, int size);
int krad_mixer_cmd (krad_mixer_ipc_client_t *client, char *cmd);
void krad_mixer_send (krad_mixer_ipc_client_t *client, char *cmd);
krad_mixer_ipc_client_t *krad_mixer_disconnect (krad_mixer_ipc_client_t *client);
