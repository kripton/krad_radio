#include "krad_ebml.h"
#include "krad_ipc_client.h"
#include "krad_radio_client.h"

struct kr_client_St {
  krad_ipc_client_t *krad_ipc_client;
  krad_ebml_t *krad_ebml;
};

krad_ebml_t *kr_client_get_ebml (kr_client_t *kr_client);
