#ifndef KRAD_RECEIVER_H
#define KRAD_RECEIVER_H

typedef struct krad_receiver_St krad_receiver_t;
typedef struct krad_receiver_client_St krad_receiver_client_t;

#include "krad_radio.h"

struct krad_receiver_St {
  krad_transponder_t *krad_transponder;
	unsigned char *buffer;
	int port;
	int sd;
	struct sockaddr_in local_address;
	int listening;
	int stop_listening;
	pthread_t listening_thread;
};

struct krad_receiver_client_St {

	krad_receiver_t *krad_receiver;
	pthread_t client_thread;

	char in_buffer[1024];
	char out_buffer[1024];
	char mount[256];
	char content_type[256];
	int got_mount;
	int got_content_type;

	int in_buffer_pos;
	int out_buffer_pos;
	
	int sd;
	int ret;
	int wrote;
};

void krad_receiver_stop_listening (krad_receiver_t *krad_receiver);
int krad_receiver_listen_on (krad_receiver_t *krad_receiver, int port);

void krad_receiver_destroy (krad_receiver_t *krad_receiver);
krad_receiver_t *krad_receiver_create (krad_transponder_t *krad_transponder);

void *krad_receiver_thread (void *arg);
void krad_receiver_destroy_client (krad_receiver_client_t *krad_receiver_client);
void krad_receiver_create_client (krad_receiver_t *krad_receiver, int sd);
void *krad_receiver_client_thread (void *arg);

#endif
