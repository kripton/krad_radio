#include "krad_pulse.h"

#ifdef KRAD_LINK
extern int do_shutdown;
#endif

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void krad_pulse_state_cb(pa_context *c, void *userdata) {

	krad_pulse_t *krad_pulse = (krad_pulse_t *)userdata;

	pa_context_state_t state;

	state = pa_context_get_state(c);

	switch  (state) {
		// These are just here for reference
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		default:
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			krad_pulse->pa_ready = 2;
			break;
		case PA_CONTEXT_READY:
			krad_pulse->pa_ready = 1;
			break;
	}
}
/*
static void krad_pulse_capture_cb(pa_stream *stream, size_t length, void *userdata) {

	krad_pulse_t *krad_pulse = (krad_pulse_t *)userdata;

	pa_usec_t usec;
	int neg;
	const void *samples;
	//int c, s;
	
	pa_stream_get_latency(stream, &usec, &neg);
	
	//printf("  latency %8d us wanted %d frames\n", (int)usec, length / 4 / 2 );
	 
	pa_stream_peek(stream, &samples, &length);

	if ((krad_ringbuffer_write_space (krad_pulse->krad_audio->input_ringbuffer[1]) >= length / 2 ) && (krad_ringbuffer_write_space (krad_pulse->krad_audio->input_ringbuffer[0]) >= length / 2 )) {


		memcpy(krad_pulse->capture_interleaved_samples, samples, length);

		pa_stream_drop(stream);

		for (s = 0; s < length / 4 / 2; s++) {
			for (c = 0; c < 2; c++) {
				krad_pulse->capture_samples[c][s] = krad_pulse->capture_interleaved_samples[s * 2 + c];
			}
		}

		for (c = 0; c < 2; c++) {
			krad_ringbuffer_write (krad_pulse->krad_audio->input_ringbuffer[c], (char *)krad_pulse->capture_samples[c], (length / 2) );
		}

		
		for (c = 0; c < 2; c++) {
		    compute_peak(krad_pulse->krad_audio, KINPUT, &krad_pulse->capture_interleaved_samples[c], c, length / 4 / 2 , 1);
		}
		
	
	}

	if (krad_pulse->krad_audio->process_callback != NULL) {
		krad_pulse->krad_audio->process_callback(length / 4 / 2, krad_pulse->krad_audio->userdata);
	}

}

static void krad_pulse_playback_cb(pa_stream *stream, size_t length, void *userdata) {

	krad_pulse_t *krad_pulse = (krad_pulse_t *)userdata;

	pa_usec_t usec;
	int neg;
	
	//int c, s;
	
	pa_stream_get_latency(stream, &usec, &neg);
	
	//printf("  latency %8d us wanted %d frames\n", (int)usec, length / 4 / 2 );

	if (krad_pulse->krad_audio->process_callback != NULL) {
		krad_pulse->krad_audio->process_callback(length / 4 / 2, krad_pulse->krad_audio->userdata);
	}


	if ((krad_ringbuffer_read_space (krad_pulse->krad_audio->output_ringbuffer[1]) >= length / 2 ) && (krad_ringbuffer_read_space (krad_pulse->krad_audio->output_ringbuffer[0]) >= length / 2 )) {
	
		for (c = 0; c < 2; c++) {
			krad_ringbuffer_read (krad_pulse->krad_audio->output_ringbuffer[c], (char *)krad_pulse->samples[c], (length / 2) );
		}

		for (s = 0; s < length / 4 / 2; s++) {
			for (c = 0; c < 2; c++) {
				krad_pulse->interleaved_samples[s * 2 + c] = krad_pulse->samples[c][s];
			}
		}
		
		for (c = 0; c < 2; c++) {
		    compute_peak(krad_pulse->krad_audio, KOUTPUT, &krad_pulse->interleaved_samples[c], c, length / 4 / 2 , 1);
		}
		
	
	} else {
	
		for (s = 0; s < length / 4 / 2; s++) {
			for (c = 0; c < 2; c++) {
				krad_pulse->interleaved_samples[s * 2 + c] = 0.0f;
			}
		}
	
	}

	pa_stream_write(stream, krad_pulse->interleaved_samples, length, NULL, 0LL, PA_SEEK_RELATIVE);

}

static void krad_pulse_stream_underflow_cb(pa_stream *s, void *userdata) {

	krad_pulse_t *krad_pulse = (krad_pulse_t *)userdata;

	// We increase the latency by 50% if we get 6 underflows and latency is under 2s
	// This is very useful for over the network playback that can't handle low latencies
	//printf("underflow\n");
	krad_pulse->underflows++;
	if (krad_pulse->underflows >= 6 && krad_pulse->latency < 2000000) {
		krad_pulse->latency = (krad_pulse->latency*3)/2;
		krad_pulse->bufattr.maxlength = pa_usec_to_bytes(krad_pulse->latency, &krad_pulse->ss);
		krad_pulse->bufattr.tlength = pa_usec_to_bytes(krad_pulse->latency, &krad_pulse->ss);  
		pa_stream_set_buffer_attr(s, &krad_pulse->bufattr, NULL, NULL);
		krad_pulse->underflows = 0;
		//printf("latency increased to %d\n", krad_pulse->latency);
	}
}

*/
void *krad_pulse_loop_thread(void *arg) {

	krad_pulse_t *krad_pulse = (krad_pulse_t *)arg;

	// Iterate the main loop and go again.  The second argument is whether
	// or not the iteration should block until something is ready to be
	// done.  Set it to zero for non-blocking.

	while (krad_pulse->shutdown == 0) {
		pa_mainloop_iterate(krad_pulse->pa_ml, 1, NULL);
	}
	
	return NULL;
	
}

