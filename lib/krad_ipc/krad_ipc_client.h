#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/utsname.h>
#include <time.h>
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
#include <sys/wait.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/mman.h>

#include "krad_radio_version.h"
#include "krad_radio_ipc.h"
#include "krad_system.h"
#include "krad_ebml.h"



#define KRAD_IPC_BUFFER_SIZE 16384
#ifndef KRAD_IPC_CLIENT
#define KRAD_IPC_CLIENT 1

#define KRAD_IPC_CLIENT_DOCTYPE "krad_ipc_client"
#define KRAD_IPC_SERVER_DOCTYPE "krad_ipc_server"
#define KRAD_IPC_DOCTYPE_VERSION KRAD_VERSION
#define KRAD_IPC_DOCTYPE_READ_VERSION KRAD_VERSION
#define EBML_ID_KRAD_IPC_CMD 0x4444

typedef struct krad_ipc_client_St krad_ipc_client_t;

struct krad_ipc_client_St {
	char sysname[64];
	int flags;
	struct sockaddr_un saddr;
	int sd;
	int tcp_port;
	char host[256];
	char ipc_path[256];
	int ipc_path_pos;
	int on_linux;
	struct stat info;
	struct utsname unixname;
	char *buffer;
	krad_ebml_t *krad_ebml;
	
	int (*handler)(krad_ipc_client_t *, void *);
	void *ptr;
	
	int nowait;

};

/*** Krad IPC ***/


krad_ipc_client_t *krad_ipc_connect (char *sysname);
void krad_ipc_disconnect (krad_ipc_client_t *client);

void krad_ipc_broadcast_subscribe (krad_ipc_client_t *client, uint32_t broadcast_id);
void krad_ipc_set_handler_callback (krad_ipc_client_t *client, int handler (krad_ipc_client_t *, void *), void *ptr);


void krad_ipc_send (krad_ipc_client_t *client, char *cmd);
int krad_ipc_recv (krad_ipc_client_t *client, char *buffer, int size);
void krad_ipc_client_handle (krad_ipc_client_t *client);

int krad_ipc_client_send_fd (krad_ipc_client_t *client, int fd);

#endif
