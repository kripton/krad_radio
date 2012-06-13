#include "krad_framepool.h"

krad_frame_t *krad_framepool_getframe (krad_framepool_t *krad_framepool) {

	int f;

	for (f = 0; f < krad_framepool->count; f++ ) {
		if (krad_framepool->frames[f].refs == 0) {
			krad_framepool_ref_frame (&krad_framepool->frames[f]);
			return &krad_framepool->frames[f];
		}
	}
	
	return NULL;

}

void krad_framepool_ref_frame (krad_frame_t *frame) {

	pthread_mutex_lock (&frame->ref_lock);
	frame->refs++;
	pthread_mutex_unlock (&frame->ref_lock);
	//printf("refs = %d\n", frame->refs);
}


void krad_framepool_unref_frame (krad_frame_t *frame) {

	pthread_mutex_lock (&frame->ref_lock);
	frame->refs--;
	pthread_mutex_unlock (&frame->ref_lock);
	//printf("refs = %d\n", frame->refs);
}

void krad_framepool_destroy (krad_framepool_t *krad_framepool) {

	int f;

	for (f = 0; f < krad_framepool->count; f++ ) {
		munlock (krad_framepool->frames[f].pixels, krad_framepool->frame_byte_size);
		free (krad_framepool->frames[f].pixels);
		pthread_mutex_destroy (&krad_framepool->frames[f].ref_lock);
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
	
	krad_framepool->frame_byte_size = krad_framepool->width * krad_framepool->height * 4;
	
	krad_framepool->frames = calloc (krad_framepool->count, sizeof(krad_frame_t));
	
	for (f = 0; f < krad_framepool->count; f++ ) {
		krad_framepool->frames[f].pixels = malloc (krad_framepool->frame_byte_size);
		if (krad_framepool->frames[f].pixels == NULL) {
			failfast ("Krad Framepool: Out of memory");
		}
		mlock (krad_framepool->frames[f].pixels, krad_framepool->frame_byte_size);
		pthread_mutex_init (&krad_framepool->frames[f].ref_lock, NULL);
	}
	
	return krad_framepool;

}
