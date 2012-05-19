#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#include "krad_radio.h"

#ifndef KRAD_COMPOSITOR_H
#define KRAD_COMPOSITOR_H

#define DEFAULT_COMPOSITOR_BUFFER_FRAMES 130

#define KRAD_COMPOSITOR_MAX_PORTS 30

typedef struct krad_compositor_St krad_compositor_t;
typedef struct krad_compositor_port_St krad_compositor_port_t;

struct krad_compositor_port_St {

	krad_compositor_t *krad_compositor;

	char sysname[256];
	int direction;
	int active;
	
	krad_frame_t *krad_frame;	
	
	krad_ringbuffer_t *frame_ring;
	
	int mjpeg;
	
};

struct krad_compositor_St {

	kradgui_t *krad_gui;

	int width;
	int height;

	int frame_byte_size;

//	int frame_rate_numerator;
//	int frame_rate_denominator;


	//FIXME temp kludge
	krad_ringbuffer_t *incoming_frames_buffer;
	int hex_x;
	int hex_y;
	int hex_size;

	krad_framepool_t *krad_framepool;

	krad_compositor_port_t *port;


	krad_ringbuffer_t *composited_frames_buffer;

};

void krad_compositor_port_push_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame);
krad_frame_t *krad_compositor_port_pull_frame (krad_compositor_port_t *krad_compositor_port);

krad_compositor_port_t *krad_compositor_mjpeg_port_create (krad_compositor_t *krad_compositor, char *sysname, int direction);
krad_compositor_port_t *krad_compositor_port_create (krad_compositor_t *krad_compositor, char *sysname, int direction);
void krad_compositor_port_destroy (krad_compositor_t *krad_compositor, krad_compositor_port_t *krad_compositor_port);

int krad_compositor_handler ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc );

void krad_compositor_mjpeg_process (krad_compositor_t *krad_compositor);
void krad_compositor_process (krad_compositor_t *compositor);
void krad_compositor_destroy (krad_compositor_t *compositor);
krad_compositor_t *krad_compositor_create (int width, int height);


#endif
