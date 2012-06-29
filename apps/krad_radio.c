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
			

			if ((strncmp(argv[2], "ls", 2) == 0) && (strlen(argv[2]) == 2)) {
				if (argc == 3) {
					krad_ipc_list_links (client);
					krad_ipc_print_response (client);

					krad_ipc_compositor_list_ports (client);
					krad_ipc_print_response (client);
					
					krad_ipc_get_portgroups (client);
					krad_ipc_print_response (client);					

				}
			}			
			
			
			if (strncmp(argv[2], "uptime", 6) == 0) {
				krad_ipc_radio_uptime (client);
				krad_ipc_print_response (client);
			}

			if (strncmp(argv[2], "info", 4) == 0) {
				krad_ipc_radio_info (client);
				krad_ipc_print_response (client);
			}
			
			if (strncmp(argv[2], "tags", 4) == 0) {

				if (argc == 3) {
					krad_ipc_get_tags (client, NULL);		
					krad_ipc_print_response (client);
				}
				if (argc == 4) {
					krad_ipc_get_tags (client, argv[3]);		
					krad_ipc_print_response (client);
				}					
				
			} else {
			
				if (strncmp(argv[2], "tag", 3) == 0) {
			
					if (argc == 4) {
						krad_ipc_get_tag (client, NULL, argv[3]);
						krad_ipc_print_response (client);
					}
				
					if (argc == 5) {
						krad_ipc_get_tag (client, argv[3], argv[4]);
						krad_ipc_print_response (client);						
					}				
				}
			}
			
			if (strncmp(argv[2], "stag", 4) == 0) {
				if (argc == 5) {
					krad_ipc_set_tag (client, NULL, argv[3], argv[4]);
				}
				if (argc == 6) {
					krad_ipc_set_tag (client, argv[3], argv[4], argv[5]);
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
			
			if (strncmp(argv[2], "weboff", 6) == 0) {
				if (argc == 3) {
					krad_ipc_weboff (client);
				}
			}
			
			if (strncmp(argv[2], "oscon", 5) == 0) {
				if (argc == 4) {
					krad_ipc_enable_osc (client, atoi(argv[3]));
				}
			}			
			
			if (strncmp(argv[2], "oscoff", 6) == 0) {
				if (argc == 3) {
					krad_ipc_disable_osc (client);
				}
			}
			
			if (strncmp(argv[2], "setdir", 6) == 0) {
				if (argc == 4) {
					krad_ipc_radio_set_dir (client, argv[3]);
				}
			}		
			
			/* Krad Mixer Commands */
			
			if (strncmp(argv[2], "lm", 2) == 0) {
				if (argc == 3) {
					krad_ipc_get_portgroups (client);
					krad_ipc_print_response (client);
				}
			}
			
			if (strncmp(argv[2], "rate", 4) == 0) {
				if (argc == 3) {
					krad_ipc_get_mixer_sample_rate (client);
					krad_ipc_print_response (client);
				}
			}			
			
			if (strncmp(argv[2], "setrate", 7) == 0) {
				if (argc == 4) {
					krad_ipc_set_mixer_sample_rate (client, atoi(argv[3]));
					krad_ipc_print_response (client);
				}
			}				
			
			if (strncmp(argv[2], "tone", 4) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_push_tone (client, argv[3]);
				}
			}			
			
			if (strncmp(argv[2], "input", 5) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_create_portgroup (client, argv[3], "input", 2);
				}
				if (argc == 5) {
					krad_ipc_mixer_create_portgroup (client, argv[3], "input", atoi (argv[4]));
				}				
			}			

			if (strncmp(argv[2], "output", 6) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_create_portgroup (client, argv[3], "output", 2);
				}
				if (argc == 5) {
					krad_ipc_mixer_create_portgroup (client, argv[3], "output", atoi (argv[4]));
				}				
			}

			if (strncmp(argv[2], "unplug", 6) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_remove_portgroup (client, argv[3]);
				}
			}

			if (strncmp(argv[2], "map", 3) == 0) {
				if (argc == 6) {
					krad_ipc_mixer_update_portgroup_map_channel (client, argv[3], atoi(argv[4]), atoi(argv[5]));
				}
			}
			
			if (strncmp(argv[2], "mixmap", 3) == 0) {
				if (argc == 6) {
					krad_ipc_mixer_update_portgroup_mixmap_channel (client, argv[3], atoi(argv[4]), atoi(argv[5]));
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
		
			if (strncmp(argv[2], "xmms2", 5) == 0) {
				if (argc == 5) {
					krad_ipc_mixer_bind_portgroup_xmms2 (client, argv[3], argv[4]);
				}
			}	
		
			if (strncmp(argv[2], "noxmms2", 7) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_unbind_portgroup_xmms2 (client, argv[3]);
				}
			}
		
			if (strncmp(argv[2], "set", 3) == 0) {
				if (argc == 6) {
					krad_ipc_set_control (client, argv[3], argv[4], atof(argv[5]));
				}
			}
			
			/* Krad Link Commands */			

			if ((strncmp(argv[2], "ll", 2) == 0) && (strlen(argv[2]) == 2)) {
				if (argc == 3) {
					krad_ipc_list_links (client);
					krad_ipc_print_response (client);
				}
			}
			
			if (strncmp(argv[2], "listen_on", 9) == 0) {
				if (argc == 4) {
					krad_ipc_enable_linker_listen (client, atoi(argv[3]));
				}
			}
			
			if (strncmp(argv[2], "listen_off", 10) == 0) {
				if (argc == 3) {
					krad_ipc_disable_linker_listen (client);
				}
			}
			
			if (strncmp(argv[2], "transmitter_on", 14) == 0) {
				if (argc == 4) {
					krad_ipc_enable_linker_transmitter (client, atoi(argv[3]));
				}
			}
			
			if (strncmp(argv[2], "transmitter_off", 15) == 0) {
				if (argc == 3) {
					krad_ipc_disable_linker_transmitter (client);
				}
			}		
			
			if ((strncmp(argv[2], "link", 4) == 0) || (strncmp(argv[2], "transmit", 8) == 0)) {
				if (argc == 7) {
					if (strncmp(argv[2], "transmitav", 10) == 0) {
						krad_ipc_create_transmit_link (client, AUDIO_AND_VIDEO, argv[3], atoi(argv[4]), argv[5], argv[6], NULL, 0, 0, 0, 0);
					} else {
						krad_ipc_create_transmit_link (client, AUDIO_ONLY, argv[3], atoi(argv[4]), argv[5], argv[6], NULL, 0, 0, 0, 0);
					}
				}
				if (argc == 8) {
					krad_ipc_create_transmit_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], NULL,
												   0, 0, 0, 0);
				}

				if (argc == 9) {
					krad_ipc_create_transmit_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
												   0, 0, 0, 0);
				}
				
				if (argc == 10) {
					krad_ipc_create_transmit_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
												   atoi(argv[9]), 0, 0, 0);
				}
				
				if (argc == 11) {
					krad_ipc_create_transmit_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
												   atoi(argv[9]), atoi(argv[10]), 0, 0);
				}
				
				if (argc == 12) {
					krad_ipc_create_transmit_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
												   atoi(argv[9]), atoi(argv[10]), atoi(argv[11]), 0);
				}
				
				if (argc == 13) {
					krad_ipc_create_transmit_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
												   atoi(argv[9]), atoi(argv[10]), atoi(argv[11]), atoi(argv[12]));
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
						krad_ipc_create_record_link (client, AUDIO_AND_VIDEO, argv[3], NULL, 0, 0, 0, 0);
					} else {
						krad_ipc_create_record_link (client, AUDIO_ONLY, argv[3], NULL, 0, 0, 0, 0);
					}
				}
				if (argc == 5) {
					krad_ipc_create_record_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], NULL,
												 0, 0, 0, 0);
				}

				if (argc == 6) {
					krad_ipc_create_record_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
												 0, 0, 0, 0);
				}
				
				if (argc == 7) {
					krad_ipc_create_record_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
												 atoi(argv[6]), 0, 0, 0);
				}
				
				if (argc == 8) {
					krad_ipc_create_record_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
												 atoi(argv[6]), atoi(argv[7]), 0, 0);
				}
				
				if (argc == 9) {
					krad_ipc_create_record_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
												 atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), 0);
				}
				
				if (argc == 10) {
					krad_ipc_create_record_link (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
												 atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]));					
				}																
				
			}
			
			if (strncmp(argv[2], "receive", 7) == 0) {
				if (argc == 4) {
					krad_ipc_create_receive_link (client, atoi(argv[3]));
				}
			}				
			
			if (strncmp(argv[2], "play", 4) == 0) {
				if (argc == 4) {
					krad_ipc_create_playback_link (client, argv[3]);
				}
				if (argc == 6) {
					krad_ipc_create_remote_playback_link (client, argv[3], atoi(argv[4]), argv[5] );
				}
			}	

			
			if (strncmp(argv[2], "rm", 2) == 0) {
				if (argc == 4) {
					krad_ipc_destroy_link (client, atoi(argv[3]));
				}
			}
			
			if (strncmp(argv[2], "update", 2) == 0) {

				if (argc == 5) {
					if (strcmp(argv[4], "vp8_keyframe") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_VP8_FORCE_KEYFRAME, 1);
					}
				}

				if (argc == 6) {
				
					if (strcmp(argv[4], "vp8_bitrate") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, atoi(argv[5]));
					}				
					if (strcmp(argv[4], "opus_bitrate") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, atoi(argv[5]));
					}				
					if (strcmp(argv[4], "opus_bandwidth") == 0) {
						krad_ipc_update_link_adv (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH, argv[5]);
					}
					if (strcmp(argv[4], "opus_signal") == 0) {
						krad_ipc_update_link_adv (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL, argv[5]);
					}
					if (strcmp(argv[4], "opus_comp") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY, atoi(argv[5]));
					}
					if (strcmp(argv[4], "opus_framesize") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE, atoi(argv[5]));
					}										
					if (strcmp(argv[4], "ogg_maxpackets") == 0) {
						krad_ipc_update_link_adv_num (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE, atoi(argv[5]));
					}
				}				
			}
			
			/* Krad Compositor Commands */
			
			if ((strncmp(argv[2], "lc", 2) == 0) && (strlen(argv[2]) == 2)) {
				if (argc == 3) {
					krad_ipc_compositor_list_ports (client);
					krad_ipc_print_response (client);
				}
			}
			
			if (strncmp(argv[2], "setport", 7) == 0) {
				if (argc == 10) {
					krad_ipc_compositor_set_port_mode (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]),
													   atoi(argv[6]), atoi(argv[7]), atof(argv[8]), atof(argv[9]));
					krad_ipc_print_response (client);
				}
			}
			
			if (strncmp(argv[2], "snap", 4) == 0) {
				if (argc == 3) {
					krad_ipc_compositor_snapshot (client);
				}
			}					
			
			if (strncmp(argv[2], "comp", 4) == 0) {
				if (argc == 3) {
					krad_ipc_compositor_info (client);
					krad_ipc_print_response (client);
				}
			}
		
			if (strncmp(argv[2], "res", 3) == 0) {
				if (argc == 5) {
					krad_ipc_compositor_set_resolution (client, atoi(argv[3]), atoi(argv[4]));
				}
			}
			
			if (strncmp(argv[2], "fps", 3) == 0) {
				if (argc == 4) {
					krad_ipc_compositor_set_frame_rate (client, atoi(argv[3]) * 1000, 1000);
				}			
				if (argc == 5) {
					krad_ipc_compositor_set_frame_rate (client, atoi(argv[3]), atoi(argv[4]));
				}
			}						

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
			
			if (strncmp(argv[2], "addsprite", 9) == 0) {
				if (argc == 6) {
					krad_ipc_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), 4,
													1.0f, 1.0f, 0.0f);
				}
				if (argc == 7) {
					krad_ipc_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
													1.0f, 1.0f, 0.0f);
				}
				if (argc == 8) {
					krad_ipc_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
													atof(argv[7]), 1.0f, 0.0f);
				}
				if (argc == 9) {
					krad_ipc_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
													atof(argv[7]), atof(argv[8]), 0.0f);
				}
				if (argc == 10) {
					krad_ipc_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
													atof(argv[7]), atof(argv[8]), atof(argv[9]));
				}
			}
			
			if (strncmp(argv[2], "setsprite", 9) == 0) {
				if (argc == 6) {
					krad_ipc_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), 4,
													1.0f, 1.0f, 0.0f);
				}
				if (argc == 7) {
					krad_ipc_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
													1.0f, 1.0f, 0.0f);
				}
				if (argc == 8) {
					krad_ipc_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
													atof(argv[7]), 1.0f, 0.0f);
				}
				if (argc == 9) {
					krad_ipc_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
													atof(argv[7]), atof(argv[8]), 0.0f);
				}
				if (argc == 10) {
					krad_ipc_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
													atof(argv[7]), atof(argv[8]), atof(argv[9]));
				}
			}
			
			if (strncmp(argv[2], "rmsprite", 8) == 0) {
				if (argc == 4) {
					krad_ipc_compositor_remove_sprite (client, atoi(argv[3]));
				}
			}
			
			if (strncmp(argv[2], "lssprite", 8) == 0) {
				if (argc == 3) {
					krad_ipc_compositor_list_sprites (client);
				}
			}											
			
			if (strncmp(argv[2], "background", 10) == 0) {
				if (argc == 4) {
					krad_ipc_compositor_background (client, argv[3]);
				}
			}			
			
			if (strncmp(argv[2], "display", 7) == 0) {
				if (argc == 3) {
					krad_ipc_compositor_open_display (client, 0, 0);
				}
				if (argc == 4) {
					krad_ipc_compositor_open_display (client, 1, 1);
				}					
				if (argc == 5) {
					krad_ipc_compositor_open_display (client, atoi(argv[3]), atoi(argv[4]));
				}				
			}
			
			if (strncmp(argv[2], "closedisplay", 12) == 0) {
				if (argc == 3) {
					krad_ipc_compositor_close_display (client);
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
