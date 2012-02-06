#include <krad_pulse.h>


// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void kradpulse_state_cb(pa_context *c, void *userdata) {

	krad_pulse_t *kradpulse = (krad_pulse_t *)userdata;

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
			kradpulse->pa_ready = 2;
			break;
		case PA_CONTEXT_READY:
			kradpulse->pa_ready = 1;
			break;
	}
}

static void kradpulse_capture_cb(pa_stream *stream, size_t length, void *userdata) {

	krad_pulse_t *kradpulse = (krad_pulse_t *)userdata;

	pa_usec_t usec;
	int neg;
	const void *samples;
	int c, s;
	
	pa_stream_get_latency(stream, &usec, &neg);
	
	//printf("  latency %8d us wanted %d frames\n", (int)usec, length / 4 / 2 );
	 
	pa_stream_peek(stream, &samples, &length);
	 
	if ((jack_ringbuffer_write_space (kradpulse->kradaudio->input_ringbuffer[1]) >= length / 2 ) && (jack_ringbuffer_write_space (kradpulse->kradaudio->input_ringbuffer[0]) >= length / 2 )) {


		memcpy(kradpulse->capture_interleaved_samples, samples, length);

		pa_stream_drop(stream);

		for (s = 0; s < length / 4 / 2; s++) {
			for (c = 0; c < 2; c++) {
				kradpulse->capture_samples[c][s] = kradpulse->capture_interleaved_samples[s * 2 + c];
			}
		}

		for (c = 0; c < 2; c++) {
			jack_ringbuffer_write (kradpulse->kradaudio->input_ringbuffer[c], (char *)kradpulse->capture_samples[c], (length / 2) );
		}

		
		for (c = 0; c < 2; c++) {
		    compute_peak(kradpulse->kradaudio, KINPUT, &kradpulse->capture_interleaved_samples[c], c, length / 4 / 2 , 1);
		}
		
	
	}

	if (kradpulse->kradaudio->process_callback != NULL) {
		kradpulse->kradaudio->process_callback(length / 4 / 2, kradpulse->kradaudio->userdata);
	}

}

static void kradpulse_playback_cb(pa_stream *stream, size_t length, void *userdata) {

	krad_pulse_t *kradpulse = (krad_pulse_t *)userdata;

	pa_usec_t usec;
	int neg;
	
	int c, s;
	
	pa_stream_get_latency(stream, &usec, &neg);
	
	//printf("  latency %8d us wanted %d frames\n", (int)usec, length / 4 / 2 );
	 
	if (kradpulse->kradaudio->process_callback != NULL) {
		kradpulse->kradaudio->process_callback(length / 4 / 2, kradpulse->kradaudio->userdata);
	}
	 
	if ((jack_ringbuffer_read_space (kradpulse->kradaudio->output_ringbuffer[1]) >= length / 2 ) && (jack_ringbuffer_read_space (kradpulse->kradaudio->output_ringbuffer[0]) >= length / 2 )) {
	
		for (c = 0; c < 2; c++) {
			jack_ringbuffer_read (kradpulse->kradaudio->output_ringbuffer[c], (char *)kradpulse->samples[c], (length / 2) );
		}

		for (s = 0; s < length / 4 / 2; s++) {
			for (c = 0; c < 2; c++) {
				kradpulse->interleaved_samples[s * 2 + c] = kradpulse->samples[c][s];
			}
		}
		
		for (c = 0; c < 2; c++) {
		    compute_peak(kradpulse->kradaudio, KOUTPUT, &kradpulse->interleaved_samples[c], c, length / 4 / 2 , 1);
		}
		
	
	} else {
	
		for (s = 0; s < length / 4 / 2; s++) {
			for (c = 0; c < 2; c++) {
				kradpulse->interleaved_samples[s * 2 + c] = 0.0f;
			}
		}
	
	}
	
	pa_stream_write(stream, kradpulse->interleaved_samples, length, NULL, 0LL, PA_SEEK_RELATIVE);

}

static void kradpulse_stream_underflow_cb(pa_stream *s, void *userdata) {

	krad_pulse_t *kradpulse = (krad_pulse_t *)userdata;

	// We increase the latency by 50% if we get 6 underflows and latency is under 2s
	// This is very useful for over the network playback that can't handle low latencies
	printf("underflow\n");
	kradpulse->underflows++;
	if (kradpulse->underflows >= 6 && kradpulse->latency < 2000000) {
		kradpulse->latency = (kradpulse->latency*3)/2;
		kradpulse->bufattr.maxlength = pa_usec_to_bytes(kradpulse->latency, &kradpulse->ss);
		kradpulse->bufattr.tlength = pa_usec_to_bytes(kradpulse->latency, &kradpulse->ss);  
		pa_stream_set_buffer_attr(s, &kradpulse->bufattr, NULL, NULL);
		kradpulse->underflows = 0;
		printf("latency increased to %d\n", kradpulse->latency);
	}
}


void *kradpulse_loop_thread(void *arg) {

	krad_pulse_t *kradpulse = (krad_pulse_t *)arg;

	// Iterate the main loop and go again.  The second argument is whether
	// or not the iteration should block until something is ready to be
	// done.  Set it to zero for non-blocking.

	while (kradpulse->shutdown == 0) {
		pa_mainloop_iterate(kradpulse->pa_ml, 1, NULL);
	}
	
	return NULL;
	
}

