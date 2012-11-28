#include "krad_websocket.h"

struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		0			/* per_session_data_size */
	},
	{
		"krad-ipc",
		callback_kr_client,
		sizeof(kr_client_session_data_t),
	},
	{
		NULL, NULL, 0		/* End of list */
	}
};

/* blame libwebsockets */
krad_websocket_t *krad_websocket_glob;


/* interpret JSON to speak Krad IPC */

void krad_ipc_from_json (kr_client_session_data_t *pss, char *value, int len) {
	
	float floatval;
	krad_link_rep_t krad_link;
	cJSON *cmd;
	cJSON *part;
	cJSON *part2;
	cJSON *part3;
	cJSON *item;
	char codecs[64];
	char audio_bitrate[64];
	int video_bitrate;
	
	video_bitrate = 0;
	part = NULL;
	part2 = NULL;
	
	cmd = cJSON_Parse (value);
	
	if (!cmd) {
		printke ("Error before: [%s]\n", cJSON_GetErrorPtr());
	} else {

		part = cJSON_GetObjectItem (cmd, "com");
		
		if ((part != NULL) && (strcmp(part->valuestring, "kradmixer") == 0)) {
			part = cJSON_GetObjectItem (cmd, "cmd");		
			if ((part != NULL) && (strcmp(part->valuestring, "update_portgroup") == 0)) {
				part = cJSON_GetObjectItem (cmd, "portgroup_name");
				part2 = cJSON_GetObjectItem (cmd, "control_name");
				part3 = cJSON_GetObjectItem (cmd, "value");
				if ((part != NULL) && (part2 != NULL) && (part3 != NULL)) {
					if (strcmp(part2->valuestring, "xmms2") == 0) {
						krad_ipc_mixer_portgroup_xmms2_cmd (pss->kr_client, part->valuestring, part3->valuestring);
					} else {
						floatval = part3->valuedouble;
						krad_ipc_set_control (pss->kr_client, part->valuestring, part2->valuestring, floatval);
					}
				}
			}
			
			if ((part != NULL) && (strcmp(part->valuestring, "push_dtmf") == 0)) {
				part = cJSON_GetObjectItem (cmd, "dtmf");
				if (part != NULL) {
					krad_ipc_mixer_push_tone (pss->kr_client, part->valuestring);
				}
			}
		}
		
		if ((part != NULL) && (strcmp(part->valuestring, "kradcompositor") == 0)) {
			part = cJSON_GetObjectItem (cmd, "cmd");		
			if ((part != NULL) && (strcmp(part->valuestring, "jsnap") == 0)) {
				krad_ipc_compositor_snapshot_jpeg (pss->kr_client);
			}	
			if ((part != NULL) && (strcmp(part->valuestring, "snap") == 0)) {
				krad_ipc_compositor_snapshot (pss->kr_client);
			}
		}		
	
		if ((part != NULL) && (strcmp(part->valuestring, "kradradio") == 0)) {
			part = cJSON_GetObjectItem (cmd, "cmd");		
			if ((part != NULL) && (strcmp(part->valuestring, "stag") == 0)) {
				part2 = cJSON_GetObjectItem (cmd, "tag_name");
				part3 = cJSON_GetObjectItem (cmd, "tag_value");
				if ((part != NULL) && (part2 != NULL) && (part3 != NULL)) {			
					krad_ipc_set_tag (pss->kr_client, NULL, part2->valuestring, part3->valuestring);
					//printk("aye got %s %s", part2->valuestring, part3->valuestring);
				}
			}	
		}
		
		if ((part != NULL) && (strcmp(part->valuestring, "kradlink") == 0)) {
			part = cJSON_GetObjectItem (cmd, "cmd");		
			if ((part != NULL) && (strcmp(part->valuestring, "update_link") == 0)) {
				part = cJSON_GetObjectItem (cmd, "link_num");
				part2 = cJSON_GetObjectItem (cmd, "control_name");
				part3 = cJSON_GetObjectItem (cmd, "value");
				if ((part != NULL) && (part2 != NULL) && (part3 != NULL)) {
					if (strcmp(part2->valuestring, "opus_signal") == 0) {
						krad_ipc_update_link_adv (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL, part3->valuestring);
					}
					if (strcmp(part2->valuestring, "opus_bandwidth") == 0) {
						krad_ipc_update_link_adv (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH, part3->valuestring);
					}
					if (strcmp(part2->valuestring, "opus_bitrate") == 0) {
						krad_ipc_update_link_adv_num (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, part3->valueint);						
					}
					if (strcmp(part2->valuestring, "opus_complexity") == 0) {
						krad_ipc_update_link_adv_num (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY, part3->valueint);
					}
					if (strcmp(part2->valuestring, "opus_frame_size") == 0) {
						krad_ipc_update_link_adv_num (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE, part3->valueint);
					}
					if (strcmp(part2->valuestring, "theora_quality") == 0) {
						krad_ipc_update_link_adv_num (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY, part3->valueint);
					}
					if (strcmp(part2->valuestring, "vp8_bitrate") == 0) {
						krad_ipc_update_link_adv_num (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, part3->valueint);
					}
					if (strcmp(part2->valuestring, "vp8_min_quantizer") == 0) {
						krad_ipc_update_link_adv_num (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER, part3->valueint);
					}
					if (strcmp(part2->valuestring, "vp8_max_quantizer") == 0) {
						krad_ipc_update_link_adv_num (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER, part3->valueint);
					}
					if (strcmp(part2->valuestring, "vp8_deadline") == 0) {
						krad_ipc_update_link_adv_num (pss->kr_client, part->valueint, EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE, part3->valueint);
					}															
				}
			} else {
			
				if ((part != NULL) && (strcmp(part->valuestring, "remove_link") == 0)) {
					part = cJSON_GetObjectItem (cmd, "link_num");
					krad_ipc_destroy_link (pss->kr_client, part->valueint);
				} else {
					if ((part != NULL) && (strcmp(part->valuestring, "add_link") == 0)) {

						memset (&krad_link, 0, sizeof(krad_link_rep_t));
						memset (codecs, 0, sizeof(codecs));
						memset (audio_bitrate, 0, sizeof(audio_bitrate));

						item = cJSON_GetObjectItem (cmd, "operation_mode");
						if (item != NULL) {

							krad_link.operation_mode = krad_link_string_to_operation_mode (item->valuestring);	
					
							if ((krad_link.operation_mode != TRANSMIT) && (krad_link.operation_mode != RECORD) && 
								(krad_link.operation_mode != CAPTURE)) {
								
								cJSON_Delete (cmd);
								return;
					
							}
							
							item = cJSON_GetObjectItem (cmd, "av_mode");
							if (item != NULL) {
								krad_link.av_mode = krad_link_string_to_av_mode (item->valuestring);
							} else {
								cJSON_Delete (cmd);
								return;
							}
								
							if (krad_link.operation_mode == CAPTURE) {
								item = cJSON_GetObjectItem (cmd, "video_source");
								if (item != NULL) {
									krad_link.video_source = krad_link_string_to_video_source (item->valuestring);
								}
								item = cJSON_GetObjectItem (cmd, "video_device");
								if (item != NULL) {
									strncpy (krad_link.video_device, item->valuestring, sizeof(krad_link.video_device));
								}
								if ((krad_link.video_source == NOVIDEO) || (strlen(krad_link.video_device) == 0)) {
									cJSON_Delete (cmd);
									return;
								}
							}					
							
							if (krad_link.operation_mode == RECORD) {
								item = cJSON_GetObjectItem (cmd, "filename");
								if (item != NULL) {
									strncpy (krad_link.filename, item->valuestring, sizeof(krad_link.filename));
								}
								if (strlen(krad_link.filename) == 0) {
									cJSON_Delete (cmd);
									return;
								}
							}
							
					
							if (krad_link.operation_mode == TRANSMIT) {
							
								item = cJSON_GetObjectItem (cmd, "host");
								if (item != NULL) {
									strncpy (krad_link.host, item->valuestring, sizeof(krad_link.host));
								}
								item = cJSON_GetObjectItem (cmd, "mount");
								if (item != NULL) {
									strncpy (krad_link.mount, item->valuestring, sizeof(krad_link.mount));
								}
								item = cJSON_GetObjectItem (cmd, "password");
								if (item != NULL) {
									strncpy (krad_link.password, item->valuestring, sizeof(krad_link.password));
								}																
								item = cJSON_GetObjectItem (cmd, "port");
								if (item != NULL) {
									krad_link.port = item->valueint;
								}
								
								if (!((strlen(krad_link.host)) && (strlen(krad_link.mount)) && (strlen(krad_link.password)) && (krad_link.port))) {
									cJSON_Delete (cmd);
									return;
								}
							}
							
							if ((krad_link.operation_mode == TRANSMIT) || (krad_link.operation_mode == RECORD)) {

								if ((krad_link.av_mode == AUDIO_ONLY) || (krad_link.av_mode == AUDIO_AND_VIDEO)) {
									item = cJSON_GetObjectItem (cmd, "audio_codec");
									if (item != NULL) {
										strcat (codecs, item->valuestring);
										krad_link.audio_codec = krad_string_to_codec (item->valuestring);
									}
									item = cJSON_GetObjectItem (cmd, "audio_bitrate");
									if (item != NULL) {
										if (krad_link.audio_codec == VORBIS) {
											krad_link.vorbis_quality = item->valuedouble;
											sprintf(audio_bitrate, "%f", krad_link.vorbis_quality);												
										}
										if (krad_link.audio_codec == OPUS) {
											krad_link.opus_bitrate = item->valueint;
											sprintf(audio_bitrate, "%d", krad_link.opus_bitrate);												
										}
										if (krad_link.audio_codec == FLAC) {
											krad_link.flac_bit_depth = item->valueint;
											sprintf(audio_bitrate, "%d", krad_link.flac_bit_depth);
										}
									}
								}
							
								if ((krad_link.av_mode == VIDEO_ONLY) || (krad_link.av_mode == AUDIO_AND_VIDEO)) {
									item = cJSON_GetObjectItem (cmd, "video_codec");
									if (item != NULL) {
										strcat (codecs, item->valuestring);
										krad_link.video_codec = krad_string_to_codec (item->valuestring);										
									}
									item = cJSON_GetObjectItem (cmd, "video_bitrate");
									if (item != NULL) {
										if (krad_link.video_codec == THEORA) {
											krad_link.theora_quality = item->valueint;
											video_bitrate = krad_link.theora_quality;
										}
										if (krad_link.video_codec == VP8) {
											krad_link.vp8_bitrate = item->valueint;
											video_bitrate = krad_link.vp8_bitrate;
										}
									}
								}
							}
							
							if ((krad_link.av_mode == VIDEO_ONLY) || (krad_link.av_mode == AUDIO_AND_VIDEO)) {								

								item = cJSON_GetObjectItem (cmd, "video_width");
								if (item != NULL) {
									krad_link.width = item->valueint;
								}								

								item = cJSON_GetObjectItem (cmd, "video_height");
								if (item != NULL) {
									krad_link.height = item->valueint;
								}
								
								item = cJSON_GetObjectItem (cmd, "fps_numerator");
								if (item != NULL) {
									krad_link.fps_numerator = item->valueint;
								}								

								item = cJSON_GetObjectItem (cmd, "fps_denominator");
								if (item != NULL) {
									krad_link.fps_denominator = item->valueint;
								}

								item = cJSON_GetObjectItem (cmd, "color_depth");
								if (item != NULL) {
									krad_link.color_depth = item->valueint;
								} else {
									krad_link.color_depth = 420;
								}
							}
							
							char *string = malloc(2048);
							krad_link_rep_to_string (&krad_link, string);
							printk("%s", string);
							free(string);

							if (krad_link.operation_mode == TRANSMIT) {
								krad_ipc_create_transmit_link (pss->kr_client, krad_link.av_mode,
															   krad_link.host, krad_link.port, krad_link.mount, krad_link.password, codecs,
														  	   krad_link.width, krad_link.height, video_bitrate, audio_bitrate);
							}
							
							if (krad_link.operation_mode == RECORD) {
								krad_ipc_create_record_link (pss->kr_client, krad_link.av_mode,
															 krad_link.filename, codecs,
															 krad_link.width, krad_link.height,
															 video_bitrate, audio_bitrate);							
							}
							
							if (krad_link.operation_mode == CAPTURE) {
								krad_ipc_create_capture_link (pss->kr_client,
															  krad_link.video_source, krad_link.video_device,
															  krad_link.width, krad_link.height,
															  krad_link.fps_numerator, krad_link.fps_denominator,
															  krad_link.av_mode, "", "");
							}
						}
					}
				}
			}	
		}		
		cJSON_Delete (cmd);
	}
}

