
#include "krad_radio.h"


void *krad_transponder_listening_thread (void *arg);
void krad_transponder_listen_destroy_client (krad_transponder_listen_client_t *krad_transponder_listen_client);
void krad_transponder_listen_create_client (krad_transponder_t *krad_transponder, int sd);
void *krad_transponder_listen_client_thread (void *arg);
