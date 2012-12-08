void krad_ipc_enable_linker_listen (krad_ipc_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t enable_linker;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINKER_CMD_LISTEN_ENABLE, &enable_linker);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_linker);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_disable_linker_listen (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t disable_linker;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINKER_CMD_LISTEN_DISABLE, &disable_linker);

	krad_ebml_finish_element (client->krad_ebml, disable_linker);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_enable_linker_transmitter (krad_ipc_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t enable_transmitter;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINKER_CMD_TRANSMITTER_ENABLE, &enable_transmitter);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_transmitter);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_disable_linker_transmitter (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t disable_transmitter;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINKER_CMD_TRANSMITTER_DISABLE, &disable_transmitter);

	krad_ebml_finish_element (client->krad_ebml, disable_transmitter);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}




void krad_ipc_create_capture_link (krad_ipc_client_t *client, krad_link_video_source_t video_source, char *device,
								   int width, int height, int fps_numerator, int fps_denominator,
								   krad_link_av_mode_t av_mode, char *audio_input, char *codec) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (CAPTURE));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE, krad_link_video_source_to_string (video_source));

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_CAPTURE_DEVICE, device);
	
	if (video_source == DECKLINK) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_CAPTURE_DECKLINK_AUDIO_INPUT, audio_input);
	}
	
	if (video_source == V4L2) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_CAPTURE_CODEC, codec);
	}	
	
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH, width);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT, height);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR, fps_numerator);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR, fps_denominator);

	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_receive_link (krad_ipc_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (RECEIVE));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "udp");
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PORT, port);
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_remote_playback_link (krad_ipc_client_t *client, char *host, int port, char *mount) {



	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (PLAYBACK));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "tcp");
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_HOST, host);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PORT, port);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_MOUNT, mount);
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);


}