/* callbacks from ipc handler to add JSON to websocket message */

void krad_websocket_set_tag (kr_client_session_data_t *kr_client_session_data, char *tag_item, char *tag_name, char *tag_value) {

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradradio");
	cJSON_AddStringToObject (msg, "info", "tag");
	cJSON_AddStringToObject (msg, "tag_item", tag_item);
	cJSON_AddStringToObject (msg, "tag_name", tag_name);
	cJSON_AddStringToObject (msg, "tag_value", tag_value);

}

void krad_websocket_set_cpu_usage (kr_client_session_data_t *kr_client_session_data, int usage) {

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradradio");
	cJSON_AddStringToObject (msg, "info", "cpu");
	cJSON_AddNumberToObject (msg, "system_cpu_usage", usage);

}

void krad_websocket_add_decklink_device ( kr_client_session_data_t *kr_client_session_data, char *name, int num) {

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());	

	cJSON_AddStringToObject (msg, "com", "kradlink");
	cJSON_AddStringToObject (msg, "cmd", "add_decklink_device");
	cJSON_AddNumberToObject (msg, "decklink_device_num", num);
	cJSON_AddStringToObject (msg, "decklink_device_name", name);
}

void krad_websocket_remove_link ( kr_client_session_data_t *kr_client_session_data, int link_num) {

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradlink");
	cJSON_AddStringToObject (msg, "cmd", "remove_link");
	cJSON_AddNumberToObject (msg, "link_num", link_num);

}

