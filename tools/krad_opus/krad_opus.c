#include "krad_opus.h"

/* Encoding */

void krad_opus_encoder_destroy (krad_opus_t *krad_opus) {
	
	int c;
	
	for (c = 0; c < krad_opus->channels; c++) {
		krad_ringbuffer_free ( krad_opus->ringbuf[c] );
		krad_ringbuffer_free ( krad_opus->resampled_ringbuf[c] );
		free (krad_opus->resampled_samples[c]);
		free (krad_opus->samples[c]);
		free (krad_opus->read_samples[c]);
		src_delete (krad_opus->src_resampler[c]);
	}
	
	
	free (krad_opus->opustags_header);
	free (krad_opus->opus_header);
	opus_multistream_encoder_destroy (krad_opus->encoder);
	free (krad_opus);

}

krad_opus_t *krad_opus_encoder_create (int channels, int input_sample_rate, int bitrate, int application) {

	int c;

	krad_opus_t *krad_opus = calloc(1, sizeof(krad_opus_t));

	krad_opus->opus_header = calloc(1, sizeof(OpusHeader));

	krad_opus->input_sample_rate = input_sample_rate;
	krad_opus->channels = channels;
	krad_opus->bitrate = bitrate;
	krad_opus->application = application;
	krad_opus->complexity = DEFAULT_OPUS_COMPLEXITY;
	krad_opus->signal = OPUS_AUTO;
	krad_opus->frame_size = DEFAULT_OPUS_FRAME_SIZE;
	krad_opus->bandwidth = OPUS_BANDWIDTH_FULLBAND;

	krad_opus->new_frame_size = krad_opus->frame_size;
	krad_opus->new_complexity = krad_opus->complexity;
	krad_opus->new_bitrate = krad_opus->bitrate;
	krad_opus->new_signal = krad_opus->signal;
	krad_opus->new_bandwidth = krad_opus->bandwidth;
	
	for (c = 0; c < krad_opus->channels; c++) {
		krad_opus->ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		krad_opus->resampled_ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		krad_opus->samples[c] = malloc(16 * 8192);
		krad_opus->resampled_samples[c] = malloc(16 * 8192);
		krad_opus->src_resampler[c] = src_new (KRAD_OPUS_SRC_QUALITY, 1, &krad_opus->src_error[c]);
		if (krad_opus->src_resampler[c] == NULL) {
			failfast ("Krad Opus Encoder: src resampler error: %s", src_strerror (krad_opus->src_error[c]));
		}
		
		krad_opus->src_data[c].src_ratio = 48000.0 / krad_opus->input_sample_rate;
	}

	if (krad_opus->channels < 3) {

		krad_opus->streams = 1;
		
		if (krad_opus->channels == 2) {
			krad_opus->coupled_streams = 1;
			krad_opus->mapping[0] = 0;
			krad_opus->mapping[1] = 1;			
		} else {
			krad_opus->coupled_streams = 0;		
			krad_opus->mapping[0] = 0;
		}
		
		krad_opus->opus_header->channel_mapping = 0;

	} else {
		krad_opus->opus_header->channel_mapping = 1;

		switch (krad_opus->channels) {
			case 3:
				krad_opus->streams = 2;
				krad_opus->coupled_streams = 1;
				krad_opus->mapping[0] = 0;
				krad_opus->mapping[1] = 1;
				krad_opus->mapping[2] = 2;
			case 4:
				krad_opus->streams = 2;
				krad_opus->coupled_streams = 2;
				krad_opus->mapping[0] = 0;
				krad_opus->mapping[1] = 1;
				krad_opus->mapping[2] = 2;
				krad_opus->mapping[3] = 3;				
			case 5:
				krad_opus->streams = 3;
				krad_opus->coupled_streams = 2;
				krad_opus->mapping[0] = 0;
				krad_opus->mapping[1] = 4;
				krad_opus->mapping[2] = 1;
				krad_opus->mapping[3] = 2;				
				krad_opus->mapping[4] = 3;
			case 6:
				krad_opus->streams = 4;
				krad_opus->coupled_streams = 2;
				krad_opus->mapping[0] = 0;
				krad_opus->mapping[1] = 4;
				krad_opus->mapping[2] = 1;
				krad_opus->mapping[3] = 2;				
				krad_opus->mapping[4] = 3;
				krad_opus->mapping[5] = 5;
			case 7:
				krad_opus->streams = 5;
				krad_opus->coupled_streams = 2;
				krad_opus->mapping[0] = 0;
				krad_opus->mapping[1] = 4;
				krad_opus->mapping[2] = 1;
				krad_opus->mapping[3] = 2;				
				krad_opus->mapping[4] = 3;
				krad_opus->mapping[5] = 5;
				krad_opus->mapping[6] = 6;				
			case 8:
				krad_opus->streams = 5;
				krad_opus->coupled_streams = 3;
				krad_opus->mapping[0] = 0;
				krad_opus->mapping[1] = 6;
				krad_opus->mapping[2] = 1;
				krad_opus->mapping[3] = 2;				
				krad_opus->mapping[4] = 3;
				krad_opus->mapping[5] = 4;
				krad_opus->mapping[6] = 5;
				krad_opus->mapping[7] = 7;				
		}
	}
	
	krad_opus->opus_header->channels = krad_opus->channels;
	krad_opus->opus_header->nb_streams = krad_opus->streams;
	krad_opus->opus_header->nb_coupled = krad_opus->coupled_streams;
	
	memcpy (krad_opus->opus_header->stream_map, krad_opus->mapping, 256);

	krad_opus->encoder = opus_multistream_encoder_create (48000,
														  krad_opus->channels,
														  krad_opus->streams,
														  krad_opus->coupled_streams,
														  krad_opus->mapping,
														  krad_opus->application,
														  &krad_opus->err);

	if (krad_opus->err != OPUS_OK) {
		failfast ("Krad Opus Encoder: Cannot create encoder: %s", opus_strerror (krad_opus->err));
	}
	

	opus_multistream_encoder_ctl (krad_opus->encoder, OPUS_GET_LOOKAHEAD (&krad_opus->lookahead));
	krad_opus->opus_header->preskip = krad_opus->lookahead;

	krad_opus->opus_header->gain = 0;
	krad_opus->opus_header->input_sample_rate = 48000;
	
	if (opus_multistream_encoder_ctl (krad_opus->encoder, OPUS_SET_BITRATE (krad_opus->bitrate)) != OPUS_OK) {
		failfast ("Krad Opus Encoder: bitrate request failed");
	}

	krad_opus->header_data_size = opus_header_to_packet (krad_opus->opus_header, krad_opus->header_data, 100);
	krad_opus->krad_codec_header.header[0] = krad_opus->header_data;
	krad_opus->krad_codec_header.header_size[0] = krad_opus->header_data_size;
	krad_opus->krad_codec_header.header_combined = krad_opus->header_data;
	krad_opus->krad_codec_header.header_combined_size = krad_opus->header_data_size;
	krad_opus->krad_codec_header.header_count = 2;

	krad_opus->krad_codec_header.header_size[1] = 
	8 + 4 + strlen (opus_get_version_string ()) + 4 + 4 + strlen ("ENCODER=") + strlen (APPVERSION);
	
	krad_opus->opustags_header = calloc (1, krad_opus->krad_codec_header.header_size[1]);
	
	memcpy (krad_opus->opustags_header, "OpusTags", 8);
	
	krad_opus->opustags_header[8] = strlen (opus_get_version_string ());
	
	memcpy (krad_opus->opustags_header + 12, opus_get_version_string (), strlen (opus_get_version_string ()));

	krad_opus->opustags_header[12 + strlen (opus_get_version_string ())] = 1;

	krad_opus->opustags_header[12 + strlen (opus_get_version_string ()) + 4] = strlen ("ENCODER=") + strlen (APPVERSION);
	
	memcpy (krad_opus->opustags_header + 12 + strlen (opus_get_version_string ()) + 4 + 4, "ENCODER=", strlen ("ENCODER="));
	
	memcpy (krad_opus->opustags_header + 12 + strlen (opus_get_version_string ()) + 4 + 4 + strlen ("ENCODER="),
			APPVERSION,
			strlen (APPVERSION));	
	
	krad_opus->krad_codec_header.header[1] = krad_opus->opustags_header;
	
	return krad_opus;

}

