#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

typedef struct krad_system_St krad_system_t;

#ifndef KRAD_SYSTEM_H
#define KRAD_SYSTEM_H

#define KRAD_SYSNAME_MIN 4
#define KRAD_SYSNAME_MAX 32

struct krad_system_St {

	struct utsname unix_info;
	time_t krad_start_time;
};

void krad_system_info_print ();
void krad_system_info_collect ();

void failfast (char* format, ...);
void printke (char* format, ...);
void printk (char* format, ...);
void krad_system_daemonize ();
void krad_system_init ();

int krad_valid_sysname (char *sysname);
int krad_valid_host_and_port (char *string);
void krad_get_host_and_port (char *string, char *host, int *port);
#endif
