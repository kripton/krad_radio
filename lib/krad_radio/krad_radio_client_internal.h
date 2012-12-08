//#ifndef KRAD_CLIENT_INTERNAL_H
//#define KRAD_CLIENT_INTERNAL_H

#include "krad_ebml.h"
#include "krad_ipc_client.h"

typedef struct kr_client_St kr_client_t;
typedef struct kr_shm_St kr_shm_t;

struct kr_client_St {
  krad_ipc_client_t *krad_ipc_client;
  krad_ebml_t *krad_ebml;
};

struct kr_shm_St {
  int fd;
  char *buffer;
  uint64_t size;
};

krad_ebml_t *kr_client_get_ebml (kr_client_t *kr_client);
int kr_send_fd (kr_client_t *client, int fd);


//#endif