int krad_opus_encoder_write (krad_opus_t *krad_opus, int channel, char *buffer, int buffer_length) {
	return krad_ringbuffer_write (krad_opus->ringbuf[channel - 1], buffer, buffer_length );
}

int krad_opus_get_bitrate (krad_opus_t *krad_opus) { 
	return krad_opus->bitrate;
}

int krad_opus_get_complexity (krad_opus_t *krad_opus) { 
	return krad_opus->complexity;
}

int krad_opus_get_frame_size (krad_opus_t *krad_opus) { 
	return krad_opus->frame_size;
}

int krad_opus_get_signal (krad_opus_t *krad_opus) { 
	return krad_opus->signal;
}

int krad_opus_get_bandwidth (krad_opus_t *krad_opus) { 
	return krad_opus->bandwidth;
}

void krad_opus_set_bitrate (krad_opus_t *krad_opus, int bitrate) { 
	krad_opus->new_bitrate = bitrate;
}

void krad_opus_set_complexity (krad_opus_t *krad_opus, int complexity) { 
	krad_opus->new_complexity = complexity;
}

void krad_opus_set_frame_size (krad_opus_t *krad_opus, int frame_size) { 
	krad_opus->new_frame_size = frame_size;
}

void krad_opus_set_signal (krad_opus_t *krad_opus, int signal) { 
	krad_opus->new_signal = signal;
}

