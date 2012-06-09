#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include <ogg/ogg.h>

#include "krad_ring.h"

#define KXPDR_VERSION "KXPDR Version 1.6b"

#define RECEIVER_COUNT 16
#define TRANSMITTERS_PER_RECEIVER 128

#define DEFAULT_RECEIVER_PORT "9040"
#define DEFAULT_TRANSMITTER_PORT "9050"

#define SERVER "Icecast 2.3.2"

#define MAXEVENTS 64
#define RING_SIZE 10000000

#define BURST_SIZE 64000

typedef struct kxpdr_St kxpdr_t;
typedef struct kxpdr_transmitter_St kxpdr_transmitter_t;
typedef struct kxpdr_receiver_St kxpdr_receiver_t;
typedef struct ogg_stream_St ogg_stream_t;
typedef struct ebml_stream_St ebml_stream_t;

struct kxpdr_St {

	int active;
	
	char not_found[256];
	int not_found_len;
	
	int incoming_receiver_connection_port;
	int incoming_transmitter_connection_port;

	int incoming_receiver_connections_sd;
	int incoming_transmitter_connections_sd;

	int incoming_receiver_connections_efd;
	int incoming_transmitter_connections_efd;

	struct epoll_event incoming_receiver_connection_event;
	struct epoll_event *incoming_receiver_connection_events;
	
	struct epoll_event incoming_transmitter_connection_event;
	struct epoll_event *incoming_transmitter_connection_events;
	
	kxpdr_receiver_t *receivers;
	
	pthread_t incoming_receiver_connections_thread;
	pthread_t incoming_transmitter_connections_thread;
	
};

struct kxpdr_transmitter_St {

	kxpdr_receiver_t *receiver;
	
	int ready;
	
	int active;
	
	int sd;
	int http_header_position;
	int stream_header_position;
	size_t ring_position_offset;
	
	size_t ring_bytes_avail;
	unsigned long byte_position;
	
	struct epoll_event event;

	kxpdr_transmitter_t *prev;
	kxpdr_transmitter_t *next;

};

struct kxpdr_receiver_St {

	kxpdr_t *kxpdr;

	int active;

	char name[256];
	char mount[256];
	
	char first_bytes[512];
	int first_bytes_len;
	
	krad_ringbuffer_t *ringbuffer;
	krad_ringbuffer_data_t read_vector[2];
	krad_ringbuffer_data_t write_vector[2];
	
	unsigned char *http_header;
	int http_header_position;
	int http_header_size;
	char content_type[256];

	unsigned char *stream_header;
	int stream_header_position;
	int stream_header_size;
	
	ogg_stream_t *ogg_stream;
	ebml_stream_t *ebml_stream;
	
	unsigned long byte_position;
	unsigned long sync_byte_position;
	
	int sd;
	int receiver_efd;
	int transmitter_efd;

	kxpdr_transmitter_t *transmitters;
	kxpdr_transmitter_t *ready_transmitters_head;
	kxpdr_transmitter_t *ready_transmitters_tail;
	int ready;
	
	
	pthread_t receiver_thread;
	pthread_t transmitter_thread;
	
	struct epoll_event receiver_event;
	struct epoll_event *receiver_events;
	
	struct epoll_event transmitter_event;
	struct epoll_event *transmitter_events;
	
};

struct ebml_stream_St {
	int monkeybyte;
};

struct ogg_stream_St {
	//ogg_stream_state oz;
	ogg_sync_state os;
	ogg_page op;
	int page_header_position;
	int page_body_position;
	int page_size;
	int kludger;
};


void kxpdr_initiate_shutdown ();
void kxpdr_destroy (kxpdr_t *kxpdr);
kxpdr_t *kxpdr_create (char *incoming_receiver_connection_port, char *incoming_transmitter_connection_port);

void kxpdr_receiver_add_ready (kxpdr_receiver_t *kxpdr_receiver, kxpdr_transmitter_t *transmitter);
void kxpdr_receiver_remove_ready (kxpdr_receiver_t *kxpdr_receiver, kxpdr_transmitter_t *transmitter);

void *receiver_thread (void *arg);
void *transmitter_thread (void *arg);
void *incoming_receiver_connections_thread (void *arg);
void *incoming_transmitter_connections_thread (void *arg);
/*
receiver connection thread-> -> receiver thread -> recieves -- single reciever
								transmitter thread -> transmitts -> X number of fd's
								
transmitter connection thread -> adds to a transmitter thread


*/
