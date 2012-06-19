#include "krad_resample_ring.h"


uint32_t krad_resample_ring_read_space (krad_resample_ring_t *krad_resample_ring) {
	return krad_ringbuffer_read_space (krad_resample_ring->krad_ringbuffer);
}

uint32_t krad_resample_ring_write_space (krad_resample_ring_t *krad_resample_ring) {
	return krad_ringbuffer_write_space (krad_resample_ring->krad_ringbuffer) / 2;
}

uint32_t krad_resample_ring_read (krad_resample_ring_t *krad_resample_ring, unsigned char *dest, uint32_t cnt) {
	return krad_ringbuffer_read (krad_resample_ring->krad_ringbuffer, (char *)dest, cnt);
}

uint32_t krad_resample_ring_write (krad_resample_ring_t *krad_resample_ring, unsigned char *src, uint32_t cnt) {
			
	if (krad_resample_ring->inbuffer_pos) {
		memcpy (krad_resample_ring->inbuffer + krad_resample_ring->inbuffer_pos, src, cnt);
		krad_resample_ring->inbuffer_pos += cnt;
		krad_resample_ring->src_data.data_in = (float *)krad_resample_ring->inbuffer;
		krad_resample_ring->src_data.input_frames = krad_resample_ring->inbuffer_pos / 4;				
	} else {
		krad_resample_ring->src_data.data_in = (float *)src;
		krad_resample_ring->src_data.input_frames = cnt / 4;
	}

	krad_resample_ring->src_data.data_out = (float *)krad_resample_ring->outbuffer;
	krad_resample_ring->src_data.output_frames = (8192 * 2) / 4;

	krad_resample_ring->src_error = src_process (krad_resample_ring->src_resampler, &krad_resample_ring->src_data);

	if (krad_resample_ring->src_error != 0) {
		failfast ("krad_resample_ring src resampler error: %s", src_strerror (krad_resample_ring->src_error));
	}

	if (krad_resample_ring->inbuffer_pos) {
	
		memmove (krad_resample_ring->inbuffer, 
				 krad_resample_ring->inbuffer + krad_resample_ring->src_data.input_frames_used * 4,
				 krad_resample_ring->inbuffer_pos - krad_resample_ring->src_data.input_frames_used * 4);
				 
		krad_resample_ring->inbuffer_pos -= krad_resample_ring->src_data.input_frames_used * 4;
	} else {

		if ((krad_resample_ring->src_data.input_frames_used) != (cnt / 4)) {
			memcpy (krad_resample_ring->inbuffer,
					src + (krad_resample_ring->src_data.input_frames_used * 4),
					(cnt - (krad_resample_ring->src_data.input_frames_used * 4)));
			krad_resample_ring->inbuffer_pos += (cnt - (krad_resample_ring->src_data.input_frames_used * 4));
		}
	}
	
	return krad_ringbuffer_write (krad_resample_ring->krad_ringbuffer,
						  (char *)krad_resample_ring->outbuffer,
								  (krad_resample_ring->src_data.output_frames_gen * 4));
}


void krad_resample_ring_destroy (krad_resample_ring_t *krad_resample_ring) {

	krad_ringbuffer_free (krad_resample_ring->krad_ringbuffer);
	free (krad_resample_ring->inbuffer);
	free (krad_resample_ring->outbuffer);
	src_delete (krad_resample_ring->src_resampler);
	free (krad_resample_ring);

}

void krad_resample_ring_set_input_sample_rate (krad_resample_ring_t *krad_resample_ring, int input_sample_rate) {

	krad_resample_ring->input_sample_rate = input_sample_rate;
	
	krad_resample_ring->src_data.src_ratio = 
		(float)krad_resample_ring->output_sample_rate / (float)krad_resample_ring->input_sample_rate;
	
	printk ("krad_resample_ring src resampler ratio is: %f", krad_resample_ring->src_data.src_ratio);	
	

}

krad_resample_ring_t *krad_resample_ring_create (uint32_t size, int input_sample_rate, int output_sample_rate) {

	krad_resample_ring_t *krad_resample_ring = calloc (1, sizeof(krad_resample_ring_t));

	krad_resample_ring->speed = 100.0;

	krad_resample_ring->size = size;
	
	krad_resample_ring->inbuffer = malloc (8192 * 2);	
	krad_resample_ring->outbuffer = malloc (8192 * 2);
	
	krad_resample_ring->output_sample_rate = output_sample_rate;	
	
	krad_resample_ring->src_resampler = src_new (KRAD_RESAMPLE_RING_SRC_QUALITY, 1, &krad_resample_ring->src_error);
	if (krad_resample_ring->src_resampler == NULL) {
		failfast ("krad_resample_ring src resampler error: %s", src_strerror (krad_resample_ring->src_error));
	}
	
	krad_resample_ring_set_input_sample_rate (krad_resample_ring, input_sample_rate);

	krad_resample_ring->krad_ringbuffer = krad_ringbuffer_create (krad_resample_ring->size);
	
	return krad_resample_ring;

}
