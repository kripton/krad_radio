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

typedef struct {
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

} krad_ipc_client_t;

int krad_ipc_client_check (krad_ipc_client_t *client, int *value);
int krad_ipc_cmd2 (krad_ipc_client_t *client, int value);


krad_ipc_client_t *krad_ipc_connect ();
int krad_ipc_client_init (krad_ipc_client_t *client);
void krad_ipc_disconnect (krad_ipc_client_t *client);
int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd);
void krad_ipc_send (krad_ipc_client_t *client, char *cmd);
int krad_ipc_wait (krad_ipc_client_t *client, char *buffer, int size);
int krad_ipc_recv (krad_ipc_client_t *client, char *buffer, int size);
#endif
