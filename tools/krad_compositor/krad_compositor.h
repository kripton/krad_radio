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

#define DEFAULT_COMPOSITOR_BUFFER_FRAMES 120
#define KRAD_COMPOSITOR_MAX_PORTS 30

typedef enum {
	SYNTHETIC = 13999,	
	X11GLX,
	WAYLAND,
} krad_display_api_t;

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

	int frame_rate_numerator;
	int frame_rate_denominator;

	int source_width;
	int source_height;
	
	int crop_width;
	int crop_height;	
	
	int crop_x;
	int crop_y;
	
	int width;
	int height;
	
	int x;
	int y;
	
	struct SwsContext *sws_converter;	
	
	int io_params_updated;
	int comp_params_updated;
	
};

struct krad_compositor_St {

	kradgui_t *krad_gui;

	int width;
	int height;

	int frame_byte_size;

	int frame_rate_numerator;
	int frame_rate_denominator;

	int hex_x;
	int hex_y;
	int hex_size;

	int bug_x;
	int bug_y;
	char *bug_filename;
	
	int render_vu_meters;

	krad_framepool_t *krad_framepool;

	krad_compositor_port_t *port;
	
	pthread_mutex_t settings_lock;
	
	int display_width;
	int display_height;
	int display_open;
	pthread_t display_thread;

	int active_ports;

	krad_display_api_t pusher;
	krad_ticker_t *krad_ticker;
	int ticker_running;
	int ticker_period;
	pthread_t ticker_thread;

	uint64_t frame_num;

};

void krad_compositor_alloc_resources (krad_compositor_t *krad_compositor);

void *krad_compositor_ticker_thread (void *arg);
void krad_compositor_start_ticker (krad_compositor_t *krad_compositor);
void krad_compositor_stop_ticker (krad_compositor_t *krad_compositor);

krad_display_api_t krad_compositor_get_pusher (krad_compositor_t *krad_compositor);
int krad_compositor_has_pusher (krad_compositor_t *krad_compositor);
void krad_compositor_set_pusher (krad_compositor_t *krad_compositor, krad_display_api_t pusher);
void krad_compositor_unset_pusher (krad_compositor_t *krad_compositor);


void krad_compositor_set_frame_rate (krad_compositor_t *krad_compositor,
									 int frame_rate_numerator, int frame_rate_denominator);


void krad_compositor_set_peak (krad_compositor_t *krad_compositor, int channel, float value);

void krad_compositor_port_set_io_params (krad_compositor_port_t *krad_compositor_port,
										   int frame_rate_numerator, int frame_rate_denominator,
										   int width, int height);

void krad_compositor_port_set_comp_params (krad_compositor_port_t *krad_compositor_port,
										   int crop_width, int crop_height, int crop_x,
										   int crop_y, int width, int height);

void krad_compositor_port_push_yuv_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame);
void krad_compositor_port_push_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame);
krad_frame_t *krad_compositor_port_pull_frame (krad_compositor_port_t *krad_compositor_port);

krad_frame_t *krad_compositor_port_pull_yuv_frame (krad_compositor_port_t *krad_compositor_port,
												   uint8_t *yuv_pixels[4], int yuv_strides[4]);

int krad_compositor_port_frames_avail (krad_compositor_port_t *krad_compositor_port);

krad_compositor_port_t *krad_compositor_mjpeg_port_create (krad_compositor_t *krad_compositor, char *sysname, int direction);
krad_compositor_port_t *krad_compositor_port_create (krad_compositor_t *krad_compositor, char *sysname, int direction,
													 int frame_rate_numerator, int frame_rate_denominator,
													 int width, int height);
void krad_compositor_port_destroy (krad_compositor_t *krad_compositor, krad_compositor_port_t *krad_compositor_port);

int krad_compositor_handler ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc );

void krad_compositor_get_info (krad_compositor_t *compositor, int *width, int *height);
void krad_compositor_mjpeg_process (krad_compositor_t *krad_compositor);
void krad_compositor_process (krad_compositor_t *compositor);
void krad_compositor_destroy (krad_compositor_t *compositor);
krad_compositor_t *krad_compositor_create (int width, int height,
										   int frame_rate_numerator, int frame_rate_denominator);


#endif
