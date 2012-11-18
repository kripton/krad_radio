#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>



#include "krad_system.h"
#include "krad_radio.h"

#ifndef KRAD_Y4M_H
#define KRAD_Y4M_H

#define Y4M_MAGIC "YUV4MPEG2"
#define MAX_YUV4_HEADER_SIZE 80
#define Y4M_FRAME_HEADER "FRAME\n"
#define Y4M_FRAME_HEADER_SIZE 6

typedef struct krad_y4m_St krad_y4m_t;

struct krad_y4m_St {

  int width;
  int height;
  int color_depth;
	
	unsigned char *planes[3];
	int strides[3];
	int size[3];	
	
	int frame_size;
	
};


void krad_y4m_destroy (krad_y4m_t *krad_y4m);
krad_y4m_t *krad_y4m_create (int width, int height, int color_depth);


int krad_y4m_generate_header (unsigned char *header, int width, int height, int frame_rate_numerator, int frame_rate_denominator, int color_depth);

int krad_y4m_parse_header (unsigned char *buffer, int *width, int *height, int *frame_rate_numerator,
                           int *frame_rate_denominator, int *color_depth);

#endif
