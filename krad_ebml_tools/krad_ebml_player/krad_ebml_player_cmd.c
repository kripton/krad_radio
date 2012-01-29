#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "krad_ebml_player.h"


int krad_shutdown;


void krad_opus_ebml_signal(int signum) {

	printf("Krad: Got signal %d\n", signum);
	krad_shutdown = 1;
}

void krad_opus_ebml_setup_signals() {

	//signal(SIGCLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, krad_opus_ebml_signal);
	signal(SIGINT, krad_opus_ebml_signal);
	signal(SIGTERM, krad_opus_ebml_signal);

}

void help () {

	printf("Hi. Welcome to krad ebml player cmd line edition\n");
	printf("krad_ebml_player_cmd opts filename.krad\n");
	printf("-a for alsa, -j for jack and -p for pulse [default]\n");
	exit(0);

}

int main (int argc, char *argv[]) {

	krad_shutdown = 0;

	krad_opus_ebml_setup_signals();

	krad_ebml_player_t *krad_ebml_player;

	int c;
	char *filename = NULL;
	
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
    	help();
    }

	krad_ebml_player = krad_ebml_player_create(krad_audio_api);
	
	//krad_ebml_player_play_stream(krad_ebml_player, "blah");
	krad_ebml_player_play_file(krad_ebml_player, filename);
	
	krad_ebml_player_destroy(krad_ebml_player);
	
	return 0;

}
