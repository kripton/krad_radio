#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <arpa/inet.h>

#ifndef KRAD_TRANSMITTER_H
#define KRAD_TRANSMITTER_H


#ifndef __MACH__
#include <sys/epoll.h>
#endif

#include "krad_radio_version.h"
#include "krad_system.h"
#include "krad_ring.h"

#define DEFAULT_MAX_RECEIVERS_PER_TRANSMISSION 128
#define DEFAULT_MAX_TRANSMISSIONS 32
#define TOTAL_RECEIVERS DEFAULT_MAX_RECEIVERS_PER_TRANSMISSION * DEFAULT_MAX_TRANSMISSIONS
#define KRAD_TRANSMITTER_SERVER APPVERSION

#define KRAD_TRANSMITTER_MAXEVENTS 64
#define DEFAULT_RING_SIZE 100000000


typedef enum {
	IS_FILE = 3150,
	IS_TCP,
} krad_transmission_receiver_type_t;


typedef struct krad_transmitter_St krad_transmitter_t;
typedef struct krad_transmission_St krad_transmission_t;
typedef struct krad_transmission_receiver_St krad_transmission_receiver_t;

struct krad_transmitter_St {

	char not_found[256];
	int not_found_len;

	int port;
	struct sockaddr_in local_address;

	int incoming_connections_sd;
	int incoming_connections_efd;

	krad_transmission_t *krad_transmissions;
	
	pthread_t listening_thread;
	pthread_rwlock_t krad_transmissions_rwlock;
	
	struct epoll_event *incoming_connection_events;
	struct epoll_event event;
	
	int listening;
	int stop_listening;
	
	krad_transmission_receiver_t *krad_transmission_receivers;
	
};

struct krad_transmission_St {

	krad_transmitter_t *krad_transmitter;
	int active;
	int ready;
	char sysname[256];
	char content_type[256];	

	char http_header[256];
	uint64_t http_header_len;

	unsigned char *header;
	uint64_t header_len;

	int connections_efd;
	
	struct epoll_event *transmission_events;
	struct epoll_event event;


	krad_ringbuffer_t *ringbuffer;
	krad_ringbuffer_t *transmission_ringbuffer;
	krad_ringbuffer_data_t tx_vec[2];
	unsigned char *transmission_buffer;
	
	uint64_t new_data;
	unsigned char *new_data_buf;
	uint64_t new_sync_point;
	uint64_t new_data_size;

	uint64_t position;
	uint64_t local_position;
	uint64_t local_offset;
	uint64_t horizon;
	uint64_t sync_point;

	int receivers;
	
	krad_transmission_receiver_t *ready_receivers;
	int ready_receiver_count;

	pthread_t transmission_thread;
};

struct krad_transmission_receiver_St {

	krad_transmitter_t *krad_transmitter;
	krad_transmission_t *krad_transmission;

	krad_transmission_receiver_type_t krad_transmission_receiver_type;

	struct epoll_event event;

	int fd;

	char buffer[256];
	uint64_t position;

	int http_header_position;
	int header_position;

	int wrote_http_header;
	int wrote_header;

	int noob;
	int ready;
	int active;

	krad_transmission_receiver_t *prev;
	krad_transmission_receiver_t *next;
};




krad_transmission_receiver_t *krad_transmitter_receiver_create (krad_transmitter_t *krad_transmitter, int fd);
void krad_transmitter_receiver_destroy (krad_transmission_receiver_t *krad_transmission_receiver);
int krad_transmitter_transmission_transmit (krad_transmission_t *krad_transmission, krad_transmission_receiver_t *krad_transmission_receiver);
void *krad_transmitter_transmission_thread (void *arg);
void *krad_transmitter_listening_thread (void *arg);
void krad_transmitter_receiver_attach (krad_transmission_receiver_t *krad_transmission_receiver, char *request);
void krad_transmitter_handle_incoming_connection (krad_transmitter_t *krad_transmitter, krad_transmission_receiver_t *krad_transmission_receiver);
void krad_transmission_add_ready (krad_transmission_t *krad_transmission, krad_transmission_receiver_t *krad_transmission_receiver);
void krad_transmission_remove_ready (krad_transmission_t *krad_transmission, krad_transmission_receiver_t *krad_transmission_receiver);

krad_transmission_t *krad_transmitter_transmission_create (krad_transmitter_t *krad_transmitter, char *name, char *content_type);
void krad_transmitter_transmission_destroy (krad_transmission_t *krad_transmission);
int krad_transmitter_transmission_set_header (krad_transmission_t *krad_transmission, unsigned char *buffer, int length);
void krad_transmitter_transmission_add_header (krad_transmission_t *krad_transmission, unsigned char *buffer, int length);
int krad_transmitter_transmission_add_data (krad_transmission_t *krad_transmission, unsigned char *buffer, int length);
int krad_transmitter_transmission_add_data_sync (krad_transmission_t *krad_transmission, unsigned char *buffer, int length);
int krad_transmitter_transmission_add_data_opt (krad_transmission_t *krad_transmission, unsigned char *buffer, int length, int sync);
int krad_transmitter_listen_on (krad_transmitter_t *krad_transmitter, int port);
void krad_transmitter_stop_listening (krad_transmitter_t *krad_transmitter);
krad_transmitter_t *krad_transmitter_create ();
void krad_transmitter_destroy (krad_transmitter_t *krad_transmitter);

#endif
