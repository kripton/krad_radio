typedef struct krad_pulse_St krad_pulse_t;

#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <stddef.h>
#include <signal.h>

#ifndef KRAD_PULSE_H
#define KRAD_PULSE_H

#include "krad_audio.h"

struct krad_pulse_St {

	krad_audio_t *krad_audio;

	pa_buffer_attr bufattr;
	pa_sample_spec ss;
	
	pa_mainloop *pa_ml;
	pa_mainloop_api *pa_mlapi;
	pa_context *pa_ctx;
	pa_stream *playstream;
	pa_stream *capturestream;
	//pa_signal_event *pa_sig;


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
	
	float *capture_samples[8];
	float *capture_interleaved_samples;
	
	pthread_t loop_thread;
	
};



void krad_pulse_destroy(krad_pulse_t *krad_pulse);
krad_pulse_t *krad_pulse_create();
void krad_pulse_write(krad_pulse_t *krad_pulse,  int channel, char *data, int len);

size_t krad_pulse_buffered_frames(krad_pulse_t *krad_pulse);

#endif

typedef struct krad_pulse_St krad_pulse_t;