void krad_ipc_create_playback_link (krad_ipc_client_t *client, char *path) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (PLAYBACK));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "filesystem");
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FILENAME, path);
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_transmit_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode,
									char *host, int port, char *mount, char *password, char *codecs,
									int video_width, int video_height, int video_bitrate, char *audio_bitrate) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	int passthru;	
	
	krad_codec_t audio_codec;
	krad_codec_t video_codec;
	
	int audio_bitrate_int;
	float vorbis_quality;
	
	audio_codec = VORBIS;
	video_codec = VP8;
	passthru = 0;

	if (codecs != NULL) {
		audio_codec = krad_string_to_audio_codec (codecs);
		video_codec = krad_string_to_video_codec (codecs);
		if (strstr(codecs, "pass") != NULL) {
			passthru = 1;
		}
	}
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (TRANSMIT));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AV_MODE, krad_link_av_mode_to_string (av_mode));

	if ((av_mode == VIDEO_ONLY) || (av_mode == AUDIO_AND_VIDEO)) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (video_codec));
		
		if (video_codec == VP8) {
			if (video_bitrate == 0) {
				video_bitrate = 92 * 8;
			}
		}
		
		if (video_codec == THEORA) {
			if ((video_width % 16) || (video_height % 16)) {
				video_width = 0;
				video_height = 0;
			}
			if (video_bitrate == 0) {
				video_bitrate = 31;
			}
		}
		
		if ((video_codec == MJPEG) || (video_codec == H264)) {
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_USE_PASSTHRU_CODEC, passthru);
		}
		
		krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH, video_width);
		krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT, video_height);
		
		if ((video_codec == VP8) || (video_codec == H264)) {
			if (video_bitrate < 100) {
				video_bitrate = 100;
			}
			
			if (video_bitrate > 10000) {
				video_bitrate = 10000;
			}
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, video_bitrate);	
		}
		
		if (video_codec == THEORA) {
		
			if (video_bitrate < 0) {
				video_bitrate = 0;
			}
			
			if (video_bitrate > 63) {
				video_bitrate = 63;
			}
		
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY, video_bitrate);	
		}	
		
	}

	if ((av_mode == AUDIO_ONLY) || (av_mode == AUDIO_AND_VIDEO)) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (audio_codec));
		if (audio_codec == FLAC) {
			if (atoi(audio_bitrate) == 24) {
				krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, 24);
			} else {
				krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, 16);
			}
		}
		if (audio_codec == VORBIS) {
			vorbis_quality = atof(audio_bitrate);
			if (vorbis_quality == 0.0) {
				vorbis_quality = 0.4;
			}
			if (vorbis_quality > 0.8) {
				vorbis_quality = 0.8;
			} 
			if (vorbis_quality < 0.2) {
				vorbis_quality = 0.2;
			}
		
			krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY, vorbis_quality);
		}
		if (audio_codec == OPUS) {
			audio_bitrate_int = atoi(audio_bitrate);
			if (audio_bitrate_int == 0) {
				audio_bitrate_int = 132000;
			} 
			if (audio_bitrate_int < 5000) {
				audio_bitrate_int = 5000;
			}
			if (audio_bitrate_int > 320000) {
				audio_bitrate_int = 320000;
			}
			
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, audio_bitrate_int);
		}
	}
	
	if (strcmp(password, "udp") == 0) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "udp");
	} else {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "tcp");
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_HOST, host);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PORT, port);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_MOUNT, mount);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PASSWORD, password);	
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_record_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode, char *filename, char *codecs,
									int video_width, int video_height, int video_bitrate, char *audio_bitrate) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	int passthru;
	
	krad_codec_t audio_codec;
	krad_codec_t video_codec;
	
	int audio_bitrate_int;
	float vorbis_quality;
	
	audio_codec = VORBIS;
	video_codec = VP8;
	passthru = 0;
		
	if (codecs != NULL) {
		audio_codec = krad_string_to_audio_codec (codecs);
		video_codec = krad_string_to_video_codec (codecs);
		if (strstr(codecs, "pass") != NULL) {
			passthru = 1;
		}
	}
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (RECORD));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AV_MODE, krad_link_av_mode_to_string (av_mode));

	if ((av_mode == VIDEO_ONLY) || (av_mode == AUDIO_AND_VIDEO)) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (video_codec));
		
		if (video_codec == VP8) {
			if (video_bitrate == 0) {
				video_bitrate = 140 * 8;
			}
		}
		
		if (video_codec == THEORA) {
			if (video_bitrate == 0) {
				video_bitrate = 41;
			}
		}
		
		if ((video_codec == MJPEG) || (video_codec == H264)) {
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_USE_PASSTHRU_CODEC, passthru);
		}

		krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH, video_width);
		krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT, video_height);
		
		if ((video_codec == VP8) || (video_codec == H264)) {
			if (video_bitrate < 100) {
				video_bitrate = 100;
			}
			
			if (video_bitrate > 10000) {
				video_bitrate = 10000;
			}
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, video_bitrate);	
		}
		
		if (video_codec == THEORA) {
		
			if (video_bitrate < 0) {
				video_bitrate = 0;
			}
			
			if (video_bitrate > 63) {
				video_bitrate = 63;
			}
		
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY, video_bitrate);	
		}	
		
	}

	if ((av_mode == AUDIO_ONLY) || (av_mode == AUDIO_AND_VIDEO)) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (audio_codec));
		if (audio_codec == FLAC) {
			if (atoi(audio_bitrate) == 24) {
				krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, 24);
			} else {
				krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, 16);
			}
		}
		if (audio_codec == VORBIS) {
			vorbis_quality = atof(audio_bitrate);
			if (vorbis_quality == 0.0) {
				vorbis_quality = 0.4;
			}
			if (vorbis_quality > 0.8) {
				vorbis_quality = 0.8;
			} 
			if (vorbis_quality < 0.2) {
				vorbis_quality = 0.2;
			}
		
			krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY, vorbis_quality);
		}
		if (audio_codec == OPUS) {
			audio_bitrate_int = atoi(audio_bitrate);
			if (audio_bitrate_int == 0) {
				audio_bitrate_int = 132000;
			} 
			if (audio_bitrate_int < 5000) {
				audio_bitrate_int = 5000;
			}
			if (audio_bitrate_int > 320000) {
				audio_bitrate_int = 320000;
			}
			
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, audio_bitrate_int);
		}
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FILENAME, filename);
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_list_v4l2 (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t list_v4l2;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_LIST_V4L2, &list_v4l2);

	krad_ebml_finish_element (client->krad_ebml, list_v4l2);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}



