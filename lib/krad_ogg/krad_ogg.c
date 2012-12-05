#include "krad_ogg.h"

int krad_ogg_track_count(krad_ogg_t *krad_ogg) {

	int t;
	int count;

	count = 0;

	for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
		if ((krad_ogg->tracks[t].serial != KRAD_OGG_NO_SERIAL) && (krad_ogg->tracks[t].ready == 1)) {
			count++;
		}
	}

	return count;
}


krad_codec_t krad_ogg_track_codec (krad_ogg_t *krad_ogg, int track) {

	if ((krad_ogg->tracks[track].serial != KRAD_OGG_NO_SERIAL) && (krad_ogg->tracks[track].ready == 1)) {
		return krad_ogg->tracks[track].codec;
	}

	return NOCODEC;
}


int krad_ogg_track_header_count(krad_ogg_t *krad_ogg, int track) {

	if ((krad_ogg->tracks[track].serial != KRAD_OGG_NO_SERIAL) && (krad_ogg->tracks[track].ready == 1)) {
		return krad_ogg->tracks[track].header_count;
	}

	return 0;
}


int krad_ogg_track_header_size (krad_ogg_t *krad_ogg, int track, int header) {

	if ((krad_ogg->tracks[track].serial != KRAD_OGG_NO_SERIAL) && (krad_ogg->tracks[track].ready == 1)) {
		return krad_ogg->tracks[track].header_len[header];
	}

	return 0;
}


int krad_ogg_read_track_header(krad_ogg_t *krad_ogg, unsigned char *buffer, int track, int header) {

	if ((krad_ogg->tracks[track].serial != KRAD_OGG_NO_SERIAL) && (krad_ogg->tracks[track].ready == 1)) {
		memcpy(buffer, krad_ogg->tracks[track].header[header], krad_ogg->tracks[track].header_len[header]);
		return krad_ogg->tracks[track].header_len[header];
	}

	return 0;
}


int krad_ogg_track_active (krad_ogg_t *krad_ogg, int track) {

	if (krad_ogg->tracks[track].serial != KRAD_OGG_NO_SERIAL) {
		return 1;
	}

	return 0;
}


int krad_ogg_track_changed (krad_ogg_t *krad_ogg, int track) {

	if (krad_ogg->tracks[track].serial != krad_ogg->tracks[track].last_serial) {
		krad_ogg->tracks[track].last_serial = krad_ogg->tracks[track].serial;
		return 1;
	}

	return 0;
}


float krad_ogg_vorbis_sample_rate(ogg_packet *packet) {

	float sample_rate;
	
	vorbis_info v_info;
	vorbis_comment v_comment;

	vorbis_info_init(&v_info);
	vorbis_comment_init(&v_comment);

	vorbis_synthesis_headerin(&v_info, &v_comment, packet);

	sample_rate = v_info.rate;
	printkd ("the sample rate is %f", sample_rate);

	vorbis_info_clear(&v_info);
	vorbis_comment_clear(&v_comment);

	return sample_rate;
}


int krad_ogg_theora_frame_rate (ogg_packet *packet) {

	int frame_rate;
	
    theora_comment t_comment;
    theora_info t_info;

    theora_info_init (&t_info);
	theora_comment_init (&t_comment);

	theora_decode_header (&t_info, &t_comment, packet);

	frame_rate = t_info.fps_numerator / t_info.fps_denominator;
	printkd ("the frame rate is %d", frame_rate);

	theora_info_clear (&t_info);
	theora_comment_clear (&t_comment);

	return frame_rate;
}


int krad_ogg_theora_keyframe_shift (ogg_packet *packet) {

	int keyframe_shift;
	
    theora_comment t_comment;
    th_info t_info;

    th_info_init (&t_info);
	theora_comment_init (&t_comment);

	//theora_decode_header (&t_info, &t_comment, packet);

	keyframe_shift = t_info.keyframe_granule_shift;
	printkd ("the keyframe shift is %d", keyframe_shift);

	th_info_clear (&t_info);
	theora_comment_clear (&t_comment);

	return keyframe_shift;
}