void krad_opus_set_bandwidth (krad_opus_t *krad_opus, int bandwidth) { 
	krad_opus->new_bandwidth = bandwidth;
}

int krad_opus_encoder_read (krad_opus_t *krad_opus, unsigned char *buffer, int *nframes) {

	int ready;
	int bytes;
	int resp;
	int s, c;

	while (krad_ringbuffer_read_space (krad_opus->ringbuf[krad_opus->channels - 1]) >= 512 * 4 ) {
		   
		for (c = 0; c < krad_opus->channels; c++) {

			krad_opus->ret = krad_ringbuffer_peek (krad_opus->ringbuf[c], (char *)krad_opus->samples[c], (512 * 4) );
			
			krad_opus->src_data[c].data_in = krad_opus->samples[c];
			krad_opus->src_data[c].input_frames = 512;
			krad_opus->src_data[c].data_out = krad_opus->resampled_samples[c];
			krad_opus->src_data[c].output_frames = 2048;
			krad_opus->src_error[c] = src_process (krad_opus->src_resampler[c], &krad_opus->src_data[c]);
			if (krad_opus->src_error[c] != 0) {
				failfast ("Krad Opus Encoder: src resampler error: %s\n", src_strerror(krad_opus->src_error[c]));
			}
			krad_ringbuffer_read_advance (krad_opus->ringbuf[c], (krad_opus->src_data[c].input_frames_used * 4) );
			krad_opus->ret = krad_ringbuffer_write (krad_opus->resampled_ringbuf[c],
										   (char *)krad_opus->resampled_samples[c],
										           (krad_opus->src_data[c].output_frames_gen * 4) );
		}
	}
	
	if (krad_opus->new_bitrate != krad_opus->bitrate) {
		krad_opus->bitrate = krad_opus->new_bitrate;
		resp = opus_multistream_encoder_ctl (krad_opus->encoder, OPUS_SET_BITRATE(krad_opus->bitrate));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: bitrate request failed %s\n", opus_strerror (resp));
		} else {
			printk  ("Krad Opus Encoder: set opus bitrate %d\n", krad_opus->bitrate);
		}
	}
	
	if (krad_opus->new_frame_size != krad_opus->frame_size) {
		krad_opus->frame_size = krad_opus->new_frame_size;
		printk ("Krad Opus Encoder: frame size is now %d\n", krad_opus->frame_size);		
	}
	
	if (krad_opus->new_complexity != krad_opus->complexity) {
		krad_opus->complexity = krad_opus->new_complexity;
		resp = opus_multistream_encoder_ctl (krad_opus->encoder, OPUS_SET_COMPLEXITY(krad_opus->complexity));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: complexity request failed %s. \n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: set opus complexity %d\n", krad_opus->complexity);
		}
	}	

	if (krad_opus->new_signal != krad_opus->signal) {
		krad_opus->signal = krad_opus->new_signal;
		resp = opus_multistream_encoder_ctl (krad_opus->encoder, OPUS_SET_SIGNAL(krad_opus->signal));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: signal request failed %s\n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: set opus signal mode %d\n", krad_opus->signal);
		}
	}
	
	if (krad_opus->new_bandwidth != krad_opus->bandwidth) {
		krad_opus->bandwidth = krad_opus->new_bandwidth;
		resp = opus_multistream_encoder_ctl (krad_opus->encoder, OPUS_SET_BANDWIDTH(krad_opus->bandwidth));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: bandwidth request failed %s\n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: Set Opus bandwidth mode %d\n", krad_opus->bandwidth);
		}
	}
		
	ready = 1;
	
	for (c = 0; c < krad_opus->channels; c++) {
		if (krad_ringbuffer_read_space (krad_opus->resampled_ringbuf[c]) < krad_opus->frame_size * 4) {
			ready = 0;
		}
	}

	if (ready == 1) {

		for (c = 0; c < krad_opus->channels; c++) {
			krad_opus->ret = krad_ringbuffer_read (krad_opus->resampled_ringbuf[c],
										  (char *)krad_opus->resampled_samples[c],
										          (krad_opus->frame_size * 4) );
		}

		for (s = 0; s < krad_opus->frame_size; s++) {
			for (c = 0; c < krad_opus->channels; c++) {
				krad_opus->interleaved_resampled_samples[s * krad_opus->channels + c] = krad_opus->resampled_samples[c][s];
			}
		}

		bytes = opus_multistream_encode_float (krad_opus->encoder,
											   krad_opus->interleaved_resampled_samples,
											   krad_opus->frame_size,
											   buffer,
											   krad_opus->frame_size * 2);

		if (bytes < 0) {
			failfast ("Krad Opus Encoding failed: %s.", opus_strerror (bytes));
		}

		*nframes = krad_opus->frame_size;

		return bytes;
	}
	
	return 0;

}


