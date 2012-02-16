#include "krad_link.h"


extern int do_shutdown;
extern int verbose;

void help() {

	printf("%s\n", APPVERSION);
	
	printf("\nkrad_link [OPTIONS] [URI]\n\n");
	
	printf("--help --verbose\n");

	printf("--nowindow --daemon\n");
	printf("--novideo --noaudio\n");
	
	printf("--alsa --jack --pulse --tone\n");
	printf("--flac --opus\n");
	printf("--password\n");
	
	exit (0);

}

int main ( int argc, char *argv[] ) {

	krad_link_t *krad_link;
	int o;
	int option_index;

	char *uri;
	
	do_shutdown = 0;
	verbose = 0;
	
	krad_link = krad_link_create ();

	krad_link->shutdown = &do_shutdown;

	while (1) {

		static struct option long_options[] = {
			{"verbose",			no_argument, 0, 'v'},
			{"help",			no_argument, 0, 'h'},
	
			{"window",			no_argument, 0, WINDOW},
			{"nowindow",		no_argument, 0, COMMAND},
			{"daemon",			no_argument, 0, DAEMON},
			
			{"novideo",			no_argument, 0, NOVIDEO},
			{"noaudio",			no_argument, 0, NOAUDIO},
			
			{"testscreen",		no_argument, 0, 't'},
			
			{"width",			required_argument, 0, 'w'},
			{"height",			required_argument, 0, 'h'},
			{"fps",				required_argument, 0, 'f'},
			{"mjpeg",			no_argument, 0, 'm'},
			{"yuv",				no_argument, 0, 'y'},

			{"alsa",			optional_argument, 0, ALSA},
			{"jack",			optional_argument, 0, JACK},
			{"pulse",			optional_argument, 0, PULSE},
			{"tone",			optional_argument, 0, TONE},
				
			{"flac",			optional_argument, 0, FLAC},
			{"opus",			optional_argument, 0, OPUS},
			{"vorbis",			optional_argument, 0, VORBIS},
						
			{"bitrate",			no_argument, 0, 'b'},
			{"keyframes",		no_argument, 0, 'k'},
			
			{"password",		required_argument, 0, 'p'},
			
			{0, 0, 0, 0}
		};

		option_index = 0;

		o = getopt_long ( argc, argv, "vhpbwh", long_options, &option_index );

		if (o == -1) {
			break;
		}

		switch ( o ) {
			case 'v':
				krad_link->verbose = 1;
				verbose = 1;
				break;
			case 'p':
				strncpy(krad_link->password, optarg, sizeof(krad_link->password));
				krad_link->operation_mode = CAPTURE;
				break;
			case 'w':
				krad_link->capture_width = atoi(optarg);
				break;
			case 'f':
				krad_link->capture_fps = atoi(optarg);
				break;
			case 'h':
				if (optarg != NULL) {
					krad_link->capture_height = atoi(optarg);
					break;
				} else {
					help ();
				}
		//memcpy(krad_link->bug, bug, sizeof(krad_link->bug));
		//krad_link->bug_x = bug_x;
		//krad_link->bug_y = bug_y;
			case 'b':
				krad_link->vpx_bitrate = atoi(optarg);
				break;
			case 'k':
				//
				break;
			case NOVIDEO:
				krad_link->video_source = NOVIDEO;
				break;
			case FLAC:
				krad_link->audio_codec = FLAC;
				break;
			case OPUS:
				krad_link->audio_codec = OPUS;
				break;
			case VORBIS:
				krad_link->audio_codec = VORBIS;
				break;
			case WINDOW:
				krad_link->interface_mode = WINDOW;
				break;
			case COMMAND:
				krad_link->interface_mode = COMMAND;
				break;
			case DAEMON:
				krad_link->interface_mode = DAEMON;
				break;
			case NOAUDIO:
				krad_link->krad_audio_api = NOAUDIO;
				krad_link->audio_codec = NOCODEC;
				break;
			case TONE:
				krad_link->krad_audio_api = TONE;
				break;
			case PULSE:
				krad_link->krad_audio_api = PULSE;
				break;
			case ALSA:
				krad_link->krad_audio_api = ALSA;
				if (optarg != NULL) {
					strncpy(krad_link->alsa_playback_device, optarg, sizeof(krad_link->alsa_playback_device));
					strncpy(krad_link->alsa_capture_device, optarg, sizeof(krad_link->alsa_capture_device));
				}
				break;
			case JACK:
				krad_link->krad_audio_api = JACK;
				break;
			case HELP:
				help ();
				break;

			default:
				abort ();
		}
	}

	if (optind < argc) {
		
		while (optind < argc) {
			
			//printf ("%s ", argv[optind]);
			//putchar ('\n');
			
			if ((argv[optind][0] == '/') || (argv[optind][0] == '~') || (argv[optind][0] == '.')) {		

				if (strncmp("/dev", argv[optind], 4) == 0) {
					memcpy(krad_link->device, argv[optind], sizeof(krad_link->device));
					krad_link->operation_mode = CAPTURE;
				} else {
				
					if (stat(argv[optind], &krad_link->file_stat) != 0) {
						strncpy(krad_link->output, argv[optind], sizeof(krad_link->output));
						krad_link->operation_mode = CAPTURE;
					} else {
						krad_link->operation_mode = RECEIVE;
						strncpy (krad_link->input, argv[optind], sizeof(krad_link->input));
					}				
				}

			} else {
			
				if (!strlen(krad_link->password)) {
					krad_link->operation_mode = RECEIVE;
				}
				
				krad_link->port = 8080;
				
				if ((strlen(argv[optind]) > 7) && (strncmp(argv[optind], "http://", 7) == 0)) {
					uri = argv[optind] + 7;
				} else {
					uri = argv[optind];
				}
				
				if ((strchr(uri, '/')) && (strlen(uri) < sizeof(krad_link->host))) {
								
					memcpy(krad_link->host, uri, strcspn(uri, ":/"));
					
					if (strchr(uri, ':') != NULL) {
						krad_link->port = atoi(strchr(uri, ':') + 1);
					}
					
					memcpy(krad_link->mount, strchr(uri, '/'), sizeof(krad_link->mount));
					
				} else {
					printf("Missing mountpoint\n");
					exit(1);
				}
			}
			
			optind++;
		}
	}

	krad_link_run ( krad_link );

	dbg("Krad Link cleanly shutdown\n");

	return 0;

}

