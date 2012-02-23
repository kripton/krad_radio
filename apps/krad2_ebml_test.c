#include "krad2_ebml.h"
#include "krad_audio.h"
#include "krad_flac.h"
#include "signal.h"

#define MAX_AUDIO_CHANNELS 8

int do_shutdown;

void krad_test_shutdown() {

	if (do_shutdown == 1) {
		printf("\nKrad Test Terminated.\n");
		exit(1);
	}

	do_shutdown = 1;

}

int main (int argc, char *argv[]) {
	
	krad2_ebml_t *krad2_ebml;
	char *filename;
	unsigned char *buffer;
	int len;
	int tracknum;
	int trackcount;
	int t;
	int playtrack;
	int64_t frames_wrote = 0;
	float *audio;
	float *samples[MAX_AUDIO_CHANNELS];
	int audio_frames;
	int c;
	int audio_channels = 2;
	
	krad_audio_t *krad_audio;
	krad_flac_t *krad_flac;
	krad_codec_t trackcodec;
	
	playtrack = 0;
	
	buffer = calloc(1, 1000000);
	
	krad_audio_api_t krad_audio_api = PULSE;
     
	while ((c = getopt (argc, argv, "ajp")) != -1) {
		switch (c) {
			case 'a':
				krad_audio_api = ALSA;
				break;
			case 'j':
				krad_audio_api = JACK;
				break;
			case 'p':
				krad_audio_api = PULSE;
				break;
			default:			
				break;
		}
	}
		
	if (optind < argc) {
         filename = argv[optind];
    } else {
		printf("Please specify a file\n");
		exit(1);
	}
	
	do_shutdown = 0;
	
	signal(SIGINT, krad_test_shutdown);
	signal(SIGTERM, krad_test_shutdown);
	
	//printf("sd df %d\n", sizeof(krad2_ebml_track_t));
	
	//exit(1);
	
//	krad2_ebml = krad2_ebml_open_stream ( "deimos.kradradio.com", 8080, "/krad_test_flac.mkv" );
	
	krad2_ebml = krad2_ebml_open_file ( filename, KRAD2_EBML_IO_READONLY );
	
	trackcount = krad2_ebml_get_track_count (krad2_ebml);
	
	//printf("track count is %d\n", trackcount);
	
	for (t = 0; t < trackcount; t++) {
		if (krad2_ebml_get_track_codec(krad2_ebml, t + 1) == FLAC) {

			playtrack = t + 1;
			
			trackcodec = FLAC;
			
			krad_flac = krad_flac_decoder_create();

			if (krad_audio_api == ALSA) {
				krad_audio = kradaudio_create("", KOUTPUT, krad_audio_api);
			} else {
				krad_audio = kradaudio_create("Krad E2", KOUTPUT, krad_audio_api);
			}
			
			len = krad2_ebml_get_track_codec_data(krad2_ebml, playtrack, buffer);
			printf("FLAC Header size: %d\n", len);
			krad_flac_decode(krad_flac, buffer, len, NULL);
		
		}
		
	}
	
	printf("playing track %d\n", playtrack);
	
	if (playtrack) {

		for (c = 0; c < audio_channels; c++) {
			samples[c] = malloc(4 * 8192);
		}	
	
		while (((len = krad2_ebml_read_packet ( krad2_ebml, &tracknum, buffer)) > 0) && (do_shutdown == 0)) {

			if (tracknum == playtrack) {
				//printf("Got a packet for track %d sized %d\n", tracknum, len);
				audio_frames = krad_flac_decode(krad_flac, buffer, len, samples);
			
				for (c = 0; c < audio_channels; c++) {
					kradaudio_write (krad_audio, c, (char *)samples[c], audio_frames * 4 );
				}

				frames_wrote += audio_frames;
			
			}
			
			while (kradaudio_buffered_frames(krad_audio) > 44100 * 3) {

				usleep(100000);
				
				printf("Playback time: %6.1f Buffered to: %6.1f \r", 
						((frames_wrote - kradaudio_buffered_frames(krad_audio)) / 44100.0),
						((krad2_ebml->current_cluster_timecode + (int64_t)krad2_ebml->last_timecode)/1000.0));
				
				fflush(stdout);
				
				if (do_shutdown) {
					break;
				}				
			}
		}
		
		printf("\n");
		
		for (c = 0; c < audio_channels; c++) {
			free (samples[c]);
		}

		krad_flac_decoder_destroy (krad_flac);
		kradaudio_destroy (krad_audio);
		
	} else {
	
	
			while (((len = krad2_ebml_read_packet ( krad2_ebml, &tracknum, buffer)) > 0) && (do_shutdown == 0)) {
				
			}
	
	}

	krad2_ebml_destroy ( krad2_ebml );

	free (buffer);

	printf("Shutdown was clean\n");

	return 0;	

}
