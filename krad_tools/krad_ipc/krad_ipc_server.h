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

#define CLIENT_SLOTS 40

typedef struct krad_ipc_server_client_St krad_ipc_server_client_t;

struct krad_ipc_server_St {

	struct sockaddr_un saddr;
	
	char ipc_path[320];
	struct utsname unixname;
	int fd;
	int flags;
	int ipc_path_pos;
	int linux_abstract_sockets;
	int i;
	krad_ipc_server_client_t *client;
	
	int clients;
	int broadcast_clients;
	int broadcast_clients_level[256];
	int c;
	pthread_rwlock_t send_lock;
	
	//signals
	
	sigset_t mask;
	sigset_t orig_mask;
	struct sigaction act;

	// new callback
	
	void *(*new_client_callback)(void *);

	
	// close callback
	
	void (*close_client_callback)(void *);


	// shutdown callback
	
	void (*shutdown_callback)(void *);

	// Handler callback

	int (*handler_callback)(char *, void *);
	void *client_callback_pointer;
	
};


typedef struct krad_ipc_server_St krad_ipc_server_t;

struct krad_ipc_server_client_St {

	int fd;
	//char *buffer;
	
	
	void *client_pointer;
	

	struct pollfd fds[1];
	int ret;
	int i;
	int clients;
	int msglen;
	int broadcast;// = 0;
	int bytes;// = 0;
	int rbytes;// = 0;
	//char path[128] = "";
	char buffer[8192 * 2];
	char recbuffer[8192 * 2];
	//char msg[512];
	float fval;// = 0.0;

	
	
	// hax
	
	// this is because of linked list fail
	int active;
	int broadcast_level;
	// link back
	krad_ipc_server_t *krad_ipc_server;
	pthread_t serve_client_thread;

};

void krad_ipc_server_set_client_broadcasts(krad_ipc_server_t *krad_ipc_server, void *client_pointer, int broadcast_level);

void krad_ipc_server_client_broadcast_skip (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level, krad_ipc_server_client_t *client);
void krad_ipc_server_client_broadcast (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level);
krad_ipc_server_t *krad_ipc_server_setup(const char *daemon, char *station);

krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server);

void *krad_ipc_server_client_loop(void *arg);
void krad_ipc_server_free(krad_ipc_server_t *krad_ipc_server);
int krad_ipc_server_run(krad_ipc_server_t *krad_ipc_server);

void krad_ipc_server_shutdown(krad_ipc_server_t *krad_ipc_server);
void daemonize(char *daemon_name);
void krad_ipc_server_set_new_client_callback(krad_ipc_server_t *krad_ipc_server, void *new_client_callback(void *));
void krad_ipc_server_set_close_client_callback(krad_ipc_server_t *krad_ipc_server, void close_client_callback(void *));
void krad_ipc_server_set_client_callback_pointer(krad_ipc_server_t *krad_ipc_server, void *pointer);
void krad_ipc_server_set_shutdown_callback(krad_ipc_server_t *krad_ipc_server, void shutdown_callback(void *));
void krad_ipc_server_set_handler_callback(krad_ipc_server_t *krad_ipc_server, int handler_callback(char *, void *));
void krad_ipc_server_want_shutdown();
//void krad_ipc_server_client_send (void *client, char *data);
void krad_ipc_server_signals_setup(krad_ipc_server_t *krad_ipc_server);
