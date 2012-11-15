#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#include "krad_system.h"

#define KRAD_DECKLINK_H
#include "krad_decklink_capture.h"

#define DEFAULT_DECKLINK_DEVICE "0"

typedef struct krad_decklink_St krad_decklink_t;

struct krad_decklink_St {

	krad_decklink_capture_t * krad_decklink_capture;

	int (*video_frame_callback)(void *, void *, int);
	int (*audio_frames_callback)(void *, void *, int);
	void *callback_pointer;
	
	int verbose;
	
	float *samples[16];
	
	unsigned char *captured_frame_rgb;
	
	int devicenum;
	char simplename[64];
	
};


void krad_decklink_set_verbose(krad_decklink_t *krad_decklink, int verbose);

void krad_decklink_destroy(krad_decklink_t *decklink);
krad_decklink_t *krad_decklink_create(char *device);

void krad_decklink_set_video_mode (krad_decklink_t *krad_decklink, int width, int height,
								   int fps_numerator, int fps_denominator);
void krad_decklink_set_audio_input (krad_decklink_t *krad_decklink, char *audio_input);
void krad_decklink_set_video_input (krad_decklink_t *krad_decklink, char *video_input);
void krad_decklink_info(krad_decklink_t *krad_decklink);
void krad_decklink_start(krad_decklink_t *krad_decklink);
void krad_decklink_stop(krad_decklink_t *krad_decklink);

int krad_decklink_detect_devices();
void krad_decklink_get_device_name (int device_num, char *device_name);