krad_codec_t krad_ogg_get_codec (ogg_packet *packet) {

	krad_codec_t codec;
    theora_comment t_comment;
    theora_info t_info;
   
	codec = NOCODEC;

	if (memcmp(packet->packet, "fishead\0", 8) == 0) {
        printkd ("found skeleton");
		return SKELETON;
	}
   
	if (memcmp (packet->packet + 1, "FLAC", 4) == 0) {
        printkd ("found flac");
		return FLAC;
	}
	
	if (memcmp (packet->packet, "Opus", 4) == 0) {
        printkd ("found opus");
		return OPUS;
	}

    if (vorbis_synthesis_idheader (packet) == 1) {
        printkd ("found vorbis");
        codec = VORBIS;
    }
    
    if (codec != NOCODEC) {
    	return codec;
    }
    
	theora_info_init (&t_info);
	theora_comment_init (&t_comment);

    if (theora_decode_header (&t_info, &t_comment, packet) == 0) {
        printkd ("found theora");
        codec = THEORA;
    }
    
	theora_info_clear (&t_info);
	theora_comment_clear (&t_comment);
    
    if (codec != NOCODEC) {
    	return codec;
    }
 
 	printke ("sucky");
 
	return NOCODEC;  
}


int krad_ogg_read_packet (krad_ogg_t *krad_ogg, int *track, uint64_t *timecode, unsigned char *buffer) {

	int t;
	int ret;
	ogg_packet packet;
	
	while (1) {
	
		for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
			if ((krad_ogg->tracks[t].serial != KRAD_OGG_NO_SERIAL) && (krad_ogg->tracks[t].ready == 0)) {
				while (krad_ogg->tracks[t].ready == 0) {
					ret = ogg_stream_packetout(&krad_ogg->tracks[t].stream_state, &packet);
					
					if ((krad_ogg->tracks[t].codec == SKELETON) || (krad_ogg->tracks[t].codec == NOCODEC)) {
						// just toss the skeleton
						while (ogg_stream_packetout(&krad_ogg->tracks[t].stream_state, &packet));
						
						ret = 0;
					}
					
					if (ret == 1) {
					
						krad_ogg->tracks[t].header_len[krad_ogg->tracks[t].header_count] = packet.bytes;
					
						krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count] = malloc(packet.bytes);
						memcpy (krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count],
								packet.packet,
								packet.bytes);
					
						if (krad_ogg->tracks[t].header_count == 0) {
					
							krad_ogg->tracks[t].codec = krad_ogg_get_codec(&packet);
							if (krad_ogg->tracks[t].codec == VORBIS) {
								krad_ogg->tracks[t].sample_rate = krad_ogg_vorbis_sample_rate(&packet);
							}
							
							if (krad_ogg->tracks[t].codec == THEORA) {
								krad_ogg->tracks[t].frame_rate = krad_ogg_theora_frame_rate(&packet);
								krad_ogg->tracks[t].keyframe_shift = krad_ogg_theora_keyframe_shift (&packet);
							}
							
						}

						krad_ogg->tracks[t].header_count++;
						if ((krad_ogg->tracks[t].header_count == 3) &&
							((krad_ogg->tracks[t].codec == VORBIS) || (krad_ogg->tracks[t].codec == THEORA))) {
							
							krad_ogg->tracks[t].ready = 1;
						}
						if ((krad_ogg->tracks[t].header_count == 2) &&
							((krad_ogg->tracks[t].codec == OPUS) || (krad_ogg->tracks[t].codec == FLAC))) {
							
							krad_ogg->tracks[t].header_count = 1;
							
							if (krad_ogg->tracks[t].codec == FLAC) {
								// remove oggflac extra stuff
								memmove (krad_ogg->tracks[t].header[0],
										 krad_ogg->tracks[t].header[0] + 9,
										 krad_ogg->tracks[t].header_len[0] - 9);
								krad_ogg->tracks[t].header_len[0] -= 9;
								if (krad_ogg->tracks[t].header_len[0] != 42) {
									printkd ("ruh oh! our oggflac expectations where not met, problem likely!");
								}
							}
							
							
							krad_ogg->tracks[t].ready = 1;
						}
					} else {
						break;
					}
				}
			}
		}
	
		for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
			if ((krad_ogg->tracks[t].serial != KRAD_OGG_NO_SERIAL) && (krad_ogg->tracks[t].ready == 1)) {
				ret = ogg_stream_packetout(&krad_ogg->tracks[t].stream_state, &packet);
				//if ((ret == 1) && (packet.bytes > 0)) {
				if (ret == 1) {
					memcpy(buffer, packet.packet, packet.bytes);
					*track = t;
				
					if (packet.e_o_s) {
						ogg_stream_clear(&krad_ogg->tracks[t].stream_state);
						krad_ogg->tracks[t].serial = KRAD_OGG_NO_SERIAL;
						krad_ogg->tracks[t].ready = 0;
						krad_ogg->tracks[t].header_count = 0;
						krad_ogg->tracks[t].codec = NOCODEC;
						while (krad_ogg->tracks[t].header_count) {
							free (krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count - 1]);
							krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count - 1] = NULL;
							krad_ogg->tracks[t].header_count--;
						}
					}
					if (packet.granulepos != -1) {
						if (krad_ogg->tracks[t].codec == VORBIS) {
							krad_ogg->tracks[t].last_granulepos =
								(packet.granulepos / krad_ogg->tracks[t].sample_rate) * 1000.0;
						}
						
						if (krad_ogg->tracks[t].codec == THEORA) {
							ogg_int64_t iframe;
							ogg_int64_t pframe;
							iframe = packet.granulepos >> krad_ogg->tracks[t].keyframe_shift;
							pframe = packet.granulepos - (iframe << krad_ogg->tracks[t].keyframe_shift);
							/* kludged, we use the default shift of 6 and assume a 3.2.1+ bitstream */
							krad_ogg->tracks[t].last_granulepos =
								((iframe + pframe - 1) / krad_ogg->tracks[t].frame_rate) * 1000.0;
						}
					}
					*timecode = krad_ogg->tracks[t].last_granulepos;
					return packet.bytes;
				}
			}
		}
	
	
		ret = krad_io_read(krad_ogg->krad_io, krad_ogg->input_buffer, 4096);
		
		if (ret > 0) {
			krad_ogg_write (krad_ogg, krad_ogg->input_buffer, ret);
		} else {
			*timecode = 0;
			break;
		}

	}

	return 0;
}