void krad_websocket_add_link ( kr_client_session_data_t *kr_client_session_data, krad_link_rep_t *krad_link) {

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradlink");
	cJSON_AddStringToObject (msg, "cmd", "add_link");
	cJSON_AddNumberToObject (msg, "link_num", krad_link->link_num);
	cJSON_AddStringToObject (msg, "operation_mode", krad_link_operation_mode_to_string(krad_link->operation_mode));

	cJSON_AddStringToObject (msg, "av_mode",  krad_link_av_mode_to_string(krad_link->av_mode));

	if (krad_link->operation_mode == CAPTURE) {
	
		cJSON_AddStringToObject (msg, "video_source",  krad_link_video_source_to_string(krad_link->video_source));
	
	}

	if (krad_link->operation_mode == TRANSMIT) {
	
		cJSON_AddStringToObject (msg, "host",  krad_link->host);
		cJSON_AddNumberToObject (msg, "port",  krad_link->port);
		cJSON_AddStringToObject (msg, "mount",  krad_link->mount);

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			cJSON_AddStringToObject (msg, "video_codec",  krad_codec_to_string (krad_link->video_codec));

			cJSON_AddNumberToObject (msg, "video_width",  krad_link->width);
			cJSON_AddNumberToObject (msg, "video_height",  krad_link->height);
			cJSON_AddNumberToObject (msg, "video_fps_numerator",  krad_link->fps_numerator);
			cJSON_AddNumberToObject (msg, "video_fps_denominator",  krad_link->fps_denominator);
			cJSON_AddNumberToObject (msg, "video_color_depth",  krad_link->color_depth);												

		}
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			cJSON_AddStringToObject (msg, "audio_codec",  krad_codec_to_string (krad_link->audio_codec));

			cJSON_AddNumberToObject (msg, "audio_channels",  krad_link->audio_channels);
			cJSON_AddNumberToObject (msg, "audio_sample_rate",  krad_link->audio_sample_rate);

		}
		
		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			if (krad_link->audio_codec == VORBIS) {
				cJSON_AddNumberToObject (msg, "vorbis_quality",  krad_link->vorbis_quality);
			}
			if (krad_link->audio_codec == FLAC) {
				cJSON_AddNumberToObject (msg, "flac_bit_depth",  krad_link->flac_bit_depth);
			}
			if (krad_link->audio_codec == OPUS) {
				cJSON_AddStringToObject (msg, "opus_signal",  krad_opus_signal_to_string (krad_link->opus_signal));
				cJSON_AddStringToObject (msg, "opus_bandwidth",  krad_opus_bandwidth_to_string (krad_link->opus_bandwidth));
				cJSON_AddNumberToObject (msg, "opus_complexity",  krad_link->opus_complexity);
				cJSON_AddNumberToObject (msg, "opus_bitrate",  krad_link->opus_bitrate);
				cJSON_AddNumberToObject (msg, "opus_frame_size",  krad_link->opus_frame_size);			
			}
		}

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			if (krad_link->video_codec == THEORA) {
				cJSON_AddNumberToObject (msg, "theora_quality",  krad_link->theora_quality);
			}
			if (krad_link->video_codec == VP8) {
				cJSON_AddNumberToObject (msg, "vp8_bitrate",  krad_link->vp8_bitrate);
				cJSON_AddNumberToObject (msg, "vp8_deadline",  krad_link->vp8_deadline);
				cJSON_AddNumberToObject (msg, "vp8_min_quantizer",  krad_link->vp8_min_quantizer);
				cJSON_AddNumberToObject (msg, "vp8_max_quantizer",  krad_link->vp8_max_quantizer);								
			}
		}
	}	
}

