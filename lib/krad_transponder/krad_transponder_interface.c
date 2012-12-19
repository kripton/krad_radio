#include "krad_transponder_interface.h"


void krad_transponder_broadcast_link_created ( krad_ipc_server_t *krad_ipc_server, krad_link_t *krad_link) {

	int c;
	uint64_t element;
	uint64_t subelement;

	for (c = 0; c < KRAD_IPC_SERVER_MAX_CLIENTS; c++) {
		//if ((krad_ipc_server->clients[c].confirmed == 1) && (krad_ipc_server->current_client != &krad_ipc_server->clients[c])) {
		if (krad_ipc_server->clients[c].broadcasts == 1) {
			if ((krad_ipc_server->current_client == &krad_ipc_server->clients[c]) || (krad_ipc_aquire_client (&krad_ipc_server->clients[c]))) {
				krad_ebml_start_element (krad_ipc_server->clients[c].krad_ebml2, EBML_ID_KRAD_TRANSPONDER_MSG, &element);	
				krad_ebml_start_element (krad_ipc_server->clients[c].krad_ebml2, EBML_ID_KRAD_TRANSPONDER_LINK_CREATED, &subelement);		
				krad_transponder_link_to_ebml ( &krad_ipc_server->clients[c], krad_link);
				krad_ebml_finish_element (krad_ipc_server->clients[c].krad_ebml2, subelement);
				krad_ebml_finish_element (krad_ipc_server->clients[c].krad_ebml2, element);
				krad_ebml_write_sync (krad_ipc_server->clients[c].krad_ebml2);
				if (krad_ipc_server->current_client != &krad_ipc_server->clients[c]) {
					krad_ipc_release_client (&krad_ipc_server->clients[c]);
				}
				
			}
		}
	}
}

