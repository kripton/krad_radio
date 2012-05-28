#include "krad_opus.h"

void kradopus_decoder_destroy(krad_opus_t *kradopus) {
	
	int c;
	
	for (c = 0; c < kradopus->channels; c++) {
		krad_ringbuffer_free ( kradopus->ringbuf[c] );
		krad_ringbuffer_free ( kradopus->resampled_ringbuf[c] );
		free(kradopus->resampled_samples[c]);
		free(kradopus->samples[c]);
		free(kradopus->read_samples[c]);
		src_delete (kradopus->src_resampler[c]);
	}
	free(kradopus->interleaved_samples);
	free (kradopus->opus_header);
	opus_multistream_decoder_destroy(kradopus->decoder);
	free (kradopus);

}

void kradopus_encoder_destroy(krad_opus_t *kradopus) {
	
	int c;
	
	for (c = 0; c < kradopus->channels; c++) {
		krad_ringbuffer_free ( kradopus->ringbuf[c] );
		krad_ringbuffer_free ( kradopus->resampled_ringbuf[c] );
		free (kradopus->resampled_samples[c]);
		free (kradopus->samples[c]);
		free (kradopus->read_samples[c]);
		src_delete (kradopus->src_resampler[c]);
	}
	
	
	free (kradopus->opustags_header);
	free (kradopus->opus_header);
	opus_multistream_encoder_destroy (kradopus->st);
	free (kradopus);

}


krad_opus_t *kradopus_decoder_create(unsigned char *header_data, int header_length, float output_sample_rate) {

	int c;

	krad_opus_t *opus = calloc(1, sizeof(krad_opus_t));

	opus->output_sample_rate = output_sample_rate;

	opus->opus_header = calloc(1, sizeof(OpusHeader));
	
	opus_header_parse(header_data, header_length, opus->opus_header);
		
	//printf("kradopus_decoder_create opus header: length %d version: %d channels: %d preskip: %d input sample rate: %d gain %d channel_mapping: %d nb_streams: %d nb_coupled: %d stream map: %s\n", header_length, opus->opus_header->version, opus->opus_header->channels, opus->opus_header->preskip, opus->opus_header->input_sample_rate, opus->opus_header->gain, opus->opus_header->channel_mapping, opus->opus_header->nb_streams, opus->opus_header->nb_coupled, opus->opus_header->stream_map);

	opus->input_sample_rate = opus->opus_header->input_sample_rate;

	//printf("output sample rate is %f\n", output_sample_rate);

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
			printf("kradopus_decoder_create src resampler error: %s\n", src_strerror(opus->src_error[c]));
			exit(1);
		}
	
		opus->src_data[c].src_ratio = output_sample_rate / opus->input_sample_rate;
	
		printf("kradopus_decoder_create src resampler ratio is: %f\n", opus->src_data[c].src_ratio);	

	}

	unsigned char mapping[256] = {0,1};

	opus->decoder = opus_multistream_decoder_create(opus->opus_header->input_sample_rate, opus->opus_header->channels, 1, opus->opus_header->channels==2 ? 1 : 0, mapping, &opus->opus_decoder_error);
	if (opus->opus_decoder_error != OPUS_OK) {
		fprintf(stderr, "Cannot create decoder: %s\n", opus_strerror(opus->opus_decoder_error));
		exit(1);
	}

	return opus;

}


