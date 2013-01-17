#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <pthread.h>

#include "kr_client.h"

#include <libwebsockets.h>
#include "cJSON.h"

#define KRAD_WEBSOCKET_MAX_POLL_FDS 200

enum fdclass {
  MYSTERY = 0,
  KRAD_IPC = 1,
  KRAD_CONTROLLER,
};

enum krad_websocket_shutdown {
  KRAD_WEBSOCKET_STARTING = -1,
  KRAD_WEBSOCKET_RUNNING,
  KRAD_WEBSOCKET_DO_SHUTDOWN,
  KRAD_WEBSOCKET_SHUTINGDOWN,
};

typedef struct krad_websocket_St krad_websocket_t;
typedef struct kr_ws_client_St kr_ws_client_t;

struct kr_ws_client_St {
  krad_websocket_t *krad_websocket;
  kr_client_t *kr_client;
  cJSON *msgs;
  char *msgstext;
  int msgstextlen;
  int kr_client_info;
  int hello_sent;
  struct libwebsocket_context *context;
  struct libwebsocket *wsi;
};

struct krad_websocket_St {
  kr_ws_client_t *sessions[KRAD_WEBSOCKET_MAX_POLL_FDS];
  struct pollfd pollfds[KRAD_WEBSOCKET_MAX_POLL_FDS];
  int count_pollfds;
  enum fdclass fdof[KRAD_WEBSOCKET_MAX_POLL_FDS];
  unsigned char *buffer;
  struct libwebsocket_context *context;

  int port;
  char sysname[64];
  pthread_t server_thread;
  int shutdown;
  krad_control_t krad_control;
};

void krad_websocket_server_destroy (krad_websocket_t *krad_websocket);
krad_websocket_t *krad_websocket_server_create (char *sysname, int port);