void krad_ipc_list_decklink (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t list_decklink;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_LIST_DECKLINK, &list_decklink);


	krad_ebml_finish_element (client->krad_ebml, list_decklink);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_list_links (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t list_links;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_LIST_LINKS, &list_links);



	krad_ebml_finish_element (client->krad_ebml, list_links);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_destroy_link (krad_ipc_client_t *client, int number) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t destroy_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_DESTROY_LINK, &destroy_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_finish_element (client->krad_ebml, destroy_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_update_link_adv_num (krad_ipc_client_t *client, int number, uint32_t ebml_id, int newval) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t update_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_UPDATE_LINK, &update_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_write_int32 (client->krad_ebml, ebml_id, newval);

	krad_ebml_finish_element (client->krad_ebml, update_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_update_link_adv (krad_ipc_client_t *client, int number, uint32_t ebml_id, char *newval) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t update_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_UPDATE_LINK, &update_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_write_string (client->krad_ebml, ebml_id, newval);

	krad_ebml_finish_element (client->krad_ebml, update_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}



int krad_link_rep_to_string (krad_link_rep_t *krad_link, char *text) {

	int pos;
	
	pos = 0;
	
	pos += sprintf (text + pos, "%s %s", 
					krad_link_operation_mode_to_string (krad_link->operation_mode),
					krad_link_av_mode_to_string (krad_link->av_mode));

	if ((krad_link->operation_mode == RECORD) || (krad_link->operation_mode == TRANSMIT)) {

		if ((krad_link->operation_mode == TRANSMIT) && (krad_link->transport_mode == UDP)) {
			pos += sprintf (text + pos, " %s", krad_link_transport_mode_to_string (krad_link->transport_mode));
		}

		if (krad_link->operation_mode == TRANSMIT) {
			pos += sprintf (text + pos, " to %s:%d%s", krad_link->host, krad_link->port, krad_link->mount);
		}

		if (krad_link->operation_mode == RECORD) {
			pos += sprintf (text + pos, " to %s", krad_link->filename);
		}
				
		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

			pos += sprintf (text + pos, " Video -");

			pos += sprintf (text + pos, " %dx%d %d/%d %d",
							krad_link->width, krad_link->height,
							krad_link->fps_numerator, krad_link->fps_denominator,
							krad_link->color_depth);

			pos += sprintf (text + pos, " Codec: %s", krad_codec_to_string (krad_link->video_codec));

			if (krad_link->video_codec == THEORA) {
				pos += sprintf (text + pos, " Quality: %d", krad_link->theora_quality); 
			}	
	
			if (krad_link->video_codec == VP8) {
				pos += sprintf (text + pos, " Bitrate: %d Min Quantizer: %d Max Quantizer: %d Deadline: %d",
								krad_link->vp8_bitrate, krad_link->vp8_min_quantizer,
								krad_link->vp8_max_quantizer, krad_link->vp8_deadline);
			}
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pos += sprintf (text + pos, " Audio -");
			pos += sprintf (text + pos, " Sample Rate: %d", krad_link->audio_sample_rate);
			pos += sprintf (text + pos, " Channels: %d", krad_link->audio_channels);
			pos += sprintf (text + pos, " Codec: %s", krad_codec_to_string (krad_link->audio_codec));
						
			if (krad_link->audio_codec == FLAC) {
				pos += sprintf (text + pos, " Bit Depth: %d", krad_link->flac_bit_depth);
			}		

			if (krad_link->audio_codec == VORBIS) {
				pos += sprintf (text + pos, " Quality: %.1f", krad_link->vorbis_quality); 
			}		

			if (krad_link->audio_codec == OPUS) {
				pos += sprintf (text + pos, " Complexity: %d Bitrate: %d Frame Size: %d Signal: %s Bandwidth: %s", krad_link->opus_complexity,
								krad_link->opus_bitrate, krad_link->opus_frame_size, krad_opus_signal_to_string (krad_link->opus_signal),
								krad_opus_bandwidth_to_string (krad_link->opus_bandwidth));

			}
		}
	}

	if (krad_link->operation_mode == RECEIVE) {
		if ((krad_link->transport_mode == UDP) || (krad_link->transport_mode == TCP)) {
			pos += sprintf (text + pos, " Port %d", krad_link->port);
		}
	}
	
	if (krad_link->operation_mode == PLAYBACK) {
		if (krad_link->transport_mode == FILESYSTEM) {
			pos += sprintf (text + pos, " File %s", krad_link->filename);
		}

		if (krad_link->transport_mode == TCP) {
			pos += sprintf (text + pos, " %s:%d%s",
							krad_link->host, krad_link->port, krad_link->mount);
		}
	}
	
	if (krad_link->operation_mode == CAPTURE) {
		pos += sprintf (text + pos, " from %s", krad_link_video_source_to_string (krad_link->video_source));
		pos += sprintf (text + pos, " with device %s", krad_link->video_device);		

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

			pos += sprintf (text + pos, " at ");

			//pos += sprintf (text + pos, " %dx%d %d/%d %d",
			//				krad_link->width, krad_link->height,
			//				krad_link->fps_numerator, krad_link->fps_denominator,
			//				krad_link->color_depth);

			pos += sprintf (text + pos, " %dx%d %d/%d",
							krad_link->width, krad_link->height,
							krad_link->fps_numerator, krad_link->fps_denominator);

		}

	}

	return pos;
}