char *link_item_ebml_to_string (uint32_t item) {

	switch ( item ) {
		case EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH:
			return "opus_bandwidth";
		case EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL:
			return "opus_signal";				
		case EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE:
			return "opus_bitrate";
		case EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY:
			return "opus_complexity";
		case EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE:
			return "opus_frame_size";					
		case EBML_ID_KRAD_LINK_LINK_VP8_BITRATE:
			return "vp8_bitrate";
		case EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER:
			return "vp8_min_quantizer";
		case EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER:
			return "vp8_max_quantizer";
		case EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE:
			return "vp8_deadline";
		case EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY:
			return "theora_quality";
		default:
			return "";
	
	}
}

void krad_websocket_update_link_string ( kr_client_session_data_t *kr_client_session_data, int link_num, uint32_t item, char *string) {

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradlink");
	
	cJSON_AddStringToObject (msg, "cmd", "update_link");
	cJSON_AddNumberToObject (msg, "link_num", link_num);
	cJSON_AddStringToObject (msg, "update_item", link_item_ebml_to_string(item));
	cJSON_AddStringToObject (msg, "update_value", string);

	//printk ("update a link %d %s %s ", link_num, link_item_ebml_to_string(item), string);

}

void krad_websocket_update_link_number ( kr_client_session_data_t *kr_client_session_data, int link_num, uint32_t item, int value) {

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradlink");
	
	cJSON_AddStringToObject (msg, "cmd", "update_link");
	cJSON_AddNumberToObject (msg, "link_num", link_num);
	cJSON_AddStringToObject (msg, "update_item", link_item_ebml_to_string(item));
	cJSON_AddNumberToObject (msg, "update_value", value);

	//printk ("update a link %d %s %d ", link_num, link_item_ebml_to_string(item), value);

}


void krad_websocket_update_portgroup ( kr_client_session_data_t *kr_client_session_data, char *portname, float floatval, char *crossfade_name, float crossfade_val ) {

	//printkd ("update a portgroup called %s", portname);

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradmixer");
	
	cJSON_AddStringToObject (msg, "cmd", "update_portgroup");
	cJSON_AddStringToObject (msg, "portgroup_name", portname);
	cJSON_AddNumberToObject (msg, "volume", floatval);
	
	cJSON_AddStringToObject (msg, "crossfade_name", crossfade_name);
	cJSON_AddNumberToObject (msg, "crossfade", crossfade_val);
	
	krad_ipc_get_tags (kr_client_session_data->kr_client, portname);

}


void krad_websocket_add_portgroup ( kr_client_session_data_t *kr_client_session_data, char *portname, float floatval, char *crossfade_name, float crossfade_val, int xmms2 ) {

	//printkd ("add a portgroup called %s withe a volume of %f", portname, floatval);

	cJSON *msg;

	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradmixer");
	
	cJSON_AddStringToObject (msg, "cmd", "add_portgroup");
	cJSON_AddStringToObject (msg, "portgroup_name", portname);
	cJSON_AddNumberToObject (msg, "volume", floatval);
	
	cJSON_AddStringToObject (msg, "crossfade_name", crossfade_name);
	cJSON_AddNumberToObject (msg, "crossfade", crossfade_val);
	
	cJSON_AddNumberToObject (msg, "xmms2", xmms2);	
	
	krad_ipc_get_tags (kr_client_session_data->kr_client, portname);

}

void krad_websocket_remove_portgroup ( kr_client_session_data_t *kr_client_session_data, char *portname ) {

	//printkd ("remove a portgroup called %s", portname);

	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradmixer");
	
	cJSON_AddStringToObject (msg, "cmd", "remove_portgroup");
	cJSON_AddStringToObject (msg, "portgroup_name", portname);

}

void krad_websocket_set_control ( kr_client_session_data_t *kr_client_session_data, char *portname, char *controlname, float floatval) {

	//printkd ("set portgroup called %s control %s with a value %f", portname, controlname, floatval);
	
	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradmixer");
	
	cJSON_AddStringToObject (msg, "cmd", "control_portgroup");
	cJSON_AddStringToObject (msg, "portgroup_name", portname);
	cJSON_AddStringToObject (msg, "control_name", controlname);
	cJSON_AddNumberToObject (msg, "value", floatval);
	
}

void krad_websocket_set_sample_rate ( kr_client_session_data_t *kr_client_session_data, int sample_rate) {

	printkd ("set_sample_rate called %d", sample_rate);
	
	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradmixer");
	
	cJSON_AddStringToObject (msg, "cmd", "set_sample_rate");
	cJSON_AddNumberToObject (msg, "sample_rate", sample_rate);
	
}


void krad_websocket_set_frame_rate ( kr_client_session_data_t *kr_client_session_data, int numerator, int denominator) {


	
	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradcompositor");
	
	cJSON_AddStringToObject (msg, "cmd", "set_frame_rate");
	cJSON_AddNumberToObject (msg, "numerator", numerator);
	cJSON_AddNumberToObject (msg, "denominator", denominator);
}

void krad_websocket_set_frame_size ( kr_client_session_data_t *kr_client_session_data, int width, int height) {


	
	cJSON *msg;
	
	cJSON_AddItemToArray(kr_client_session_data->msgs, msg = cJSON_CreateObject());
	
	cJSON_AddStringToObject (msg, "com", "kradcompositor");
	
	cJSON_AddStringToObject (msg, "cmd", "set_frame_size");
	cJSON_AddNumberToObject (msg, "width", width);
	cJSON_AddNumberToObject (msg, "height", height);
}

/* IPC Handler */

