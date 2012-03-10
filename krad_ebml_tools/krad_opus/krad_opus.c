#include "krad_opus.h"

void kradopus_decoder_destroy(krad_opus_t *kradopus) {
	
	int c;
	
	for (c = 0; c < kradopus->channels; c++) {
		krad_ringbuffer_free ( kradopus->ringbuf[c] );
		//printf("Ringbuffer unlocked and freed (opus)\n");
		krad_ringbuffer_free ( kradopus->resampled_ringbuf[c] );
		//printf("Ringbuffer unlocked and freed (opus resampler)\n");
		free(kradopus->resampled_samples[c]);
		free(kradopus->samples[c]);
		free(kradopus->read_samples[c]);
	}
	free(kradopus->interleaved_samples);
	free (kradopus->opus_header);
	opus_multistream_decoder_destroy(kradopus->decoder);
	if (kradopus->resampler != NULL) {
		speex_resampler_reset_mem(kradopus->resampler);
	}
	free (kradopus);

}

void kradopus_encoder_destroy(krad_opus_t *kradopus) {
	
	int c;
	
	for (c = 0; c < kradopus->channels; c++) {
		krad_ringbuffer_free ( kradopus->ringbuf[c] );
		//printf("Ringbuffer unlocked and freed (opus)\n");
		krad_ringbuffer_free ( kradopus->resampled_ringbuf[c] );
		//printf("Ringbuffer unlocked and freed (opus resampler)\n");
		free(kradopus->resampled_samples[c]);
		free(kradopus->samples[c]);
		free(kradopus->read_samples[c]);
	}
	
	free (kradopus->opus_header);
	opus_multistream_encoder_destroy(kradopus->st);
	if (kradopus->resampler != NULL) {
		speex_resampler_reset_mem(kradopus->resampler);
	}
	free (kradopus);

}


krad_opus_t *kradopus_decoder_create(unsigned char *header_data, int header_length, float output_sample_rate) {

	krad_opus_t *opus = calloc(1, sizeof(krad_opus_t));

	opus->output_sample_rate = output_sample_rate;
	opus->speed = 1.0f;

	opus->opus_header = calloc(1, sizeof(OpusHeader));
	
	opus_header_parse(header_data, header_length, opus->opus_header);
		
	printf("kradopus_decoder_create opus header: length %d version: %d channels: %d preskip: %d input sample rate: %d gain %d channel_mapping: %d nb_streams: %d nb_coupled: %d stream map: %s\n", header_length, opus->opus_header->version, opus->opus_header->channels, opus->opus_header->preskip, opus->opus_header->input_sample_rate, opus->opus_header->gain, opus->opus_header->channel_mapping, opus->opus_header->nb_streams, opus->opus_header->nb_coupled, opus->opus_header->stream_map);

	opus->input_sample_rate = opus->opus_header->input_sample_rate;

	//printf("output sample rate is %f\n", output_sample_rate);

	int c;
	
	
	opus->channels = opus->opus_header->channels;
	
	opus->interleaved_samples = malloc(8192 * 2);
	
	for (c = 0; c < opus->opus_header->channels; c++) {
		opus->ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);

		//printf("Ringbuffer created with %5zu bytes (opus decoder)\n", krad_ringbuffer_write_space(opus->ringbuf[c]));
		
		opus->resampled_ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);

		//printf("Ringbuffer created with %5zu bytes (opus decoder resampler)\n", krad_ringbuffer_write_space(opus->resampled_ringbuf[c]));

		opus->samples[c] = malloc(4 * 8192);
		//printf("opus decoder malloced %d bytes\n", 8192);
		opus->read_samples[c] = malloc(4 * 8192);
		//printf("opus decoder malloced %d bytes\n", 8192);

		opus->resampled_samples[c] = malloc(8192);
		//printf("opus decoder malloced %d bytes\n", 8192);
	}
	
	opus->resampler = speex_resampler_init(opus->channels, opus->opus_header->input_sample_rate, opus->output_sample_rate, 10, &opus->opus_decoder_error);

	//printf("resampler error was: %d\n", opus->opus_decoder_error);

   unsigned char mapping[256] = {0,1};

	opus->decoder = opus_multistream_decoder_create(opus->opus_header->input_sample_rate, opus->opus_header->channels, 1, opus->opus_header->channels==2 ? 1 : 0, mapping, &opus->opus_decoder_error);
	if (opus->opus_decoder_error != OPUS_OK)
	{
		fprintf(stderr, "Cannot create decoder: %s\n", opus_strerror(opus->opus_decoder_error));
		exit(1);
	}


	kradopus_decoder_set_speed(opus, 250);

	return opus;

}