int krad_ipc_client_read_link ( krad_ipc_client_t *client, char *text, krad_link_rep_t **krad_link_rep) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	krad_link_rep_t *krad_link;
	
	char string[1024];
	memset (string, '\0', 1024);
	
	krad_link = calloc (1, sizeof (krad_link_rep_t));
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_LINK_LINK) {
		//printk ("hrm wtf1\n");
	} else {
		bytes_read += ebml_data_size + 9;
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	krad_link->link_num = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_LINK_LINK_AV_MODE) {
		//printk ("hrm wtf2\n");
	} else {
		//printk ("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);

	if (strcmp(string, "audio only") == 0) {
		krad_link->av_mode = AUDIO_ONLY;
	}
	
	if (strcmp(string, "video only") == 0) {
		krad_link->av_mode = VIDEO_ONLY;
	}
	
	if (strcmp(string, "audio and video") == 0) {
		krad_link->av_mode = AUDIO_AND_VIDEO;
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_LINK_LINK_OPERATION_MODE) {
		//printk ("hrm wtf3\n");
	} else {
		//printk ("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);

	krad_link->operation_mode = krad_link_string_to_operation_mode (string);
	
	if (krad_link->operation_mode == RECEIVE) {
	
		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			//printk ("hrm wtf4\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		if ((krad_link->transport_mode == UDP) || (krad_link->transport_mode == TCP)) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
				//printk ("hrm wtf5\n");
			} else {
				//printk ("tag value size %zu\n", ebml_data_size);
			}

			krad_link->port = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
		}
	
	}
	
	if (krad_link->operation_mode == PLAYBACK) {
	
		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			//printk ("hrm wtf4\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		if (krad_link->transport_mode == FILESYSTEM) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_FILENAME) {
				//printk ("hrm wtf5\n");
			} else {
				//printk ("tag value size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->filename, ebml_data_size);
		}
	
		if (krad_link->transport_mode == TCP) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
				//printk ("hrm wtf4\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->host, ebml_data_size);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
				//printk ("hrm wtf5\n");
			} else {
				//printk ("tag value size %zu\n", ebml_data_size);
			}

			krad_link->port = krad_ebml_read_number (client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
				//printk ("hrm wtf6\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->mount, ebml_data_size);
		
		}	
	
	
	}
	
	if (krad_link->operation_mode == CAPTURE) {
	
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE) {
				//printk ("hrm wtf2v\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
			krad_link->video_source = krad_link_string_to_video_source (string);
			
	}

	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
	
	
		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
	
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC) {
				//printk ("hrm wtf2v\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
			krad_link->video_codec = krad_string_to_codec (string);
		
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
	
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC) {
				//printk ("hrm wtf2a\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
			krad_link->audio_codec = krad_string_to_codec (string);
		}
	
		if (krad_link->operation_mode == TRANSMIT) {

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
				//printk ("hrm wtf4\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
			
			krad_link->transport_mode = krad_link_string_to_transport_mode (string);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
				//printk ("hrm wtf4\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->host, ebml_data_size);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
				//printk ("hrm wtf5\n");
			} else {
				//printk ("tag value size %zu\n", ebml_data_size);
			}

			krad_link->port = krad_ebml_read_number (client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
				//printk ("hrm wtf6\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->mount, ebml_data_size);
		}
		
		if (krad_link->operation_mode == RECORD) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_FILENAME) {
				//printk ("hrm wtf4\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->filename, ebml_data_size);			
		
		}


		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_AUDIO_CHANNELS) {
				krad_link->audio_channels = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_AUDIO_SAMPLE_RATE) {
				krad_link->audio_sample_rate = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}					

		}

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH) {
				krad_link->width = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT) {
				krad_link->height = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR) {
				krad_link->fps_numerator = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR) {
				krad_link->fps_denominator = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}									

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VIDEO_COLOR_DEPTH) {
				krad_link->color_depth = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}			
		}		

		if (((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->audio_codec == OPUS)) {

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL) {
				krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
				krad_link->opus_signal = krad_opus_string_to_signal (string);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH) {
				krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
				krad_link->opus_bandwidth = krad_opus_string_to_bandwidth (string);					
			}

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE) {
				krad_link->opus_bitrate = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY) {
				krad_link->opus_complexity = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE) {
				krad_link->opus_frame_size = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}	

			//EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE, atoi(argv[5]));
		}

		if (((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->audio_codec == VORBIS)) {
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY) {
				krad_link->vorbis_quality = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
			}
		}

		if (((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->audio_codec == FLAC)) {
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH) {
				krad_link->flac_bit_depth = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}			
		}

		if (((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->video_codec == THEORA)) {
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY) {
				krad_link->theora_quality = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
		}

		if (((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->video_codec == VP8)) {

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_BITRATE) {
				krad_link->vp8_bitrate = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER) {
				krad_link->vp8_min_quantizer = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
						
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER) {
				krad_link->vp8_max_quantizer = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE) {
				krad_link->vp8_deadline = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
		}
	}
	
	if (text != NULL) {
		krad_link_rep_to_string ( krad_link, text );
	}
	
	if (krad_link_rep != NULL) {
		*krad_link_rep = krad_link;
	} else {
		free (krad_link);
	}
	
	return bytes_read;

}
