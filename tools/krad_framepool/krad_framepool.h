#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <sys/mman.h>

#include <pthread.h>

typedef struct krad_framepool_St krad_framepool_t;
typedef struct krad_frame_St krad_frame_t;

struct krad_frame_St {

	int *pixels;
	int refs;
	int mjpeg_size;	
	pthread_mutex_t ref_lock;
};

struct krad_framepool_St {

	int width;
	int height;
	int frame_byte_size;
	int count;

	krad_frame_t *frames;

};

krad_frame_t *krad_framepool_getframe (krad_framepool_t *krad_framepool);

void krad_framepool_ref_frame (krad_frame_t *frame);
void krad_framepool_unref_frame (krad_frame_t *frame);

void krad_framepool_destroy (krad_framepool_t *krad_framepool);
krad_framepool_t *krad_framepool_create (int width, int height, int count);
