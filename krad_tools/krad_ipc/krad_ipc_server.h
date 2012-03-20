#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/utsname.h>

#include <syslog.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
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

#define KRAD_IPC_MAX_CLIENTS 35

typedef struct krad_ipc_server_client_St krad_ipc_server_client_t;
typedef struct krad_ipc_server_St krad_ipc_server_t;

struct krad_ipc_server_St {

	struct sockaddr_un saddr;
	char ipc_path[512];
	struct utsname unixname;
	int fd;
	int flags;
	int linux_abstract_sockets;
	int i;
	
	krad_ipc_server_client_t *client;
	
	int clients;
	int broadcast_clients;
	int broadcast_clients_level[256];
	int c;
	pthread_rwlock_t send_lock;


	pthread_t server_thread;

	int (*handler)(void *, void *, int *, void *);
	void *pointer;
	
};

struct krad_ipc_server_client_St {

	krad_ipc_server_t *krad_ipc_server;

	int fd;	
	struct pollfd fds[1];
	int ret;
	int i;
	int clients;
	int msglen;
	int broadcast;
	int bytes;
	int rbytes;

	char buffer[4096 * 4];
	char recbuffer[4096 * 4];
	float fval;


	int active;
	int broadcast_level;

	pthread_t serve_client_thread;

};

void krad_ipc_server_set_client_broadcasts(krad_ipc_server_t *krad_ipc_server, void *client_pointer, int broadcast_level);
void krad_ipc_server_client_broadcast_skip (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level, krad_ipc_server_client_t *client);
void krad_ipc_server_client_broadcast (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level);

krad_ipc_server_t *krad_ipc_server_create (char *callsign_or_ipc_path_or_port);
krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server);
void *krad_ipc_server_client_loop(void *arg);

//void krad_ipc_server_client_send (void *client, char *data);

void *krad_ipc_server_run (void *arg);
void krad_ipc_server_destroy (krad_ipc_server_t *krad_ipc_server);

krad_ipc_server_t *krad_ipc_server (char *callsign_or_ipc_path_or_port, int handler (void *, void *, int *, void *), void *ptr);

