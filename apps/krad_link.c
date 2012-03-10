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
			{"bug",				required_argument, 0, 'B'},
			
			{"width",			required_argument, 0, 'w'},
			{"height",			required_argument, 0, 'h'},
			{"fps",				required_argument, 0, 'f'},
			{"mjpeg",			no_argument, 0, 'm'},

			{"key",				required_argument, 0, 'K'},
			{"minkey",			required_argument, 0, 'k'},
			{"maxkey",			required_argument, 0, 'l'},

			{"alsa",			optional_argument, 0, ALSA},
			{"jack",			optional_argument, 0, JACK},
			{"pulse",			optional_argument, 0, PULSE},
			{"tone",			optional_argument, 0, TONE},
				
			{"flac",			optional_argument, 0, FLAC},
			{"opus",			optional_argument, 0, OPUS},
			{"vorbis",			optional_argument, 0, VORBIS},
						
			{"bitrate",			required_argument, 0, 'r'},
			
			{"password",		required_argument, 0, 'p'},
			
			{"x11",				optional_argument, 0, 'X'},
			{"udp",				optional_argument, 0, 'U'},
			
			{0, 0, 0, 0}
		};

		option_index = 0;

		o = getopt_long ( argc, argv, "vhpwh", long_options, &option_index );

		if (o == -1) {
			break;
		}

		switch ( o ) {
			case 'U':
				krad_link->udp_mode = 1;
				krad_link->interface_mode = COMMAND;
				krad_link->video_source = NOVIDEO;
				krad_link->video_codec = NOCODEC;
				krad_link->audio_codec = OPUS;
				krad_link->operation_mode = RECEIVE;
				if (optarg != NULL) {
					krad_link->udp_recv_port = atoi(optarg);
					krad_link->udp_send_port = atoi(optarg);
					break;
				}
				break;
			case 'X':
				krad_link->x11_capture = 1;
				krad_link->capture_width = 1920;
				krad_link->capture_height = 1080;
				krad_link->render_meters = 1;
				break;
			case 'v':
				krad_link->verbose = 1;
				verbose = 1;
				break;
			case 'p':
				strncpy(krad_link->password, optarg, sizeof(krad_link->password));
				krad_link->operation_mode = CAPTURE;
				break;
			case 'm':
				krad_link->mjpeg_mode = 1;
				break;
			case 'w':
				krad_link->capture_width = atoi(optarg);
				break;
			case 't':
				krad_link->video_source = TEST;
				if (krad_link->x11_capture == 0) {
					krad_link->capture_width = 1280;
					krad_link->capture_height = 720;
				}
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
			case 'B':
				strncpy (krad_link->bug, optarg, sizeof(krad_link->bug));
				krad_link->bug_x = 64;
				krad_link->bug_y = 64;
				break;
			case 'L':
				krad_link->transport_mode = UDP;
				break;
			case 'T':
				krad_link->transport_mode = UDP;
				break;
			case 'r':
				krad_link->vpx_encoder_config.rc_target_bitrate = atoi(optarg);
				break;
			case 'K':
				krad_link->vpx_encoder_config.kf_min_dist = atoi(optarg);
				krad_link->vpx_encoder_config.kf_max_dist = atoi(optarg);
				break;
			case 'k':
				krad_link->vpx_encoder_config.kf_min_dist = atoi(optarg);
				break;
			case 'l':
				krad_link->vpx_encoder_config.kf_max_dist = atoi(optarg);
				break;
			case NOVIDEO:
				krad_link->video_source = NOVIDEO;
				krad_link->video_codec = NOCODEC;
				break;
			case FLAC:
				krad_link->audio_codec = FLAC;
				break;
			case OPUS:
				krad_link->audio_codec = OPUS;
				break;
			case VORBIS:
				krad_link->audio_codec = VORBIS;
				if (optarg != NULL) {
					krad_link->vorbis_quality = atof(optarg);
				}	
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
				if (optarg != NULL) {
					strncpy(krad_link->tone_preset, optarg, sizeof(krad_link->tone_preset));
				}
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
				if (optarg != NULL) {
					strncpy(krad_link->jack_ports, optarg, sizeof(krad_link->jack_ports));
				}
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
			
			if (krad_link->udp_mode == 1) {
			
				krad_link->operation_mode = CAPTURE;
				uri = argv[optind];
				memcpy(krad_link->host, uri, strcspn(uri, ":/\0"));
				if (strchr(uri, ':') != NULL) {
					krad_link->udp_send_port = atoi(strchr(uri, ':') + 1);
				}
			
			
			} else {
			
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
			}
			
			optind++;
		}
	}

	krad_link_run ( krad_link );

	dbg("Krad Link cleanly shutdown\n");

	return 0;

}