void krad_ogg_process (krad_ogg_t *krad_ogg) {

	ogg_page page;
	int serial;
	int t;
	
	while (ogg_sync_pageout(&krad_ogg->sync_state, &page) == 1) {
		
		serial = ogg_page_serialno(&page);
		
		if (ogg_page_bos(&page)) {
		
			for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
				if (krad_ogg->tracks[t].serial == KRAD_OGG_NO_SERIAL) {
					break;
				}
			}

			krad_ogg->tracks[t].serial = serial;
			ogg_stream_init(&krad_ogg->tracks[t].stream_state, serial);
			ogg_stream_pagein(&krad_ogg->tracks[t].stream_state, &page);
			
			continue;
		}
		
		for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
			if (krad_ogg->tracks[t].serial == serial) {
				break;
			}
		}

		if ( t != KRAD_OGG_MAX_TRACKS) {
			ogg_stream_pagein(&krad_ogg->tracks[t].stream_state, &page);
		}
	}
}


int krad_ogg_write (krad_ogg_t *krad_ogg, unsigned char *buffer, int length) {

    char* input_buffer;
    int ret;
    
	input_buffer = ogg_sync_buffer (&krad_ogg->sync_state, length);
    memcpy (input_buffer, buffer, length);
    ret = ogg_sync_wrote (&krad_ogg->sync_state, length);

	if (ret != 0) {
		printke ("uh oh! ogg_sync_wrote problem");
		return -1;
	}

	krad_ogg_process (krad_ogg);

	return length;
}


