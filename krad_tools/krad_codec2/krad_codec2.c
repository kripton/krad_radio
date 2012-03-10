#include "krad_codec2.h"

void krad_speex_resampler_info() {

	int in_rate, out_rate, qual;
	int in_rates[] = {8000, 16000, 32000, 44100, 48000};
	int out_rates[] = {8000, 16000, 32000, 44100, 48000};
	int qualities[] = {0,1,2,3,4,5,6,7,8,9,10};
	
	SpeexResamplerState *speex_resampler;
	int speex_resampler_error;
	int input_latency;
	int output_latency;
	
	for (in_rate = 0; in_rate < 5; in_rate++) {
		for (out_rate = 0; out_rate < 5; out_rate++) {
			for (qual = 0; qual < 11; qual++) {
				speex_resampler = speex_resampler_init(1, in_rates[in_rate], out_rates[out_rate], qualities[qual], &speex_resampler_error);
				if (speex_resampler_error != 0) {
					printf("speex resampler error! %s\n", speex_resampler_strerror(speex_resampler_error));
				}
				
				input_latency = speex_resampler_get_input_latency(speex_resampler);
				output_latency = speex_resampler_get_output_latency(speex_resampler);
				printf("Speex Resampler Info: Input Rate: %d Output Rate: %d Quality %d Input Delay %d Output Delay %d\n",
					    in_rates[in_rate], out_rates[out_rate], qualities[qual], input_latency, output_latency);
				speex_resampler_destroy(speex_resampler);
			}
		}
	}


}

int krad_codec2_encode(krad_codec2_t *krad_codec2, float *audio, int frames, unsigned char *buffer) {

	int codec2_frames;
	int byte_position;
	int sample_position;
	
	sample_position = 0;
	byte_position = 0;

	codec2_frames = (frames / 6) / 160;
	//printf("encode begin\n");
	
	if (KRAD_CODEC2_USE_SPEEX_RESAMPLER) {
	
		krad_codec2->samples_in = frames;
		krad_codec2->samples_out = KRAD_CODEC2_SLICE_SIZE_MAX;
		krad_codec2->speex_resampler_error = speex_resampler_process_float(krad_codec2->speex_resampler, 0, audio, &krad_codec2->samples_in, krad_codec2->float_samples_out, &krad_codec2->samples_out);
		if (krad_codec2->speex_resampler_error != 0) {
			printf("speex resampler error! %s\n", speex_resampler_strerror(krad_codec2->speex_resampler_error));
		}
		//printf("codec2 encode speex resampler took %d gave %d\n", krad_codec2->samples_in, krad_codec2->samples_out);
		if (krad_codec2->samples_in != frames) {
			printf("\n\n\n******oh no speex resampler did not consume all samples!\n\n\n*********\n");
			exit(1);
		}
		krad_codec2_float_to_int16 (krad_codec2->short_samples_out, krad_codec2->float_samples_out, krad_codec2->samples_out);
	
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

	codec2_destroy(krad_codec2->codec2);
	free(krad_codec2->float_samples_in);
	free(krad_codec2->float_samples_out);
	free(krad_codec2->short_samples_in);
	free(krad_codec2->short_samples_out);
	speex_resampler_destroy(krad_codec2->speex_resampler);
	free(krad_codec2);

}

krad_codec2_t *krad_codec2_encoder_create(int input_channels, int input_sample_rate) {

	krad_codec2_t *krad_codec2 = calloc(1, sizeof(krad_codec2_t));

	krad_codec2->input_channels = input_channels;
	krad_codec2->input_sample_rate = input_sample_rate;

	krad_codec2->short_samples_in = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->short_samples_out = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->float_samples_in = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);
	krad_codec2->float_samples_out = calloc(1, KRAD_CODEC2_SLICE_SIZE_MAX * 4);

	krad_codec2->speex_resampler = speex_resampler_init(1, input_sample_rate, 8000, 1, &krad_codec2->speex_resampler_error);
	if (krad_codec2->speex_resampler_error != 0) {
		printf("speex resampler error! %s\n", speex_resampler_strerror(krad_codec2->speex_resampler_error));
	}
	
	krad_codec2->codec2 = codec2_create();
	
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
	
	krad_codec2->speex_resampler = speex_resampler_init(1, 8000, output_sample_rate, 1, &krad_codec2->speex_resampler_error);
	if (krad_codec2->speex_resampler_error != 0) {
		printf("speex resampler error! %s\n", speex_resampler_strerror(krad_codec2->speex_resampler_error));
	}
	
	krad_codec2->codec2 = codec2_create();
	
	return krad_codec2;

}


void krad_codec2_decoder_destroy(krad_codec2_t *krad_codec2) {

    codec2_destroy(krad_codec2->codec2);
	free(krad_codec2->float_samples_in);
	free(krad_codec2->float_samples_out);
	free(krad_codec2->short_samples_in);
	free(krad_codec2->short_samples_out);
	speex_resampler_destroy(krad_codec2->speex_resampler);
	free(krad_codec2);

}

int krad_codec2_decode(krad_codec2_t *krad_codec2, unsigned char *buffer, int len, float *audio) {

	int codec2_frames;
	int byte_position;
	int sample_position;
	
	sample_position = 0;
	byte_position = 0;

	codec2_frames = len / 7;
	//printf("decode begin\n");
	while (codec2_frames) {
		codec2_decode (krad_codec2->codec2, krad_codec2->short_samples_in + sample_position, buffer + byte_position);
		sample_position += KRAD_CODEC2_SAMPLES_PER_FRAME;
		byte_position += KRAD_CODEC2_BYTES_PER_FRAME;
		codec2_frames--;
	}
	
	if (KRAD_CODEC2_USE_SPEEX_RESAMPLER) {
		krad_codec2_int16_to_float (krad_codec2->float_samples_in, krad_codec2->short_samples_in, sample_position);
		krad_codec2->samples_in = sample_position;
		krad_codec2->samples_out = KRAD_CODEC2_SLICE_SIZE_MAX;
		krad_codec2->speex_resampler_error = speex_resampler_process_float(krad_codec2->speex_resampler, 0, krad_codec2->float_samples_in, &krad_codec2->samples_in, audio, &krad_codec2->samples_out);
		if (krad_codec2->speex_resampler_error != 0) {
			printf("speex resampler error! %s\n", speex_resampler_strerror(krad_codec2->speex_resampler_error));
		}
		//printf("codec2 decode speex resampler took %d gave %d\n", krad_codec2->samples_in, krad_codec2->samples_out);
		if (krad_codec2->samples_in != sample_position) {
			printf("\n\n\n******oh no speex resampler did not consume all samples!\n\n\n*********\n");
			exit(1);
		}
		
		return krad_codec2->samples_out;
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

