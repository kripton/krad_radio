#include "krad_codec2.h"

/*
		krad_link->krad_codec2_decoder = 
			krad_codec2_decoder_create(1, krad_link->krad_radio->krad_mixer->sample_rate);
		krad_link->krad_codec2_encoder = 
			krad_codec2_encoder_create(1, krad_link->krad_radio->krad_mixer->sample_rate);
	
	if (krad_link->krad_codec2_decoder != NULL) {
		krad_codec2_decoder_destroy(krad_link->krad_codec2_decoder);
	}
	
	if (krad_link->krad_codec2_encoder != NULL) {
		krad_codec2_encoder_destroy(krad_link->krad_codec2_encoder);
	}
	
*/

/*
	unsigned char *codec2_buffer;
	int codec2_bytes;

	codec2_bytes = 0;
	codec2_buffer = calloc(1, 8192 * 4 * 4);

	if (codec2_test == 1) {
		//krad_smash (audio, 960);
		if (c == 0) {
			codec2_bytes = krad_codec2_encode(krad_link->krad_codec2_encoder, audio, 960, codec2_buffer);
			memset(audio, '0', 960 * 4);
			returned_samples = krad_codec2_decode(krad_link->krad_codec2_decoder, codec2_buffer, codec2_bytes, audio);
			//kradaudio_write (krad_link->krad_audio, c, (char *)audio, returned_samples * 4 );
		} else {
			memset(audio, '0', 960 * 4);
			//kradaudio_write (krad_link->krad_audio, c, (char *)audio, returned_samples * 4 );
		}
	}

	free(codec2_buffer);
	
*/

int krad_codec2_encode(krad_codec2_t *krad_codec2, float *audio, int frames, unsigned char *buffer) {

	int codec2_frames;
	int byte_position;
	int sample_position;
	
	sample_position = 0;
	byte_position = 0;

	if (frames == 0) {
		return 0;
	}

	codec2_frames = (frames / 6) / 160;
	//printf("encode begin\n");
	
	if (KRAD_CODEC2_USE_SRC_RESAMPLER) {
	
		krad_codec2->src_data.data_in = audio;
		krad_codec2->src_data.input_frames = frames;
		krad_codec2->src_data.data_out = krad_codec2->float_samples_out;
		krad_codec2->src_data.output_frames = KRAD_CODEC2_SLICE_SIZE_MAX;
		krad_codec2->src_error = src_process (krad_codec2->src_resampler, &krad_codec2->src_data);
		if (krad_codec2->src_error != 0) {
			printke ("krad_codec2_encode src resampler error: %s\n", src_strerror(krad_codec2->src_error));
		}
		if (krad_codec2->src_data.input_frames_used != frames) {
			failfast ("\n\n\n******oh no src resampler did not consume all samples!\n\n\n*********\n");
		}
		
		if (krad_codec2->src_run_count == 0) {
			krad_codec2->src_run_count++;
			return 0;
		} else {
			if (krad_codec2->src_run_count == 1) {
				krad_codec2->expected_input_frames_used = krad_codec2->src_data.input_frames_used;
				krad_codec2->expected_output_frames_gen = krad_codec2->src_data.output_frames_gen;
				krad_codec2->src_run_count++;
			} else {
				if ((krad_codec2->src_data.input_frames_used != krad_codec2->expected_input_frames_used) || (krad_codec2->src_data.output_frames_gen != krad_codec2->expected_output_frames_gen)) {
					failfast ("krad_codec2_encode src resampler took %ld gave %ld\n", krad_codec2->src_data.input_frames_used, krad_codec2->src_data.output_frames_gen);
				}
			}
		}
	
		krad_codec2_float_to_int16 (krad_codec2->short_samples_out, krad_codec2->float_samples_out, krad_codec2->src_data.output_frames_gen);
	} else {
		krad_codec2_float_to_int16 (krad_codec2->short_samples_in, audio, frames);
		krad_linear_resample_48_8 (krad_codec2->short_samples_out, krad_codec2->short_samples_in, frames);
	}
	
	while (codec2_frames) {
		codec2_encode (krad_codec2->codec2, buffer + byte_position, krad_codec2->short_samples_out + sample_position);
		sample_position += KRAD_CODEC2_SAMPLES_PER_FRAME;
		byte_position += KRAD_CODEC2_BYTES_PER_FRAME;
		codec2_frames--;
	}
	//printf("encode end f%d bp%d sp%d\n", codec2_frames, byte_position, sample_position);
	return byte_position;

}

