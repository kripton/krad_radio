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

#ifndef KRAD_SYSTEM_H
#define KRAD_SYSTEM_H

#define KRAD_SYSNAME_MIN 4
#define KRAD_SYSNAME_MAX 32

void failfast (char* format, ...);
void printke (char* format, ...);
void printk (char* format, ...);
void krad_system_daemonize ();
void krad_system_init ();

int krad_valid_sysname (char *sysname);

#endif
