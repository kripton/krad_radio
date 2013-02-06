//#ifndef KRAD_CLIENT_INTERNAL_H
//#define KRAD_CLIENT_INTERNAL_H

#include "krad_ebml.h"
#include "krad_ipc_client.h"
#include "krad_radio_client.h"

struct kr_response_St {
  kr_client_t *kr_client;

  kr_address_t address;
  uint32_t type;

  unsigned char *buffer;
  uint32_t size;  
};

struct kr_client_St {
  krad_ipc_client_t *krad_ipc_client;
  krad_ebml_t *krad_ebml;
  char *name;
  
  int subscriber;
  int last_delivery_was_final;
};

struct kr_shm_St {
  int fd;
  char *buffer;
  uint64_t size;
};

typedef int (*rep_to_string_t)( unsigned char *, uint64_t, char ** );

krad_ebml_t *kr_client_get_ebml (kr_client_t *kr_client);
int kr_send_fd (kr_client_t *client, int fd);


//#endif
