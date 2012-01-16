
#ifndef KRAD_PULSE_H
#define KRAD_PULSE_H

#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <krad_audio.h>
#include <stddef.h>

typedef struct krad_pulse_St krad_pulse_t;

struct krad_pulse_St {

	krad_audio_t *kradaudio;

	pa_buffer_attr bufattr;
	pa_sample_spec ss;
	
	pa_mainloop *pa_ml;
	pa_mainloop_api *pa_mlapi;
	pa_context *pa_ctx;
	pa_stream *playstream;

	int underflows;
	int latency;
	int r;
	int pa_ready;
	int retval;
	unsigned int a;
	double amp;
	
	int shutdown;
	
	float *samples[8];
	float *interleaved_samples;
	
	pthread_t loop_thread;
	
};



void kradpulse_destroy(krad_pulse_t *kradpulse);
krad_pulse_t *kradpulse_create();
void kradpulse_write(krad_pulse_t *kradpulse,  int channel, char *data, int len);

size_t kradpulse_buffered_frames(krad_pulse_t *kradpulse);

#endif

typedef struct krad_pulse_St krad_pulse_t;
