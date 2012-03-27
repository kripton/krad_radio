#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>

#include <sys/utsname.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <pthread.h>

#include "krad_ebml.h"

#define KRAD_IPC_BUFFER_SIZE 16384
#ifndef KRAD_IPC_CLIENT
#define KRAD_IPC_CLIENT 1

#define KRAD_IPC_CLIENT_DOCTYPE "krad_ipc_client"
#define KRAD_IPC_SERVER_DOCTYPE "krad_ipc_server"
#define KRAD_IPC_DOCTYPE_VERSION 6
#define KRAD_IPC_DOCTYPE_READ_VERSION 6
#define EBML_ID_KRAD_IPC_CMD 0x4444

typedef struct krad_ipc_client_St krad_ipc_client_t;

struct krad_ipc_client_St {
	int flags;
	struct sockaddr_un saddr;
	int sd;
	char ipc_path[256];
	int ipc_path_pos;
	int on_linux;
	struct stat info;
	struct utsname unixname;
	char *buffer;
	krad_ebml_t *krad_ebml;
	
	int (*handler)(krad_ipc_client_t *, void *);
	void *ptr;
	

};


void krad_ipc_create_link (krad_ipc_client_t *client, char *name);
void krad_ipc_list_links (krad_ipc_client_t *client);


int krad_ipc_client_read_portgroup ( krad_ipc_client_t *client, char *portname, float *volume );

int krad_ipc_client_read_mixer_control ( krad_ipc_client_t *client, char **portgroup_name, char **control_name, float *value );
void krad_ipc_set_handler_callback (krad_ipc_client_t *client, int handler (krad_ipc_client_t *, void *), void *ptr);

void krad_ipc_get_portgroups (krad_ipc_client_t *client);
void krad_ipc_set_control (krad_ipc_client_t *client, char *portgroup_name, char *control_name, float control_value);
void krad_ipc_print_response (krad_ipc_client_t *client);
void krad_ipc_get_tags (krad_ipc_client_t *client);

void krad_ipc_get_tag (krad_ipc_client_t *client, char *tag_name);
void krad_ipc_set_tag (krad_ipc_client_t *client, char *tag_name, char *tag_value);

void krad_ipc_client_handle (krad_ipc_client_t *client);
int krad_ipc_client_poll (krad_ipc_client_t *client);
int krad_ipc_cmd2 (krad_ipc_client_t *client, int value);


krad_ipc_client_t *krad_ipc_connect ();
int krad_ipc_client_init (krad_ipc_client_t *client);
void krad_ipc_disconnect (krad_ipc_client_t *client);
int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd);
void krad_ipc_send (krad_ipc_client_t *client, char *cmd);
int krad_ipc_wait (krad_ipc_client_t *client, char *buffer, int size);
int krad_ipc_recv (krad_ipc_client_t *client, char *buffer, int size);
#endif
