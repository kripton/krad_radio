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

#include "krad_ebml.h"

#include "krad_radio_ipc.h"

#ifndef KRAD_IPC_SERVER_H
#define KRAD_IPC_SERVER_H

#define KRAD_IPC_SERVER_MAX_CLIENTS 35
#define KRAD_IPC_SERVER_TIMEOUT_MS 250
#define KRAD_IPC_SERVER_TIMEOUT_US KRAD_IPC_SERVER_TIMEOUT_MS * 1000

#define KRAD_IPC_CLIENT_DOCTYPE "krad_ipc_client"
#define KRAD_IPC_SERVER_DOCTYPE "krad_ipc_server"
#define KRAD_IPC_DOCTYPE_VERSION 6
#define KRAD_IPC_DOCTYPE_READ_VERSION 6

#define EBML_ID_KRAD_IPC_CMD 0x4444

enum krad_ipc_shutdown {
	KRAD_IPC_STARTING = -1,
	KRAD_IPC_RUNNING,
	KRAD_IPC_DO_SHUTDOWN,
	KRAD_IPC_SHUTINGDOWN,

};


typedef struct krad_ipc_server_client_St krad_ipc_server_client_t;
typedef struct krad_ipc_server_St krad_ipc_server_t;

struct krad_ipc_server_St {

	struct sockaddr_un saddr;
	struct utsname unixname;
	int on_linux;
	int sd;
	int flags;
	int shutdown;

	int socket_count;
	
	krad_ipc_server_client_t *clients;
	krad_ipc_server_client_t *current_client;

	pthread_t server_thread;

	struct pollfd sockets[KRAD_IPC_SERVER_MAX_CLIENTS + 1];
	krad_ipc_server_client_t *sockets_clients[KRAD_IPC_SERVER_MAX_CLIENTS + 1];	

	int (*handler)(void *, int *, void *);
	void *pointer;
	
};

struct krad_ipc_server_client_St {

	krad_ipc_server_t *krad_ipc_server;

	krad_ebml_t *krad_ebml;
	krad_ebml_t *krad_ebml2;
	
	int confirmed;
	
	int sd;

	char input_buffer[4096 * 4];
	char output_buffer[4096 * 4];
	char command_buffer[4096 * 4];
	int command_response_len;
	
	int input_buffer_pos;
	int output_buffer_pos;

	int active;

};


void krad_ipc_server_respond_list_start ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t *list);
void krad_ipc_server_respond_add_tag ( krad_ipc_server_t *krad_ipc_server, char *tag_name, char *tag_value);
void krad_ipc_server_respond_list_finish ( krad_ipc_server_t *krad_ipc_server, uint64_t list);


void krad_ipc_server_read_tag ( krad_ipc_server_t *krad_ipc_server, char **tag_name, char **tag_value );



void krad_ipc_server_set_client_broadcasts(krad_ipc_server_t *krad_ipc_server, void *client_pointer, int broadcast_level);
void krad_ipc_server_client_broadcast_skip (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level, krad_ipc_server_client_t *client);
void krad_ipc_server_client_broadcast (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level);

void krad_ipc_disconnect_client (krad_ipc_server_client_t *client);
void krad_ipc_server_update_pollfds (krad_ipc_server_t *krad_ipc_server);
krad_ipc_server_t *krad_ipc_server_create (char *callsign_or_ipc_path_or_port);
krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server);

void krad_ipc_server_broadcast_number ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t number);
void krad_ipc_server_respond_number ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t number);
int krad_ipc_server_read_command (krad_ipc_server_t *krad_ipc_server, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr);
uint64_t krad_ipc_server_read_number (krad_ipc_server_t *krad_ipc_server, uint64_t data_size);
//void krad_ipc_server_client_send (void *client, char *data);

void *krad_ipc_server_run (void *arg);
void krad_ipc_server_destroy (krad_ipc_server_t *krad_ipc_server);

krad_ipc_server_t *krad_ipc_server (char *callsign_or_ipc_path_or_port, int handler (void *, int *, void *), void *pointer);

#endif

