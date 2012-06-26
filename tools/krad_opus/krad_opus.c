#include "krad_opus.h"

void krad_opus_decoder_destroy(krad_opus_t *krad_opus) {
	
	int c;
	
	for (c = 0; c < krad_opus->channels; c++) {
		krad_ringbuffer_free ( krad_opus->ringbuf[c] );
		krad_ringbuffer_free ( krad_opus->resampled_ringbuf[c] );
		free(krad_opus->resampled_samples[c]);
		free(krad_opus->samples[c]);
		free(krad_opus->read_samples[c]);
		src_delete (krad_opus->src_resampler[c]);
	}
	free(krad_opus->interleaved_samples);
	free (krad_opus->opus_header);
	opus_multistream_decoder_destroy(krad_opus->decoder);
	free (krad_opus);

}

void krad_opus_encoder_destroy(krad_opus_t *krad_opus) {
	
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
	opus_multistream_encoder_destroy (krad_opus->st);
	free (krad_opus);

}


krad_opus_t *krad_opus_decoder_create(unsigned char *header_data, int header_length, float output_sample_rate) {

	int c;

	krad_opus_t *opus = calloc(1, sizeof(krad_opus_t));

	opus->output_sample_rate = output_sample_rate;

	opus->opus_header = calloc(1, sizeof(OpusHeader));
	
	if (opus_header_parse (header_data, header_length, opus->opus_header) != 1) {
		printk ("krad_opus_decoder_create problem reading opus header");	
	}

	opus->input_sample_rate = opus->opus_header->input_sample_rate;

	opus->channels = opus->opus_header->channels;
	
	opus->interleaved_samples = malloc(16 * 8192);
	
	for (c = 0; c < opus->opus_header->channels; c++) {
		opus->ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		opus->resampled_ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		opus->samples[c] = malloc (16 * 8192);
		opus->read_samples[c] = malloc (16 * 8192);
		opus->resampled_samples[c] = malloc (16 * 8192);

		opus->src_resampler[c] = src_new (KRAD_OPUS_SRC_QUALITY, 1, &opus->src_error[c]);
		if (opus->src_resampler[c] == NULL) {
			failfast ("krad_opus_decoder_create src resampler error: %s\n", src_strerror(opus->src_error[c]));
		}
	
		opus->src_data[c].src_ratio = output_sample_rate / opus->input_sample_rate;
	
		printk ("krad_opus_decoder_create src resampler ratio is: %f\n", opus->src_data[c].src_ratio);	

	}

	unsigned char mapping[256] = {0,1};

	opus->decoder = opus_multistream_decoder_create (opus->opus_header->input_sample_rate,
													 opus->opus_header->channels,
													 1,
													 opus->opus_header->channels==2 ? 1 : 0,
													 mapping,
													 &opus->opus_decoder_error);
	if (opus->opus_decoder_error != OPUS_OK) {
		failfast ("Cannot create decoder: %s\n", opus_strerror(opus->opus_decoder_error));
	}

	return opus;

}