void krad_ogg_destroy (krad_ogg_t *krad_ogg) {
	
	int t;
	
	ogg_sync_clear (&krad_ogg->sync_state);

	for (t = 0; t < krad_ogg->track_count; t++) {
    ogg_stream_clear (&krad_ogg->tracks[t].stream_state);	
		while (krad_ogg->tracks[t].header_count) {
			free (krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count - 1]);
			krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count - 1] = NULL;
			krad_ogg->tracks[t].header_count--;
		}
	}
	
	free (krad_ogg->tracks);
	
	free (krad_ogg->input_buffer);
	
	if (krad_ogg->krad_io) {
		krad_io_destroy (krad_ogg->krad_io);
	}
	
	free (krad_ogg);
}

krad_ogg_t *krad_ogg_create() {

	int t;	
	krad_ogg_t *krad_ogg;
	
	krad_ogg = calloc (1, sizeof(krad_ogg_t));

	krad_ogg->tracks = calloc (KRAD_OGG_MAX_TRACKS, sizeof(krad_ogg_track_t));

	for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++ ) {
		krad_ogg->tracks[t].serial = KRAD_OGG_NO_SERIAL;
		krad_ogg->tracks[t].last_serial = KRAD_OGG_NO_SERIAL;
	}

	krad_ogg->input_buffer = calloc(1, 4096);

	ogg_sync_init (&krad_ogg->sync_state);

	return krad_ogg;
}


krad_ogg_t *krad_ogg_open_file (char *filename, krad_io_mode_t mode) {

	krad_ogg_t *krad_ogg;
	
	krad_ogg = krad_ogg_create ();
	
	krad_ogg->krad_io = krad_io_open_file (filename, mode);
	
	return krad_ogg;

}

krad_ogg_t *krad_ogg_open_stream (char *host, int port, char *mount, char *password) {

	krad_ogg_t *krad_ogg;

	krad_ogg = krad_ogg_create ();
	
	krad_ogg->krad_io = krad_io_open_stream (host, port, mount, password);
	
	return krad_ogg;
}

krad_ogg_t *krad_ogg_open_transmission (krad_transmission_t *krad_transmission) {

	krad_ogg_t *krad_ogg;

	krad_ogg = krad_ogg_create ();
	
	krad_ogg->krad_transmission = krad_transmission;
	
	return krad_ogg;
}



void krad_ogg_set_max_packets_per_page (krad_ogg_t *krad_ogg, int max_packets) {

	int t;	

	for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++ ) {
		krad_ogg->tracks[t].max_packets_per_page = max_packets;
	}
	
}

int krad_ogg_output_aux_headers (krad_ogg_t *krad_ogg) {

	int h;
	int t;
	ogg_packet packet;
	ogg_page page;
	
	for (t = 0; t < krad_ogg->track_count; t++) {
		if (krad_ogg->tracks[t].header_count > 1) {
			for (h = 1; h < krad_ogg->tracks[t].header_count; h++) {
			
				printkd ("Krad Ogg Track %d header packet %d sized %d", t, h, krad_ogg->tracks[t].header_len[h]);
			
				packet.packet = krad_ogg->tracks[t].header[h];
				packet.bytes = krad_ogg->tracks[t].header_len[h];
				packet.b_o_s = 0;
				packet.e_o_s = 0;
				packet.granulepos = 0;
				packet.packetno = krad_ogg->tracks[t].packet_num;
				krad_ogg->tracks[t].packet_num++;
				
				ogg_stream_packetin (&krad_ogg->tracks[t].stream_state, &packet);
			}
			
			while (ogg_stream_flush(&krad_ogg->tracks[t].stream_state, &page) != 0) {
	
				if (krad_ogg->krad_io) {
	
					krad_io_write (krad_ogg->krad_io, page.header, page.header_len);
					krad_io_write (krad_ogg->krad_io, page.body, page.body_len);
					krad_io_write_sync (krad_ogg->krad_io);
				}
				
				if (krad_ogg->krad_transmission) {		
				
					krad_transmitter_transmission_add_header (krad_ogg->krad_transmission, page.header, page.header_len);
					krad_transmitter_transmission_add_header (krad_ogg->krad_transmission, page.body, page.body_len);
				
				}
				
				printkd ("Krad Ogg Track %d created aux page %d sized %lu", 
						 t, h,
						 page.header_len + page.body_len);
			}			
		}
	}

	krad_ogg->output_aux_headers = 1;

	return 0;

}


