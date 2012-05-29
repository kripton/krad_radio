#include "krad_ipc_client.h"

int main (int argc, char *argv[]) {

	krad_ipc_client_t *client;
	
	if (argc == 1) {
		printf("Specify a station..\n");
	}
	
	if (argc == 2) {
		printf("Specify a command..\n");
	}	
	
	if (argc > 2) {

		if (!krad_valid_host_and_port (argv[1])) {
			if (!krad_valid_sysname(argv[1])) {
				failfast ("");
			}
		}
	
		client = krad_ipc_connect (argv[1]);
	
		if (client != NULL) {
	
			/* Krad Radio Commands */
			
			if (strncmp(argv[2], "uptime", 6) == 0) {
				krad_ipc_radio_uptime (client);
				krad_ipc_print_response (client);
			}

			if (strncmp(argv[2], "info", 4) == 0) {
				krad_ipc_radio_info (client);
				krad_ipc_print_response (client);
			}
			
			if (strncmp(argv[2], "tag", 3) == 0) {
			
				if (strncmp(argv[2], "tags", 4) == 0) {
	
					krad_ipc_get_tags (client);		
					krad_ipc_print_response (client);
				} else {
			
					if (argc == 4) {
						krad_ipc_get_tag (client, argv[3]);
						krad_ipc_print_response (client);	
					}
				
					if (argc == 5) {
						krad_ipc_set_tag (client, argv[3], argv[4]);
					}
				}
			}

			if (strncmp(argv[2], "remoteon", 8) == 0) {
				if (argc == 4) {
					krad_ipc_enable_remote (client, atoi(argv[3]));
					krad_ipc_print_response (client);
				}
			}			
			
			if (strncmp(argv[2], "remoteoff", 9) == 0) {
				if (argc == 3) {
					krad_ipc_disable_remote (client);
				}
			}
			
			if (strncmp(argv[2], "webon", 5) == 0) {
				if (argc == 5) {
					krad_ipc_webon (client, atoi(argv[3]), atoi(argv[4]));
				}
			}			
			
			if (strncmp(argv[2], "weboff", 5) == 0) {
				if (argc == 3) {
					krad_ipc_weboff (client);
				}
			}			
			
			/* Krad Mixer Commands */
			
			if (strncmp(argv[2], "mix", 3) == 0) {
				if (argc == 3) {
					krad_ipc_get_portgroups (client);
					krad_ipc_print_response (client);
				}
			}
			
			if (strncmp(argv[2], "input", 5) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_create_portgroup (client, argv[3], "input");
				}
			}			

			if (strncmp(argv[2], "output", 6) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_create_portgroup (client, argv[3], "output");
				}
			}

			if (strncmp(argv[2], "unplug", 6) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_remove_portgroup (client, argv[3]);
				}
			}
			
			if (strncmp(argv[2], "xfade", 5) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_update_portgroup (client, argv[3], EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, "");
				}
				if (argc == 5) {
					krad_ipc_mixer_update_portgroup (client, argv[3], EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, argv[4]);
				}
			}			
		
			if (strncmp(argv[2], "set", 3) == 0) {
				if (argc == 6) {
					krad_ipc_set_control (client, argv[3], argv[4], atof(argv[5]));
				}
			}
			
			/* Krad Link Commands */			

			if (strncmp(argv[2], "ls", 2) == 0) {
				if (argc == 3) {
					krad_ipc_list_links (client);
					krad_ipc_print_response (client);
				}
			}
			
			if ((strncmp(argv[2], "link", 4) == 0) || (strncmp(argv[2], "transmit", 8) == 0)) {
				if (argc == 7) {
					if (strncmp(argv[2], "transmitav", 10) == 0) {
						krad_ipc_create_transmit_link (client, AUDIO_AND_VIDEO, argv[3], atoi(argv[4]), argv[5], argv[6], NULL);
					} else {
						krad_ipc_create_transmit_link (client, AUDIO_ONLY, argv[3], atoi(argv[4]), argv[5], argv[6], NULL);
					}
				}
				if (argc == 8) {
					krad_ipc_create_transmit_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], NULL);
				}

				if (argc == 9) {
					krad_ipc_create_transmit_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8]);
				}
				
			}		

			if (strncmp(argv[2], "capture", 7) == 0) {
				if (argc == 4) {
					krad_ipc_create_capture_link (client, krad_link_string_to_video_source (argv[3]));
				}
			}
			
			if (strncmp(argv[2], "record", 6) == 0) {
				if (argc == 4) {
					if (strncmp(argv[2], "recordav", 8) == 0) {
						krad_ipc_create_record_link (client, AUDIO_AND_VIDEO, argv[3], NULL);
					} else {
						krad_ipc_create_record_link (client, AUDIO_ONLY, argv[3], NULL);
					}
				}
				if (argc == 5) {
					krad_ipc_create_record_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], NULL);
				}

				if (argc == 6) {
					krad_ipc_create_record_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5] );
				}
			}
			
			if (strncmp(argv[2], "play", 4) == 0) {
				//if (argc ==  ) {
					//krad_ipc_create_playback_link (client, );
				//}
			}	

			
			if (strncmp(argv[2], "rm", 2) == 0) {
				if (argc == 4) {
					krad_ipc_destroy_link (client, atoi(argv[3]));
				}
			}
			
			if (strncmp(argv[2], "update", 2) == 0) {
				if (argc == 5) {
					krad_ipc_update_link (client, atoi(argv[3]), atoi(argv[4]));
				}
				
				if (argc == 6) {
					if (strcmp(argv[4], "bandwidth") == 0) {
						krad_ipc_update_link_adv (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH, argv[5]);
					}
					if (strcmp(argv[4], "signal") == 0) {
						krad_ipc_update_link_adv (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL, argv[5]);
					}
					if (strcmp(argv[4], "comp") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY, atoi(argv[5]));
					}
					if (strcmp(argv[4], "framesize") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE, atoi(argv[5]));
					}										
					if (strcmp(argv[4], "maxpackets") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE, atoi(argv[5]));
					}
				}				
			}
			
			/* Krad Compositor Commands */			

			if (strncmp(argv[2], "hex", 3) == 0) {
				if (argc == 6) {
					krad_ipc_compositor_hex (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
				}
			}

			if (strncmp(argv[2], "bug", 3) == 0) {
				if (argc == 6) {
					krad_ipc_compositor_bug (client, atoi(argv[3]), atoi(argv[4]), argv[5]);
				}
			}
			
			if (strncmp(argv[2], "vuon", 4) == 0) {
				krad_ipc_compositor_vu (client, 1);
			}			

			if (strncmp(argv[2], "vuoff", 5) == 0) {
				krad_ipc_compositor_vu (client, 0);
			}			

			
			krad_ipc_disconnect (client);
		}
	
	}
	
	return 0;
	
}
