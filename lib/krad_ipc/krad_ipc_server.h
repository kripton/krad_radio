#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#include "krad_radio_version.h"
#include "krad_system.h"
#include "krad_ebml.h"
#include "krad_radio_ipc.h"

#ifndef KRAD_IPC_SERVER_H
#define KRAD_IPC_SERVER_H

#define KRAD_IPC_SERVER_MAX_CLIENTS 64
#define KRAD_IPC_SERVER_TIMEOUT_MS 100
#define KRAD_IPC_SERVER_TIMEOUT_US KRAD_IPC_SERVER_TIMEOUT_MS * 1000

#define KRAD_IPC_CLIENT_DOCTYPE "krad_ipc_client"
#define KRAD_IPC_SERVER_DOCTYPE "krad_ipc_server"
#define KRAD_IPC_DOCTYPE_VERSION KRAD_VERSION
#define KRAD_IPC_DOCTYPE_READ_VERSION KRAD_VERSION

#define EBML_ID_KRAD_IPC_CMD 0x4444


#define MAX_BROADCASTS 128


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
	int tcp_sd;
	int tcp_port;		
	int flags;
	int shutdown;

	int socket_count;
	
	krad_ipc_server_client_t *clients;
	krad_ipc_server_client_t *current_client;

	pthread_t server_thread;

	struct pollfd sockets[KRAD_IPC_SERVER_MAX_CLIENTS + 2];
	krad_ipc_server_client_t *sockets_clients[KRAD_IPC_SERVER_MAX_CLIENTS + 2];	

	int (*handler)(void *, int *, void *);
	void *pointer;
	
	
	uint32_t broadcasts[MAX_BROADCASTS];
	int broadcasts_count;
	
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

	pthread_mutex_t client_lock;


	int broadcasts;

};

int krad_ipc_server_recvfd (krad_ipc_server_client_t *client);

void krad_ipc_release_client (krad_ipc_server_client_t *client);
int krad_ipc_aquire_client (krad_ipc_server_client_t *client);

void krad_ipc_server_register_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t broadcast_ebml_id );
void krad_ipc_server_add_client_to_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t broadcast_ebml_id );

void krad_ipc_server_simplest_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint32_t ebml_subid, uint32_t val);


void krad_ipc_server_advanced_string_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint32_t ebml_subid, uint32_t ebml_subid2, int num, uint32_t ebml_subid3, char *string);
void krad_ipc_server_advanced_number_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint32_t ebml_subid, uint32_t ebml_subid2, int num, uint32_t ebml_subid3, int num2);

void krad_ipc_server_broadcast_tag ( krad_ipc_server_t *krad_ipc_server, char *item, char *name, char *value, int internal);

void krad_ipc_server_respond_string ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, char *string);

void krad_ipc_server_response_finish ( krad_ipc_server_t *krad_ipc_server, uint64_t response);
void krad_ipc_server_response_start ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t *response);
void krad_ipc_server_response_list_start ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t *list);
void krad_ipc_server_response_add_tag ( krad_ipc_server_t *krad_ipc_server, char *tag_item, char *tag_name, char *tag_value);
void krad_ipc_server_response_list_finish ( krad_ipc_server_t *krad_ipc_server, uint64_t list);


void krad_ipc_server_response_add_portgroup ( krad_ipc_server_t *krad_ipc_server, char *name, int channels,
											  int io_type, float volume,  char *mixgroup, char *crossfade_name, float crossfade_value, int xmms2 );
											  
void krad_ipc_server_read_tag ( krad_ipc_server_t *krad_ipc_server, char **tag_item, char **tag_name, char **tag_value );



void krad_ipc_server_set_client_broadcasts(krad_ipc_server_t *krad_ipc_server, void *client_pointer, int broadcast_level);
void krad_ipc_server_client_broadcast_skip (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level, krad_ipc_server_client_t *client);
void krad_ipc_server_client_broadcast (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level);

void krad_ipc_disconnect_client (krad_ipc_server_client_t *client);
void krad_ipc_server_update_pollfds (krad_ipc_server_t *krad_ipc_server);
krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server, int sd);

void krad_ipc_server_broadcast_portgroup_created ( krad_ipc_server_t *krad_ipc_server, char *name, int channels,
											  	   int io_type, float volume, char *mixbus, int xmms2 );
void krad_ipc_server_mixer_broadcast2 ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint32_t ebml_subid, char *portname, uint32_t ebml_subid2, char *string);
void krad_ipc_server_simple_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint32_t ebml_subid, uint32_t ebml_subid2, char *string);
void krad_ipc_server_simple_number_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint32_t ebml_subid, uint32_t ebml_subid2, int num);
void krad_ipc_server_mixer_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint32_t ebml_subid, char *portname, char *controlname, float floatval);
void krad_ipc_server_respond_number ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, int32_t number);
int krad_ipc_server_read_command (krad_ipc_server_t *krad_ipc_server, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr);
uint64_t krad_ipc_server_read_number (krad_ipc_server_t *krad_ipc_server, uint64_t data_size);
//void krad_ipc_server_client_send (void *client, char *data);

void *krad_ipc_server_run_thread (void *arg);
void krad_ipc_server_disable (krad_ipc_server_t *krad_ipc_server);
void krad_ipc_server_destroy (krad_ipc_server_t *krad_ipc_server);
void krad_ipc_server_run (krad_ipc_server_t *krad_ipc_server);
int krad_ipc_server_tcp_socket_create (int port);
void krad_ipc_server_disable_remote (krad_ipc_server_t *krad_ipc_server);
int krad_ipc_server_enable_remote (krad_ipc_server_t *krad_ipc_server, int port);
krad_ipc_server_t *krad_ipc_server_init (char *sysname);
krad_ipc_server_t *krad_ipc_server_create (char *sysname, int handler (void *, int *, void *), void *pointer);

#endif

