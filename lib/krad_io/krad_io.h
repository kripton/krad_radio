#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <limits.h>

#include "krad_system.h"

#ifndef KRAD_IO_H
#define KRAD_IO_H

#define KRAD_IO_WRITE_BUFFER_SIZE 8192 * 1024 * 2

typedef struct krad_io_base64_St krad_io_base64_t;

struct krad_io_base64_St {
	int len;
	int chunk;
	char *out;
	char *result;	
};

typedef struct krad_io_St krad_io_t;

typedef enum {
	KRAD_IO_READONLY,
	KRAD_IO_WRITEONLY,
	//krad_EBML_IO_READWRITE,
} krad_io_mode_t;

struct krad_io_St {

	int seekable;
	krad_io_mode_t mode;
	char *uri;
	int (* write)(krad_io_t *krad_io, void *buffer, size_t length);
	int (* read)(krad_io_t *krad_io, void *buffer, size_t length);
	int64_t (* seek)(krad_io_t *krad_io, int64_t offset, int whence);
	int64_t (* tell)(krad_io_t *krad_io);
	int32_t (* open)(krad_io_t *krad_io);
	int32_t (* close)(krad_io_t *krad_io);
	
	int ptr;
	char *host;
	char *mount;
	char *password;
	int port;
	int sd;
	

	unsigned char *write_buffer;
	uint64_t write_buffer_pos;

};

krad_io_t *krad_io_create();
void krad_io_destroy(krad_io_t *krad_io);

int64_t krad_io_tell(krad_io_t *krad_io);
int krad_io_write(krad_io_t *krad_io, void *buffer, size_t length);
int krad_io_write_sync(krad_io_t *krad_io);
int krad_io_read(krad_io_t *krad_io, void *buffer, size_t length);
int krad_io_seek(krad_io_t *krad_io, int64_t offset, int whence);

krad_io_t *krad_io_open_file(char *filename, krad_io_mode_t mode);
krad_io_t *krad_io_open_stream(char *host, int port, char *mount, char *password);


#endif
