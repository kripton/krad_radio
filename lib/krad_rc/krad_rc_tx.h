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

typedef struct krad_rc_tx_St krad_rc_tx_t;

struct krad_rc_tx_St {

	int sd;
	unsigned char *data;
	int port;
	char *ip;

};


void krad_rc_tx_command (krad_rc_tx_t *krad_rc_tx, unsigned char *data, int size);
void krad_rc_tx_destroy (krad_rc_tx_t *krad_rc_tx);
krad_rc_tx_t *krad_rc_tx_create (char *ip, int port);