void krad_transponder_ebml_to_link ( krad_ipc_server_t *krad_ipc_server, krad_link_t *krad_link ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;

	char string[512];
	
	memset (string, '\0', 512);

	//FIXME default
	krad_link->av_mode = AUDIO_AND_VIDEO;

	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_TRANSPONDER_LINK) {
		printk ("hrm wtf");
	} else {
		//printk ("tag size %zu", ebml_data_size);
	}
	
	krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_LINK_LINK_OPERATION_MODE) {
		printk ("hrm wtf");
	} else {
		//printk ("tag size %zu", ebml_data_size);
	}
	
	krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
	krad_link->operation_mode = krad_link_string_to_operation_mode (string);

	if (krad_link->operation_mode == RECEIVE) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
			printk ("hrm wtf3");
		} else {
			//printk ("tag value size %zu", ebml_data_size);
		}

		krad_link->port = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
	
		//TEMP KLUDGE
		krad_link->av_mode = AUDIO_ONLY;

	}
	
	if (krad_link->operation_mode == PLAYBACK) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			printk ("hrm wtf2");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		if (krad_link->transport_mode == FILESYSTEM) {
	
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_FILENAME) {
				printk ("hrm wtf3");
			} else {
				//printk ("tag value size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->input, ebml_data_size);
		}
		
		if (krad_link->transport_mode == TCP) {
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
				printk ("hrm wtf2");
			} else {
				//printk ("tag name size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->host, ebml_data_size);
	
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
				printk ("hrm wtf3");
			} else {
				//printk ("tag value size %zu", ebml_data_size);
			}

			krad_link->port = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
				printk ("hrm wtf2");
			} else {
				//printk ("tag name size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->mount, ebml_data_size);
		}
	}
	
	if (krad_link->operation_mode == CAPTURE) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
		if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE) {
			printk ("hrm wtf");
		} else {
			//printk ("tag size %zu", ebml_data_size);
		}
	
		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
		krad_link->video_source = krad_link_string_to_video_source (string);
		
		if (krad_link->video_source == DECKLINK) {
			krad_link->av_mode = AUDIO_AND_VIDEO;
		}
		
		if ((krad_link->video_source == V4L2) || (krad_link->video_source == X11)) {
			krad_link->av_mode = VIDEO_ONLY;
		}

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
		if (ebml_id != EBML_ID_KRAD_LINK_LINK_CAPTURE_DEVICE) {
			printk ("hrm wtf");
		} else {
			//printk ("tag size %zu", ebml_data_size);
		}
	
		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->device, ebml_data_size);
	
		if (krad_link->video_source == V4L2) {
			if (strlen(krad_link->device) == 0) {
				strncpy(krad_link->device, DEFAULT_V4L2_DEVICE, sizeof(krad_link->device));
			}
			
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
			if (ebml_id != EBML_ID_KRAD_LINK_LINK_CAPTURE_CODEC) {
				printk ("hrm wtf");
			} else {
				//printk ("tag size %zu", ebml_data_size);
			}
	
			string[0] = '\0';
			
			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);

			krad_link->video_passthru = 0;

			if (strlen(string)) {
				krad_link->video_codec = krad_string_to_codec (string);
				if (krad_link->video_codec == H264) {
					krad_link->video_passthru = 1;
				}
		    if (strstr(string, "pass") != NULL) {
					krad_link->video_passthru = 1;
				}
			}
		}

		if (krad_link->video_source == DECKLINK) {
			if (strlen(krad_link->device) == 0) {
				strncpy(krad_link->device, DEFAULT_DECKLINK_DEVICE, sizeof(krad_link->device));
			}
			
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
			if (ebml_id != EBML_ID_KRAD_LINK_LINK_CAPTURE_DECKLINK_AUDIO_INPUT) {
				printk ("hrm wtf");
			} else {
				//printk ("tag size %zu", ebml_data_size);
			}
	
			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->audio_input, ebml_data_size);	
		}		

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH) {
			printk ("hrm wtf2v");
		} else {
			krad_link->capture_width = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
		}
		
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT) {
			printk ("hrm wtf2v");
		} else {
			krad_link->capture_height = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
		}

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR) {
			printk ("hrm wtf2v");
		} else {
			krad_link->fps_numerator = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
		}
		
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR) {
			printk ("hrm wtf2v");
		} else {
			krad_link->fps_denominator = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
		}
	}
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
	
		if (ebml_id != EBML_ID_KRAD_LINK_LINK_AV_MODE) {
			printk ("hrm wtf0");
		} else {
			//printk ("tag size %zu", ebml_data_size);
		}
	
	
		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
	
		krad_link->av_mode = krad_link_string_to_av_mode (string);

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC) {
				printk ("hrm wtf2v1");
			} else {
				//printk ("tag name size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
			
			krad_link->video_codec = krad_string_to_codec (string);
			
			if ((krad_link->video_codec == H264) || (krad_link->video_codec == MJPEG)) {

				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

				if (ebml_id != EBML_ID_KRAD_LINK_LINK_USE_PASSTHRU_CODEC) {
					printk ("hrm wtf2v2");
				} else {
					//printk ("tag name size %zu", ebml_data_size);
				}

				krad_link->video_passthru = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);

				if (krad_link->video_codec == MJPEG) {
					//FIXME should be optional
					krad_link->video_passthru = 1;
				}

			}			
			
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH) {
				printk ("hrm wtf2v3");
			} else {
				krad_link->encoding_width = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT) {
				printk ("hrm wtf2v4");
			} else {
				krad_link->encoding_height = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
			}
			
			
			if ((krad_link->video_codec == VP8) || (krad_link->video_codec == H264)) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

				if (ebml_id != EBML_ID_KRAD_LINK_LINK_VP8_BITRATE) {
					printk ("hrm wtf2v5");
				} else {
					krad_link->vp8_bitrate = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}
			
			if (krad_link->video_codec == THEORA) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

				if (ebml_id != EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY) {
					printk ("hrm wtf2v6");
				} else {
					krad_link->theora_quality = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}			
			
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC) {
				printk ("hrm wtf2a7");
			} else {
				//printk ("tag name size %zu", ebml_data_size);
			}

			krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
			
			krad_link->audio_codec = krad_string_to_codec (string);
			
			if (krad_link->audio_codec == VORBIS) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
				if (ebml_id == EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY) {
					krad_link->vorbis_quality = krad_ebml_read_float (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}

			if (krad_link->audio_codec == OPUS) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
				if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE) {
					krad_link->opus_bitrate = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}
			
			if (krad_link->audio_codec == FLAC) {
				krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
				if (ebml_id == EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH) {
					krad_link->flac_bit_depth = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);
				}
			}			
			
			
		}
	}
	
	
	if (krad_link->operation_mode == TRANSMIT) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			printk ("hrm wtf28");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
			printk ("hrm wtf29");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->host, ebml_data_size);
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
			printk ("hrm wtf310");
		} else {
			//printk ("tag value size %zu", ebml_data_size);
		}

		krad_link->port = krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, ebml_data_size);

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
			printk ("hrm wtf212");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->mount, ebml_data_size);

		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PASSWORD) {
			printk ("hrm wtf213");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->password, ebml_data_size);

		if (strstr(krad_link->mount, "flac") != NULL) {
			krad_link->audio_codec = FLAC;
		}
			
		if (strstr(krad_link->mount, ".opus") != NULL) {
			krad_link->audio_codec = OPUS;
		}

	}
	
	if (krad_link->operation_mode == RECORD) {
	
		krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_FILENAME) {
			printk ("hrm wtf214");
		} else {
			//printk ("tag name size %zu", ebml_data_size);
		}

		krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, krad_link->output, ebml_data_size);

		if (strstr(krad_link->output, "flac") != NULL) {
			krad_link->audio_codec = FLAC;
		}
			
		if (strstr(krad_link->output, ".opus") != NULL) {
			krad_link->audio_codec = OPUS;
		}

		krad_link->transport_mode = FILESYSTEM;

	}
}