krad_opus_t *krad_opus_encoder_create(float input_sample_rate, int channels, int bitrate, int application) {

	krad_opus_t *opus = calloc(1, sizeof(krad_opus_t));

	opus->opus_header = calloc(1, sizeof(OpusHeader));

	opus->input_sample_rate = input_sample_rate;
	opus->channels = channels;
	opus->bitrate = bitrate;
	opus->application = application;
	opus->complexity = DEFAULT_OPUS_COMPLEXITY;
	opus->signal = OPUS_AUTO;
	opus->frame_size = DEFAULT_OPUS_FRAME_SIZE;
	opus->bandwidth = OPUS_BANDWIDTH_FULLBAND;

	opus->new_frame_size = opus->frame_size;
	opus->new_complexity = opus->complexity;
	opus->new_bitrate = opus->bitrate;
	opus->new_signal = opus->signal;
	opus->new_bandwidth = opus->bandwidth;
	
	int c;
	
	for (c = 0; c < opus->channels; c++) {
		opus->ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		opus->resampled_ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		opus->samples[c] = malloc(16 * 8192);
		opus->resampled_samples[c] = malloc(16 * 8192);

		opus->src_resampler[c] = src_new (KRAD_OPUS_SRC_QUALITY, 1, &opus->src_error[c]);
		if (opus->src_resampler[c] == NULL) {
			failfast ("krad_opus_encoder_create src resampler error: %s\n", src_strerror(opus->src_error[c]));
		}
		
		opus->src_data[c].src_ratio = 48000.0 / input_sample_rate;
	
		//printf("krad_opus_encoder_create src resampler ratio is: %f\n", opus->src_data[c].src_ratio);

	}
	
	opus->mapping[0] = 0;
	opus->mapping[1] = 1;
	opus->st_string = "mono";

	//printf("krad opus input sample rate %f\n", input_sample_rate);

	opus->st = opus_multistream_encoder_create (48000,
												opus->channels,
												1,
												opus->channels==2,
												opus->mapping,
												opus->application,
												&opus->err);

	if (opus->err != OPUS_OK) {
		failfast ("Cannot create encoder: %s\n", opus_strerror(opus->err));
	}
	
	opus->opus_header->channels = opus->channels;
	opus_multistream_encoder_ctl (opus->st, OPUS_GET_LOOKAHEAD(&opus->lookahead));
	opus->opus_header->preskip = opus->lookahead;
	//if (opus->resampler)
	//  opus->opus_header->preskip += speex_resampler_get_output_latency(opus->resampler);
	opus->opus_header->channel_mapping = 0;
	opus->opus_header->nb_streams = 1;
	opus->opus_header->nb_coupled = 1;
	/* 0 dB gain is the recommended unless you know what you're doing */
	opus->opus_header->gain = 0;
	opus->opus_header->input_sample_rate = 48000;

	/* Extra samples that need to be read to compensate for the pre-skip */
	opus->extra_samples = (int)opus->opus_header->preskip * (opus->input_sample_rate/48000.);

	if (opus->channels == 2) {
		opus->st_string = "stereo";
	}
	
	if (opus_multistream_encoder_ctl (opus->st, OPUS_SET_BITRATE (opus->bitrate)) != OPUS_OK) {
		failfast ("bitrate request failed\n");
	}

	opus->header_data_size = opus_header_to_packet(opus->opus_header, opus->header_data, 100);
	opus->krad_codec_header.header[0] = opus->header_data;
	opus->krad_codec_header.header_size[0] = opus->header_data_size;
	opus->krad_codec_header.header_combined = opus->header_data;
	opus->krad_codec_header.header_combined_size = opus->header_data_size;
	opus->krad_codec_header.header_count = 2;

	//opus->krad_codec_header.header_size[1] = 25;
	//opus->krad_codec_header.header[1] = (unsigned char *)"OpusTags\x09\x00\x00\x00KradRadio\x00\x00\x00\x00";
	opus->krad_codec_header.header_size[1] = 
	8 + 4 + strlen (opus_get_version_string ()) + 4 + 4 + strlen ("ENCODER=") + strlen (APPVERSION);
	
	opus->opustags_header = calloc (1, opus->krad_codec_header.header_size[1]);
	
	memcpy (opus->opustags_header, "OpusTags", 8);
	
	opus->opustags_header[8] = strlen (opus_get_version_string ());
	
	memcpy (opus->opustags_header + 12, opus_get_version_string (), strlen (opus_get_version_string ()));

	opus->opustags_header[12 + strlen (opus_get_version_string ())] = 1;

	opus->opustags_header[12 + strlen (opus_get_version_string ()) + 4] = strlen ("ENCODER=") + strlen (APPVERSION);
	
	memcpy (opus->opustags_header + 12 + strlen (opus_get_version_string ()) + 4 + 4, "ENCODER=", strlen ("ENCODER="));
	
	memcpy (opus->opustags_header + 12 + strlen (opus_get_version_string ()) + 4 + 4 + strlen ("ENCODER="),
			APPVERSION,
			strlen (APPVERSION));	
	
	opus->krad_codec_header.header[1] = opus->opustags_header;
	
	return opus;

}