void krad_codec2_encoder_destroy(krad_codec2_t *krad_codec2) {

	codec2_destroy (krad_codec2->codec2);
	free (krad_codec2->float_samples_in);
	free (krad_codec2->float_samples_out);
	free (krad_codec2->short_samples_in);
	free (krad_codec2->short_samples_out);
	src_delete (krad_codec2->src_resampler);
	free (krad_codec2);

}

krad_codec2_t *krad_codec2_encoder_create(int input_channels, int input_sample_rate) {

	krad_codec2_t *krad_codec2 = calloc(1, sizeof(krad_codec2_t));

	krad_codec2->input_channels = input_channels;
	krad_codec2->input_sample_rate = input_sample_rate;

	krad_codec2->short_samples_in = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->short_samples_out = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->float_samples_in = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->float_samples_out = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	
	krad_codec2->src_resampler = src_new (KRAD_CODEC2_SRC_QUALITY, 1, &krad_codec2->src_error);
	if (krad_codec2->src_resampler == NULL) {
		printf("krad_codec2_encoder_create src resampler error: %s\n", src_strerror(krad_codec2->src_error));
	}
	
	krad_codec2->src_data.src_ratio = 8000.0 / input_sample_rate;
	
	printf("krad_codec2_encoder_create src resampler ratio is: %f\n", krad_codec2->src_data.src_ratio);
	
	krad_codec2->codec2 = codec2_create(0);
	
	return krad_codec2;

}


krad_codec2_t *krad_codec2_decoder_create(int output_channels, int output_sample_rate) {

	krad_codec2_t *krad_codec2 = calloc(1, sizeof(krad_codec2_t));

	krad_codec2->output_channels = output_channels;
	krad_codec2->output_sample_rate = output_sample_rate;

	krad_codec2->short_samples_in = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->short_samples_out = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->float_samples_in = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->float_samples_out = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	
	krad_codec2->src_resampler = src_new (KRAD_CODEC2_SRC_QUALITY, 1, &krad_codec2->src_error);
	if (krad_codec2->src_resampler == NULL) {
		printf("krad_codec2_decoder_create src resampler error: %s\n", src_strerror(krad_codec2->src_error));
	}
	
	krad_codec2->src_data.src_ratio = output_sample_rate / 8000.0;
	
	printf("krad_codec2_decoder_create src resampler ratio is: %f\n", krad_codec2->src_data.src_ratio);
	
	krad_codec2->codec2 = codec2_create(0);
	
	return krad_codec2;

}


void krad_codec2_decoder_destroy(krad_codec2_t *krad_codec2) {

	codec2_destroy (krad_codec2->codec2);
	free (krad_codec2->float_samples_in);
	free (krad_codec2->float_samples_out);
	free (krad_codec2->short_samples_in);
	free (krad_codec2->short_samples_out);
	src_delete (krad_codec2->src_resampler);
	free (krad_codec2);

}

int krad_codec2_decode(krad_codec2_t *krad_codec2, unsigned char *buffer, int len, float *audio) {

	int codec2_frames;
	int byte_position;
	int sample_position;
	
	sample_position = 0;
	byte_position = 0;

	if (len == 0) {
		return 0;
	}

	codec2_frames = len / 7;
	//printf("decode begin\n");
	while (codec2_frames) {
		codec2_decode (krad_codec2->codec2, krad_codec2->short_samples_in + sample_position, buffer + byte_position);
		sample_position += KRAD_CODEC2_SAMPLES_PER_FRAME;
		byte_position += KRAD_CODEC2_BYTES_PER_FRAME;
		codec2_frames--;
	}
	
	if (KRAD_CODEC2_USE_SRC_RESAMPLER) {
		krad_codec2_int16_to_float (krad_codec2->float_samples_in, krad_codec2->short_samples_in, sample_position);
	
		krad_codec2->src_data.data_in = krad_codec2->float_samples_in;
		krad_codec2->src_data.input_frames = sample_position;
		krad_codec2->src_data.data_out = audio;
		krad_codec2->src_data.output_frames = KRAD_CODEC2_SLICE_SIZE_MAX;
		krad_codec2->src_error = src_process (krad_codec2->src_resampler, &krad_codec2->src_data);
		if (krad_codec2->src_error != 0) {
			printf("krad_codec2_decode src resampler error: %s\n", src_strerror(krad_codec2->src_error));
		}
		if (krad_codec2->src_run_count == 0) {
			krad_codec2->src_run_count++;
			return 0;
		} else {
			if (krad_codec2->src_run_count == 1) {
				krad_codec2->expected_input_frames_used = krad_codec2->src_data.input_frames_used;
				krad_codec2->expected_output_frames_gen = krad_codec2->src_data.output_frames_gen;
				krad_codec2->src_run_count++;
			} else {
				if ((krad_codec2->src_data.input_frames_used != krad_codec2->expected_input_frames_used) || (krad_codec2->src_data.output_frames_gen != krad_codec2->expected_output_frames_gen)) {
					printf("krad_codec2_encode src resampler took %ld gave %ld\n", krad_codec2->src_data.input_frames_used, krad_codec2->src_data.output_frames_gen);
					exit(1);
				}
			}
		}
		
		if (krad_codec2->src_data.input_frames_used != sample_position) {
			printf("\n\n\n******oh no src resampler did not consume all samples!\n\n\n*********\n");
			exit(1);
		}
	
		return krad_codec2->src_data.output_frames_gen;
	
	} else {
		krad_linear_resample_8_48 (krad_codec2->short_samples_out, krad_codec2->short_samples_in, sample_position);
		krad_codec2_int16_to_float (audio, krad_codec2->short_samples_out, sample_position * 6);
		//printf("decode end f%d bp%d sp%d\n", codec2_frames, byte_position, sample_position);
		return sample_position * 6;
	}
	
}

