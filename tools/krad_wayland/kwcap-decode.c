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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <cairo.h>

#include "kwcap-decode.h"
#include "krad_system.h"

static void
kwcap_decoder_decode_rectangle(struct kwcap_decoder *decoder,
			      struct wcap_rectangle *rect)
{
	uint32_t v, *p = decoder->p, *d;
	int width = rect->x2 - rect->x1, height = rect->y2 - rect->y1;
	int x, i, j, k, l, count = width * height;
	unsigned char r, g, b, dr, dg, db;

	d = decoder->frame + (rect->y2 - 1) * decoder->width;
	x = rect->x1;
	i = 0;
	while (i < count) {
		v = *p++;
		l = v >> 24;
		if (l < 0xe0) {
			j = l + 1;
		} else {
			j = 1 << (l - 0xe0 + 7);
		}

		dr = (v >> 16);
		dg = (v >>  8);
		db = (v >>  0);
		for (k = 0; k < j; k++) {
			r = (d[x] >> 16) + dr;
			g = (d[x] >>  8) + dg;
			b = (d[x] >>  0) + db;
			d[x] = 0xff000000 | (r << 16) | (g << 8) | b;
			x++;
			if (x == rect->x2) {
				x = rect->x1;
				d -= decoder->width;
			}
		}
		i += j;
	}

	if (i != count)
		printf("rle encoding longer than expected (%d expected %d)\n",
		       i, count);

	decoder->p = p;
}

int
kwcap_decoder_get_frame(struct kwcap_decoder *decoder)
{
	struct wcap_rectangle *rects;
	struct kwcap_frame_header *header;
	//uint32_t *s;
	uint32_t i;
	//int width, height;
	int ret;
	uint32_t remaining;
	
	remaining = 0;
	
	ret = 0;
	header = malloc(sizeof(struct kwcap_frame_header));
	//return 0;
	while (ret != sizeof(struct kwcap_frame_header)) {
		ret += read (decoder->fd, header + ret, sizeof(struct kwcap_frame_header) - ret);
	}

	remaining = header->frame_size - sizeof(struct kwcap_frame_header);
	
	decoder->msecs = header->msecs;
	decoder->count++;

	//printf ("\nframe size is %u rects is %u msecs is %u\n", header->frame_size, header->nrects, header->msecs);


	ret = 0;

	rects = malloc(sizeof(struct wcap_rectangle) * header->nrects);	

	while (ret != (sizeof(struct wcap_rectangle) * header->nrects)) {
		ret += read (decoder->fd, rects + ret, (sizeof(struct wcap_rectangle) * header->nrects) - ret);
	}
	
	
	//printf ("\nread %u\n", ret);
	
	ret = 0;
	
	remaining -= sizeof(struct wcap_rectangle) * header->nrects;
	
	//FIXME broke writer
	//remaining = header->frame_size;
	
		//printf ("\nremaining %u\n", remaining);
	
	while (ret != remaining) {
		ret += read (decoder->fd, decoder->read_buffer + ret, remaining - ret);
	}


	//rects = (void *) (header + 1);
	decoder->p = (uint32_t *) (rects + header->nrects);
	
	decoder->p = (uint32_t *) decoder->read_buffer;
	
	for (i = 0; i < header->nrects; i++) {
		//width = rects[i].x2 - rects[i].x1;
		//height = rects[i].y2 - rects[i].y1;
		kwcap_decoder_decode_rectangle(decoder, &rects[i]);
	}
	


	if ((header->nrects == 1) && (decoder->count > 1) && (header->frame_size == 4)) {
		free (rects);
		free (header);
		return kwcap_decoder_get_frame(decoder);
	}
	free (rects);
	free (header);
	return 1;
}

