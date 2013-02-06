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

#ifdef IS_LINUX
#include <ifaddrs.h>
#endif

#include "krad_radio_version.h"
#include "krad_system.h"
#include "krad_ebml.h"
#include "krad_radio_ipc.h"


#include "krad_radio_client.h"

#ifndef KRAD_IPC_SERVER_H
#define KRAD_IPC_SERVER_H


#define MAX_REMOTES 16
#define KRAD_IPC_SERVER_MAX_CLIENTS 16
#define KRAD_IPC_SERVER_TIMEOUT_MS 100
#define KRAD_IPC_SERVER_TIMEOUT_US KRAD_IPC_SERVER_TIMEOUT_MS * 1000

#define KRAD_IPC_CLIENT_DOCTYPE "krad_ipc_client"
#define KRAD_IPC_SERVER_DOCTYPE "krad_ipc_server"
#define KRAD_IPC_DOCTYPE_VERSION KRAD_VERSION
#define KRAD_IPC_DOCTYPE_READ_VERSION KRAD_VERSION

#define EBML_ID_KRAD_IPC_CMD 0x4444


#define MAX_BROADCASTS 128
#define MAX_BROADCASTERS 16

enum krad_ipc_shutdown {
  KRAD_IPC_STARTING = -1,
  KRAD_IPC_RUNNING,
  KRAD_IPC_DO_SHUTDOWN,
  KRAD_IPC_SHUTINGDOWN,

};

typedef struct krad_ipc_broadcaster_St krad_ipc_broadcaster_t;
typedef struct krad_ipc_server_client_St krad_ipc_server_client_t;
typedef struct krad_ipc_server_St krad_ipc_server_t;
typedef struct krad_broadcast_msg_St krad_broadcast_msg_t;

struct krad_broadcast_msg_St {
  unsigned char *buffer;
  uint32_t size;
  krad_ipc_server_client_t *skip_client;
};

struct krad_ipc_broadcaster_St {
  krad_ipc_server_t *ipc_server;
  krad_ringbuffer_t *msg_ring;
  int sockets[2];
};

struct krad_ipc_server_St {

  struct sockaddr_un saddr;
  struct utsname unixname;
  int on_linux;
  int sd;
  int tcp_sd[MAX_REMOTES];
  int tcp_port[MAX_REMOTES];
  char *tcp_interface[MAX_REMOTES];
  int flags;
  int shutdown;

  int socket_count;
  
  krad_control_t krad_control;
  
  krad_ipc_server_client_t *clients;
  krad_ipc_server_client_t *current_client;

  pthread_t server_thread;

  struct pollfd sockets[KRAD_IPC_SERVER_MAX_CLIENTS + MAX_BROADCASTERS + MAX_REMOTES + 2];
  krad_ipc_server_client_t *sockets_clients[KRAD_IPC_SERVER_MAX_CLIENTS + MAX_BROADCASTERS + MAX_REMOTES + 2];  

  int (*handler)(void *, int *, void *);
  void *pointer;
  
  krad_ipc_broadcaster_t *sockets_broadcasters[MAX_BROADCASTERS + MAX_REMOTES + 2];
  krad_ipc_broadcaster_t *broadcasters[MAX_BROADCASTERS];
  int broadcasters_count;  
  
  uint32_t broadcasts[MAX_BROADCASTS];
  int broadcasts_count;
  
};

struct krad_ipc_server_client_St {

  krad_ipc_server_t *krad_ipc_server;

  krad_ebml_t *krad_ebml;
  krad_ebml_t *krad_ebml2;
  
  int confirmed;
  
  int sd;

  char input_buffer[4096 * 2];
  char output_buffer[4096 * 2];
  //char command_buffer[4096 * 4];
  int command_response_len;
  
  int input_buffer_pos;
  int output_buffer_pos;

  int active;
  int broadcasts;
};

int krad_ipc_server_recvfd (krad_ipc_server_client_t *client);
void krad_ipc_server_add_client_to_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t broadcast_ebml_id );

void krad_ipc_server_response_start_with_address_and_type ( krad_ipc_server_t *krad_ipc_server,
                                                            kr_address_t *address,
                                                            uint32_t message_type,
                                                            uint64_t *response);

void krad_ipc_server_write_message_type (krad_ipc_server_t *krad_ipc_server, uint32_t message_type);

void krad_ipc_server_payload_start ( krad_ipc_server_t *krad_ipc_server, uint64_t *payload);
void krad_ipc_server_payload_finish ( krad_ipc_server_t *krad_ipc_server, uint64_t payload);
void krad_ipc_server_response_start ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t *response);
void krad_ipc_server_response_finish ( krad_ipc_server_t *krad_ipc_server, uint64_t response);

void krad_ipc_server_respond_string ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, char *string);
void krad_ipc_server_response_list_start ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t *list);
void krad_ipc_server_response_add_tag ( krad_ipc_server_t *krad_ipc_server, char *tag_item, char *tag_name, char *tag_value);
void krad_ipc_server_response_list_finish ( krad_ipc_server_t *krad_ipc_server, uint64_t list);
void krad_ipc_server_read_tag ( krad_ipc_server_t *krad_ipc_server, char **tag_item, char **tag_name, char **tag_value );
void krad_ipc_server_respond_number ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, int32_t number);
int krad_ipc_server_read_command (krad_ipc_server_t *krad_ipc_server, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr);
uint64_t krad_ipc_server_read_number (krad_ipc_server_t *krad_ipc_server, uint64_t data_size);


int krad_broadcast_msg_destroy (krad_broadcast_msg_t **broadcast_msg);
krad_broadcast_msg_t *krad_broadcast_msg_create (unsigned char *buffer, uint32_t size);

int krad_ipc_server_broadcaster_broadcast ( krad_ipc_broadcaster_t *broadcaster, krad_broadcast_msg_t **broadcast_msg );

void krad_ipc_server_broadcaster_register_broadcast ( krad_ipc_broadcaster_t *broadcaster, uint32_t broadcast_ebml_id );
krad_ipc_broadcaster_t *krad_ipc_server_broadcaster_register ( krad_ipc_server_t *ipc_server );
int krad_ipc_server_broadcaster_unregister ( krad_ipc_broadcaster_t **broadcaster );

int krad_ipc_server_current_client_is_subscriber (krad_ipc_server_t *ipc);

void krad_ipc_server_disable_remote (krad_ipc_server_t *krad_ipc_server, char *interface, int port);
int krad_ipc_server_enable_remote (krad_ipc_server_t *krad_ipc_server, char *interface, int port);
void krad_ipc_server_disable (krad_ipc_server_t *krad_ipc_server);
void krad_ipc_server_destroy (krad_ipc_server_t *ipc_server);
void krad_ipc_server_run (krad_ipc_server_t *krad_ipc_server);
krad_ipc_server_t *krad_ipc_server_create (char *appname, char *sysname, int handler (void *, int *, void *), void *pointer);

#endif

