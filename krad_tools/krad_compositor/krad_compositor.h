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

#define DEFAULT_COMPOSITOR_BUFFER_FRAMES 15

typedef struct krad_compositor_St krad_compositor_t;

struct krad_compositor_St {

	kradgui_t *krad_gui;

	int width;
	int height;

	int frame_byte_size;

	//FIXME temp kludge
	krad_ringbuffer_t *incoming_frames_buffer;
	int hex_x;
	int hex_y;
	int hex_size;


	krad_ringbuffer_t *composited_frames_buffer;

};


int krad_compositor_handler ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc );

void krad_compositor_process (krad_compositor_t *compositor);
void krad_compositor_destroy (krad_compositor_t *compositor);
krad_compositor_t *krad_compositor_create (int width, int height);


#endif