#define NORMALIZED_FLOAT_MIN -1.0f
#define NORMALIZED_FLOAT_MAX  1.0f
#define SAMPLE_16BIT_MAX  32767
#define SAMPLE_16BIT_MIN  -32767
#define SAMPLE_16BIT_SCALING  32767.0f
#define f_round(f) lrintf(f)

#define float_16(s, d)\
	if ((s) <= NORMALIZED_FLOAT_MIN) {\
		(d) = SAMPLE_16BIT_MIN;\
	} else if ((s) >= NORMALIZED_FLOAT_MAX) {\
		(d) = SAMPLE_16BIT_MAX;\
	} else {\
		(d) = f_round ((s) * SAMPLE_16BIT_SCALING);\
	}

void krad_codec2_float_to_int16 (int16_t *dst,  float *src, int len) {
	while (len--) {
		float_16 (*src, *((int16_t*) dst));
		dst++;
		src++;
	}
}

void krad_codec2_int16_to_float (float *out, int16_t *in, int len) {

	while (len)	{	
		len--;
		out[len] = (float) ((in[len] << 16) / (8.0 * 0x10000000));
	};

}


void krad_linear_resample_8_48 (short * output, short * input, int n) {

	// output ought to be 6 times as large as input (48000/8000).

	int i;
  
	for (i = 0; i < n - 1; i++) {
		output[i*6+0] = input[i]*6/6 + input[i+1]*0/6;
		output[i*6+1] = input[i]*5/6 + input[i+1]*1/6;
		output[i*6+2] = input[i]*4/6 + input[i+1]*2/6;
		output[i*6+3] = input[i]*3/6 + input[i+1]*3/6;
		output[i*6+4] = input[i]*2/6 + input[i+1]*4/6;
		output[i*6+5] = input[i]*1/6 + input[i+1]*5/6;
	}

}

void krad_linear_resample_48_8 (short * output, short * input, int n) {

	// output ought to be 6 times as small as input (8000/48000).

	int i;
  
	for (i = 0; i < n - 1; i++) {
		output[i] = (input[i*6+0] + input[i*6+1] + input[i*6+2] + input[i*6+3] + input[i*6+4] + input[i*6+5]) / 6;
	}

}

void krad_smash (float *samples, int len) {

	int16_t short_samples_in[KRAD_CODEC2_SLICE_SIZE_MAX];
	int16_t short_samples_out[KRAD_CODEC2_SLICE_SIZE_MAX];
	
	krad_codec2_float_to_int16 (short_samples_in,  samples, len);
	krad_linear_resample_48_8 (short_samples_out, short_samples_in, len);
	krad_linear_resample_8_48 (short_samples_in, short_samples_out, len / 6);
	krad_codec2_int16_to_float (samples, short_samples_in, len);

}


void krad_codec2_decoder_int24_to_float_array (const int *in, float *out, int len);
void krad_codec2_decoder_int16_to_float_array (const int *in, float *out, int len);
void krad_codec2_decoder_info(krad_codec2_t *krad_codec2);