krad_opus_t *kradopus_encoder_create(float input_sample_rate, int channels, int bitrate, int mode) {

	krad_opus_t *opus = calloc(1, sizeof(krad_opus_t));

	opus->opus_header = calloc(1, sizeof(OpusHeader));

	opus->input_sample_rate = input_sample_rate;
	opus->channels = channels;
	opus->bitrate = bitrate;
	opus->mode = mode;
	
	int c;
	
	for (c = 0; c < opus->channels; c++) {
		opus->ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		//printf("Ringbuffer created with %5zu bytes (opus)\n", krad_ringbuffer_write_space(opus->ringbuf[c]));
		opus->resampled_ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
		//printf("Ringbuffer created with %5zu bytes (opus resampler)\n", krad_ringbuffer_write_space(opus->resampled_ringbuf[c]));
		opus->samples[c] = malloc(8192);
		//printf("opus decoder malloced %d bytes\n", 8192);
		opus->resampled_samples[c] = malloc(8192);
		//printf("opus decoder malloced %d bytes\n", 8192);
	}
	
	
	opus->mapping[0] = 0;
	opus->mapping[1] = 1;
	opus->st_string = "mono";

	opus->resampler = speex_resampler_init(opus->channels, opus->input_sample_rate, 48000, 10, &opus->err);

	//printf("resampler error was: %d\n", opus->err);

	opus->frame_size = 960;

	opus->st = opus_multistream_encoder_create(48000, opus->channels, 1, opus->channels==2, opus->mapping, opus->mode, &opus->err);

	if (opus->err != OPUS_OK) {
		fprintf(stderr, "Cannot create encoder: %s\n", opus_strerror(opus->err));
		exit(1);
	}

	/*
	if (opus->bitrate < 64) {
		printf("Using voice mode\n");
		opus_multistream_encoder_ctl(opus->st, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
	} else {
		printf("Using music mode\n");
		opus_multistream_encoder_ctl(opus->st, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
	}
	*/
	
	opus->opus_header->channels = opus->channels;
	opus_multistream_encoder_ctl(opus->st, OPUS_GET_LOOKAHEAD(&opus->lookahead));
	opus->opus_header->preskip = opus->lookahead;
	if (opus->resampler)
	  opus->opus_header->preskip += speex_resampler_get_output_latency(opus->resampler);
	opus->opus_header->channel_mapping = 0;
	opus->opus_header->nb_streams = 1;
	opus->opus_header->nb_coupled = 1;
	/* 0 dB gain is the recommended unless you know what you're doing */
	opus->opus_header->gain = 0;
	opus->opus_header->input_sample_rate = 48000;

	/* Extra samples that need to be read to compensate for the pre-skip */
	opus->extra_samples = (int)opus->opus_header->preskip * (opus->input_sample_rate/48000.);

	if (opus->channels == 2) {
		opus->st_string="stereo";
	}
	
	if (opus_multistream_encoder_ctl(opus->st, OPUS_SET_BITRATE(opus->bitrate)) != OPUS_OK) {
		fprintf (stderr, "bitrate request failed\n");
		exit(1);
	}

	opus->header_data_size = opus_header_to_packet(opus->opus_header, opus->header_data, 100);


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

				spx_uint32_t in_length = resample_process_size;
				spx_uint32_t out_length = in_length * 2;

				kradopus->err = speex_resampler_process_float(kradopus->resampler, channel - 1, kradopus->read_samples[channel - 1], &in_length, kradopus->resampled_samples[channel - 1], &out_length);

				krad_ringbuffer_read_advance (kradopus->ringbuf[channel - 1], (in_length * 4) );

				kradopus->ret = krad_ringbuffer_write (kradopus->resampled_ringbuf[channel - 1], (char *)kradopus->resampled_samples[channel - 1], (out_length * 4) );

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


int kradopus_read_opus(krad_opus_t *kradopus, unsigned char *buffer) {

	
	int i, j, c;

	while (krad_ringbuffer_read_space (kradopus->ringbuf[DEFAULT_CHANNEL_COUNT - 1]) >= 960 * 4 ) {
		for (c = 0; c < kradopus->channels; c++) {

			kradopus->ret = krad_ringbuffer_peek (kradopus->ringbuf[c], (char *)kradopus->samples[c], (960 * 4) );

			spx_uint32_t in_length = 960;
			spx_uint32_t out_length = in_length * 2;

			kradopus->err = speex_resampler_process_float(kradopus->resampler, c, kradopus->samples[c], &in_length, kradopus->resampled_samples[c], &out_length);

			//printf("%d resampler error was: %d\n", c, kradopus->err);
			//printf("%d speex resampler: in len: %d out len: %d\n", c, in_length, out_length);

			krad_ringbuffer_read_advance (kradopus->ringbuf[c], (in_length * 4) );


			kradopus->ret = krad_ringbuffer_write (kradopus->resampled_ringbuf[c], (char *)kradopus->resampled_samples[c], (out_length * 4) );

		}	

	}
	

	if ((krad_ringbuffer_read_space (kradopus->resampled_ringbuf[1]) >= 960 * 4 ) && (krad_ringbuffer_read_space (kradopus->resampled_ringbuf[0]) >= 960 * 4 )) {

		for (c = 0; c < kradopus->channels; c++) {
			kradopus->ret = krad_ringbuffer_read (kradopus->resampled_ringbuf[c], (char *)kradopus->resampled_samples[c], (960 * 4) );
		}

		for (i = 0; i < 960; i++) {
			for (j = 0; j < 2; j++) {
				kradopus->interleaved_resampled_samples[i * 2 + j] = kradopus->resampled_samples[j][i];
				//printf("sample num %d chan %d is %f\n", i, j, kradopus->interleaved_resampled_samples[i * 2 + j]); 
			}
		}

		//opus->id++;

		kradopus->num_bytes = opus_multistream_encode_float(kradopus->st, kradopus->interleaved_resampled_samples, kradopus->frame_size, buffer, 9000);

		//printf("opus encoder: nb frames is %d samples per frame %d\n", opus_packet_get_nb_frames ( buffer, kradopus->num_bytes ), opus_packet_get_samples_per_frame ( buffer, 48000 ));

		if (kradopus->num_bytes < 0) {
			fprintf(stderr, "Encoding failed: %s. Aborting.\n", opus_strerror(kradopus->num_bytes));
		 	exit(1);
		}

		return kradopus->num_bytes;
	}
	
	return 0;

}


void kradopus_decoder_set_speed(krad_opus_t *kradopus, float speed) {

	kradopus->speed = speed * 0.01;

	int output_rate_int;
	int input_rate_int;
	output_rate_int = kradopus->output_sample_rate;
	input_rate_int = kradopus->input_sample_rate * kradopus->speed;
	
	speex_resampler_set_rate(kradopus->resampler, input_rate_int, output_rate_int);

	//printf("res %f  setting speed to INT %d -- %f from %f wihch means a rate of %f\n", kradopus->output_sample_rate, output_rate_int, kradopus->speed, speed, kradopus->output_sample_rate * kradopus->speed);

}


float kradopus_decoder_get_speed(krad_opus_t *kradopus) {

	return kradopus->speed * 100.0;

}

