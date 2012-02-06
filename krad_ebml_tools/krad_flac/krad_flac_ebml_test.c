#include <krad_ebml.h>
#include "krad_flac.h"
#include "krad_tone.h"
#include "krad_audio.h"

#define TEST_COUNT1 122
#define TEST_COUNT2 1

#define APPVERSION "Krad FLAC EBML TEST 0.42"

void flac_ebml_encode_test() {
	
	krad_flac_t *krad_flac;
	krad_tone_t *krad_tone;
	kradebml_t *krad_ebml;
	//krad_audio_t *audio;
	
	char *filename = "/home/oneman/kode/flacrox.mkv";
	
	int count;
	int audiotrack;
	int bit_depth;
	int sample_rate;
	int channels;
	
	int lingering_frames;
	
	unsigned char *buffer;
	int bytes;
	float *audio;
	
	count = 0;

	channels = 1;
	bit_depth = 16;
	sample_rate = 44100;
	
	krad_tone = krad_tone_create(sample_rate);
	krad_tone_add_preset(krad_tone, "dialtone");

	buffer = calloc(1, 8192 * 8);
	audio = calloc(1, 8192 * channels * 4);

	krad_flac = krad_flac_encoder_create(channels, sample_rate, bit_depth);
	bytes = krad_flac_encoder_read_min_header(krad_flac, buffer);
	
	krad_ebml = kradebml_create();
	//kradebml_open_output_stream(ebml, "192.168.1.2", 9080, "/teststream.webm", "secretkode");
	kradebml_open_output_file(krad_ebml, filename);
	kradebml_header(krad_ebml, "matroska", APPVERSION);
	//videotrack = kradebml_add_video_track(krad_ebml, "V_VP8", 10, width, height);
	audiotrack = kradebml_add_audio_track(krad_ebml, "A_FLAC", sample_rate, channels, buffer, bytes);
	kradebml_write(krad_ebml);
	
	krad_flac_encode_info(krad_flac);
	
	kradebml_cluster(krad_ebml, 0);
	kradebml_write(krad_ebml);
		
	for (count = 0; count < TEST_COUNT1; count++) {
		krad_tone_run(krad_tone, audio, 4096);
		bytes = krad_flac_encode(krad_flac, audio, 4096, buffer);
	
		if (bytes > 0) {
			printf("encoded %d bytes\n", bytes);
			kradebml_add_audio(krad_ebml, audiotrack, buffer, bytes, 4096);
			kradebml_write(krad_ebml);	
		}
		//krad_flac_encode_info(krad_flac);
	}
	
	lingering_frames = krad_flac_encoder_frames_remaining(krad_flac);
	bytes = krad_flac_encoder_finish(krad_flac, buffer);
	printf("got %d flac finiishing bytes\n", bytes);
	kradebml_add_audio(krad_ebml, audiotrack, buffer, bytes, lingering_frames);
	kradebml_write(krad_ebml);
	
	
	//bytes = krad_flac_encoder_read_header(krad_flac, buffer);

	
	kradebml_destroy(krad_ebml);
	
	krad_flac_encoder_destroy(krad_flac);
	krad_tone_destroy(krad_tone);

	free(audio);
	free(buffer);
	
}


void flac_ebml_decode_test(char *inputfile) {

	krad_flac_t *krad_flac;
	kradebml_t *krad_ebml;
	krad_audio_t *krad_audio;
	krad_audio_api_t audio_api;

	char *filename;

	int count;
	
	
	if (inputfile == NULL) {
		filename = "/home/oneman/kode/flacrox.mkv";
	//	filename = "/home/oneman/kode/webmtestfiles/CQZB6jmhM-U.webm";
	//	filename = "/home/oneman/kode/kradlink/test/test_flac2.mkv";
	//	filename = "/home/oneman/kode/kradlink/test/testcox.mkv";
	} else {
		filename = inputfile;
	}

	unsigned char *buffer;
	int bytes;
	float *audio;
	
	
	audio_api = PULSE;
	krad_audio = kradaudio_create("Krad EBML Video HUD Test Player", KOUTPUT, audio_api);

	if (audio_api == JACK) {
		krad_jack_t *jack = (krad_jack_t *)krad_audio->api;
		kradjack_connect_port(jack->jack_client, "Krad EBML Video HUD Test Player:Left", "desktop_recorder:input_1");
		kradjack_connect_port(jack->jack_client, "Krad EBML Video HUD Test Player:Right", "desktop_recorder:input_2");
	}

	for (count = 0; count < TEST_COUNT2; count++) {
	
		krad_flac = krad_flac_decoder_create();

		buffer = calloc(1, 8192 * 8);
		audio = calloc(1, 8192 * 4 * 4);

		krad_ebml = kradebml_create();
		kradebml_open_input_file(krad_ebml, filename);
		//kradebml_open_input_stream(krad_ebml_player->ebml, "192.168.1.2", 9080, "/teststream.krado");
	
		kradebml_debug(krad_ebml);

		bytes = kradebml_read_audio_header(krad_ebml, 1, buffer);

		printf("got flac header bytes of %d\n", bytes);
		//exit(1);
		krad_flac_decode(krad_flac, buffer, bytes, audio);
		
		while ((bytes = kradebml_read_audio(krad_ebml, buffer)) > 0) {
		
			printf("got flac data bytes of %d\n", bytes);
			krad_flac_decode(krad_flac, buffer, bytes, audio);
			kradaudio_write (krad_audio, 0, (char *)audio, 4096 * 4 );
			kradaudio_write (krad_audio, 1, (char *)audio, 4096 * 4 );

			while (kradaudio_buffered_frames(krad_audio) > 44100) {
				usleep(40000);
			}

		}

		while (kradaudio_buffered_frames(krad_audio) > 8192) {
			usleep(40000);
		}
		
		krad_flac_decoder_info(krad_flac);

		krad_flac_decoder_destroy(krad_flac);
		
		kradaudio_destroy(krad_audio);
		
		kradebml_destroy(krad_ebml);
		free(audio);
		free(buffer);
	}

}


int main (int argc, char *argv[]) {

	if (argc == 1) {
		flac_ebml_encode_test();
		flac_ebml_decode_test(NULL);
	}
	
	if (argc == 2) {
		flac_ebml_decode_test(argv[1]);
	}
	
	
	return 0;

}