int krad_ogg_add_video_track (krad_ogg_t *krad_ogg, krad_codec_t codec, int fps_numerator,
												   int fps_denominator, int width, int height) {

	// invent vp8 header?
	
	return krad_ogg_add_video_track_with_private_data (krad_ogg, codec, fps_numerator,
													   fps_denominator, width, height,
													   NULL, 0, 0);

}

int krad_ogg_add_video_track_with_private_data (krad_ogg_t *krad_ogg, krad_codec_t codec, int fps_numerator,
												int fps_denominator, int width, int height, unsigned char *header[],
												int header_size[], int header_count) {

	int track;
	
	track = krad_ogg_add_track (krad_ogg, codec, header, header_size, header_count);
	
	krad_ogg->tracks[track].width = width;
	krad_ogg->tracks[track].height = height;
	krad_ogg->tracks[track].fps_numerator = fps_numerator;
	krad_ogg->tracks[track].fps_denominator = fps_denominator;
	
	
	if (codec == THEORA) {
		// FIXME but, probably safeish
		krad_ogg->tracks[track].keyframe_shift = 6;
	}
	
	return track;	

}

int krad_ogg_add_audio_track (krad_ogg_t *krad_ogg, krad_codec_t codec, int sample_rate, int channels, 
							  unsigned char *header[], int header_size[], int header_count) {

	int track;
	
	track = krad_ogg_add_track (krad_ogg, codec, header, header_size, header_count);
	
	krad_ogg->tracks[track].sample_rate = sample_rate;
	krad_ogg->tracks[track].channels = channels;
	
	return track;					  
							  
}


int krad_ogg_add_track (krad_ogg_t *krad_ogg, krad_codec_t codec, 
						unsigned char *header[], int header_size[], int header_count) { 

	int h;
	int track;
	unsigned char *temp_header;
	ogg_packet packet;
	ogg_page page;
	
	track = krad_ogg->track_count;
	krad_ogg->track_count++;

	krad_ogg->tracks[track].writing = 1;

	krad_ogg->tracks[track].codec = codec;
	krad_ogg->tracks[track].serial = rand();
	krad_ogg->tracks[track].packet_num = 0;
	
	krad_ogg->tracks[track].max_packets_per_page = KRAD_OGG_DEFAULT_MAX_PACKETS_PER_PAGE;
	krad_ogg->tracks[track].packets_on_current_page = 0;

	ogg_stream_init (&krad_ogg->tracks[track].stream_state, krad_ogg->tracks[track].serial);

	if (codec == FLAC) {
		// adding ogg flac mapping to flac header
		temp_header = calloc (1, 9 + 42);
		memcpy (temp_header, "\x7F\x46\x4C\x41\x43\x01\x00\x00\x01", 9);
		memcpy (temp_header + 9, header[0], 42);
	}
	
	if (header_count) {
	
		krad_ogg->tracks[track].header_len[0] = header_size[0];		
		krad_ogg->tracks[track].header[0] = malloc (header_size[0]);
		memcpy (krad_ogg->tracks[track].header[0], header[0], header_size[0]);
		krad_ogg->tracks[track].header_count++;
	
		if (codec == FLAC) {
			packet.packet = temp_header;
			packet.bytes = 9 + 42;	
		} else {
			printkd ("ogg header packet %d sized %d", 0, header_size[0]);		
			packet.packet = header[0];
			packet.bytes = header_size[0];
		}	
		packet.b_o_s = 0;
		packet.e_o_s = 0;
		packet.granulepos = 0;
		packet.packetno = krad_ogg->tracks[track].packet_num;
		if (packet.packetno == 0) {
			packet.b_o_s = 1;
		}
		krad_ogg->tracks[track].packet_num++;
		ogg_stream_packetin (&krad_ogg->tracks[track].stream_state, &packet);
	
		while (ogg_stream_flush(&krad_ogg->tracks[track].stream_state, &page) != 0) {
			
			if (krad_ogg->krad_io) {

				krad_io_write (krad_ogg->krad_io, page.header, page.header_len);
				krad_io_write (krad_ogg->krad_io, page.body, page.body_len);
				krad_io_write_sync (krad_ogg->krad_io);
			}
			
			if (krad_ogg->krad_transmission) {		
			
				krad_transmitter_transmission_add_header (krad_ogg->krad_transmission, page.header, page.header_len);
				krad_transmitter_transmission_add_header (krad_ogg->krad_transmission, page.body, page.body_len);
			
			}			

			printkd ("created page sized %lu", page.header_len + page.body_len);			
			
		}
	}

	if (codec == FLAC) {
		free (temp_header);
	}
	
	for (h = 1; h < header_count; h++) {
		krad_ogg->tracks[track].header_len[h] = header_size[h];	
		krad_ogg->tracks[track].header[h] = malloc (krad_ogg->tracks[track].header_len[h]);
		memcpy (krad_ogg->tracks[track].header[h], header[h], krad_ogg->tracks[track].header_len[h]);
		krad_ogg->tracks[track].header_count++;
	}	
	

	return track;								
}