krad_opus_t *kradopus_encoder_create(float input_sample_rate, int channels, int bitrate, int application) {

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
			printf("kradopus_encoder_create src resampler error: %s\n", src_strerror(opus->src_error[c]));
			exit(1);
		}
		
		opus->src_data[c].src_ratio = 48000.0 / input_sample_rate;
	
		//printf("kradopus_encoder_create src resampler ratio is: %f\n", opus->src_data[c].src_ratio);

	}
	
	opus->mapping[0] = 0;
	opus->mapping[1] = 1;
	opus->st_string = "mono";

	//printf("krad opus input sample rate %f\n", input_sample_rate);

	opus->st = opus_multistream_encoder_create(48000, opus->channels, 1, opus->channels==2, opus->mapping, opus->application, &opus->err);

	if (opus->err != OPUS_OK) {
		fprintf(stderr, "Cannot create encoder: %s\n", opus_strerror(opus->err));
		exit(1);
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
		fprintf (stderr, "bitrate request failed\n");
		exit (1);
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

int kradopus_read_audio(krad_opus_t *kradopus, int channel, char *buffer, int buffer_length) {

	
	int resample_process_size = 512;

	kradopus->ret = krad_ringbuffer_peek (kradopus->resampled_ringbuf[channel - 1], (char *)buffer, buffer_length );

	if (kradopus->ret >= buffer_length) {
		krad_ringbuffer_read_advance (kradopus->resampled_ringbuf[channel - 1], buffer_length );
		return kradopus->ret;
	} else {

		while (krad_ringbuffer_read_space (kradopus->resampled_ringbuf[channel - 1]) < buffer_length) {

			if (krad_ringbuffer_read_space (kradopus->ringbuf[channel - 1]) >= resample_process_size * 4 ) {

				kradopus->ret = krad_ringbuffer_peek (kradopus->ringbuf[channel - 1], (char *)kradopus->read_samples[channel - 1], (resample_process_size * 4) );
			
				kradopus->src_data[channel - 1].data_in = kradopus->read_samples[channel - 1];
				kradopus->src_data[channel - 1].input_frames = resample_process_size;
				kradopus->src_data[channel - 1].data_out = kradopus->resampled_samples[channel - 1];
				kradopus->src_data[channel - 1].output_frames = 2048;
				kradopus->src_error[channel - 1] = src_process (kradopus->src_resampler[channel - 1], &kradopus->src_data[channel - 1]);
				if (kradopus->src_error[channel - 1] != 0) {
					printf("kradopus_read_audio src resampler error: %s\n", src_strerror(kradopus->src_error[channel - 1]));
					exit(1);
				}
				
				krad_ringbuffer_read_advance (kradopus->ringbuf[channel - 1], (kradopus->src_data[channel - 1].input_frames_used * 4) );

				kradopus->ret = krad_ringbuffer_write (kradopus->resampled_ringbuf[channel - 1], (char *)kradopus->resampled_samples[channel - 1], (kradopus->src_data[channel - 1].output_frames_gen * 4) );

				if (krad_ringbuffer_read_space (kradopus->resampled_ringbuf[channel - 1]) >= buffer_length ) {
					return krad_ringbuffer_read (kradopus->resampled_ringbuf[channel - 1], buffer, buffer_length );
				}

			} else {
				return 0;
			}
		
		}

		
	}
	
	return 0;
	
}

int kradopus_write_audio(krad_opus_t *kradopus, int channel, char *buffer, int buffer_length) {
	return krad_ringbuffer_write (kradopus->ringbuf[channel - 1], buffer, buffer_length );
}


int kradopus_write_opus(krad_opus_t *kradopus, unsigned char *buffer, int length) {

	int i;

	kradopus->opus_decoder_error = opus_multistream_decode_float(kradopus->decoder, buffer, length, kradopus->interleaved_samples, 960 * 2, 0);

	if (kradopus->opus_decoder_error < 0) {
		printf("decoder error was: %d\n", kradopus->opus_decoder_error);
	}

	//printf("opus decoder: nb frames is %d samples per frame %d\n", opus_packet_get_nb_frames ( buffer, length ), opus_packet_get_samples_per_frame ( buffer, 48000 ));

	for (i = 0; i < 960; i++) {
		kradopus->samples[0][i] = kradopus->interleaved_samples[i * 2 + 0];
		kradopus->samples[1][i] = kradopus->interleaved_samples[i * 2 + 1];
		//printf("sample num %d chan %d is %f\n", i, 0, kradopus->interleaved_samples[i * 2 + 0]); 
		//printf("sample num %d chan %d is %f\n", i, 1, kradopus->interleaved_samples[i * 2 + 1]); 
	}


	krad_ringbuffer_write (kradopus->ringbuf[0], (char *)kradopus->samples[0], (960 * 4) );
	krad_ringbuffer_write (kradopus->ringbuf[1], (char *)kradopus->samples[1], (960 * 4) );


	return 0;

}

int kradopus_get_bitrate (krad_opus_t *kradopus) { 
	return kradopus->bitrate;
}

int kradopus_get_complexity (krad_opus_t *kradopus) { 
	return kradopus->complexity;
}

int kradopus_get_frame_size (krad_opus_t *kradopus) { 
	return kradopus->frame_size;
}

int kradopus_get_signal (krad_opus_t *kradopus) { 
	return kradopus->signal;
}

int kradopus_get_bandwidth (krad_opus_t *kradopus) { 
	return kradopus->bandwidth;
}

void kradopus_set_bitrate (krad_opus_t *kradopus, int bitrate) { 
	kradopus->new_bitrate = bitrate;
}

void kradopus_set_complexity (krad_opus_t *kradopus, int complexity) { 
	kradopus->new_complexity = complexity;
}

void kradopus_set_frame_size (krad_opus_t *kradopus, int frame_size) { 
	kradopus->new_frame_size = frame_size;
}

void kradopus_set_signal (krad_opus_t *kradopus, int signal) { 
	kradopus->new_signal = signal;
}

void kradopus_set_bandwidth (krad_opus_t *kradopus, int bandwidth) { 
	kradopus->new_bandwidth = bandwidth;
}

int kradopus_read_opus (krad_opus_t *kradopus, unsigned char *buffer, int *nframes) {

	int resp;
	
	int i, j, c;

	while ((krad_ringbuffer_read_space (kradopus->ringbuf[1]) >= 512 * 4 ) && (krad_ringbuffer_read_space (kradopus->ringbuf[0]) >= 512 * 4 )) {
		for (c = 0; c < kradopus->channels; c++) {

			kradopus->ret = krad_ringbuffer_peek (kradopus->ringbuf[c], (char *)kradopus->samples[c], (512 * 4) );
			
			kradopus->src_data[c].data_in = kradopus->samples[c];
			kradopus->src_data[c].input_frames = 512;
			kradopus->src_data[c].data_out = kradopus->resampled_samples[c];
			kradopus->src_data[c].output_frames = 2048;
			kradopus->src_error[c] = src_process (kradopus->src_resampler[c], &kradopus->src_data[c]);
			if (kradopus->src_error[c] != 0) {
				failfast ("kradopus_read_opus src resampler error: %s\n", src_strerror(kradopus->src_error[c]));
			}
			krad_ringbuffer_read_advance (kradopus->ringbuf[c], (kradopus->src_data[c].input_frames_used * 4) );
			kradopus->ret = krad_ringbuffer_write (kradopus->resampled_ringbuf[c], (char *)kradopus->resampled_samples[c], (kradopus->src_data[c].output_frames_gen * 4) );
		}
	}
	
	if (kradopus->new_bitrate != kradopus->bitrate) {
		kradopus->bitrate = kradopus->new_bitrate;
		resp = opus_multistream_encoder_ctl (kradopus->st, OPUS_SET_BITRATE(kradopus->bitrate));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: bitrate request failed %s\n", opus_strerror (resp));
		} else {
			printk  ("Krad Opus Encoder: set opus bitrate %d\n", kradopus->bitrate);
		}
	}
	
	if (kradopus->new_frame_size != kradopus->frame_size) {
		kradopus->frame_size = kradopus->new_frame_size;
		printk ("Krad Opus Encoder: frame size is now %d\n", kradopus->frame_size);		
	}
	
	if (kradopus->new_complexity != kradopus->complexity) {
		kradopus->complexity = kradopus->new_complexity;
		resp = opus_multistream_encoder_ctl (kradopus->st, OPUS_SET_COMPLEXITY(kradopus->complexity));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: complexity request failed %s. \n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: set opus complexity %d\n", kradopus->complexity);
		}
	}	

	if (kradopus->new_signal != kradopus->signal) {
		kradopus->signal = kradopus->new_signal;
		resp = opus_multistream_encoder_ctl (kradopus->st, OPUS_SET_SIGNAL(kradopus->signal));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: signal request failed %s\n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: set opus signal mode %d\n", kradopus->signal);
		}
	}
	
	if (kradopus->new_bandwidth != kradopus->bandwidth) {
		kradopus->bandwidth = kradopus->new_bandwidth;
		resp = opus_multistream_encoder_ctl (kradopus->st, OPUS_SET_BANDWIDTH(kradopus->bandwidth));
		if (resp != OPUS_OK) {
			failfast ("Krad Opus Encoder: bandwidth request failed %s\n", opus_strerror(resp));
		} else {
			printk ("Krad Opus Encoder: set opus bandwidth mode %d\n", kradopus->bandwidth);
		}
	}

	if ((krad_ringbuffer_read_space (kradopus->resampled_ringbuf[1]) >= kradopus->frame_size * 4 ) && (krad_ringbuffer_read_space (kradopus->resampled_ringbuf[0]) >= kradopus->frame_size * 4 )) {

		for (c = 0; c < kradopus->channels; c++) {
			kradopus->ret = krad_ringbuffer_read (kradopus->resampled_ringbuf[c], (char *)kradopus->resampled_samples[c], (kradopus->frame_size * 4) );
		}

		for (i = 0; i < kradopus->frame_size; i++) {
			for (j = 0; j < 2; j++) {
				kradopus->interleaved_resampled_samples[i * 2 + j] = kradopus->resampled_samples[j][i];
				//printf("sample num %d chan %d is %f\n", i, j, kradopus->interleaved_resampled_samples[i * 2 + j]); 
			}
		}

		//opus->id++;

		kradopus->num_bytes = opus_multistream_encode_float(kradopus->st, kradopus->interleaved_resampled_samples, kradopus->frame_size, buffer, 500000);

		//printf("opus encoder: nb frames is %d samples per frame %d\n", opus_packet_get_nb_frames ( buffer, kradopus->num_bytes ), opus_packet_get_samples_per_frame ( buffer, 48000 ));

		if (kradopus->num_bytes < 0) {
			fprintf(stderr, "Encoding failed: %s. Aborting.\n", opus_strerror(kradopus->num_bytes));
		 	exit(1);
		}

		*nframes = kradopus->frame_size;

		return kradopus->num_bytes;
	}
	
	return 0;

}