void krad_transponder_link_to_ebml ( krad_ipc_server_client_t *client, krad_link_t *krad_link) {

	uint64_t link;

	krad_ebml_start_element (client->krad_ebml2, EBML_ID_KRAD_TRANSPONDER_LINK, &link);	

	krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_TRANSPONDER_LINK_NUMBER, krad_link->link_num);

	switch ( krad_link->av_mode ) {

		case AUDIO_ONLY:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "audio only");
			break;
		case VIDEO_ONLY:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "video only");
			break;
		case AUDIO_AND_VIDEO:		
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AV_MODE, "audio and video");
			break;
	}
	
	
	switch ( krad_link->operation_mode ) {

		case TRANSMIT:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "transmit");
			break;
		case RECORD:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "record");
			break;
		case PLAYBACK:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "playback");
			break;
		case RECEIVE:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "receive");
			break;
		case CAPTURE:
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "capture");
			break;
		default:		
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, "Other/Unknown");
			break;
	}
	
	if (krad_link->operation_mode == RECEIVE) {

		krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
								krad_link_transport_mode_to_string (krad_link->transport_mode));

		if ((krad_link->transport_mode == UDP) || (krad_link->transport_mode == TCP)) {
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
		}
	}	
	
	if (krad_link->operation_mode == CAPTURE) {
		krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE, krad_link_video_source_to_string (krad_link->video_source));
	}
	
	if (krad_link->operation_mode == PLAYBACK) {
	
		krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
								krad_link_transport_mode_to_string (krad_link->transport_mode));
	
		if (krad_link->transport_mode == FILESYSTEM) {
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FILENAME, krad_link->input);
		}

		if (krad_link->transport_mode == TCP) {
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_HOST, krad_link->host);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_MOUNT, krad_link->mount);
		}
	}
	
	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
		switch ( krad_link->av_mode ) {
			case AUDIO_ONLY:
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (krad_link->audio_codec));
				break;
			case VIDEO_ONLY:
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (krad_link->video_codec));
				break;
			case AUDIO_AND_VIDEO:		
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (krad_link->video_codec));				
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (krad_link->audio_codec));
				break;
		}

		if (krad_link->operation_mode == TRANSMIT) {
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, 
									krad_link_transport_mode_to_string (krad_link->transport_mode));
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_HOST, krad_link->host);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_PORT, krad_link->port);
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_MOUNT, krad_link->mount);
		}
		
		if (krad_link->operation_mode == RECORD) {
			krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FILENAME, krad_link->output);
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_CHANNELS, 2);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_AUDIO_SAMPLE_RATE, 48000);
		}

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH, krad_link->encoding_width);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT, krad_link->encoding_height);						
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR, krad_link->encoding_fps_numerator);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR, krad_link->encoding_fps_denominator);
			krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VIDEO_COLOR_DEPTH, 420);							
		}		

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			if (krad_link->audio_codec == VORBIS) {
				krad_ebml_write_float (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY, krad_link->krad_vorbis->quality);
			}

			if (krad_link->audio_codec == FLAC) {
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, krad_link->krad_flac->bit_depth);
			}

			if (krad_link->audio_codec == OPUS) {
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL, 
										krad_opus_signal_to_string (krad_opus_get_signal (krad_link->krad_opus)));
				krad_ebml_write_string (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH, 
										krad_opus_bandwidth_to_string (krad_opus_get_bandwidth (krad_link->krad_opus)));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, krad_opus_get_bitrate (krad_link->krad_opus));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY, krad_opus_get_complexity (krad_link->krad_opus));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE, krad_opus_get_frame_size (krad_link->krad_opus));

				//EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE, atoi(argv[5]));
			}
		
		}

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			if (krad_link->video_codec == THEORA) {
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY, krad_theora_encoder_quality_get (krad_link->krad_theora_encoder));
			}

			if (krad_link->video_codec == VP8) {
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, krad_vpx_encoder_bitrate_get (krad_link->krad_vpx_encoder));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER, krad_vpx_encoder_min_quantizer_get (krad_link->krad_vpx_encoder));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER, krad_vpx_encoder_max_quantizer_get (krad_link->krad_vpx_encoder));
				krad_ebml_write_int32 (client->krad_ebml2, EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE, krad_vpx_encoder_deadline_get (krad_link->krad_vpx_encoder));
			}
		}
	}
	
	krad_ebml_finish_element (client->krad_ebml2, link);

}

