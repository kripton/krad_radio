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

#define KRAD_IPC_BUFFER_SIZE 16384
#ifndef KRAD_IPC_CLIENT
#define KRAD_IPC_CLIENT 1

typedef struct {
	int flags;
	struct sockaddr_un saddr;
	int fd;
	char ipc_path[256];
	int ipc_path_pos;
	int linux_abstract_sockets;
	struct stat info;
	struct utsname unixname;
	char *buffer;

} krad_ipc_client_t;


krad_ipc_client_t *krad_ipc_connect ();
int krad_ipc_client_init (krad_ipc_client_t *client);
void krad_ipc_disconnect (krad_ipc_client_t *client);
int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd);
void krad_ipc_send (krad_ipc_client_t *client, char *cmd);
int krad_ipc_wait (krad_ipc_client_t *client, char *buffer, int size);
int krad_ipc_recv (krad_ipc_client_t *client, char *buffer, int size);
#endif
