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

void failfast (char* format, ...);
void printke (char* format, ...);
void printk (char* format, ...);
void krad_system_daemonize ();
void krad_system_init ();