int krad_websocket_ipc_handler ( kr_client_t *krad_ipc, void *ptr ) {

	kr_client_session_data_t *kr_client_session_data = (kr_client_session_data_t *)ptr;

	int has_xmms2;

	uint32_t message;
	uint32_t ebml_id;	
	uint64_t ebml_data_size;
	//uint64_t element;
	int list_count;
	int list_size;
	int i;
	float floatval;	
	char portname_actual[256];
	char controlname_actual[1024];
	char string_actual[1024];	
	int bytes_read;
	string_actual[0] = '\0';	
	portname_actual[0] = '\0';
	controlname_actual[0] = '\0';
	
	char *portname = portname_actual;
	char *controlname = controlname_actual;
	char *string = string_actual;	
	char crossfadename_actual[1024];	
	char *crossfadename = crossfadename_actual;
	float crossfade;	
	
	uint64_t number;
	uint64_t numbers[16];	
	
	char tag_item_actual[256];	
	char tag_name_actual[256];
	char tag_value_actual[1024];
	
	tag_name_actual[0] = '\0';
	tag_item_actual[0] = '\0';
	tag_value_actual[0] = '\0';
	
	char *tag_item = tag_item_actual;
	char *tag_name = tag_name_actual;
	char *tag_value = tag_value_actual;
	
	krad_link_rep_t *krad_link_rep;
	
	krad_link_rep = NULL;
	
	has_xmms2 = 0;	
	
	bytes_read = 0;
	ebml_id = 0;
	list_size = 0;	
	floatval = 0;
	message = 0;
	ebml_data_size = 0;
	//element = 0;
	i = 0;
	
	krad_ebml_read_element ( krad_ipc->krad_ebml, &message, &ebml_data_size);

	switch ( message ) {


	
		case EBML_ID_KRAD_RADIO_MSG:
				//printf("Received KRAD_RADIO_MSG %zu bytes of data.\n", ebml_data_size);		
		
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);
			switch ( ebml_id ) {
			
				case EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE:
					number = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					//printk ("System CPU Usage: %d%%", number);
					krad_websocket_set_cpu_usage (kr_client_session_data, number);
					break;

				case EBML_ID_KRAD_RADIO_TAG_LIST:
					//printf("Received Tag list %"PRIu64" bytes of data.\n", ebml_data_size);
					list_size = ebml_data_size;
					while ((list_size) && ((bytes_read += krad_ipc_client_read_tag ( krad_ipc, &tag_item, &tag_name, &tag_value )) <= list_size)) {
						//printk ("%s: %s - %s", tag_item, tag_name, tag_value);
						krad_websocket_set_tag (kr_client_session_data, tag_item, tag_name, tag_value);
						if (bytes_read == list_size) {
							break;
						}
					}
					break;
				case EBML_ID_KRAD_RADIO_TAG:
					krad_ipc_client_read_tag_inner ( krad_ipc, &tag_item, &tag_name, &tag_value );
					//printk ("%s: %s - %s", tag_item, tag_name, tag_value);
					krad_websocket_set_tag (kr_client_session_data, tag_item, tag_name, tag_value);
					break;

				case EBML_ID_KRAD_RADIO_UPTIME:
					number = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					printkd ("Uptime: %"PRIu64"", number);
					break;
				case EBML_ID_KRAD_RADIO_SYSTEM_INFO:
					krad_ebml_read_string (krad_ipc->krad_ebml, string, ebml_data_size);
					printkd ("%s", string);
					break;

			}
	
	
			break;
	
		case EBML_ID_KRAD_LINK_MSG:
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);

			switch ( ebml_id ) {
			
				case EBML_ID_KRAD_LINK_DECKLINK_LIST:

					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);
					list_count = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					for (i = 0; i < list_count; i++) {
						krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);						
						krad_ebml_read_string (krad_ipc->krad_ebml, string, ebml_data_size);
						printk("%d: %s\n", i, string);
						krad_websocket_add_decklink_device (kr_client_session_data, string, i);						
					}	
					break;
							
			
				case EBML_ID_KRAD_LINK_LINK_LIST:
					//printf("Received LINK control list %"PRIu64" bytes of data.\n", ebml_data_size);

					list_size = ebml_data_size;
					i = 0;
					while ((list_size) && ((bytes_read += krad_ipc_client_read_link ( krad_ipc, string, &krad_link_rep)) <= list_size)) {
						//printkd ("%d: %s\n", i, string);						
						krad_websocket_add_link (kr_client_session_data, krad_link_rep);
						i++;
						free (krad_link_rep);
						
						if (bytes_read == list_size) {
							break;
						}
					}	
					break;
					
					
				case EBML_ID_KRAD_LINK_LINK_CREATED:
				
					krad_ipc_client_read_link ( krad_ipc, string, &krad_link_rep);
					krad_websocket_add_link (kr_client_session_data, krad_link_rep);
					free (krad_link_rep);
					
					break;

				case EBML_ID_KRAD_LINK_LINK_UPDATED:
			
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);				
					numbers[0] = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);

					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);			
					if ((ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH) || (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL)) {
						krad_ebml_read_string (krad_ipc->krad_ebml, string, ebml_data_size);
						krad_websocket_update_link_string (kr_client_session_data, numbers[0], ebml_id, string);
					} else {
						numbers[1] = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
						krad_websocket_update_link_number (kr_client_session_data, numbers[0], ebml_id, numbers[1]);						
					}

					break;

				case EBML_ID_KRAD_LINK_LINK_DESTROYED:
					
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);				
					numbers[0] = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					
					krad_websocket_remove_link (kr_client_session_data, numbers[0]);
					
					break;

				default:
					//printkd ("Received KRAD_LINK_MSG %"PRIu64" bytes of data.\n", ebml_data_size);
					break;
			}

			break;

		case EBML_ID_KRAD_COMPOSITOR_MSG:
			//printkd ("krad_radio_websocket_ipc_handler got message from krad COMPOSITOR\n");
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);
			
			switch ( ebml_id ) {
				case EBML_ID_KRAD_COMPOSITOR_FRAME_RATE:
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);				
					numbers[0] = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);					
					numbers[1] = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					krad_websocket_set_frame_rate ( kr_client_session_data, numbers[0], numbers[1]);
					break;
				case EBML_ID_KRAD_COMPOSITOR_FRAME_SIZE:
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);				
					numbers[0] = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);					
					numbers[1] = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					krad_websocket_set_frame_size ( kr_client_session_data, numbers[0], numbers[1]);
					break;
			}
					
			
			break;
			
		case EBML_ID_KRAD_MIXER_MSG:
//			printkd ("krad_radio_websocket_ipc_handler got message from krad mixer\n");
//			krad_ipc_server_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_CONTROL, portname, controlname, floatval);
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);
			
			switch ( ebml_id ) {
				case EBML_ID_KRAD_MIXER_CONTROL:
					//printkd ("Received mixer control list %"PRIu64" bytes of data.\n", ebml_data_size);

					krad_ipc_client_read_mixer_control ( krad_ipc, &portname, &controlname, &floatval );
					krad_websocket_set_control ( kr_client_session_data, portname, controlname, floatval);
					
					break;	
				case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
					//printkd ("Received PORTGROUP list %"PRIu64" bytes of data.\n", ebml_data_size);
					list_size = ebml_data_size;
					while ((list_size) && ((bytes_read += krad_ipc_client_read_portgroup ( krad_ipc, portname, &floatval, crossfadename, &crossfade, &has_xmms2 )) <= list_size)) {
						krad_websocket_add_portgroup (kr_client_session_data, portname, floatval, crossfadename, crossfade, has_xmms2);
						
						if (bytes_read == list_size) {
							break;
						}
					}	
					break;
				case EBML_ID_KRAD_MIXER_PORTGROUP_CREATED:
					//printk ("PORTGROUP_CREATED msg %"PRIu64" bytes", ebml_data_size );
					
					krad_ipc_client_read_portgroup ( krad_ipc, portname, &floatval, crossfadename, &crossfade, &has_xmms2 );

					krad_websocket_add_portgroup (kr_client_session_data, portname, floatval, crossfadename, crossfade, has_xmms2);

					break;
				
				case EBML_ID_KRAD_MIXER_PORTGROUP_DESTROYED:
				
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);	

					if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
						//printkd ("hrm wtf2\n");
					} else {
						//printkd ("tag name size %zu\n", ebml_data_size);
					}
				
					krad_ebml_read_string (krad_ipc->krad_ebml, portname_actual, ebml_data_size);
					//printkd ("PORTGROUP_DESTROYED msg %zu bytes  \n", ebml_data_size );
					
					krad_websocket_remove_portgroup (kr_client_session_data, portname_actual);
					
					break;
					
				case EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED:
					//printkd ("PORTGROUP_UPDATED msg %"PRIu64" bytes  \n", ebml_data_size );
				
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);

					if (ebml_id == EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {				
			
						krad_ebml_read_string (krad_ipc->krad_ebml, portname_actual, ebml_data_size);
			
			
						krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);	

						if (ebml_id == EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
					
							krad_ebml_read_string (krad_ipc->krad_ebml, crossfadename, ebml_data_size);
					
							//printkd ("ya %s %s\n", portname_actual, crossfadename);
					
						}
					}
				
				
					break;
					
				case EBML_ID_KRAD_MIXER_SAMPLE_RATE:
					number = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					krad_websocket_set_sample_rate ( kr_client_session_data, number);
					break;
					
					
			}
		
		
			break;
	
	}

	return 0;

}

/*
int ebml_frag_to_js (char *js_data, unsigned char *ebml_frag) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	uint64_t a_number;
	uint32_t ebml_frag_pos;
	
	ebml_frag_pos = 0;

	ebml_frag_pos += krad_ebml_read_element_from_frag (&ebml_frag[ebml_frag_pos], &ebml_id, &ebml_data_size);

	a_number = krad_ebml_read_number_from_frag (&ebml_frag[ebml_frag_pos], ebml_data_size);

	printkd ( "frag got %u %"PRIu64" - %"PRIu64"\n", ebml_id, ebml_data_size, a_number);
	
	
	
	return sprintf(js_data, "KM%u:%"PRIu64"", ebml_id, a_number);

}
*/

/****	Poll Functions	****/

void add_poll_fd (int fd, short events, int fd_is, kr_client_session_data_t *pss, void *bspointer) {

	krad_websocket_t *krad_websocket = krad_websocket_glob;
	// incase its not one of the 4 krad types, and it was left set as something when deleted
	krad_websocket->fdof[krad_websocket->count_pollfds] = MYSTERY;

	char fd_text[128];

	strcpy(fd_text, "Mystery");
	
	if (fd_is == KRAD_IPC) {
		krad_websocket->fdof[krad_websocket->count_pollfds] = KRAD_IPC;
		krad_websocket->sessions[krad_websocket->count_pollfds] = pss;
		strcpy(fd_text, "Krad IPC");
	}

	krad_websocket->pollfds[krad_websocket->count_pollfds].fd = fd;
	krad_websocket->pollfds[krad_websocket->count_pollfds].events = events;
	krad_websocket->pollfds[krad_websocket->count_pollfds++].revents = 0;

	//printkd ("New %s FD %d, Total: %d", fd_text, fd, krad_websocket->count_pollfds);

}

void set_poll_mode_fd (int fd, short events) {

	krad_websocket_t *krad_websocket = krad_websocket_glob;
	int n;

	for (n = 0; n < krad_websocket->count_pollfds; n++) {
		if (krad_websocket->pollfds[n].fd == fd) {
			krad_websocket->pollfds[n].events = events;
			//printkd ("poll fd was found!");
		}
	}

}

void set_poll_mode_pollfd (struct pollfd *apollfd, short events) {

	apollfd->events = events;

}

void del_poll_fd(user) {

	krad_websocket_t *krad_websocket = krad_websocket_glob;
	int n;
	krad_websocket->count_pollfds--;

	for (n = 0; n < krad_websocket->count_pollfds; n++) {
		if (krad_websocket->pollfds[n].fd == (int)(long)user) {
			while (n < krad_websocket->count_pollfds) {
				krad_websocket->pollfds[n] = krad_websocket->pollfds[n + 1];
				krad_websocket->sessions[n] = krad_websocket->sessions[n + 1];
				krad_websocket->fdof[n] = krad_websocket->fdof[n + 1];
				n++;
			}
		}
	}
	
	//printkd ("Removed FD %d, Total: %d", (int)(long)user, krad_websocket->count_pollfds);		

}


/* WebSocket Functions */