/* Decoding */


void krad_opus_decoder_destroy (krad_opus_t *krad_opus) {
	
	int c;
	
	for (c = 0; c < krad_opus->channels; c++) {
		krad_ringbuffer_free ( krad_opus->ringbuf[c] );
		krad_ringbuffer_free ( krad_opus->resampled_ringbuf[c] );
		free (krad_opus->resampled_samples[c]);
		free (krad_opus->samples[c]);
		free (krad_opus->read_samples[c]);
		src_delete (krad_opus->src_resampler[c]);
	}
	free (krad_opus->interleaved_samples);
	free (krad_opus->opus_header);
	opus_multistream_decoder_destroy (krad_opus->decoder);
	free (krad_opus);

}

krad_opus_t *krad_opus_decoder_create (unsigned char *header_data, int header_length, float output_sample_rate) {

	int c;

	krad_opus_t *krad_opus = calloc (1, sizeof(krad_opus_t));

	krad_opus->output_sample_rate = output_sample_rate;

	krad_opus->opus_header = calloc (1, sizeof(OpusHeader));
	
	if (opus_header_parse (header_data, header_length, krad_opus->opus_header) != 1) {
		failfast ("krad_opus_decoder_create problem reading opus header");	
	}

	// oops
	//krad_opus->input_sample_rate = krad_opus->opus_header->input_sample_rate;

	krad_opus->channels = krad_opus->opus_header->channels;

	krad_opus->interleaved_samples = malloc(16 * 8192);
	
	for (c = 0; c < krad_opus->channels; c++) {
		krad_opus->ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		krad_opus->resampled_ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		krad_opus->samples[c] = malloc (16 * 8192);
		krad_opus->read_samples[c] = malloc (16 * 8192);
		krad_opus->resampled_samples[c] = malloc (16 * 8192);

		krad_opus->src_resampler[c] = src_new (KRAD_OPUS_SRC_QUALITY, 1, &krad_opus->src_error[c]);
		if (krad_opus->src_resampler[c] == NULL) {
			failfast ("krad_opus_decoder_create src resampler error: %s", src_strerror (krad_opus->src_error[c]));
		}
	
		krad_opus->src_data[c].src_ratio = output_sample_rate / 48000;
	
		printk ("krad_opus_decoder_create src resampler ratio is: %f", krad_opus->src_data[c].src_ratio);	

	}

	krad_opus->streams = krad_opus->opus_header->nb_streams;
	krad_opus->coupled_streams = krad_opus->opus_header->nb_coupled;

	memcpy (krad_opus->mapping, krad_opus->opus_header->stream_map, 256);

	printk ("krad_opus_decoder_create channels %d streams %d coupled %d",
			krad_opus->channels,
			krad_opus->streams,
			krad_opus->coupled_streams
			);	

	krad_opus->decoder = opus_multistream_decoder_create (48000,
														  krad_opus->opus_header->channels,
														  krad_opus->streams,
														  krad_opus->coupled_streams,
														  krad_opus->mapping,
														  &krad_opus->opus_decoder_error);
	if (krad_opus->opus_decoder_error != OPUS_OK) {
		failfast ("Cannot create decoder: %s", opus_strerror (krad_opus->opus_decoder_error));
	}

	return krad_opus;

}

