#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#define KRAD_DECKLINK_H
#include "krad_decklink_capture.h"

typedef struct krad_decklink_St krad_decklink_t;

struct krad_decklink_St {

	krad_decklink_capture_t * krad_decklink_capture;

	int (*video_frame_callback)(void *, void *, int);
	int (*audio_frames_callback)(void *, void *, int);
	void *callback_pointer;
	
	int verbose;
	
	float *samples[16];
	
	unsigned char *captured_frame_rgb;
	
};


void krad_decklink_set_verbose(krad_decklink_t *krad_decklink, int verbose);

void krad_decklink_destroy(krad_decklink_t *decklink);
krad_decklink_t *krad_decklink_create();

void krad_decklink_info(krad_decklink_t *krad_decklink);
void krad_decklink_start(krad_decklink_t *krad_decklink);
void krad_decklink_stop(krad_decklink_t *krad_decklink);