//void *krad_pulse_signal_callback(void *arg) {
void krad_pulse_signal_callback(pa_mainloop_api*m, pa_signal_event *e, int sig, void *userdata) {

	krad_pulse_t *krad_pulse = (krad_pulse_t *)userdata;
	
	krad_pulse->shutdown = 1;

#ifdef KRAD_LINK
do_shutdown = 1;
#endif

}

void krad_pulse_destroy(krad_pulse_t *krad_pulse) {

	krad_pulse->shutdown = 1;

	pthread_join (krad_pulse->loop_thread, NULL);

	// clean up and disconnect
	pa_context_disconnect (krad_pulse->pa_ctx);
	pa_context_unref (krad_pulse->pa_ctx);
	pa_mainloop_free (krad_pulse->pa_ml);

	free (krad_pulse->samples[0]);
	free (krad_pulse->samples[1]);

	free (krad_pulse->interleaved_samples);
	
	free (krad_pulse->capture_samples[0]);
	free (krad_pulse->capture_samples[1]);

	free (krad_pulse->capture_interleaved_samples);
	
	free (krad_pulse);

}


krad_pulse_t *krad_pulse_create (krad_audio_t *krad_audio) {


	krad_pulse_t *krad_pulse;
	
	if ((krad_pulse = calloc (1, sizeof (krad_pulse_t))) == NULL) {
		failfast ("mem alloc fail");
	}
	
	krad_pulse->krad_audio = krad_audio;


	krad_pulse->samples[0] = malloc(24 * 8192);
	krad_pulse->samples[1] = malloc(24 * 8192);
	krad_pulse->interleaved_samples = malloc(48 * 8192);

	krad_pulse->capture_samples[0] = malloc(24 * 8192);
	krad_pulse->capture_samples[1] = malloc(24 * 8192);
	krad_pulse->capture_interleaved_samples = malloc(48 * 8192);

	krad_pulse->latency = 20000; // start latency in micro seconds
	krad_pulse->underflows = 0;
	krad_pulse->pa_ready = 0;
	krad_pulse->retval = 0;

	// Create a mainloop API and connection to the default server
	krad_pulse->pa_ml = pa_mainloop_new();
	krad_pulse->pa_mlapi = pa_mainloop_get_api (krad_pulse->pa_ml);
	krad_pulse->pa_ctx = pa_context_new (krad_pulse->pa_mlapi, krad_pulse->krad_audio->krad_mixer->name);
	
	pa_context_connect(krad_pulse->pa_ctx, NULL, 0, NULL);

	// This function defines a callback so the server will tell us it's state.
	// Our callback will wait for the state to be ready.  The callback will
	// modify the variable to 1 so we know when we have a connection and it's
	// ready.
	// If there's an error, the callback will set pa_ready to 2
	pa_context_set_state_callback(krad_pulse->pa_ctx, krad_pulse_state_cb, krad_pulse);

	// We can't do anything until PA is ready, so just iterate the mainloop
	// and continue
	while (krad_pulse->pa_ready == 0) {
		pa_mainloop_iterate(krad_pulse->pa_ml, 1, NULL);
	}

	if (krad_pulse->pa_ready == 2) {
		krad_pulse->retval = -1;
		failfast ("pulseaudio fail");
	}

	krad_pulse->ss.rate = 44100;
	krad_pulse->ss.channels = 2;
	krad_pulse->ss.format = PA_SAMPLE_FLOAT32LE;
	
	krad_pulse->bufattr.fragsize = (uint32_t)-1;
	krad_pulse->bufattr.maxlength = pa_usec_to_bytes(krad_pulse->latency, &krad_pulse->ss);
	krad_pulse->bufattr.minreq = pa_usec_to_bytes(0, &krad_pulse->ss);
	krad_pulse->bufattr.prebuf = (uint32_t)-1;
	krad_pulse->bufattr.tlength = pa_usec_to_bytes(krad_pulse->latency, &krad_pulse->ss);
	
	
	/*
	if ((krad_audio->direction == KOUTPUT) || (krad_audio->direction == KDUPLEX)) {
		krad_pulse->playstream = pa_stream_new(krad_pulse->pa_ctx, "Playback", &krad_pulse->ss, NULL);
	
		if (!krad_pulse->playstream) {
			printf("playback pa_stream_new failed\n");
			exit(1);
		}
	
		pa_stream_set_write_callback(krad_pulse->playstream, krad_pulse_playback_cb, krad_pulse);
		pa_stream_set_underflow_callback(krad_pulse->playstream, krad_pulse_stream_underflow_cb, krad_pulse);
	 
		krad_pulse->r = pa_stream_connect_playback(krad_pulse->playstream, NULL, &krad_pulse->bufattr, 
									 			  PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);

		if (krad_pulse->r < 0) {
			printf("pa_stream_connect_playback failed\n");
			krad_pulse->retval = -1;
			printf("pulseaudio fail\n");
			exit(1);
		}
	
	}
	
	if ((krad_audio->direction == KINPUT) || (krad_audio->direction == KDUPLEX)) {
		krad_pulse->capturestream = pa_stream_new(krad_pulse->pa_ctx, "Capture", &krad_pulse->ss, NULL);
	
		if (!krad_pulse->capturestream) {
			printf("capture pa_stream_new failed\n");
			exit(1);
		}
		
	
		pa_stream_set_read_callback(krad_pulse->capturestream, krad_pulse_capture_cb, krad_pulse);
		pa_stream_set_underflow_callback(krad_pulse->capturestream, krad_pulse_stream_underflow_cb, krad_pulse);
	 
		krad_pulse->r = pa_stream_connect_record(krad_pulse->capturestream, NULL, &krad_pulse->bufattr, PA_STREAM_NOFLAGS );

		if (krad_pulse->r < 0) {
			printf("pa_stream_connect_capture failed\n");
			krad_pulse->retval = -1;
			printf("pulseaudio fail\n");
			exit(1);
		}
		
	}
	
	//krad_audio->sample_rate = krad_pulse->ss.rate;
	*/
	pthread_create( &krad_pulse->loop_thread, NULL, krad_pulse_loop_thread, krad_pulse);

	return krad_pulse;

}

