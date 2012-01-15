#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <krad_ebml_creator.h>

#define APPVERSION "Krad EBML creator cmd 0.3"

#define DEFAULT_DEVICE "/dev/video0"


int krad_shutdown;


void krad_ebml_creator_signal(int signum) {

	printf("Krad: Got signal %d\n", signum);
	krad_shutdown = 1;
}

void krad_ebml_creator_setup_signals() {

	//signal(SIGCLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, krad_ebml_creator_signal);
	signal(SIGINT, krad_ebml_creator_signal);
	signal(SIGTERM, krad_ebml_creator_signal);

}

void help () {

	printf("Hi. Welcome to krad ebml creator cmd line edition\n");
	printf("krad_ebml_creator_cmd opts\n");
	printf("-a for alsa, -j for jack and -p for pulse [default]\n");
	exit(0);

}

int main (int argc, char *argv[]) {

	krad_shutdown = 0;

	krad_ebml_creator_setup_signals();

	krad_ebml_creator_t *krad_ebml_creator;

	int c;
	
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
	
	if (1 == argc) {
    	help();
    }

	krad_ebml_creator = krad_ebml_creator_create(APPVERSION, krad_audio_api);
	
	//krad_ebml_creator_stream(krad_ebml_creator, "/home/oneman/kode/teststreamfile.krado");
	
	krad_ebml_creator_stream_video(krad_ebml_creator, "/home/oneman/kode/teststreamfile.webm");
	
	while (1) {
		if (krad_shutdown == 1) {
			krad_ebml_creator_stop_wait(krad_ebml_creator);
			break;
		}
		usleep(25000);
	}
	
	krad_ebml_creator_destroy(krad_ebml_creator);
	
	return 0;

}
