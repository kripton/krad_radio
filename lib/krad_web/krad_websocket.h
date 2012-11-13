#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <pthread.h>

#include <libwebsockets.h>

#include "krad_system.h"
#include "krad_mixer_common.h"
#include "krad_ipc_client.h"
#include "krad_radio_ipc.h"

#include "cJSON.h"

#define KRAD_WEBSOCKET_MAX_POLL_FDS 200
#define KRAD_WEBSOCKET_SERVER_TIMEOUT_MS 25
#define KRAD_WEBSOCKET_SERVER_TIMEOUT_US KRAD_WEBSOCKET_SERVER_TIMEOUT_MS * 1000

enum fdclass {
	MYSTERY = 0,
	KRAD_IPC = 1,
};

enum krad_websocket_shutdown {
	KRAD_WEBSOCKET_STARTING = -1,
	KRAD_WEBSOCKET_RUNNING,
	KRAD_WEBSOCKET_DO_SHUTDOWN,
	KRAD_WEBSOCKET_SHUTINGDOWN,

};

typedef struct krad_websocket_St krad_websocket_t;
typedef struct krad_ipc_session_data_St krad_ipc_session_data_t;

struct krad_ipc_session_data_St {

	krad_websocket_t *krad_websocket;
	krad_ipc_client_t *krad_ipc_client;
	cJSON *msgs;
	char *msgstext;
	int msgstextlen;
	int krad_ipc_info;
//	int krad_ipc_data_len;
//	char krad_ipc_data[4096 * 4];
	struct libwebsocket_context *context;
	struct libwebsocket *wsi;
	
	int hello_sent;
	
};

struct krad_websocket_St {
	krad_ipc_session_data_t *sessions[KRAD_WEBSOCKET_MAX_POLL_FDS];
	struct pollfd pollfds[KRAD_WEBSOCKET_MAX_POLL_FDS];
	int count_pollfds;
	enum fdclass fdof[KRAD_WEBSOCKET_MAX_POLL_FDS];
	unsigned char *buffer;
	struct libwebsocket_context *context;

	int port;
	char sysname[64];
	pthread_t server_thread;
	int shutdown;
};

int callback_http (struct libwebsocket_context *this, struct libwebsocket *wsi,
				   enum libwebsocket_callback_reasons reason, void *user,
				   void *in, size_t len);

int callback_krad_ipc (struct libwebsocket_context * this,
					   struct libwebsocket *wsi,
					   enum libwebsocket_callback_reasons reason, void *user,
					   void *in, size_t len);

void set_poll_mode_pollfd(struct pollfd *apollfd, short events);
void set_poll_mode_fd(int fd, short events);

void krad_websocket_server_destroy (krad_websocket_t *krad_websocket);
krad_websocket_t *krad_websocket_server_create (char *sysname, int port);
void *krad_websocket_server_run (void *arg);