int callback_http (struct libwebsocket_context *this, struct libwebsocket *wsi,
				   enum libwebsocket_callback_reasons reason, void *user,
				   void *in, size_t len) {

	int n;
	//char client_name[128];
	//char client_ip[128];

	krad_websocket_t *krad_websocket = krad_websocket_glob;

	switch (reason) {
		/*
		case LWS_CALLBACK_HTTP:
			printke ("serving HTTP URI %s\n", (char *)in);

			if (in && strncmp(in, "/favicon.ico", 12) == 0) {
				if (libwebsockets_serve_http_file(wsi,
					 "/favicon.ico", "image/x-icon"))
					printke ("Failed to send favicon\n");
				break;
			}

			// send the script... when it runs it'll start websockets

			if (libwebsockets_serve_http_file(wsi,
					  "/test.html", "text/html"))
				printke (Failed to send HTTP file\n");
			break;
		*/
		case LWS_CALLBACK_ADD_POLL_FD:
			krad_websocket->fdof[krad_websocket->count_pollfds] = MYSTERY;
			krad_websocket->pollfds[krad_websocket->count_pollfds].fd = (int)(long)user;
			krad_websocket->pollfds[krad_websocket->count_pollfds].events = (int)len;
			krad_websocket->pollfds[krad_websocket->count_pollfds++].revents = 0;
			break;

		case LWS_CALLBACK_DEL_POLL_FD:
			del_poll_fd(user, len);
			break;

		case LWS_CALLBACK_SET_MODE_POLL_FD:
			for (n = 0; n < krad_websocket->count_pollfds; n++) {
				if (krad_websocket->pollfds[n].fd == (int)(long)user) {
					krad_websocket->pollfds[n].events |= (int)(long)len;
				}
			}
			break;

		case LWS_CALLBACK_CLEAR_MODE_POLL_FD:
			for (n = 0; n < krad_websocket->count_pollfds; n++) {
				if (krad_websocket->pollfds[n].fd == (int)(long)user) {
					krad_websocket->pollfds[n].events &= ~(int)(long)len;
				}
			}
			break;

		default:
			break;
	}

	return 0;
}


int callback_kr_client (struct libwebsocket_context *this, struct libwebsocket *wsi, 
					   enum libwebsocket_callback_reasons reason, 
					   void *user, void *in, size_t len)
{
	int ret;
	krad_websocket_t *krad_websocket = krad_websocket_glob;
	kr_client_session_data_t *pss = user;
	unsigned char *p = &krad_websocket->buffer[LWS_SEND_BUFFER_PRE_PADDING];
	
	switch (reason) {

		case LWS_CALLBACK_ESTABLISHED:

			pss->context = this;
			pss->wsi = wsi;
			pss->krad_websocket = krad_websocket_glob;

			pss->kr_client = kr_connect (pss->krad_websocket->sysname);
			if (pss->kr_client != NULL) {
				pss->kr_client_info = 0;
				pss->hello_sent = 0;			
				krad_ipc_set_handler_callback (pss->kr_client, krad_websocket_ipc_handler, pss);
				krad_ipc_get_mixer_sample_rate (pss->kr_client);
				krad_ipc_compositor_get_frame_rate (pss->kr_client);
				krad_ipc_compositor_get_frame_size (pss->kr_client);			
				krad_ipc_get_portgroups (pss->kr_client);
				krad_ipc_list_decklink (pss->kr_client);
				krad_ipc_list_links (pss->kr_client);
				krad_ipc_get_tags (pss->kr_client, NULL);
				krad_ipc_broadcast_subscribe (pss->kr_client, EBML_ID_KRAD_RADIO_GLOBAL_BROADCAST);
				add_poll_fd (pss->kr_client->sd, POLLIN, KRAD_IPC, pss, NULL);
			}
			break;

		case LWS_CALLBACK_CLOSED:

			kr_disconnect (pss->kr_client);
			del_poll_fd(pss->kr_client->sd);
			pss->hello_sent = 0;
			pss->context = NULL;
			pss->wsi = NULL;
		
			break;

		case LWS_CALLBACK_BROADCAST:

			memcpy (p, in, len);
		
			//printkd ("bcast happens\n");
		
			libwebsocket_write(wsi, p, len, LWS_WRITE_TEXT);
			break;


		case LWS_CALLBACK_SERVER_WRITEABLE:


			if (pss->kr_client_info == 1) {
				
				//printf ("wanted to write %d bytes to browser\n", pss->krad_ipc_data_len);
				memcpy (p, pss->msgstext, pss->msgstextlen + 1);
				ret = libwebsocket_write(wsi, p, pss->msgstextlen, LWS_WRITE_TEXT);
				if (ret < 0) {
					printke ("krad_ipc ERROR writing to socket");
					return 1;
				}
				pss->kr_client_info = 0;
				pss->msgstextlen = 0;
				free (pss->msgstext);
			}

			break;

		case LWS_CALLBACK_RECEIVE:

			krad_ipc_from_json (pss, in, len);
		
			break;

		default:
			break;
	}

	return 0;
}


krad_websocket_t *krad_websocket_server_create (char *sysname, int port) {


	krad_websocket_t *krad_websocket = calloc (1, sizeof (krad_websocket_t));

	krad_websocket_glob = krad_websocket;

	krad_websocket->shutdown = KRAD_WEBSOCKET_STARTING;

	krad_websocket->port = port;
	strcpy (krad_websocket->sysname, sysname);

	krad_websocket->buffer = calloc(1, 32768 * 8);

	krad_websocket->context = libwebsocket_create_context (krad_websocket->port, NULL, protocols,
										   				   libwebsocket_internal_extensions, 
										   				   NULL, NULL, -1, -1, 0, NULL);
	if (krad_websocket->context == NULL) {
		printke ("libwebsocket init failed");
		krad_websocket_server_destroy (krad_websocket);
		return NULL;
	}
	
	pthread_create (&krad_websocket->server_thread, NULL, krad_websocket_server_run, (void *)krad_websocket);
	pthread_detach (krad_websocket->server_thread);	

	return krad_websocket;

}

