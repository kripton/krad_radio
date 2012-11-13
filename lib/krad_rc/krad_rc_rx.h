#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <malloc.h>
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

#include "krad_rc.h"


typedef struct krad_rc_rx_St krad_rc_rx_t;

struct krad_rc_rx_St {

	int sd;
	char *data;
	int port;
	struct sockaddr_in local_address;
	struct sockaddr_in remote_address;
	
	int run;
	
	krad_rc_t *krad_rc;
	
};

void krad_rc_rx_run (krad_rc_rx_t *krad_rc_rx);
void krad_rc_rx_destroy (krad_rc_rx_t *krad_rc_rx);
krad_rc_rx_t *krad_rc_rx_create (krad_rc_t *krad_rc, int port);
