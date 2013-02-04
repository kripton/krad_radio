//#ifndef KRAD_CLIENT_INTERNAL_H
//#define KRAD_CLIENT_INTERNAL_H

#include "krad_ebml.h"
#include "krad_ipc_client.h"
#include "krad_radio_client.h"

typedef struct kr_item_St kr_item_t;

struct kr_item_St {
  uint32_t type;
  unsigned char *buffer;
  uint32_t size;
};

struct kr_response_St {
  kr_client_t *kr_client;

  kr_address_t address;
  uint32_t type;
  
  /* get rid of me */
  kr_item_t kr_item;
  /* end rid of me */

  unsigned char *buffer;
  uint32_t size;  
};

struct kr_client_St {
  krad_ipc_client_t *krad_ipc_client;
  krad_ebml_t *krad_ebml;
  char *name;
};

struct kr_shm_St {
  int fd;
  char *buffer;
  uint64_t size;
};

kr_rep_t *kr_item_to_rep (kr_item_t *kr_item);
int kr_item_to_string (kr_item_t *kr_item, char **string);
int kr_item_read_into_string (kr_item_t *kr_item, char *string);
const char *kr_item_get_type_string (kr_item_t *item);
int kr_response_list_get_item (kr_response_t *kr_response, int item_num, kr_item_t **kr_item);
int kr_response_get_item (kr_response_t *kr_response, kr_item_t **item);

typedef int (*item_callback_t)( unsigned char *, uint64_t, char ** );


krad_ebml_t *kr_client_get_ebml (kr_client_t *kr_client);
int kr_send_fd (kr_client_t *client, int fd);


//#endif
