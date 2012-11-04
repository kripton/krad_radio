#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#include "krad_system.h"

typedef struct krad_osc_St krad_osc_t;

struct krad_osc_St {

	unsigned char *buffer;

	int port;
	int sd;
	struct sockaddr_in local_address;
	int listening;
	int stop_listening;
	pthread_t listening_thread;
};

void krad_osc_parse_message (krad_osc_t *krad_osc, unsigned char *message, int size);

void *krad_osc_listening_thread (void *arg);

void krad_osc_stop_listening (krad_osc_t *krad_osc);
int krad_osc_listen (krad_osc_t *krad_osc, int port);

void krad_osc_destroy (krad_osc_t *krad_osc);
krad_osc_t *krad_osc_create ();