struct kwcap_decoder *
kwcap_decoder_create(const char *filename, int external_buffer)
{
	struct kwcap_decoder *decoder;
	struct wcap_header *header;
	int frame_size;
	struct stat buf;

	decoder = malloc(sizeof *decoder);
	
	decoder->read_buffer = calloc(1, 1920 * 1080 * 8);
	
	if (decoder == NULL)
		return NULL;

	decoder->fd = open(filename, O_RDONLY);
	if (decoder->fd == -1)
		return NULL;

	//fstat(decoder->fd, &buf);
	//decoder->size = buf.st_size;
	//decoder->map = mmap(NULL, decoder->size,
	//		    PROT_READ, MAP_PRIVATE, decoder->fd, 0);
		
	header = malloc(sizeof(struct wcap_header));
	
	read (decoder->fd, header, sizeof(struct wcap_header));
	
	decoder->format = header->format;
	decoder->count = 0;
	decoder->width = header->width;
	decoder->height = header->height;
	//decoder->p = header + 1;
	//decoder->end = decoder->map + decoder->size;

	frame_size = header->width * header->height * 4;

	free (header);

	if (external_buffer == 1) {
		decoder->external_buffer = 1;
	} else {
		decoder->frame = malloc(frame_size);
		memset(decoder->frame, 0, frame_size);
	}
	return decoder;
}

struct kwcap_decoder *
kwcap_decoder_listen(int port, int external_buffer)
{
	struct kwcap_decoder *decoder;
	struct wcap_header *header;
	int frame_size;
	struct stat buf;

	decoder = malloc(sizeof *decoder);
	
	decoder->port = port;
	
	decoder->read_buffer = calloc(1, 1920 * 1080 * 8);
	
	if (decoder == NULL)
		return NULL;









	decoder->port = port;
	decoder->listening = 1;
	
	decoder->local_address.sin_family = AF_INET;
	decoder->local_address.sin_port = htons (decoder->port);
	decoder->local_address.sin_addr.s_addr = htonl (INADDR_ANY);
	
	if ((decoder->sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Krad linker system call socket error\n");
		decoder->listening = 0;
		decoder->port = 0;		
		return 1;
	}

	if (bind (decoder->sd, (struct sockaddr *)&decoder->local_address, sizeof(decoder->local_address)) == -1) {
		printke ("Krad linker bind error for tcp port %d\n", decoder->port);
		close (decoder->sd);
		decoder->listening = 0;
		decoder->port = 0;
		return 1;
	}
	
	if (listen (decoder->sd, SOMAXCONN) <0) {
		printf("krad_http system call listen error\n");
		close (decoder->sd);
		exit(1);
	}	



	int ret;
	int addr_size;

	struct sockaddr_in remote_address;
	struct pollfd sockets[1];
	
	printk ("Krad linker Listening thread starting\n");
	
	addr_size = 0;
	ret = 0;
	memset (&remote_address, 0, sizeof(remote_address));	

	addr_size = sizeof (remote_address);
	
	//while (decoder->stop_listening == 0) {

		sockets[0].fd = decoder->sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, -1);	

		if (ret < 0) {
			printke ("Krad linker Failed on poll\n");
			//decoder->stop_listening = 1;
			
		}
	
		if (ret > 0) {
		
			if ((decoder->fd = accept(decoder->sd, (struct sockaddr *)&remote_address, (socklen_t *)&addr_size)) < 0) {
				close (decoder->sd);
				failfast ("decoder socket error on accept mayb a signal or such\n");
			}

			//decoder_listen_create_client (decoder, client_fd);

		}
	//}



	printf("hi!\n");
	
	if (decoder->fd == -1)
		return NULL;

	//fstat(decoder->fd, &buf);
	//decoder->size = buf.st_size;
	//decoder->map = mmap(NULL, decoder->size,
	//		    PROT_READ, MAP_PRIVATE, decoder->fd, 0);
		
	header = malloc(sizeof(struct wcap_header));
	
	read (decoder->fd, header, sizeof(struct wcap_header));
	
	decoder->format = header->format;
	decoder->count = 0;
	decoder->width = header->width;
	decoder->height = header->height;
	//decoder->p = header + 1;
	//decoder->end = decoder->map + decoder->size;

	frame_size = header->width * header->height * 4;

	free (header);

	if (external_buffer == 1) {
		decoder->external_buffer = 1;
	} else {
		decoder->frame = malloc(frame_size);
		memset(decoder->frame, 0, frame_size);
	}
	return decoder;
}



void
kwcap_decoder_set_external_buffer(struct kwcap_decoder *decoder, void *buffer)
{
	decoder->frame = buffer;
}

void
kwcap_decoder_destroy(struct kwcap_decoder *decoder)
{
	//munmap(decoder->map, decoder->size);
	close (decoder->fd);
	if (decoder->external_buffer == 0) {
		free(decoder->frame);
	}
	free (decoder->read_buffer);
	free(decoder);
}
