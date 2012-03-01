#include "krad_ogg.h"

krad_codec_t krad_ogg_get_track_codec (krad_ogg_t *krad_ogg, int tracknumber) {

	if ((krad_ogg->tracks[tracknumber].serial != 0) && (krad_ogg->tracks[tracknumber].ready == 1)) {
		return krad_ogg->tracks[tracknumber].codec;
	}

	return NOCODEC;

}


int krad_ogg_get_track_codec_header_count(krad_ogg_t *krad_ogg, int tracknumber) {


	if ((krad_ogg->tracks[tracknumber].serial != 0) && (krad_ogg->tracks[tracknumber].ready == 1)) {
		return krad_ogg->tracks[tracknumber].header_count;
	}


	return 0;

}

int krad_ogg_get_track_codec_header_data_size(krad_ogg_t *krad_ogg, int tracknumber, int header_number) {

	if ((krad_ogg->tracks[tracknumber].serial != 0) && (krad_ogg->tracks[tracknumber].ready == 1)) {
		return krad_ogg->tracks[tracknumber].header_len[header_number];
	}

	return 0;

}

int krad_ogg_get_track_codec_header_data(krad_ogg_t *krad_ogg, int tracknumber, unsigned char *buffer, int header_number) {

	if ((krad_ogg->tracks[tracknumber].serial != 0) && (krad_ogg->tracks[tracknumber].ready == 1)) {
		memcpy(buffer, krad_ogg->tracks[tracknumber].header[header_number], krad_ogg->tracks[tracknumber].header_len[header_number]);
		return krad_ogg->tracks[tracknumber].header_len[header_number];
	}

	return 0;

}


int krad_ogg_get_track_count(krad_ogg_t *krad_ogg) {

	int t;
	int count;

	count = 0;

	for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
		if ((krad_ogg->tracks[t].serial != 0) && (krad_ogg->tracks[t].ready == 1)) {
			count++;
		}
	}

	return count;

}

int krad_ogg_track_status (krad_ogg_t *krad_ogg, int tracknumber) {

	if (krad_ogg->tracks[tracknumber].serial != 0) {
		return 1;
	}

	return 0;

}

int krad_ogg_track_changed (krad_ogg_t *krad_ogg, int tracknumber) {

	if (krad_ogg->tracks[tracknumber].serial != krad_ogg->tracks[tracknumber].last_serial) {
		krad_ogg->tracks[tracknumber].last_serial = krad_ogg->tracks[tracknumber].serial;
		return 1;
	}

	return 0;

}


int krad_ogg_read_packet (krad_ogg_t *krad_ogg, int *tracknumber, unsigned char *buffer) {

	int t;
	int ret;
	ogg_packet packet;
	
	for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
		if ((krad_ogg->tracks[t].serial != 0) && (krad_ogg->tracks[t].ready == 0)) {
			while (krad_ogg->tracks[t].ready == 0) {
				ret = ogg_stream_packetout(&krad_ogg->tracks[t].stream_state, &packet);
				if (ret == 1) {
					
					krad_ogg->tracks[t].header_len[krad_ogg->tracks[t].header_count] = packet.bytes;
					
					krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count] = malloc(packet.bytes);
					memcpy(krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count], packet.packet, packet.bytes);
					
					krad_ogg->tracks[t].header_count++;
					if (krad_ogg->tracks[t].header_count == 3) {					
						krad_ogg->tracks[t].ready = 1;
					}
				} else {
					break;
				}
			}
		}
	}
	
	for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
		if ((krad_ogg->tracks[t].serial != 0) && (krad_ogg->tracks[t].ready == 1)) {
			ret = ogg_stream_packetout(&krad_ogg->tracks[t].stream_state, &packet);
			if (ret == 1) {
				memcpy(buffer, packet.packet, packet.bytes);
				*tracknumber = t;
				
				if (packet.e_o_s) {
					krad_ogg->tracks[t].serial = 0;
					krad_ogg->tracks[t].ready = 0;
					krad_ogg->tracks[t].codec = NOCODEC;
					while (krad_ogg->tracks[t].header_count) {
						free (krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count - 1]);
						krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count - 1] = NULL;
						krad_ogg->tracks[t].header_count--;
					}
				}
				
				return packet.bytes;
			}
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
				if (krad_ogg->tracks[t].serial == 0) {
					break;
				}
			}

			krad_ogg->tracks[t].serial = serial;
			ogg_stream_init(&krad_ogg->tracks[t].stream_state, serial);
			ogg_stream_pagein(&krad_ogg->tracks[t].stream_state, &page);
			
			continue;
		}
		
		
		//if (ogg_page_eos(&page)) {
		
			for (t = 0; t < KRAD_OGG_MAX_TRACKS; t++) {
				if (krad_ogg->tracks[t].serial == serial) {
					break;
				}
			}

			ogg_stream_pagein(&krad_ogg->tracks[t].stream_state, &page);
			
		//	continue;
		//}
	}


}


int krad_ogg_write (krad_ogg_t *krad_ogg, unsigned char *buffer, int length) {

    char* input_buffer;
    int ret;
    
	input_buffer = ogg_sync_buffer(&krad_ogg->sync_state, length);
    memcpy (input_buffer, buffer, length);
    ret = ogg_sync_wrote(&krad_ogg->sync_state, length);

	if (ret != 0) {
		printf("uh oh! ogg_sync_wrote problem\n");
		return -1;
	}


	if (!krad_ogg->ready) {
		krad_ogg_process (krad_ogg);
	}

	return length;

}



void krad_ogg_destroy(krad_ogg_t *krad_ogg) {
	
	int t;
	
	ogg_sync_clear(&krad_ogg->sync_state);

	for (t = 0; t < krad_ogg->track_count; t++) {
		while (krad_ogg->tracks[t].header_count) {
			free (krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count - 1]);
			krad_ogg->tracks[t].header[krad_ogg->tracks[t].header_count - 1] = NULL;
			krad_ogg->tracks[t].header_count--;
		}
	}
	
	free(krad_ogg->tracks);
	
	free (krad_ogg);

}

krad_ogg_t *krad_ogg_create() {

	krad_ogg_t *krad_ogg = calloc(1, sizeof(krad_ogg_t));

	krad_ogg->tracks = calloc(KRAD_OGG_MAX_TRACKS, sizeof(krad_ogg_track_t));

	ogg_sync_init(&krad_ogg->sync_state);

	return krad_ogg;

}