int krad_transponder_handler ( krad_transponder_t *krad_transponder, krad_ipc_server_t *krad_ipc ) {

	krad_link_t *krad_link;

	uint32_t ebml_id;

	uint32_t command;
	uint64_t ebml_data_size;

	uint64_t element;
	uint64_t response;
	
	char string[256];	
	
	uint64_t bigint;
	//int regint;
	uint8_t tinyint;
	int k;
	int devices;
	//float floatval;
	
	string[0] = '\0';
	bigint = 0;
	k = 0;
	
	pthread_mutex_lock (&krad_transponder->change_lock);	

	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {

		case EBML_ID_KRAD_TRANSPONDER_CMD_LIST_LINKS:
			//printk ("krad transponder handler! LIST_LINKS");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_TRANSPONDER_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_TRANSPONDER_LINK_LIST, &element);	
			
			for (k = 0; k < KRAD_TRANSPONDER_MAX_LINKS; k++) {
				if (krad_transponder->krad_link[k] != NULL) {
					printk ("Link %d Active: %s", k, krad_transponder->krad_link[k]->mount);
					krad_transponder_link_to_ebml ( krad_ipc->current_client, krad_transponder->krad_link[k]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
			break;
			
		case EBML_ID_KRAD_TRANSPONDER_CMD_CREATE_LINK:
			//printk ("krad transponder handler! CREATE_LINK");
			for (k = 0; k < KRAD_TRANSPONDER_MAX_LINKS; k++) {
				if (krad_transponder->krad_link[k] == NULL) {

					krad_transponder->krad_link[k] = krad_link_create (k);
					krad_link = krad_transponder->krad_link[k];
					krad_link->link_num = k;
					krad_link->krad_radio = krad_transponder->krad_radio;
					krad_link->krad_transponder = krad_transponder;

					krad_transponder_ebml_to_link ( krad_ipc, krad_link );
					
					krad_link_run (krad_link);
				
					if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
						if (krad_link_wait_codec_init (krad_link) == 0) {
							krad_transponder_broadcast_link_created ( krad_ipc, krad_link );
						}
					} else {
						krad_transponder_broadcast_link_created ( krad_ipc, krad_link );
					}

					break;
				}
			}
			break;
		case EBML_ID_KRAD_TRANSPONDER_CMD_DESTROY_LINK:
			tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
			k = tinyint;
			//printk ("krad transponder handler! DESTROY_LINK: %d %u", k, tinyint);
			
			if (krad_transponder->krad_link[k] != NULL) {
				krad_link_destroy (krad_transponder->krad_link[k]);
				krad_transponder->krad_link[k] = NULL;
			}
			
			krad_ipc_server_simple_number_broadcast ( krad_ipc,
													  EBML_ID_KRAD_TRANSPONDER_MSG,
													  EBML_ID_KRAD_TRANSPONDER_LINK_DESTROYED,
													  EBML_ID_KRAD_TRANSPONDER_LINK_NUMBER,
											 		  k);			
			
			break;
		case EBML_ID_KRAD_TRANSPONDER_CMD_UPDATE_LINK:
			//printk ("krad transponder handler! UPDATE_LINK");

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	
			
			if (ebml_id == EBML_ID_KRAD_TRANSPONDER_LINK_NUMBER) {
			
				tinyint = krad_ipc_server_read_number (krad_ipc, ebml_data_size);
				k = tinyint;
				//printk ("krad transponder handler! UPDATE_LINK: %d %u", k, tinyint);
			
				if (krad_transponder->krad_link[k] != NULL) {

					krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

					if (krad_transponder->krad_link[k]->audio_codec == OPUS) {

						/*
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_APPLICATION) {
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
							if (strncmp(string, "OPUS_APPLICATION_VOIP", 21) == 0) {
								krad_opus_set_application (krad_transponder->krad_link[k]->krad_opus, OPUS_APPLICATION_VOIP);
							}
							if (strncmp(string, "OPUS_APPLICATION_AUDIO", 22) == 0) {
								krad_opus_set_application (krad_transponder->krad_link[k]->krad_opus, OPUS_APPLICATION_AUDIO);
							}
							if (strncmp(string, "OPUS_APPLICATION_RESTRICTED_LOWDELAY", 36) == 0) {
								krad_opus_set_application (krad_transponder->krad_link[k]->krad_opus, OPUS_APPLICATION_RESTRICTED_LOWDELAY);
							}
						}
						*/
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL) {
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

							krad_opus_set_signal (krad_transponder->krad_link[k]->krad_opus, 
													krad_opus_string_to_signal(string));
						}
						
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH) {
							
							krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
							
							krad_opus_set_bandwidth (krad_transponder->krad_link[k]->krad_opus, 
													krad_opus_string_to_bandwidth(string));
						}						

						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if ((bigint >= 500) && (bigint <= 512000)) {
								krad_opus_set_bitrate (krad_transponder->krad_link[k]->krad_opus, bigint);
							}
						}
						
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if ((bigint >= 0) && (bigint <= 10)) {
								krad_opus_set_complexity (krad_transponder->krad_link[k]->krad_opus, bigint);
							}
						}						
				
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
							if ((bigint == 120) || (bigint == 240) || (bigint == 480) || (bigint == 960) || (bigint == 1920) || (bigint == 2880)) {
								krad_opus_set_frame_size (krad_transponder->krad_link[k]->krad_opus, bigint);
							}
						}
						
						//FIXME verify ogg container
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
							if ((bigint > 0) && (bigint < 200)) {					
								krad_ogg_set_max_packets_per_page (krad_transponder->krad_link[k]->krad_container->krad_ogg, bigint);
							}
						}
					}
					
					if (krad_transponder->krad_link[k]->video_codec == THEORA) {
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
							krad_theora_encoder_quality_set (krad_transponder->krad_link[k]->krad_theora_encoder, bigint);
						}
					}				
					
					if (krad_transponder->krad_link[k]->video_codec == VP8) {
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_BITRATE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_bitrate_set (krad_transponder->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_min_quantizer_set (krad_transponder->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_max_quantizer_set (krad_transponder->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_deadline_set (krad_transponder->krad_link[k]->krad_vpx_encoder, bigint);
							}
						}												
						if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_FORCE_KEYFRAME) {
							bigint = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
					
							if (bigint > 0) {
								krad_vpx_encoder_want_keyframe (krad_transponder->krad_link[k]->krad_vpx_encoder);
							}
						}
					}

					if ((ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH) || (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL)) {

						krad_ipc_server_advanced_string_broadcast ( krad_ipc,
																  EBML_ID_KRAD_TRANSPONDER_MSG,
																  EBML_ID_KRAD_TRANSPONDER_LINK_UPDATED,
																  EBML_ID_KRAD_TRANSPONDER_LINK_NUMBER,
														 		  k,
														 		  ebml_id,
														 		  string);

					} else {
						krad_ipc_server_advanced_number_broadcast ( krad_ipc,
																  EBML_ID_KRAD_TRANSPONDER_MSG,
																  EBML_ID_KRAD_TRANSPONDER_LINK_UPDATED,
																  EBML_ID_KRAD_TRANSPONDER_LINK_NUMBER,
														 		  k,
														 		  ebml_id,
														 		  bigint);
					}
				}
			}
			
			break;
			
		case EBML_ID_KRAD_TRANSPONDER_CMD_LIST_DECKLINK:
			printk ("krad transponder handler! LIST_DECKLINK");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_TRANSPONDER_MSG, &response);
			
			devices = krad_decklink_detect_devices();

			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_TRANSPONDER_DECKLINK_LIST, &element);
			krad_ebml_write_int32 (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_LIST_COUNT, devices);
			
			for (k = 0; k < devices; k++) {

				krad_decklink_get_device_name (k, string);
				krad_ebml_write_string (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_TRANSPONDER_DECKLINK_DEVICE_NAME, string);
					
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
			break;
	
		case EBML_ID_KRAD_TRANSPONDER_CMD_LIST_V4L2:
			printk ("krad transponder handler! LIST_V4L2");
			
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_TRANSPONDER_MSG, &response);
			
			devices = krad_v4l2_detect_devices ();

			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_TRANSPONDER_V4L2_LIST, &element);
			krad_ebml_write_int32 (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_LIST_COUNT, devices);
			
			for (k = 0; k < devices; k++) {

				if (krad_v4l2_get_device_filename (k, string) > 0) {
					krad_ebml_write_string (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_TRANSPONDER_V4L2_DEVICE_FILENAME, string);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
						
			break;
	
		case EBML_ID_KRAD_TRANSPONDER_CMD_LISTEN_ENABLE:
		
			krad_ebml_read_element ( krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TCP_PORT) {
				printke ("hrm wtf6");
			}
		
			bigint = krad_ebml_read_number ( krad_ipc->current_client->krad_ebml, ebml_data_size);
		
			krad_transponder_listen (krad_transponder, bigint);
		
			break;

		case EBML_ID_KRAD_TRANSPONDER_CMD_LISTEN_DISABLE:
		
			krad_transponder_stop_listening (krad_transponder);
		
			break;
			
			
		case EBML_ID_KRAD_TRANSPONDER_CMD_TRANSMITTER_ENABLE:
		
			krad_ebml_read_element ( krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TCP_PORT) {
				printke ("hrm wtf6");
			}
		
			bigint = krad_ebml_read_number ( krad_ipc->current_client->krad_ebml, ebml_data_size);
		
			krad_transmitter_listen_on (krad_transponder->krad_transmitter, bigint);
		
			break;

		case EBML_ID_KRAD_TRANSPONDER_CMD_TRANSMITTER_DISABLE:
		
			krad_transmitter_stop_listening (krad_transponder->krad_transmitter);
		
			break;			

	}

	pthread_mutex_unlock (&krad_transponder->change_lock);

	return 0;
}