int krad_opus_read_audio(krad_opus_t *krad_opus, int channel, char *buffer, int buffer_length) {

	
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

int krad_opus_write_audio(krad_opus_t *krad_opus, int channel, char *buffer, int buffer_length) {
	return krad_ringbuffer_write (krad_opus->ringbuf[channel - 1], buffer, buffer_length );
}


int krad_opus_write_opus (krad_opus_t *krad_opus, unsigned char *buffer, int length) {

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

int krad_opus_read_opus (krad_opus_t *krad_opus, unsigned char *buffer, int *nframes) {

	int resp;
	
	int i, j, c;

	while ((krad_ringbuffer_read_space (krad_opus->ringbuf[1]) >= 512 * 4 ) &&
		   (krad_ringbuffer_read_space (krad_opus->ringbuf[0]) >= 512 * 4 )) {
		   
		for (c = 0; c < krad_opus->channels; c++) {

			krad_opus->ret = krad_ringbuffer_peek (krad_opus->ringbuf[c], (char *)krad_opus->samples[c], (512 * 4) );
			
			krad_opus->src_data[c].data_in = krad_opus->samples[c];
			krad_opus->src_data[c].input_frames = 512;
			krad_opus->src_data[c].data_out = krad_opus->resampled_samples[c];
			krad_opus->src_data[c].output_frames = 2048;
			krad_opus->src_error[c] = src_process (krad_opus->src_resampler[c], &krad_opus->src_data[c]);
			if (krad_opus->src_error[c] != 0) {
				failfast ("krad_opus_read_opus src resampler error: %s\n", src_strerror(krad_opus->src_error[c]));
			}
			krad_ringbuffer_read_advance (krad_opus->ringbuf[c], (krad_opus->src_data[c].input_frames_used * 4) );
			krad_opus->ret = krad_ringbuffer_write (krad_opus->resampled_ringbuf[c],
										   (char *)krad_opus->resampled_samples[c],
										           (krad_opus->src_data[c].output_frames_gen * 4) );
		}
	}
	
	if (krad_opus->new_bitrate != krad_opus->bitrate) {
		krad_opus->bitrate = krad_opus->new_bitrate;
		resp = opus_multistream_encoder_ctl (krad_opus->st, OPUS_SET_BITRATE(krad_opus->bitrate));
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
		resp = opus_multistream_encoder_ctl (krad_opus->st, OPUS_SET_COMPLEXITY(krad_opus->complexity));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: complexity request failed %s. \n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: set opus complexity %d\n", krad_opus->complexity);
		}
	}	

	if (krad_opus->new_signal != krad_opus->signal) {
		krad_opus->signal = krad_opus->new_signal;
		resp = opus_multistream_encoder_ctl (krad_opus->st, OPUS_SET_SIGNAL(krad_opus->signal));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: signal request failed %s\n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: set opus signal mode %d\n", krad_opus->signal);
		}
	}
	
	if (krad_opus->new_bandwidth != krad_opus->bandwidth) {
		krad_opus->bandwidth = krad_opus->new_bandwidth;
		resp = opus_multistream_encoder_ctl (krad_opus->st, OPUS_SET_BANDWIDTH(krad_opus->bandwidth));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: bandwidth request failed %s\n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: set opus bandwidth mode %d\n", krad_opus->bandwidth);
		}
	}

	if ((krad_ringbuffer_read_space (krad_opus->resampled_ringbuf[1]) >= krad_opus->frame_size * 4 ) &&
		(krad_ringbuffer_read_space (krad_opus->resampled_ringbuf[0]) >= krad_opus->frame_size * 4 )) {

		for (c = 0; c < krad_opus->channels; c++) {
			krad_opus->ret = krad_ringbuffer_read (krad_opus->resampled_ringbuf[c],
										  (char *)krad_opus->resampled_samples[c],
										          (krad_opus->frame_size * 4) );
		}

		for (i = 0; i < krad_opus->frame_size; i++) {
			for (j = 0; j < 2; j++) {
				krad_opus->interleaved_resampled_samples[i * 2 + j] = krad_opus->resampled_samples[j][i];
			}
		}

		krad_opus->num_bytes = opus_multistream_encode_float (krad_opus->st,
															 krad_opus->interleaved_resampled_samples,
															 krad_opus->frame_size,
															 buffer,
															 500000);

		if (krad_opus->num_bytes < 0) {
			failfast ("Encoding failed: %s. Aborting.\n", opus_strerror(krad_opus->num_bytes));
		}

		*nframes = krad_opus->frame_size;

		return krad_opus->num_bytes;
	}
	
	return 0;

}

