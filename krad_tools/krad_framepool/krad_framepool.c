#include "krad_framepool.h"

krad_frame_t *krad_framepool_getframe (krad_framepool_t *krad_framepool) {

	int f;

	for (f = 0; f < krad_framepool->count; f++ ) {
		if (krad_framepool->frames[f].refs == 0) {
			return &krad_framepool->frames[f];
		}
	}
	
	return NULL;

}

void krad_framepool_ref_frame (krad_framepool_t *krad_framepool, krad_frame_t *frame) {

	frame->refs++;

}


void krad_framepool_unref_frame (krad_framepool_t *krad_framepool, krad_frame_t *frame) {

	frame->refs--;

}

void krad_framepool_destroy (krad_framepool_t *krad_framepool) {

	int f;

	for (f = 0; f < krad_framepool->count; f++ ) {
		free (krad_framepool->frames[f].pixels);
	}

	free (krad_framepool->frames);
	free (krad_framepool);

}

krad_framepool_t *krad_framepool_create (int width, int height, int count) {

	krad_framepool_t *krad_framepool = calloc (1, sizeof(krad_framepool_t));

	int f;

	krad_framepool->width = width;
	krad_framepool->height = height;
	krad_framepool->count = count;
	
	krad_framepool->frames = calloc (krad_framepool->count, sizeof(krad_frame_t));
	
	for (f = 0; f < krad_framepool->count; f++ ) {
		krad_framepool->frames[f].pixels = malloc (krad_framepool->width * krad_framepool->height * 4);
	}
	
	return krad_framepool;

}