void krad_ogg_add_video (krad_ogg_t *krad_ogg, int track, unsigned char *buffer, int buffer_size, int keyframe) {

	ogg_packet packet;
	ogg_page page;
	
	if (krad_ogg->output_aux_headers == 0) {
		krad_ogg_output_aux_headers (krad_ogg);
	}
	
	packet.packet = buffer;
	packet.bytes = buffer_size;
	packet.b_o_s = 0;
	packet.e_o_s = 0;
	packet.granulepos = 0;
	
	if (keyframe == 1) {
		//printk ("got a keyframe at %d", krad_ogg->tracks[track].frames);
		krad_ogg->tracks[track].frames_since_keyframe = 0;
	} else {
		krad_ogg->tracks[track].frames_since_keyframe++;
	}
	
	if (keyframe == -1) {
    packet.e_o_s = 1;
	}	
	
	krad_ogg->tracks[track].frames += 1;
	
	if (keyframe == 1) {
		packet.granulepos = krad_ogg->tracks[track].frames << krad_ogg->tracks[track].keyframe_shift;
	} else {
	
		packet.granulepos = ((krad_ogg->tracks[track].frames - krad_ogg->tracks[track].frames_since_keyframe) << 
							  krad_ogg->tracks[track].keyframe_shift) + krad_ogg->tracks[track].frames_since_keyframe;
	}
	
	if (keyframe != 0) {
		
		// flush the stream before we put in the packet w/ keyframe or e_o_s
	
		while (ogg_stream_flush (&krad_ogg->tracks[track].stream_state, &page) != 0) {
	
			if (krad_ogg->krad_io) {

				krad_io_write (krad_ogg->krad_io, page.header, page.header_len);
				krad_io_write (krad_ogg->krad_io, page.body, page.body_len);
				krad_io_write_sync (krad_ogg->krad_io);
			}
		
			if (krad_ogg->krad_transmission) {	
		
				krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.header, page.header_len);
				krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.body, page.body_len);
		
			}
	
			//printk ("created page sized %lu", page.header_len + page.body_len);

			krad_ogg->tracks[track].packets_on_current_page = 0;
	
		}
	}	
	
	packet.packetno = krad_ogg->tracks[track].packet_num;
	ogg_stream_packetin (&krad_ogg->tracks[track].stream_state, &packet);

	krad_ogg->tracks[track].packet_num++;	
	krad_ogg->tracks[track].packets_on_current_page++;
	
	if ((krad_ogg->tracks[track].packets_on_current_page < krad_ogg->tracks[track].max_packets_per_page) && 
		(keyframe == 0)) {
	
		while (ogg_stream_pageout (&krad_ogg->tracks[track].stream_state, &page) != 0) {
		
			if (krad_ogg->krad_io) {

				krad_io_write (krad_ogg->krad_io, page.header, page.header_len);
				krad_io_write (krad_ogg->krad_io, page.body, page.body_len);
				krad_io_write_sync (krad_ogg->krad_io);
			}
			
			if (krad_ogg->krad_transmission) {
			
				//if (keyframe == 1) {
				//	krad_transmitter_transmission_sync_point (krad_ogg->krad_transmission);
				//}			
			
				krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.header, page.header_len);
				krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.body, page.body_len);
			
			}

			//printk ("created page sized %lu", page.header_len + page.body_len);

			krad_ogg->tracks[track].packets_on_current_page = 0;
		}
	
	} else {
	
		while (ogg_stream_flush (&krad_ogg->tracks[track].stream_state, &page) != 0) {
		
			if (krad_ogg->krad_io) {

				krad_io_write (krad_ogg->krad_io, page.header, page.header_len);
				krad_io_write (krad_ogg->krad_io, page.body, page.body_len);
				krad_io_write_sync (krad_ogg->krad_io);
			}
			
			if (krad_ogg->krad_transmission) {		
			
				if (keyframe == 1) {
					krad_transmitter_transmission_add_data_sync (krad_ogg->krad_transmission, page.header, page.header_len);
				} else {
					krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.header, page.header_len);
				}
			
				krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.body, page.body_len);
			
			}
		
			//printk ("created page sized %lu", page.header_len + page.body_len);

			krad_ogg->tracks[track].packets_on_current_page = 0;
		
		}
	}

}