void *krad_websocket_server_run (void *arg) {

	krad_websocket_t *krad_websocket = (krad_websocket_t *)arg;

	int n = 0;
	char info[256] = "";

	krad_system_set_thread_name ("kr_websocket");

	krad_websocket->shutdown = KRAD_WEBSOCKET_RUNNING;

	while (!krad_websocket->shutdown) {

		n = poll (krad_websocket->pollfds, krad_websocket->count_pollfds, 500);
		
		if (krad_websocket->shutdown) {
			break;
		}
		
		if (n < 0) {
			break;
		}

		if (n) {
		
			for (n = 0; n < krad_websocket->count_pollfds; n++) {
				
				if (krad_websocket->pollfds[n].revents) {
				
					if (krad_websocket->pollfds[n].revents && (krad_websocket->fdof[n] == MYSTERY)) {
						//sprintf(info + strlen(info), " it was a MYSTERY in the service of lws! %d", n);
						libwebsocket_service_fd (krad_websocket->context, &krad_websocket->pollfds[n]);	
						continue;
					}
				

					if ((krad_websocket->pollfds[n].revents & POLLERR) || (krad_websocket->pollfds[n].revents & POLLHUP)) {
							
						if (krad_websocket->pollfds[n].revents & POLLERR) {				
							sprintf(info + strlen(info), "Poll ERR on FD number %d", n);
						}
					
						if (krad_websocket->pollfds[n].revents & POLLHUP) {				
							sprintf(info + strlen(info), "Poll HUP on FD number %d", n);
						}
						
						switch ( krad_websocket->fdof[n] ) {
						
							case KRAD_IPC:
								//sprintf(info + strlen(info), " it was Krad Mixer", n);
								//kr_disconnect(sessions[n]->krad_ipc);
								//del_poll_fd(n);
								//n++;
								break;
							case MYSTERY:
								//sprintf(info + strlen(info), " it was a MSYSTERY!", n);
								//libwebsocket_service_fd(context, &pollfds[n]);
								//del_poll_fd(n);
								//n++;
								break;
						
						}
					
					} else {
					
						if (krad_websocket->pollfds[n].revents & POLLIN) {
				
							sprintf(info + strlen(info), "Poll IN on FD number %d", n);
							
							switch ( krad_websocket->fdof[n] ) {
								case KRAD_IPC:
								
									krad_websocket->sessions[n]->msgs = cJSON_CreateArray();
	
									cJSON *msg;
	
									if (krad_websocket->sessions[n]->hello_sent == 0) {
										cJSON_AddItemToArray (krad_websocket->sessions[n]->msgs, msg = cJSON_CreateObject());
	
										cJSON_AddStringToObject (msg, "com", "kradradio");
										cJSON_AddStringToObject (msg, "info", "sysname");
										cJSON_AddStringToObject (msg, "infoval", krad_websocket->sysname);
	
	
										cJSON_AddItemToArray (krad_websocket->sessions[n]->msgs, msg = cJSON_CreateObject());
	
										cJSON_AddStringToObject (msg, "com", "kradradio");
										cJSON_AddStringToObject (msg, "info", "motd");
										cJSON_AddStringToObject (msg, "infoval", "kradradio json alpha");
										
										krad_websocket->sessions[n]->hello_sent = 1;
									
									}
									
									krad_ipc_client_handle (krad_websocket->sessions[n]->kr_client);
									
									krad_websocket->sessions[n]->msgstext = cJSON_Print (krad_websocket->sessions[n]->msgs);
									krad_websocket->sessions[n]->msgstextlen = strlen (krad_websocket->sessions[n]->msgstext);
									cJSON_Delete (krad_websocket->sessions[n]->msgs);
									
									krad_websocket->sessions[n]->kr_client_info = 1;
									libwebsocket_callback_on_writable(krad_websocket->sessions[n]->context, krad_websocket->sessions[n]->wsi);
									break;
		
								case MYSTERY:
									//sprintf(info + strlen(info), " it was a MSYSTERY!", n);
									//libwebsocket_service_fd(context, &pollfds[n]);
									break;
							}
						}
						
						if (krad_websocket->pollfds[n].revents & POLLOUT) {
						
							sprintf(info, "Poll OUT on FD number %d", n);
						
							switch ( krad_websocket->fdof[n] ) {
								case KRAD_IPC:
									sprintf(info + strlen(info), " it was Krad IPC %d", n);
									break;
							
								case MYSTERY:
									//sprintf(info + strlen(info), " it was a MSYSTERY!", n);
									//libwebsocket_service_fd(context, &pollfds[n]);
									break;
							}
						}
					}
					
					//printf("%s\n", info);
					strcpy(info, "");
					
				}
			}
			
		} else {
		
			//printf("Timeout! %d fds currently... \n", krad_websocket->count_pollfds);
		
		}
	}
	
	krad_websocket->shutdown = KRAD_WEBSOCKET_SHUTINGDOWN;
	
	return NULL;

}


void krad_websocket_server_destroy (krad_websocket_t *krad_websocket) {

	int patience;
	
  printk ("Krad Websocket shutdown started");	
	
	if (krad_websocket != NULL) {
	
		patience = KRAD_WEBSOCKET_SERVER_TIMEOUT_US * 3;
	
		if (krad_websocket->shutdown == KRAD_WEBSOCKET_RUNNING) {
			krad_websocket->shutdown = KRAD_WEBSOCKET_DO_SHUTDOWN;
	
			while ((krad_websocket->shutdown != KRAD_WEBSOCKET_SHUTINGDOWN) && (patience > 0)) {
				usleep (KRAD_WEBSOCKET_SERVER_TIMEOUT_US / 4);
				patience -= KRAD_WEBSOCKET_SERVER_TIMEOUT_US / 4;
			}
		}

		free (krad_websocket->buffer);
		libwebsocket_context_destroy (krad_websocket->context);
		free (krad_websocket);
		krad_websocket_glob = NULL;
	}
	
  printk ("Krad Websocket shutdown complete");	
	
}
