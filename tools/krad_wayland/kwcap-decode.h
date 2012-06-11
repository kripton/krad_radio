/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#ifndef _WCAP_DECODE_
#define _WCAP_DECODE_

#define WCAP_HEADER_MAGIC	0x57434150

#define WCAP_FORMAT_XRGB8888	0x34325258
#define WCAP_FORMAT_XBGR8888	0x34324258
#define WCAP_FORMAT_RGBX8888	0x34325852
#define WCAP_FORMAT_BGRX8888	0x34325842

struct wcap_header {
	uint32_t magic;
	uint32_t format;
	uint32_t width, height;
};

struct kwcap_frame_header {
	uint32_t frame_size;
	uint32_t msecs;
	uint32_t nrects;
};

struct wcap_rectangle {
	int32_t x1, y1, x2, y2;
};

struct kwcap_decoder {
	int fd;
	size_t size;
	void *map, *p, *end;
	uint32_t *frame;
	uint32_t format;
	uint32_t msecs;
	uint32_t count;
	int width, height;
	int external_buffer;
	void *read_buffer;
	int port;
	int listening;
	struct sockaddr_in local_address;
	int sd;
};

struct kwcap_decoder *kwcap_decoder_listen (int port, int external_buffer);
int kwcap_decoder_get_frame(struct kwcap_decoder *decoder);
struct kwcap_decoder *kwcap_decoder_create(const char *filename, int external_buffer);

void kwcap_decoder_set_external_buffer(struct kwcap_decoder *decoder, void *external_buffer);
void kwcap_decoder_destroy(struct kwcap_decoder *decoder);

#endif
