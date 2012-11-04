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
#include <pthread.h>

#ifndef __MACH__
#include <sys/prctl.h>
#include <malloc.h>
#else
#include "krad_mach.h"
#endif

#ifndef KRAD_SYSTEM_H
#define KRAD_SYSTEM_H


typedef struct krad_system_St krad_system_t;
typedef struct krad_system_cpu_monitor_St krad_system_cpu_monitor_t;

#define KRAD_SYSNAME_MIN 4
#define KRAD_SYSNAME_MAX 32

#define KRAD_BUFLEN_CPUSTAT 128
#define KRAD_CPU_MONITOR_INTERVAL 1800

struct krad_system_cpu_monitor_St {

	int fd;
	int c;
	char buffer[KRAD_BUFLEN_CPUSTAT];

	int last_total;
	int last_idle;
	int total;
	int idle;
	int usage;
	int diff_idle;
	int diff_total;
	
	int user;
	int nice;
	int system;

	int ret;
	
	int interval;
	
	int on;	
	
	pthread_t monitor_thread;
	
	void *callback_pointer;
	void (*cpu_monitor_callback)( void *, uint32_t);
	
	int unset_cpu_monitor_callback;

};

struct krad_system_St {

	char info_string[1024];
	int info_string_len;
	
	uint32_t system_cpu_usage;
	
	krad_system_cpu_monitor_t kcm;
	struct utsname unix_info;
	time_t krad_start_time;
	uint64_t uptime;

	int log_fd;
	pthread_mutex_t log_lock;

};

void krad_system_log_on (char *filename);
void krad_system_log_off ();
void krad_system_unset_monitor_cpu_callback ();
void krad_system_set_monitor_cpu_callback (void *callback_pointer, 
									 void (*cpu_monitor_callback)( void *, uint32_t));

void krad_system_set_monitor_cpu_interval (int ms);
void *krad_system_monitor_cpu_thread (void *arg);
void krad_system_monitor_cpu_off ();
void krad_system_monitor_cpu_off_fast ();
int krad_system_get_cpu_usage ();
void krad_system_monitor_cpu_on ();

char *krad_system_info();
uint64_t krad_system_daemon_uptime ();
void krad_system_info_collect ();

uint64_t ktime();
void failfast (char* format, ...);
void printke (char* format, ...);
void printkd (char* format, ...);
void printk (char* format, ...);
void krad_system_daemonize ();
void krad_system_init ();

void krad_system_set_thread_name (char *name);
int krad_system_set_socket_nonblocking (int sd);
int krad_valid_sysname (char *sysname);
int krad_valid_host_and_port (char *string);
void krad_get_host_and_port (char *string, char *host, int *port);
#endif