void kradpulse_destroy(krad_pulse_t *kradpulse) {

	kradpulse->shutdown = 1;

	pthread_join(kradpulse->loop_thread, NULL);
	
	// clean up and disconnect
	pa_context_disconnect(kradpulse->pa_ctx);
	pa_context_unref(kradpulse->pa_ctx);
	pa_mainloop_free(kradpulse->pa_ml);

	free(kradpulse->samples[0]);
	free(kradpulse->samples[1]);

	free(kradpulse->interleaved_samples);
	
	free(kradpulse->capture_samples[0]);
	free(kradpulse->capture_samples[1]);

	free(kradpulse->capture_interleaved_samples);
	
	free(kradpulse);

}


krad_pulse_t *kradpulse_create(krad_audio_t *kradaudio) {


	krad_pulse_t *kradpulse;
	
	if ((kradpulse = calloc (1, sizeof (krad_pulse_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}
	
	kradpulse->kradaudio = kradaudio;


	kradpulse->samples[0] = malloc(24 * 8192);
	kradpulse->samples[1] = malloc(24 * 8192);
	kradpulse->interleaved_samples = malloc(48 * 8192);

	kradpulse->capture_samples[0] = malloc(24 * 8192);
	kradpulse->capture_samples[1] = malloc(24 * 8192);
	kradpulse->capture_interleaved_samples = malloc(48 * 8192);

	kradpulse->latency = 20000; // start latency in micro seconds
	kradpulse->underflows = 0;
	kradpulse->pa_ready = 0;
	kradpulse->retval = 0;

	// Create a mainloop API and connection to the default server
	kradpulse->pa_ml = pa_mainloop_new();
	kradpulse->pa_mlapi = pa_mainloop_get_api(kradpulse->pa_ml);
	kradpulse->pa_ctx = pa_context_new(kradpulse->pa_mlapi, kradpulse->kradaudio->name);
	
	pa_context_connect(kradpulse->pa_ctx, NULL, 0, NULL);

	// This function defines a callback so the server will tell us it's state.
	// Our callback will wait for the state to be ready.  The callback will
	// modify the variable to 1 so we know when we have a connection and it's
	// ready.
	// If there's an error, the callback will set pa_ready to 2
	pa_context_set_state_callback(kradpulse->pa_ctx, kradpulse_state_cb, kradpulse);

	// We can't do anything until PA is ready, so just iterate the mainloop
	// and continue
	while (kradpulse->pa_ready == 0) {
		pa_mainloop_iterate(kradpulse->pa_ml, 1, NULL);
	}

	if (kradpulse->pa_ready == 2) {
		kradpulse->retval = -1;
		printf("pulseaudio fail\n");
		exit(1);
	}

	kradpulse->ss.rate = 48000;
	kradpulse->ss.channels = 2;
	kradpulse->ss.format = PA_SAMPLE_FLOAT32LE;
	
	kradpulse->bufattr.fragsize = (uint32_t)-1;
	kradpulse->bufattr.maxlength = pa_usec_to_bytes(kradpulse->latency, &kradpulse->ss);
	kradpulse->bufattr.minreq = pa_usec_to_bytes(0, &kradpulse->ss);
	kradpulse->bufattr.prebuf = (uint32_t)-1;
	kradpulse->bufattr.tlength = pa_usec_to_bytes(kradpulse->latency, &kradpulse->ss);
	
	if ((kradaudio->direction == KOUTPUT) || (kradaudio->direction == KDUPLEX)) {
		kradpulse->playstream = pa_stream_new(kradpulse->pa_ctx, "Playback", &kradpulse->ss, NULL);
	
		if (!kradpulse->playstream) {
			printf("playback pa_stream_new failed\n");
			exit(1);
		}
	
		pa_stream_set_write_callback(kradpulse->playstream, kradpulse_playback_cb, kradpulse);
		pa_stream_set_underflow_callback(kradpulse->playstream, kradpulse_stream_underflow_cb, kradpulse);
	 
		kradpulse->r = pa_stream_connect_playback(kradpulse->playstream, NULL, &kradpulse->bufattr, 
									 			  PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);

		if (kradpulse->r < 0) {
			printf("pa_stream_connect_playback failed\n");
			kradpulse->retval = -1;
			printf("pulseaudio fail\n");
			exit(1);
		}
	
	}
	
	if ((kradaudio->direction == KINPUT) || (kradaudio->direction == KDUPLEX)) {
		kradpulse->capturestream = pa_stream_new(kradpulse->pa_ctx, "Capture", &kradpulse->ss, NULL);
	
		if (!kradpulse->capturestream) {
			printf("capture pa_stream_new failed\n");
			exit(1);
		}
		
	
		pa_stream_set_read_callback(kradpulse->capturestream, kradpulse_capture_cb, kradpulse);
		pa_stream_set_underflow_callback(kradpulse->capturestream, kradpulse_stream_underflow_cb, kradpulse);
	 
		kradpulse->r = pa_stream_connect_record(kradpulse->capturestream, NULL, &kradpulse->bufattr, PA_STREAM_NOFLAGS );

		if (kradpulse->r < 0) {
			printf("pa_stream_connect_capture failed\n");
			kradpulse->retval = -1;
			printf("pulseaudio fail\n");
			exit(1);
		}
		
	}
	
	
	kradaudio->sample_rate = kradpulse->ss.rate;
	

	pthread_create( &kradpulse->loop_thread, NULL, kradpulse_loop_thread, kradpulse);

	return kradpulse;

}