void krad_ogg_add_audio (krad_ogg_t *krad_ogg, int track, unsigned char *buffer, int buffer_size, int frames) {

	ogg_packet packet;
	ogg_page page;
	
	if (krad_ogg->output_aux_headers == 0) {
		krad_ogg_output_aux_headers (krad_ogg);
	}
	
	packet.packet = buffer;
	packet.bytes = buffer_size;
	packet.b_o_s = 0;
	packet.e_o_s = 0;
	packet.granulepos = krad_ogg->tracks[track].last_granulepos + frames;
	packet.packetno = krad_ogg->tracks[track].packet_num;
	ogg_stream_packetin (&krad_ogg->tracks[track].stream_state, &packet);

	krad_ogg->tracks[track].packet_num++;
	krad_ogg->tracks[track].last_granulepos += frames;
	
	krad_ogg->tracks[track].packets_on_current_page++;
	
	if (krad_ogg->tracks[track].packets_on_current_page < krad_ogg->tracks[track].max_packets_per_page) {
	
		while (ogg_stream_pageout (&krad_ogg->tracks[track].stream_state, &page) != 0) {
		
			if (krad_ogg->krad_io) {

				krad_io_write (krad_ogg->krad_io, page.header, page.header_len);
				krad_io_write (krad_ogg->krad_io, page.body, page.body_len);
				krad_io_write_sync (krad_ogg->krad_io);
			}
			
			if (krad_ogg->krad_transmission) {		
			
				if (krad_ogg->track_count == 1) {
					krad_transmitter_transmission_add_data_sync (krad_ogg->krad_transmission, page.header, page.header_len);
				} else {
					krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.header, page.header_len);
				}		

				krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.body, page.body_len);
			
			}
		
			//printk ("created page sized %lu", page.header_len + page.body_len);

			krad_ogg->tracks[track].packets_on_current_page = 0;
		}
	
	} else {
	
		while (ogg_stream_flush (&krad_ogg->tracks[track].stream_state, &page) != 0) {
		
			if (krad_ogg->krad_io) {

				krad_io_write (krad_ogg->krad_io, page.header, page.header_len);
				krad_io_write (krad_ogg->krad_io, page.body, page.body_len);
				krad_io_write_sync (krad_ogg->krad_io);
			}
			
			if (krad_ogg->krad_transmission) {		
			
				if (krad_ogg->track_count == 1) {
					krad_transmitter_transmission_add_data_sync (krad_ogg->krad_transmission, page.header, page.header_len);
				} else {
					krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.header, page.header_len);
				}
				krad_transmitter_transmission_add_data (krad_ogg->krad_transmission, page.body, page.body_len);
			
			}
		
			//printk ("created page sized %lu", page.header_len + page.body_len);

			krad_ogg->tracks[track].packets_on_current_page = 0;
		
		}
	}
}

