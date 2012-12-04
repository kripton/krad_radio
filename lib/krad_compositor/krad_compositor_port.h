#ifndef KRAD_COMPOSITOR_PORT_H
#define KRAD_COMPOSITOR_PORT_H

#include "krad_radio.h"

struct krad_compositor_port_St {

	krad_compositor_t *krad_compositor;

	char sysname[256];
	int direction;
	
	krad_frame_t *last_frame;
	krad_ringbuffer_t *frame_ring;
	
	int passthru;

	int source_width;
	int source_height;
	
	int crop_x;
	int crop_y;
	
	int crop_width;
	int crop_height;
	
	int crop_start_pixel[4];
	
	struct SwsContext *sws_converter;	
	int yuv_color_depth;
	
	int io_params_updated;
	int comp_params_updated;
	
	uint64_t start_timecode;
	
	
	int local;
	int shm_sd;
	int msg_sd;
	char *local_buffer;
	int local_buffer_size;
	krad_frame_t *local_frame;	
	
  krad_compositor_subunit_t *krad_compositor_subunit;
	
};

#endif // KRAD_COMPOSITOR_PORT_H