int krad_opus_decoder_write (krad_opus_t *krad_opus, unsigned char *buffer, int length) {

	int i;
	
	int frames_decoded;

	krad_opus->opus_decoder_error = opus_multistream_decode_float (krad_opus->decoder,
																  buffer,
																  length,
																  krad_opus->interleaved_samples,
																  2880 * 2,
																  0);

	if (krad_opus->opus_decoder_error < 0) {
		failfast ("Krad Opus decoder error: %d\n", krad_opus->opus_decoder_error);
	} else {
		frames_decoded = krad_opus->opus_decoder_error;
	}

	for (i = 0; i < frames_decoded; i++) {
		krad_opus->samples[0][i] = krad_opus->interleaved_samples[i * 2 + 0];
		krad_opus->samples[1][i] = krad_opus->interleaved_samples[i * 2 + 1];
	}


	krad_ringbuffer_write (krad_opus->ringbuf[0], (char *)krad_opus->samples[0], (frames_decoded * 4) );
	krad_ringbuffer_write (krad_opus->ringbuf[1], (char *)krad_opus->samples[1], (frames_decoded * 4) );


	return 0;

}

int krad_opus_decoder_read (krad_opus_t *krad_opus, int channel, char *buffer, int buffer_length) {

	
	int resample_process_size = 512;

	krad_opus->ret = krad_ringbuffer_peek (krad_opus->resampled_ringbuf[channel - 1], (char *)buffer, buffer_length );

	if (krad_opus->ret >= buffer_length) {
		krad_ringbuffer_read_advance (krad_opus->resampled_ringbuf[channel - 1], buffer_length );
		return krad_opus->ret;
	} else {

		while (krad_ringbuffer_read_space (krad_opus->resampled_ringbuf[channel - 1]) < buffer_length) {

			if (krad_ringbuffer_read_space (krad_opus->ringbuf[channel - 1]) >= resample_process_size * 4 ) {

				krad_opus->ret = krad_ringbuffer_peek (krad_opus->ringbuf[channel - 1],
											  (char *)krad_opus->read_samples[channel - 1],
											          (resample_process_size * 4) );
			
				krad_opus->src_data[channel - 1].data_in = krad_opus->read_samples[channel - 1];
				krad_opus->src_data[channel - 1].input_frames = resample_process_size;
				krad_opus->src_data[channel - 1].data_out = krad_opus->resampled_samples[channel - 1];
				krad_opus->src_data[channel - 1].output_frames = 2048;
				krad_opus->src_error[channel - 1] = src_process (krad_opus->src_resampler[channel - 1],
																&krad_opus->src_data[channel - 1]);
				if (krad_opus->src_error[channel - 1] != 0) {
					failfast ("krad_opus_read_audio src resampler error: %s\n",
							  src_strerror(krad_opus->src_error[channel - 1]));
				}
				
				krad_ringbuffer_read_advance (krad_opus->ringbuf[channel - 1],
											 (krad_opus->src_data[channel - 1].input_frames_used * 4) );

				krad_opus->ret = krad_ringbuffer_write (krad_opus->resampled_ringbuf[channel - 1],
				                               (char *)krad_opus->resampled_samples[channel - 1],
				                                       (krad_opus->src_data[channel - 1].output_frames_gen * 4) );

				if (krad_ringbuffer_read_space (krad_opus->resampled_ringbuf[channel - 1]) >= buffer_length ) {
					return krad_ringbuffer_read (krad_opus->resampled_ringbuf[channel - 1], buffer, buffer_length );
				}

			} else {
				return 0;
			}
		
		}

		
	}
	
	return 0;
	
}


