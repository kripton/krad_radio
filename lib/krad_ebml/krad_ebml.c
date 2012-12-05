#include "krad_ebml.h"

int krad_ebml_streamio_write(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length);
int krad_ebml_transmissionio_write(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length);

char *krad_ebml_version() {
	return KRADEBML_VERSION;
}

char *krad_ebml_codec_to_ebml_codec_id (krad_codec_t codec) {

	switch (codec) {
		case VORBIS:
			return "A_VORBIS";
		case FLAC:
			return "A_FLAC";
		case OPUS:
			return "A_OPUS";
		case VP8:
			return "V_VP8";
		case KVHS:
			return "V_KVHS";			
		case THEORA:
			return "V_THEORA";
		case MJPEG:
			return "V_MJPEG";
		case H264:
			return "V_MPEG4/ISO/AVC";			
		default:
			return "No Codec";
	}
}

inline void rmemcpy(void *dst, void *src, int len) {

	unsigned char *a_dst;
	unsigned char *a_src;
	int count;
	
	count = 0;
	len -= 1;
	a_dst = dst;
	a_src = src;

	while (len > -1) {
		a_dst[len--] = a_src[count++];
	}
}

void krad_ebml_base64_encode(char *dest, char *src) {

	kradx_base64_t *base64 = calloc(1, sizeof(kradx_base64_t));

	static char base64table[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
									'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 
									'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', 
									'5', '6', '7', '8', '9', '+', '/' };

	base64->len = strlen(src);
	base64->out = (char *) malloc(base64->len * 4 / 3 + 4);
	base64->result = base64->out;

	while(base64->len > 0) {
		base64->chunk = (base64->len > 3) ? 3 : base64->len;
		*base64->out++ = base64table[(*src & 0xFC) >> 2];
		*base64->out++ = base64table[((*src & 0x03) << 4) | ((*(src + 1) & 0xF0) >> 4)];
		switch(base64->chunk) {
			case 3:
				*base64->out++ = base64table[((*(src + 1) & 0x0F) << 2) | ((*(src + 2) & 0xC0) >> 6)];
				*base64->out++ = base64table[(*(src + 2)) & 0x3F];
				break;

			case 2:
				*base64->out++ = base64table[((*(src + 1) & 0x0F) << 2)];
				*base64->out++ = '=';
				break;

			case 1:
				*base64->out++ = '=';
				*base64->out++ = '=';
				break;
		}

		src += base64->chunk;
		base64->len -= base64->chunk;
	}

	*base64->out = 0;

	strcpy(dest, base64->result);
	free(base64->result);
	free(base64);

}

/*		write			*/

void krad_ebml_write_reversed (krad_ebml_t *krad_ebml, void *buffer, uint32_t len) {

	unsigned char buffer_rev[8];
	
	rmemcpy(buffer_rev, buffer, len);
	krad_ebml_write(krad_ebml, buffer_rev, len);
}

void krad_ebml_write_data_size_update (krad_ebml_t *krad_ebml, uint64_t data_size) {

    data_size |= (0x000000000000080LLU << ((EBML_DATA_SIZE_UNKNOWN_LENGTH - 1) * 7));

    krad_ebml_write_reversed(krad_ebml, (void *)&data_size, EBML_DATA_SIZE_UNKNOWN_LENGTH);

}

void krad_ebml_write_data_size (krad_ebml_t *krad_ebml, uint64_t data_size) {

    //Max 0x0100000000000000LLU

	uint32_t data_size_length;
    uint64_t data_size_length_mask;

    data_size_length_mask = 0x00000000000000FFLLU;
	data_size_length = 1;

	while (data_size_length < 8) {
	
		if (data_size < data_size_length_mask) {
			break;
		}

		data_size_length_mask <<= 7;
	    data_size_length++;
	}

    data_size |= (0x000000000000080LLU << ((data_size_length - 1) * 7));

    krad_ebml_write_reversed(krad_ebml, (void *)&data_size, data_size_length);
}

void krad_ebml_write_data (krad_ebml_t *krad_ebml, uint64_t element, void *data, uint64_t length) {

	krad_ebml_write_element (krad_ebml, element);
	krad_ebml_write_data_size (krad_ebml, length);
    krad_ebml_write (krad_ebml, data, length);

}

void krad_ebml_write_string (krad_ebml_t *krad_ebml, uint64_t element, char *string) {

	uint64_t size;
	
	size = strlen(string);

	krad_ebml_write_element (krad_ebml, element);
	krad_ebml_write_data_size (krad_ebml, size);
    krad_ebml_write (krad_ebml, string, strlen(string));
}


void krad_ebml_write_int64 (krad_ebml_t *krad_ebml, uint64_t element, int64_t number) {

	krad_ebml_write_element (krad_ebml, element);
	krad_ebml_write_data_size (krad_ebml, 8);
    krad_ebml_write_reversed (krad_ebml, &number, 8);
}

void krad_ebml_write_int32 (krad_ebml_t *krad_ebml, uint64_t element, int32_t number) {

	krad_ebml_write_element (krad_ebml, element);
	krad_ebml_write_data_size (krad_ebml, 4);
    krad_ebml_write_reversed (krad_ebml, &number, 4);
}

void krad_ebml_write_int16 (krad_ebml_t *krad_ebml, uint64_t element, int16_t number) {

	krad_ebml_write_element (krad_ebml, element);
	krad_ebml_write_data_size (krad_ebml, 2);
    krad_ebml_write_reversed (krad_ebml, &number, 2);
}

void krad_ebml_write_int8 (krad_ebml_t *krad_ebml, uint64_t element, int8_t number) {

	krad_ebml_write_element (krad_ebml, element);
	krad_ebml_write_data_size (krad_ebml, 1);
    krad_ebml_write_reversed (krad_ebml, &number, 1);
}

void krad_ebml_write_float (krad_ebml_t *krad_ebml, uint64_t element, float number) {

	krad_ebml_write_element (krad_ebml, element);
	krad_ebml_write_data_size (krad_ebml, 4);
    krad_ebml_write_reversed (krad_ebml, &number, 4);
}

void krad_ebml_write_double (krad_ebml_t *krad_ebml, uint64_t element, double number) {

	krad_ebml_write_element (krad_ebml, element);
	krad_ebml_write_data_size (krad_ebml, 8);
    krad_ebml_write_reversed (krad_ebml, &number, 8);
}



void krad_ebml_write_element (krad_ebml_t *krad_ebml, uint32_t element) {

    uint32_t element_len;

	element_len = 1;

	if (element >= 0x00000100) {
		element_len = 2;
	}

	if (element >= 0x00010000) {
		element_len = 3;
	}

	if (element >= 0x01000000) {
		element_len = 4;
	}

	krad_ebml_write_reversed(krad_ebml, (void *)&element, element_len);
}

void krad_ebml_start_element (krad_ebml_t *krad_ebml, uint32_t element, uint64_t *position) {

	krad_ebml_write_element (krad_ebml, element);
	*position = krad_ebml_tell(krad_ebml);
	krad_ebml_write_data_size (krad_ebml, EBML_DATA_SIZE_UNKNOWN);
	
}

void krad_ebml_finish_element (krad_ebml_t *krad_ebml, uint64_t element_position) {

	uint64_t current_position;
	uint64_t element_data_size;

	current_position = krad_ebml_tell(krad_ebml);
	element_data_size = current_position - element_position - EBML_DATA_SIZE_UNKNOWN_LENGTH;
	
	if ((current_position - element_position) <= krad_ebml->io_adapter.write_buffer_pos) {

		krad_ebml_seek(krad_ebml, element_position, SEEK_SET);
		krad_ebml_write_data_size_update (krad_ebml, element_data_size);
		krad_ebml_seek(krad_ebml, current_position, SEEK_SET);
	
	}
}

void krad_ebml_header_advanced (krad_ebml_t *krad_ebml, char *doctype, int doctype_version, int doctype_read_version) {
    
    
	uint64_t ebml_header;

	krad_ebml_start_element (krad_ebml, EBML_ID_HEADER, &ebml_header);
	
	krad_ebml_write_int8 (krad_ebml, EBML_ID_EBMLVERSION, 1);	
	krad_ebml_write_int8 (krad_ebml, EBML_ID_EBMLREADVERSION, 1);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_EBMLMAXIDLENGTH, 4);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_EBMLMAXSIZELENGTH, 8);
	krad_ebml_write_string (krad_ebml, EBML_ID_DOCTYPE, doctype);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_DOCTYPEVERSION, doctype_version);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_DOCTYPEREADVERSION, doctype_read_version);

	krad_ebml_finish_element (krad_ebml, ebml_header);


}

void krad_ebml_header (krad_ebml_t *krad_ebml, char *doctype, char *appversion) {
    
    
	uint64_t ebml_header;

	krad_ebml_start_element (krad_ebml, EBML_ID_HEADER, &ebml_header);
	
	krad_ebml_write_int8 (krad_ebml, EBML_ID_EBMLVERSION, 1);	
	krad_ebml_write_int8 (krad_ebml, EBML_ID_EBMLREADVERSION, 1);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_EBMLMAXIDLENGTH, 4);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_EBMLMAXSIZELENGTH, 8);
	krad_ebml_write_string (krad_ebml, EBML_ID_DOCTYPE, doctype);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_DOCTYPEVERSION, 2);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_DOCTYPEREADVERSION, 2);

	krad_ebml_finish_element (krad_ebml, ebml_header);

	krad_ebml_start_segment(krad_ebml, appversion);

}

void krad_ebml_write_cues (krad_ebml_t *krad_ebml) {

	int c;
	
	uint64_t cues;
	uint64_t cuepoint;	
	uint64_t cue_track_positions;	
	
	krad_ebml_start_element (krad_ebml, EBML_ID_CUES, &cues);	
	
	for (c = 0; c < krad_ebml->current_cluster; c++) {
	
		krad_ebml_start_element (krad_ebml, EBML_ID_CUEPOINT, &cuepoint);	
		krad_ebml_write_int64 (krad_ebml, EBML_ID_CUETIME, krad_ebml->clusters[c].timecode);	
		krad_ebml_start_element (krad_ebml, EBML_ID_CUETRACKPOSITIONS, &cue_track_positions);
		krad_ebml_write_int8 (krad_ebml, EBML_ID_CUETRACK, 1);
		krad_ebml_write_int64 (krad_ebml, EBML_ID_CUECLUSTERPOSITION, krad_ebml->clusters[c].position);
		krad_ebml_finish_element (krad_ebml, cue_track_positions);
		krad_ebml_finish_element (krad_ebml, cuepoint);	
	
	}

	krad_ebml_finish_element (krad_ebml, cues);

	krad_ebml_write_sync (krad_ebml);

}

void krad_ebml_write_seekhead (krad_ebml_t *krad_ebml) {

	uint64_t seekhead;
	uint64_t seekid;	
	char *voiddata;
	uint64_t seekitem;

	krad_ebml_fileio_seek (&krad_ebml->io_adapter, krad_ebml->void_space, SEEK_SET);


	krad_ebml->segment_size = 0;

	krad_ebml_start_element (krad_ebml, EBML_ID_SEEKHEAD, &seekhead);

	krad_ebml_start_element (krad_ebml, EBML_ID_SEEK, &seekitem);
	
	krad_ebml_start_element (krad_ebml, EBML_ID_SEEK_ID, &seekid);
	krad_ebml_write_element (krad_ebml, EBML_ID_SEGMENT_INFO);
	krad_ebml_finish_element (krad_ebml, seekid);
	krad_ebml_write_int64 (krad_ebml, EBML_ID_SEEK_POSITION, krad_ebml->segment_info_position);
	krad_ebml_finish_element (krad_ebml, seekitem);

	krad_ebml_start_element (krad_ebml, EBML_ID_SEEK, &seekitem);
	krad_ebml_start_element (krad_ebml, EBML_ID_SEEK_ID, &seekid);
	krad_ebml_write_element (krad_ebml, EBML_ID_SEGMENT_TRACKS);
	krad_ebml_finish_element (krad_ebml, seekid);
	krad_ebml_write_int64 (krad_ebml, EBML_ID_SEEK_POSITION, krad_ebml->tracks_info_position);
	krad_ebml_finish_element (krad_ebml, seekitem);

	krad_ebml_start_element (krad_ebml, EBML_ID_SEEK, &seekitem);
	krad_ebml_start_element (krad_ebml, EBML_ID_SEEK_ID, &seekid);
	krad_ebml_write_element (krad_ebml, EBML_ID_CLUSTER);
	krad_ebml_finish_element (krad_ebml, seekid);
	krad_ebml_write_int64 (krad_ebml, EBML_ID_SEEK_POSITION, krad_ebml->cluster_start_position);
	krad_ebml_finish_element (krad_ebml, seekitem);

	krad_ebml_start_element (krad_ebml, EBML_ID_SEEK, &seekitem);
	krad_ebml_start_element (krad_ebml, EBML_ID_SEEK_ID, &seekid);
	krad_ebml_write_element (krad_ebml, EBML_ID_CUES);
	krad_ebml_finish_element (krad_ebml, seekid);
	krad_ebml_write_int64 (krad_ebml, EBML_ID_SEEK_POSITION, krad_ebml->cues_position);
	krad_ebml_finish_element (krad_ebml, seekitem);


	krad_ebml_finish_element (krad_ebml, seekhead);


	krad_ebml_write_sync (krad_ebml);


	voiddata = calloc (1, VOID_START_SIZE - krad_ebml->segment_size);
	krad_ebml_write_data (krad_ebml, EBML_ID_VOID, voiddata, VOID_START_SIZE - krad_ebml->segment_size);
	krad_ebml_write_sync (krad_ebml);
	free (voiddata);

}

void krad_ebml_finish_file_segment (krad_ebml_t *krad_ebml) {

	if (krad_ebml->segment != 0) {
		krad_ebml_write_sync (krad_ebml);

		krad_ebml->cues_position = krad_ebml->segment_size;
    //krad_ebml->cues_position = krad_ebml_fileio_tell (&krad_ebml->io_adapter);		
		
		
		krad_ebml_write_cues (krad_ebml);

		krad_ebml_write_sync (krad_ebml);

		krad_ebml_fileio_seek (&krad_ebml->io_adapter, krad_ebml->segment, SEEK_SET);
		//printk ("data size is %zu pos is %zu", krad_ebml->segment_size, krad_ebml->segment);
		//krad_ebml_write_data_size (krad_ebml, krad_ebml->segment_size);
		krad_ebml_write_data_size_update (krad_ebml, krad_ebml->segment_size);
		//krad_ebml_write_data_size_update (krad_ebml, EBML_DATA_SIZE_UNKNOWN);
		krad_ebml_write_sync (krad_ebml);

		if (1) {
			krad_ebml_fileio_seek (&krad_ebml->io_adapter, krad_ebml->segment + 20 + VOID_START_SIZE + VOID_START_SIZE_SIZE, SEEK_SET);
			krad_ebml_write_float (krad_ebml, EBML_ID_DURATION, krad_ebml->segment_duration);
			krad_ebml_write_sync (krad_ebml);
		}
		
		krad_ebml_write_seekhead (krad_ebml);

		krad_ebml->segment_size = 0;
		krad_ebml->segment = 0;
	}

}

void krad_ebml_start_file_segment (krad_ebml_t *krad_ebml) {

	char *voiddata;
	
	krad_ebml_write_element (krad_ebml, EBML_ID_SEGMENT);
	krad_ebml_write_sync (krad_ebml);
	krad_ebml->segment = krad_ebml_fileio_tell (&krad_ebml->io_adapter);
	krad_ebml_write_data_size (krad_ebml, EBML_DATA_SIZE_UNKNOWN);
	krad_ebml_write_sync (krad_ebml);	
	krad_ebml->segment_size = 0;

	voiddata = calloc (1, VOID_START_SIZE);
	krad_ebml->void_space = krad_ebml_fileio_tell (&krad_ebml->io_adapter);	
	krad_ebml_write_data (krad_ebml, EBML_ID_VOID, voiddata, VOID_START_SIZE);
	free (voiddata);

}

void krad_ebml_start_segment(krad_ebml_t *krad_ebml, char *appversion) {

	uint64_t segment_info;
	char version_string[32];

	strcpy(version_string, "Krad EBML ");
	strcat(version_string, KRADEBML_VERSION);
 
	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) &&
		(krad_ebml->io_adapter.write == krad_ebml_fileio_write)) {
		krad_ebml_start_file_segment (krad_ebml);
		krad_ebml_write_sync (krad_ebml);
		//krad_ebml->segment_info_position = krad_ebml_fileio_tell (&krad_ebml->io_adapter);
		krad_ebml->segment_info_position = krad_ebml->segment_size;
	} else {
		krad_ebml_start_element (krad_ebml, EBML_ID_SEGMENT, &krad_ebml->segment);
	}
	krad_ebml_start_element (krad_ebml, EBML_ID_SEGMENT_INFO, &segment_info);
	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) &&
		(krad_ebml->io_adapter.write == krad_ebml_fileio_write)) {
		krad_ebml_write_data (krad_ebml, EBML_ID_VOID, "000000000", 5);
	}
	krad_ebml_write_string (krad_ebml, EBML_ID_SEGMENT_TITLE, "Krad Radio Broadcast");
	krad_ebml_write_int32 (krad_ebml, EBML_ID_TIMECODESCALE, 1000000);
	krad_ebml_write_string (krad_ebml, EBML_ID_MUXINGAPP, version_string);
	krad_ebml_write_string (krad_ebml, EBML_ID_WRITINGAPP, appversion);
	krad_ebml_finish_element (krad_ebml, segment_info);


	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) &&
		(krad_ebml->io_adapter.write == krad_ebml_fileio_write)) {
		krad_ebml_write_sync (krad_ebml);
		//krad_ebml->tracks_info_position = krad_ebml_fileio_tell (&krad_ebml->io_adapter);
    krad_ebml->tracks_info_position = krad_ebml->segment_size;
	}

	krad_ebml_start_element (krad_ebml, EBML_ID_SEGMENT_TRACKS, &krad_ebml->tracks_info);

}

void krad_ebml_write_tag (krad_ebml_t *krad_ebml, char *name, char *value) {

	uint64_t tags;
	uint64_t tag;
	uint64_t tag_targets;
	uint64_t simple_tag;
	
	krad_ebml_start_element (krad_ebml, EBML_ID_TAGS, &tags);
	krad_ebml_start_element (krad_ebml, EBML_ID_TAG, &tag);

	krad_ebml_start_element (krad_ebml, EBML_ID_TAG_TARGETS, &tag_targets);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_TAG_TARGETTYPEVALUE, 50);
	krad_ebml_finish_element (krad_ebml, tag_targets);

	krad_ebml_start_element (krad_ebml, EBML_ID_TAG_SIMPLE, &simple_tag);	
	krad_ebml_write_string (krad_ebml, EBML_ID_TAG_NAME, name);
	krad_ebml_write_string (krad_ebml, EBML_ID_TAG_STRING, value);
	krad_ebml_finish_element (krad_ebml, simple_tag);

	krad_ebml_finish_element (krad_ebml, tag);
	krad_ebml_finish_element (krad_ebml, tags);

}

int krad_ebml_new_tracknumber (krad_ebml_t *krad_ebml) {

	int tracknumber;
	
	tracknumber = krad_ebml->current_track;

	krad_ebml->track_count++;
	krad_ebml->current_track++;
	
	return tracknumber;

}

void krad_ebml_bump_tracknumber (krad_ebml_t *krad_ebml) {

	krad_ebml_new_tracknumber (krad_ebml);
	krad_ebml->track_count--;

}

int krad_ebml_generate_track_uid (int track_number) {

  uint64_t t;
  uint64_t r;
  uint64_t rval;
  
  t = time(NULL) * track_number;
  r = rand();
  r = r << 32;
  r +=  rand();
  rval = t ^ r;

  return rval;

}

int krad_ebml_add_video_track_with_private_data (krad_ebml_t *krad_ebml, krad_codec_t codec, int fps_numerator, int fps_denominator, int width, int height, unsigned char *private_data, int private_data_size) {

	int track_number;
	uint64_t track_info;
	uint64_t video_info;

	krad_ebml->total_video_frames = 0;
	krad_ebml->fps_numerator = fps_numerator;
	krad_ebml->fps_denominator = fps_denominator;
	
	track_number = krad_ebml_new_tracknumber(krad_ebml);

	krad_ebml_start_element (krad_ebml, EBML_ID_TRACK, &track_info);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_TRACKNUMBER, track_number);
	krad_ebml_write_int64 (krad_ebml, EBML_ID_TRACK_UID, krad_ebml_generate_track_uid (track_number));
	krad_ebml_write_string (krad_ebml, EBML_ID_CODECID, krad_ebml_codec_to_ebml_codec_id (codec));
	krad_ebml_write_int8 (krad_ebml, EBML_ID_TRACKTYPE, 1);

	if (private_data_size > 0) {
		krad_ebml_write_data (krad_ebml, EBML_ID_CODECDATA, private_data, private_data_size);
	}

	krad_ebml_start_element (krad_ebml, EBML_ID_VIDEOSETTINGS, &video_info);
	krad_ebml_write_int16 (krad_ebml, EBML_ID_VIDEOWIDTH, width);
	krad_ebml_write_int16 (krad_ebml, EBML_ID_VIDEOHEIGHT, height);
	// StereoMode ?
	krad_ebml_finish_element (krad_ebml, video_info);
	krad_ebml_finish_element (krad_ebml, track_info);	

	return track_number;

}

int krad_ebml_add_video_track(krad_ebml_t *krad_ebml, krad_codec_t codec, int fps_numerator, int fps_denominator, int width, int height) {
	return krad_ebml_add_video_track_with_private_data(krad_ebml, codec, fps_numerator, fps_denominator, width, height, NULL, 0);
}

int krad_ebml_add_subtitle_track(krad_ebml_t *krad_ebml, char *codec_id) {

	int track_number;
	uint64_t track_info;
	
	track_number = krad_ebml_new_tracknumber(krad_ebml);

	krad_ebml_start_element (krad_ebml, EBML_ID_TRACK, &track_info);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_TRACKNUMBER, track_number);
	krad_ebml_write_int64 (krad_ebml, EBML_ID_TRACK_UID, krad_ebml_generate_track_uid (track_number));
	krad_ebml_write_string (krad_ebml, EBML_ID_CODECID, codec_id);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_TRACKTYPE, 0x11);
	krad_ebml_finish_element (krad_ebml, track_info);


	return track_number;

}

int krad_ebml_add_audio_track(krad_ebml_t *krad_ebml, krad_codec_t codec, int sample_rate, int channels, unsigned char *private_data, int private_data_size) {

	int track_number;
	uint64_t track_info;
	uint64_t audio_info;

	krad_ebml->audio_sample_rate = sample_rate;
	
	//	kradebml->audio_channels = channels;

	track_number = krad_ebml_new_tracknumber(krad_ebml);

  krad_ebml->tracks[track_number].codec = codec;

	krad_ebml_start_element (krad_ebml, EBML_ID_TRACK, &track_info);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_TRACKNUMBER, track_number);
	krad_ebml_write_int64 (krad_ebml, EBML_ID_TRACK_UID, krad_ebml_generate_track_uid (track_number));
	krad_ebml_write_string (krad_ebml, EBML_ID_CODECID, krad_ebml_codec_to_ebml_codec_id (codec));
	krad_ebml_write_int8 (krad_ebml, EBML_ID_TRACKTYPE, 2);

	krad_ebml_start_element (krad_ebml, EBML_ID_AUDIOSETTINGS, &audio_info);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_AUDIOCHANNELS, channels);
	
	krad_ebml_write_float (krad_ebml, EBML_ID_AUDIOSAMPLERATE, sample_rate);
	krad_ebml_write_int8 (krad_ebml, EBML_ID_AUDIOBITDEPTH, 16);

	krad_ebml_finish_element (krad_ebml, audio_info);
	
	if (private_data_size > 0) {
		krad_ebml_write_data (krad_ebml, EBML_ID_CODECDATA, private_data, private_data_size);
	}

	krad_ebml_finish_element (krad_ebml, track_info);

	return track_number;

}

void krad_ebml_add_video(krad_ebml_t *krad_ebml, int track_num, unsigned char *buffer, int buffer_len, int keyframe) {

    uint32_t block_length;
    unsigned char track_number;
    unsigned short block_timecode;
    unsigned char flags;
	int64_t timecode;

	flags = 0;
	block_timecode = 0;
    block_length = buffer_len + 4;
    block_length |= 0x10000000;

    track_number = track_num;
    track_number |= 0x80;

    if (keyframe) {
		flags |= 0x80;
	}
	
	timecode = round (1000000000 * krad_ebml->total_video_frames / krad_ebml->fps_numerator * krad_ebml->fps_denominator / 1000000);

	krad_ebml->total_video_frames++;
		
	if (keyframe) {
		krad_ebml_cluster (krad_ebml, timecode);
	}
	
	/* Must be after clustering esp. in case of keyframe */ 
	block_timecode = timecode - krad_ebml->cluster_timecode;	

	if (timecode > krad_ebml->segment_timecode) {
		krad_ebml->segment_timecode = timecode;
		krad_ebml->segment_duration = krad_ebml->segment_timecode;
	}

    krad_ebml_write_element (krad_ebml, EBML_ID_SIMPLEBLOCK);
    
	krad_ebml_write_reversed (krad_ebml, &block_length, 4);

	krad_ebml_write (krad_ebml, &track_number, 1);
	krad_ebml_write_reversed (krad_ebml, &block_timecode, 2);
	
	krad_ebml_write (krad_ebml, &flags, 1);
	krad_ebml_write (krad_ebml, buffer, buffer_len);
	
	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) &&
		((krad_ebml->io_adapter.write == krad_ebml_streamio_write) ||
		(krad_ebml->io_adapter.write == krad_ebml_transmissionio_write))) {		
		krad_ebml_write_sync (krad_ebml);
	}
	
}

void krad_ebml_add_audio(krad_ebml_t *krad_ebml, int track_num, unsigned char *buffer, int buffer_len, int frames) {

	int64_t timecode;
	unsigned long  block_length;
    unsigned char  track_number;
	short block_timecode;
	unsigned char  flags;
	
	block_timecode = 0;
	flags = 0;
	flags |= 0x80;

    block_length = buffer_len + 4;
    block_length |= 0x10000000;

    track_number = track_num;
    track_number |= 0x80;

  if (krad_ebml->tracks[track_num].codec != VORBIS) {
  	timecode = round ((1000000000 * krad_ebml->total_audio_frames / krad_ebml->audio_sample_rate / 1000000));
	}

	krad_ebml->total_audio_frames += frames;
	krad_ebml->audio_frames_since_cluster += frames;

  if (krad_ebml->tracks[track_num].codec == VORBIS) {
  	timecode = round ((1000000000 * krad_ebml->total_audio_frames / krad_ebml->audio_sample_rate / 1000000));
	}

	if ((timecode == 0) && (krad_ebml->track_count == 1)) {
			krad_ebml_cluster(krad_ebml, timecode);
	}

	if ((krad_ebml->audio_frames_since_cluster >= krad_ebml->audio_sample_rate) && (krad_ebml->track_count == 1)) {
			krad_ebml_cluster(krad_ebml, timecode);
			krad_ebml->audio_frames_since_cluster = 0;
	}

	block_timecode = timecode - krad_ebml->cluster_timecode;

	if (timecode > krad_ebml->segment_timecode) {
		krad_ebml->segment_timecode = timecode;
		krad_ebml->segment_duration = krad_ebml->segment_timecode;
	}

    krad_ebml_write_element (krad_ebml, EBML_ID_SIMPLEBLOCK);

	krad_ebml_write_reversed(krad_ebml, &block_length, 4);

	krad_ebml_write(krad_ebml, &track_number, 1);
	krad_ebml_write_reversed(krad_ebml, &block_timecode, 2);
	
	krad_ebml_write(krad_ebml, &flags, 1);
	krad_ebml_write(krad_ebml, buffer, buffer_len);
	
	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) &&
		((krad_ebml->io_adapter.write == krad_ebml_streamio_write) ||
		(krad_ebml->io_adapter.write == krad_ebml_transmissionio_write))) {		
		krad_ebml_write_sync (krad_ebml);
	}
	
}

void krad_ebml_cluster(krad_ebml_t *krad_ebml, int64_t timecode) {

	if (krad_ebml->tracks_info != 0) {
		krad_ebml_finish_element (krad_ebml, krad_ebml->tracks_info);
		krad_ebml->tracks_info = 0;
		krad_ebml_write_sync (krad_ebml);
		

		if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) &&
			(krad_ebml->io_adapter.write == krad_ebml_fileio_write)) {		
			krad_ebml_write_sync (krad_ebml);
			//krad_ebml->cluster_start_position = krad_ebml_fileio_tell (&krad_ebml->io_adapter);
			krad_ebml->cluster_start_position = krad_ebml->segment_size;
		}
	}

	if (krad_ebml->cluster != 0) {
		krad_ebml_finish_element (krad_ebml, krad_ebml->cluster);
		krad_ebml_write_sync (krad_ebml);
		if (krad_ebml->io_adapter.write == krad_ebml_transmissionio_write) {
			krad_transmitter_transmission_add_data_sync (krad_ebml->krad_transmission, (unsigned char *)"", 0);
		}
	}

	krad_ebml->cluster_timecode = timecode;


	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) &&
		(krad_ebml->io_adapter.write == krad_ebml_fileio_write) &&
		(krad_ebml->record_cluster_info == 1)) {
		
		krad_ebml_write_sync (krad_ebml);
		krad_ebml->clusters[krad_ebml->current_cluster].position = krad_ebml->segment_size;
		krad_ebml->clusters[krad_ebml->current_cluster].timecode = krad_ebml->cluster_timecode;
		krad_ebml->clusters[krad_ebml->current_cluster].size = 0;
		
		krad_ebml->current_cluster++;
		if (krad_ebml->current_cluster == krad_ebml->cluster_recording_space) {
			krad_ebml->cluster_recording_space = krad_ebml->cluster_recording_space * 2;
			krad_ebml->clusters = realloc (krad_ebml->clusters, krad_ebml->cluster_recording_space);
		}
		
	}


	krad_ebml_start_element (krad_ebml, EBML_ID_CLUSTER, &krad_ebml->cluster);
	krad_ebml_write_int64 (krad_ebml, EBML_ID_CLUSTER_TIMECODE, krad_ebml->cluster_timecode);
}

/*		read 			*/


char *ebml_identify(uint32_t element_id) {

	switch (element_id) {
	
		case EBML_ID_HEADER:
			return "EBML Header";
		case EBML_ID_CLUSTER:
			return "Cluster";
		case EBML_ID_SEGMENT:
			return "Segment Header";
		case EBML_ID_SEGMENT_INFO:
			return "Segment Info";
		case EBML_ID_SEGMENT_TRACKS:
			return "Segment Tracks";
		case EBML_ID_SIMPLEBLOCK:
			return "Simple Block";
		case EBML_ID_CLUSTER_TIMECODE:
			return "Cluster Timecode";
		case EBML_ID_MUXINGAPP:
			return "Muxing App";
		case EBML_ID_WRITINGAPP:
			return "Writing App";
		case EBML_ID_CODECID:
			return "Codec";			
		case EBML_ID_VIDEOWIDTH:
			return "Width";
		case EBML_ID_VIDEOHEIGHT:
			return "Height";
		case EBML_ID_AUDIOCHANNELS:
			return "Channels";
		case EBML_ID_DEFAULTDURATION:
			return "Default Duration";
		case EBML_ID_TRACKNUMBER:
			return "Track Number";
		case EBML_ID_AUDIOBITDEPTH:
			return "Bit Depth";		
		case EBML_ID_BLOCKGROUP:
			return "Block Group";
		case EBML_ID_3D:
			return "3D Mode";			
		default:
			//printf("%x", element_id);
			return "";										
	}
}


inline uint32_t ebml_length(unsigned char byte) {

	if (byte & EBML_LENGTH_1) {
		return 1;
	}

	if (byte & EBML_LENGTH_2) {
		return 2;
	}
	
	if (byte & EBML_LENGTH_3) {
		return 3;
	}
	
	if (byte & EBML_LENGTH_4) {
		return 4;
	}
	
	if (byte & EBML_LENGTH_5) {
		return 5;
	}
	
	if (byte & EBML_LENGTH_6) {
		return 6;
	}
	
	if (byte & EBML_LENGTH_7) {
		return 7;
	}
	
	if (byte & EBML_LENGTH_8) {
		return 8;
	}

	return 0;

}

int krad_ebml_read_xiph_lace_value ( unsigned char *bytes, int *bytes_read ) {

	unsigned char byte;
	unsigned int value;
	int maxlen;
	unsigned int offset;
	
	offset = 0;
	value = 0;
	maxlen = 10;
	
	while (maxlen--) {
		*bytes_read += 1;
		byte = bytes[offset];
		if (byte != 255) {
	
			value += byte;
		
			//printk("xiph lace value is %u\n", value);
			return value;
	
		} else {
			value += 255;
			offset += 1;
		}
	}
	
	return -1;
}

void krad_ebml_read_tags( krad_ebml_t *krad_ebml, int tags_length ) {

	int bytes_read;
	//uint32_t ebml_id;
	//uint64_t ebml_data_size;
	
	bytes_read = 0;
	
	while (bytes_read != tags_length) {
	
		break;
	
	}
	


}

int krad_ebml_read_simpleblock( krad_ebml_t *krad_ebml, int len , int *tracknumber, uint64_t *timecode, unsigned char *buffer) {

	int tracknum;
	short block_timecode;
	unsigned char temp[8];

	int ret;

	unsigned char flags;

	unsigned int lacing;
	unsigned char laced_frames;
	
	unsigned int framecount;
	int total_size_of_frames;
	unsigned int block_bytes_read;
	
	int last_frame_size;
	
	int64_t frame_size;
	uint32_t frame_size_len;
	
	unsigned char byte;
	
	int64_t signed_drop[] = { 0x3f, 0x1fff, 0xfffff, 0x7ffffff, 0x3ffffffffLL, 0x1ffffffffffLL, 0xffffffffffffLL, 0x7fffffffffffffLL };
	
	total_size_of_frames = 0;
	framecount = 0;
	last_frame_size = 0;
	lacing = 0;
	block_bytes_read = 0;
	block_timecode = 0;
	tracknum = 0;

	krad_ebml_read ( krad_ebml, &byte, 1 );

	tracknum = (byte - 0x80);
	if (tracknumber != NULL) {
	  *tracknumber = tracknum;
	}
	//printf("tracknum is %d\n", tracknum);
	
	krad_ebml_read ( krad_ebml, &temp, 2 );
	rmemcpy ( &block_timecode, &temp, 2);
	
	krad_ebml->last_block_timecode = block_timecode;
	
	krad_ebml->current_timecode = krad_ebml->current_cluster_timecode + (int64_t)krad_ebml->last_block_timecode;
	//printf("timecode is %6.3f\n", ((krad_ebml->current_cluster_timecode + (int64_t)block_timecode)/1000.0));
	if (timecode != NULL) {	
	  *timecode = krad_ebml->current_timecode;
	}
	
	krad_ebml_read ( krad_ebml, &flags, 1 );
	
	//printf("flags is %x\n", flags);
	
	if ((flags & 0x04) && (flags & 0x02)) {
	//	printf("EBML Lacing\n");
		lacing = 1;
	}

	if ((!(flags & 0x04)) && (flags & 0x02)) {
	//	printf("Xiph Lacing\n");
		lacing = 2;
	}
	
	if ((!(flags & 0x04)) && (!(flags & 0x02))) {
	//	printf("No Lacing\n");
	}
	
	if ((flags & 0x04) && (!(flags & 0x02))) {
	//	printf("Fixed Lacing\n");
		lacing = 3;
	}

//	krad_ebml_seek ( krad_ebml, len - 4, SEEK_CUR );

	if (lacing == 0) {
		return krad_ebml_read ( krad_ebml, buffer, len - 4 );
	} else {
	
		krad_ebml_read ( krad_ebml, &laced_frames, 1 );
		
		block_bytes_read = 5;
		
		laced_frames += 1;
		
		framecount = 0;
		
		//printf("%u laced frames\n", laced_frames);
							
			if (lacing == 1) {
			

				while (framecount != laced_frames - 1) {
					block_bytes_read += krad_ebml_read ( krad_ebml, &byte, 1 );
					frame_size_len = ebml_length(byte);
			
					//printf("length of frame size one is %u\n", frame_size_len);
					
					
					memset(temp, 0, 8);
					
					// data size
					if (frame_size_len > 1) {
						ret = krad_ebml_read ( krad_ebml, &temp, frame_size_len - 1 );
						if (ret != frame_size_len - 1) {
							failfast ("Krad EBML failure reading %d", ret);
						}
						block_bytes_read += ret;
						
						frame_size = (uint64_t)byte;


					if (frame_size_len == 2) {
						frame_size &= (EBML_LENGTH_2 - 1);
					}

					if (frame_size_len == 3) {
						frame_size &= (EBML_LENGTH_3 - 1);
					}

					if (frame_size_len == 4) {
						frame_size &= (EBML_LENGTH_4 - 1);
					}

					if (frame_size_len == 5) {
						frame_size &= (EBML_LENGTH_5 - 1);
					}

					if (frame_size_len == 6) {
						frame_size &= (EBML_LENGTH_6 - 1);
					}

					if (frame_size_len == 7) {
						frame_size &= (EBML_LENGTH_7 - 1);
					}

					if (frame_size_len == 8) {
						frame_size &= (EBML_LENGTH_8 - 1);
					}

					frame_size <<= 8 * (frame_size_len - 1);
		
					rmemcpy ( &frame_size, &temp, frame_size_len - 1);
				} else {		
					frame_size = (byte - EBML_LENGTH_1);
				}
				
					//printf("frame size is %d\n", frame_size);
					
					if (framecount != 0) {
					
						frame_size -= signed_drop[frame_size_len - 1];
					
						frame_size = last_frame_size + frame_size;
					
					}
					last_frame_size = frame_size;
					
					//printf("frame size is %u\n", frame_size);
					
					krad_ebml->frame_sizes[framecount] = frame_size;
					
					total_size_of_frames += frame_size;

					framecount++;
				}
				
				
				
				frame_size = len - block_bytes_read - total_size_of_frames;
				//frame_size = last_frame_size + frame_size;
				//printf("last frame size is %u\n", frame_size);
				
				krad_ebml->frame_sizes[framecount] = frame_size;
				
				krad_ebml->read_laced_frames = framecount;
				
				krad_ebml->current_laced_frame = 0;
				
				//printf("reading first ebml laced frame %u bytes\n", krad_ebml->frame_sizes[krad_ebml->current_laced_frame]);
				
				return krad_ebml_read ( krad_ebml, buffer, krad_ebml->frame_sizes[krad_ebml->current_laced_frame] );
			}
			
			if (lacing == 2) {
			
        int bytes = 0;
        
				while (framecount != laced_frames - 1) {
				
					block_bytes_read += krad_ebml_read ( krad_ebml, &byte, 1 );
					if (byte != 255) {
					
						bytes += byte;
						
						
						//printk ("frame size is %d\n", bytes);
						krad_ebml->frame_sizes[framecount] = bytes;
						total_size_of_frames += bytes;
						framecount++;
            bytes = 0;
					
					} else {
						bytes += 255;
					}
				
				
				}
			
			
				frame_size = len - block_bytes_read - total_size_of_frames;
				
				//if (((block_bytes_read - 5) - (laced_frames - 1)) > 0) {
				//  frame_size =- (((block_bytes_read - 5) - (laced_frames - 1)) * 255);
				//}
				
				//frame_size = last_frame_size + frame_size;
				//printk ("last frame size is %u\n", frame_size);
				
				krad_ebml->frame_sizes[framecount] = frame_size;
				
				krad_ebml->read_laced_frames = framecount;
				
				krad_ebml->current_laced_frame = 0;
				
				//printk ("reading first xiph laced frame %u bytes\n", krad_ebml->frame_sizes[krad_ebml->current_laced_frame]);
				
				return krad_ebml_read ( krad_ebml, buffer, krad_ebml->frame_sizes[krad_ebml->current_laced_frame] );
			
			
			}
			
			if (lacing == 3) {
			
			

				
				krad_ebml->read_laced_frames = laced_frames - 1;
				framecount = laced_frames;
				while (framecount ) {
					framecount--;
					krad_ebml->frame_sizes[framecount] = (len - 5) / laced_frames;
				}
				
							
				krad_ebml->current_laced_frame = 0;
				
				//printf("reading first fixed laced frame %u bytes\n", krad_ebml->frame_sizes[krad_ebml->current_laced_frame]);
				
				return krad_ebml_read ( krad_ebml, buffer, krad_ebml->frame_sizes[krad_ebml->current_laced_frame] );
			
			
			}

	}
	
	return -1;
	
}


uint64_t krad_ebml_read_number_from_frag (unsigned char *ebml_frag, uint64_t ebml_data_size) {

	unsigned char temp[7];
	uint64_t number;
	
	number = 0;
	
	memset (temp, '\0', sizeof(temp));
	memcpy (&temp, &ebml_frag[0], ebml_data_size);
	rmemcpy ( &number, &temp, ebml_data_size);
	
	return number;

}

int krad_ebml_read_element_from_frag (unsigned char *ebml_frag, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr) {
	
	int frag_pos;
	uint32_t ebml_id;
	uint32_t ebml_id_length;
	uint64_t ebml_data_size;
	uint32_t ebml_data_size_length;
	unsigned char byte;
	unsigned char temp[7];

	frag_pos = 0;
	ebml_id = 0;
	ebml_data_size = 0;
	
	byte = ebml_frag[frag_pos++];
	
	// ID length
	ebml_id_length = ebml_length ( byte );

	if (ebml_id_length > 4) {
		failfast ("Krad EBML failure: EBML ID length > 4!");
	}

	//printf("id length is %u\n", ebml_id_length);
	memcpy((unsigned char *)&ebml_id + (ebml_id_length - 1), &byte, 1);

	// ID
	if (ebml_id_length > 1) {

		memcpy (&temp, &ebml_frag[frag_pos], ebml_id_length - 1);
		frag_pos += ebml_id_length - 1;

		rmemcpy ( &ebml_id, &temp, ebml_id_length - 1);
	}

	// data size length
	byte = ebml_frag[frag_pos++];
	ebml_data_size_length = ebml_length ( byte );
	
	//printf("data size length is %u\n", ebml_data_size_length);

	// data size
	if (ebml_data_size_length > 1) {
		
		memcpy (&temp, &ebml_frag[frag_pos], ebml_data_size_length - 1);
		frag_pos += ebml_data_size_length - 1;
		
		ebml_data_size = (uint64_t)byte;

		if (ebml_data_size_length == 2) {
			ebml_data_size &= (EBML_LENGTH_2 - 1);
		}

		if (ebml_data_size_length == 3) {
			ebml_data_size &= (EBML_LENGTH_3 - 1);
		}

		if (ebml_data_size_length == 4) {
			ebml_data_size &= (EBML_LENGTH_4 - 1);
		}

		if (ebml_data_size_length == 5) {
			ebml_data_size &= (EBML_LENGTH_5 - 1);
		}

		if (ebml_data_size_length == 6) {
			ebml_data_size &= (EBML_LENGTH_6 - 1);
		}

		if (ebml_data_size_length == 7) {
			ebml_data_size &= (EBML_LENGTH_7 - 1);
		}

		if (ebml_data_size_length == 8) {
			ebml_data_size &= (EBML_LENGTH_8 - 1);
		}

		ebml_data_size <<= 8 * (ebml_data_size_length - 1);
		
		rmemcpy ( &ebml_data_size, &temp, ebml_data_size_length - 1);
	} else {		
		ebml_data_size = (byte - EBML_LENGTH_1);
	}
	
	//printf("data size is %zu\n", ebml_data_size);
	
	*ebml_id_ptr = ebml_id;
	*ebml_data_size_ptr = ebml_data_size;
	
	return frag_pos;
	
}


int krad_ebml_read_element (krad_ebml_t *krad_ebml, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr) {
	
	int ret;
	uint32_t ebml_id;
	uint32_t ebml_id_length;
	uint64_t ebml_data_size;
	uint32_t ebml_data_size_length;
	unsigned char byte;
	unsigned char temp[7];

	
	if (!(ret = krad_ebml_read ( krad_ebml, &byte, 1 )) > 0) {
		printke ("Krad EBML read failure %d", ret);
		return 0;
	}
	
	if (krad_ebml->tracks_size > 0) {
		krad_ebml->tracks_pos += ret;
	}

	ebml_id = 0;
	ebml_data_size = 0;
	
	// ID length
	ebml_id_length = ebml_length ( byte );

	if (ebml_id_length > 4) {
		failfast ("Krad EBML failure: EBML ID > 4!");
		exit (1);
	}

  //printk("id length is %u\n", ebml_id_length);
	memcpy((unsigned char *)&ebml_id + (ebml_id_length - 1), &byte, 1);

	// ID
	if (ebml_id_length > 1) {
		ret = krad_ebml_read ( krad_ebml, &temp, ebml_id_length - 1 );
		if (krad_ebml->tracks_size > 0) {
			krad_ebml->tracks_pos += ret;
		}
		rmemcpy ( &ebml_id, &temp, ebml_id_length - 1);
	}

	// data size length
	ret = krad_ebml_read ( krad_ebml, &byte, 1 );
	if (ret != 1) {
		failfast ("Krad EBML failure reading data size length %d", ret);
	}
	if (krad_ebml->tracks_size > 0) {
		krad_ebml->tracks_pos += ret;
	}
	ebml_data_size_length = ebml_length ( byte );
	//printf("data size length is %u\n", ebml_data_size_length);

	// data size
	if (ebml_data_size_length > 1) {
		ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size_length - 1 );
		if (ret != ebml_data_size_length - 1) {
			failfast ("Krad EBML failure reading data %d", ret);
		}
		if (krad_ebml->tracks_size > 0) {
			krad_ebml->tracks_pos += ret;
		}
		ebml_data_size = (uint64_t)byte;

		if (ebml_data_size_length == 2) {
			ebml_data_size &= (EBML_LENGTH_2 - 1);
		}

		if (ebml_data_size_length == 3) {
			ebml_data_size &= (EBML_LENGTH_3 - 1);
		}

		if (ebml_data_size_length == 4) {
			ebml_data_size &= (EBML_LENGTH_4 - 1);
		}

		if (ebml_data_size_length == 5) {
			ebml_data_size &= (EBML_LENGTH_5 - 1);
		}

		if (ebml_data_size_length == 6) {
			ebml_data_size &= (EBML_LENGTH_6 - 1);
		}

		if (ebml_data_size_length == 7) {
			ebml_data_size &= (EBML_LENGTH_7 - 1);
		}

		if (ebml_data_size_length == 8) {
			ebml_data_size &= (EBML_LENGTH_8 - 1);
		}

		ebml_data_size <<= 8 * (ebml_data_size_length - 1);
		
		rmemcpy ( &ebml_data_size, &temp, ebml_data_size_length - 1);
	} else {		
		ebml_data_size = (byte - EBML_LENGTH_1);
	}
	
	//printf("data size is %zu\n", ebml_data_size);
	
	*ebml_id_ptr = ebml_id;
	*ebml_data_size_ptr = ebml_data_size;
	
	return 1;
	
}


uint64_t krad_ebml_read_command (krad_ebml_t *krad_ebml, unsigned char *buffer) {

	int ret;
	uint32_t ebml_id;
	uint64_t ebml_data_size;

	ret = krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);		

	if (ret == 0) {
		return 0;
	}

	//printf("data size is %" PRIu64 "\n", ebml_data_size);

	//if (ebml_id == EBML_ID_HEADER) {
	//	ret = krad_ebml_read ( krad_ebml, buffer, ebml_data_size );
	//	printf("%s %s\n", ebml_identify(ebml_id), buffer);
	//}
	
	return ebml_data_size;
}


uint64_t krad_ebml_read_number (krad_ebml_t *krad_ebml, uint64_t ebml_data_size) {

	int ret;
	unsigned char temp[7];
	uint64_t number;
	
	number = 0;
	
	memset (temp, '\0', sizeof(temp));

	ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size );
	if (ret != ebml_data_size) {
		failfast ("Krad EBML failure reading a number %d %"PRIu64"", ret, ebml_data_size);
	}

	rmemcpy ( &number, &temp, ebml_data_size);
	return number;

}

float krad_ebml_read_float (krad_ebml_t *krad_ebml, uint64_t ebml_data_size) {

	int ret;
	unsigned char temp[7];
	float number;
	
	number = 0;
	
	memset (temp, '\0', sizeof(temp));

	ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size );
	if (ret != ebml_data_size) {
		failfast ("Krad EBML failure reading a float %d %"PRIu64"\n", ret, ebml_data_size);
	}

	rmemcpy ( &number, &temp, ebml_data_size);
	return number;

}

uint64_t krad_ebml_read_string (krad_ebml_t *krad_ebml, char *string, uint64_t ebml_data_size) {

	int ret;

	ret = krad_ebml_read ( krad_ebml, string, ebml_data_size );
	if (ret != ebml_data_size) {
		failfast ("Krad EBML failure reading a string %d %"PRIu64"\n", ret, ebml_data_size);
	}
	string[ebml_data_size] = '\0';
	return ebml_data_size + 1;

}

int krad_ebml_read_packet (krad_ebml_t *krad_ebml, int *track, uint64_t *timecode, unsigned char *buffer) {

	int ret;
	uint32_t ebml_id;
	
	uint32_t ebml_seek_id;	
	//uint32_t ebml_id_length;
	uint64_t ebml_data_size;
	//uint32_t ebml_data_size_length;
	
	uint64_t position;
	
	//unsigned char byte;
	unsigned char temp[7];

	int skip;
	
	char string[512];
	memset(string, '\0', sizeof(string));
	
	int number;
	int known;

  ebml_seek_id = 0;
	known = 0;
	number = 0;
	
	skip = 1;
	
	if ((timecode != NULL) && (!(krad_ebml->read_laced_frames))) {
		*timecode = 0;
	}

	while (1) {
	
		if ((krad_ebml->header_read == 0) && (krad_ebml->tracks_size > 0) && (krad_ebml->tracks_pos == krad_ebml->tracks_size)) {
				krad_ebml->header_read = 1;
				return 0;
		}
	
	
		if (krad_ebml->read_laced_frames) {

			krad_ebml->current_laced_frame += 1;
			krad_ebml->read_laced_frames -= 1;
			//printf ("reading %d laced frame %"PRIu64" bytes\n", krad_ebml->current_laced_frame, krad_ebml->frame_sizes[krad_ebml->current_laced_frame]);
			return krad_ebml_read ( krad_ebml, buffer, krad_ebml->frame_sizes[krad_ebml->current_laced_frame] );
		}
	
	
    if ((krad_ebml->first_cluster_pos == 0) || (krad_ebml->segment_pos == 0)) {
      position = krad_ebml_tell (krad_ebml);
    }
	
		ret = krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);

		if (ret == 0) {
			return 0;
		}

		//printf("data size is %" PRIu64 "\n", ebml_data_size);

		if (ebml_id == EBML_ID_HEADER) {
			krad_ebml->ebml_level = 0;
			skip = 0;
		}

		if (ebml_id == EBML_ID_SEGMENT) {
		
			if (krad_ebml->segment_pos == 0) {
		  	krad_ebml->segment_pos = position;
		  	krad_ebml->segment_offset_pos = krad_ebml_tell (krad_ebml);
		    printk ("Got SEGMENT position at: %"PRIu64"", krad_ebml->segment_pos);
		    printk ("Got SEGMENT ofset position at: %"PRIu64"", krad_ebml->segment_offset_pos);		    
			}

			krad_ebml->ebml_level = 0;
			skip = 0;
		}

		if (ebml_id == EBML_ID_CLUSTER) {
			krad_ebml->ebml_level = 1;
			skip = 0;
			krad_ebml->cluster_count++;
			if (ebml_data_size > krad_ebml->largest_cluster) {
				krad_ebml->largest_cluster = ebml_data_size;
			}
			
			if (ebml_data_size < krad_ebml->smallest_cluster) {
				krad_ebml->smallest_cluster = ebml_data_size;
			}
			
			if (krad_ebml->first_cluster_pos == 0) {
		  	krad_ebml->first_cluster_pos = position;
		    printk ("Got first cluster position at: %"PRIu64"", krad_ebml->first_cluster_pos);
			}
			/*
			if (krad_ebml->header_read == 0) {
				krad_ebml->header_read = 1;
				int toseek;
				toseek = (4 + ebml_data_size_length) * -1;
				krad_ebml_seek ( krad_ebml, toseek, SEEK_CUR);
				return 0;
			}
			*/
		}
		
		if (ebml_id == EBML_ID_SEGMENT_INFO) {
			krad_ebml->ebml_level = 1;
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_SEGMENT_TRACKS) {
			krad_ebml->ebml_level = 1;
			skip = 0;
			krad_ebml->tracks_size = ebml_data_size;
		}

		if (ebml_id == EBML_ID_TRACK) {
			krad_ebml->ebml_level = 2;
			skip = 0;
			krad_ebml->track_count++;
			krad_ebml->current_track = krad_ebml->track_count;
		}
				
		if ((ebml_id == EBML_ID_TAGS) || (ebml_id == EBML_ID_TAG) || (ebml_id == EBML_ID_TAG_SIMPLE)) {
			krad_ebml->tags_position = 0;
			//krad_ebml_read_tags( krad_ebml, ebml_data_size);
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_TAG_NAME) {
			//krad_ebml->ebml_level = 1;
			if (ebml_data_size < sizeof(krad_ebml->tags)) {
				ret = krad_ebml_read ( krad_ebml, krad_ebml->tags, ebml_data_size );
				if (ret != ebml_data_size) {
					failfast ("Krad EBML failure reading a tag name %d", ret);
				}
				krad_ebml->tags[ret] = '\0';
				strcat(krad_ebml->tags, ": ");
				krad_ebml->tags_position += ret + 2;
			}
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_TAG_STRING) {
			//krad_ebml->ebml_level = 1;
			if (ebml_data_size < sizeof(krad_ebml->tags) + krad_ebml->tags_position) {
				ret = krad_ebml_read ( krad_ebml, krad_ebml->tags + krad_ebml->tags_position, ebml_data_size );
				if (ret != ebml_data_size) {
					failfast ("Krad EBML failure reading a tag string %d", ret);
				}
				krad_ebml->tags[ret + krad_ebml->tags_position] = '\0';
				//printf("Got Tag! %s\n", krad_ebml->tags);
			}
			skip = 0;
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_SIMPLEBLOCK) {
			krad_ebml->ebml_level = 2;
			krad_ebml->block_count++;
			if (1) {
				skip = 0;
				return krad_ebml_read_simpleblock( krad_ebml, ebml_data_size, track, timecode, buffer );
			} else {
				skip = 1;
			}
		}

		if (ebml_id == EBML_ID_VIDEOSETTINGS) {
			krad_ebml->ebml_level = 3;
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_AUDIOSETTINGS) {
			krad_ebml->ebml_level = 3;
			skip = 0;
		}
		
		if (((ebml_id == EBML_ID_MUXINGAPP) || (ebml_id == EBML_ID_WRITINGAPP) || (ebml_id == EBML_ID_CODECID) || 
			(ebml_id == EBML_ID_DOCTYPE) || (ebml_id == EBML_ID_SEGMENT_TITLE)) 
			&& (ebml_data_size < sizeof(string))) {
			ret = krad_ebml_read ( krad_ebml, &string, ebml_data_size );
			if (ret != ebml_data_size) {
				failfast ("Krad EBML failure reading ... %d", ret);
			}
			if (krad_ebml->tracks_size > 0) {
				krad_ebml->tracks_pos += ret;
			}
			
			//printf("%s", string);
			
			krad_ebml->tracks[krad_ebml->current_track].codec = NOCODEC;
			
			if (strncmp(string, "A_VORBIS", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = VORBIS;
			}
					
			if (strncmp(string, "A_FLAC", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = FLAC;
			}
			
			if ((strncmp(string, "A_OPUS", 8) == 0) || (strncmp(string, "A_KRAD_OPUS", 11) == 0)) {
			
				if (strncmp(string, "A_KRAD_OPUS", 11) == 0) {
					// kludge, but good catch, we need to account for things such as a "track 2" without
					// a track 1, and other nonsense
					krad_ebml->track_count++;
					krad_ebml->current_track = krad_ebml->track_count;
				}
				krad_ebml->tracks[krad_ebml->current_track].codec = OPUS;
			}
			
			if (strncmp(string, "V_THEORA", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = THEORA;
			}
			
			if (strncmp(string, "V_VP8", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = VP8;
			}
			
			if (strncmp(string, "V_KVHS", 9) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = KVHS;
			}
			
			// If we found one of our codecs
			if (krad_ebml->tracks[krad_ebml->current_track].codec != NOCODEC) {
				krad_ebml->tracks[krad_ebml->current_track].changed = 1;				
			}
				
			memset(string, '\0', sizeof(string));
			skip = 0;
			known = 1;
		}
		
		
		if (ebml_id == EBML_ID_CODECDATA) {
		
			krad_ebml->tracks[krad_ebml->current_track].codec_data = calloc(1, ebml_data_size);
			
			
			
			ret = krad_ebml_read ( krad_ebml, krad_ebml->tracks[krad_ebml->current_track].codec_data, ebml_data_size );
			if (ret != ebml_data_size) {
				failfast ("Krad EBML failure reading codec data %d", ret);
			} else {
				//printf("read %d\n", ret);
			}
			if (krad_ebml->tracks_size > 0) {
				krad_ebml->tracks_pos += ret;
			}
			krad_ebml->tracks[krad_ebml->current_track].codec_data_size = ebml_data_size;
			
			skip = 0;
			
			if ((krad_ebml->tracks[krad_ebml->current_track].codec == THEORA) || (krad_ebml->tracks[krad_ebml->current_track].codec == VORBIS)) {
			
				int bytes_read;
			
				bytes_read = 0;
			
				// first byte is the number of elements
				bytes_read = 1;

				krad_ebml->tracks[krad_ebml->current_track].headers = 3;
			
				krad_ebml->tracks[krad_ebml->current_track].header_len[0] = krad_ebml_read_xiph_lace_value ( krad_ebml->tracks[krad_ebml->current_track].codec_data + bytes_read, &bytes_read );
				krad_ebml->tracks[krad_ebml->current_track].header_len[1] = krad_ebml_read_xiph_lace_value ( krad_ebml->tracks[krad_ebml->current_track].codec_data + bytes_read, &bytes_read );
				krad_ebml->tracks[krad_ebml->current_track].header_len[2] = krad_ebml->tracks[krad_ebml->current_track].codec_data_size - (bytes_read + (krad_ebml->tracks[krad_ebml->current_track].header_len[0] + krad_ebml->tracks[krad_ebml->current_track].header_len[1]));
				
				krad_ebml->tracks[krad_ebml->current_track].header[0] = krad_ebml->tracks[krad_ebml->current_track].codec_data + bytes_read;
				krad_ebml->tracks[krad_ebml->current_track].header[1] = krad_ebml->tracks[krad_ebml->current_track].codec_data + bytes_read + krad_ebml->tracks[krad_ebml->current_track].header_len[0];
				krad_ebml->tracks[krad_ebml->current_track].header[2] = krad_ebml->tracks[krad_ebml->current_track].codec_data + bytes_read + krad_ebml->tracks[krad_ebml->current_track].header_len[0] + krad_ebml->tracks[krad_ebml->current_track].header_len[1];



				//printk ("got xiph headers codec data size %"PRIu64" -- bytes read %d -- %d %d %d\n",
				//		ebml_data_size,
				//    bytes_read,
				//		krad_ebml->tracks[krad_ebml->current_track].header_len[0],
				//		krad_ebml->tracks[krad_ebml->current_track].header_len[1],
				//		krad_ebml->tracks[krad_ebml->current_track].header_len[2]);
			
			}
			
			if ((krad_ebml->tracks[krad_ebml->current_track].codec == FLAC) || (krad_ebml->tracks[krad_ebml->current_track].codec == OPUS)) {
				// it should be just one...			
				krad_ebml->tracks[krad_ebml->current_track].headers = 1;
				krad_ebml->tracks[krad_ebml->current_track].header[0] = krad_ebml->tracks[krad_ebml->current_track].codec_data;
				krad_ebml->tracks[krad_ebml->current_track].header_len[0] = krad_ebml->tracks[krad_ebml->current_track].codec_data_size;
				
			}
	
			if (krad_ebml->tracks[krad_ebml->current_track].codec == VP8) {
				// Really nothing..
				krad_ebml->tracks[krad_ebml->current_track].headers = 0;
				krad_ebml->tracks[krad_ebml->current_track].header_len[0] = 0;
			}		
			
		}
		

		if ((ebml_id == EBML_ID_VIDEOWIDTH) || (ebml_id == EBML_ID_VIDEOHEIGHT) ||
			(ebml_id == EBML_ID_AUDIOCHANNELS) || (ebml_id == EBML_ID_TRACKNUMBER) || (ebml_id == EBML_ID_DEFAULTDURATION) ||
			(ebml_id == EBML_ID_AUDIOBITDEPTH) || (ebml_id == EBML_ID_3D) || (ebml_id == EBML_ID_CLUSTER_TIMECODE)) {
			ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size );
			if (ret != ebml_data_size) {
				failfast ("Krad EBML failure reading trackinfo item %d", ret);
			}
			if (krad_ebml->tracks_size > 0) {
				krad_ebml->tracks_pos += ret;
			}
			rmemcpy ( &number, &temp, ebml_data_size);

			if (krad_ebml->header_read == 0) {
				//printf("%s %d", ebml_identify(ebml_id), number);
			}
			
			if (ebml_id == EBML_ID_CLUSTER_TIMECODE) {
				krad_ebml->current_cluster_timecode = number;
			}
			// need to remove
			if (ebml_id == EBML_ID_VIDEOWIDTH) {
				krad_ebml->width = number;
			}
			// need to remove
			if (ebml_id == EBML_ID_VIDEOHEIGHT) {
				krad_ebml->height = number;
			}

			if (ebml_id == EBML_ID_VIDEOWIDTH) {
				krad_ebml->tracks[krad_ebml->current_track].width = number;
			}

			if (ebml_id == EBML_ID_VIDEOHEIGHT) {
				krad_ebml->tracks[krad_ebml->current_track].height = number;
			}
			
			if (ebml_id == EBML_ID_AUDIOBITDEPTH) {
				krad_ebml->tracks[krad_ebml->current_track].bit_depth = number;
			}
			
			if (ebml_id == EBML_ID_AUDIOCHANNELS) {
				krad_ebml->tracks[krad_ebml->current_track].channels = number;
			}
			
			number = 0;
			skip = 0;
			known = 1;
		}
		
		if (ebml_id == EBML_ID_AUDIOSAMPLERATE) {
		
			float srate;
			double samplerate;
			
			ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size );
			if (ret != ebml_data_size) {
				failfast ("Krad EBML failure reading Sample Rate %d", ret);
			}
			if (krad_ebml->tracks_size > 0) {
				krad_ebml->tracks_pos += ret;
			}
			int adj = 0;
			if (ebml_data_size == 4) {
				rmemcpy ( &srate, &temp, ebml_data_size );
				//printf("Sample Rate %f", srate);
				samplerate = srate;
			} else {
				rmemcpy ( &samplerate, &temp, ebml_data_size - adj);
				//printf("Sample Rate %f", samplerate);
			}
	
			krad_ebml->tracks[krad_ebml->current_track].sample_rate = samplerate;
	
			skip = 0;
			known = 1;
		}
		
		if (ebml_id == EBML_ID_DURATION) {
		
			float dur;
			double duration;
			
			ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size );
			if (ret != ebml_data_size) {
				failfast ("Krad EBML failure reading Sample Rate %d", ret);
			}
			if (krad_ebml->tracks_size > 0) {
				krad_ebml->tracks_pos += ret;
			}
			int adj = 0;
			if (ebml_data_size == 4) {
				rmemcpy ( &dur, &temp, ebml_data_size );
				//printf("Sample Rate %f", srate);
				duration = dur;
			} else {
				rmemcpy ( &duration, &temp, ebml_data_size - adj);
				//printf("Sample Rate %f", samplerate);
			}
	
			krad_ebml->segment_duration = duration;
	
			skip = 0;
			known = 1;
		}
		
		if (krad_ebml->header_read == 0) {
			if (known == 0) {
				//printf("%s size %" PRIu64 " ", ebml_identify(ebml_id), ebml_data_size);
				//printf("\n");
			} else {
				//printf("\n");
			}
		}
		
		if (ebml_id == EBML_ID_SEEKHEAD) {
      printk ("found seekhead");		
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_SEEK) {
      printk ("found seek");		
			skip = 0;
		}		
		
		if (ebml_id == EBML_ID_SEEK_ID) {
		  ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size );
		  rmemcpy ( &ebml_seek_id, &temp, ebml_data_size);
      printk ("found seek id");		
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_SEEK_POSITION) {
      printk ("found seek POSITION");		
		  position = krad_ebml_read_number (krad_ebml, ebml_data_size);
      
      if (ebml_seek_id == EBML_ID_CUES) {
        krad_ebml->cues_pos = position;
        krad_ebml->cues_file_pos = krad_ebml->cues_pos + krad_ebml->segment_offset_pos;
		    printk ("Got CUES position from seekhead at: %"PRIu64"", krad_ebml->cues_pos);
		    printk ("Got CUES file position at: %"PRIu64"", krad_ebml->cues_file_pos);
		    
		    if (krad_ebml->cues_file_pos != 0) {
          position = krad_ebml_tell (krad_ebml);
          
          krad_ebml_seek (krad_ebml, krad_ebml->cues_file_pos, SEEK_SET);
          krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
          krad_ebml->cues_size = ebml_data_size + (krad_ebml_tell (krad_ebml) - krad_ebml->cues_file_pos);
		      printk ("Got CUES size: %"PRIu64"", krad_ebml->cues_size);
		      krad_ebml->cues_file_end_range = krad_ebml->cues_file_pos + krad_ebml->cues_size;
		      printk ("Got CUES file range: %"PRIu64"-%"PRIu64"", krad_ebml->cues_file_pos, krad_ebml->cues_file_end_range);		      
          krad_ebml_seek (krad_ebml, position, SEEK_SET);
		    }
		    	    
      }
   
      if (ebml_seek_id == EBML_ID_CLUSTER) {
        krad_ebml->first_cluster_pos_from_seekhead = position;
		    printk ("Got first CLUSTER position from seekhead at: %"PRIu64"", krad_ebml->first_cluster_pos_from_seekhead);        
      }   
      		  		
			skip = 0;
		}		
			
		if (skip) {
			if (krad_ebml->stream == 1) {
				if (ebml_data_size != EBML_DATA_SIZE_UNKNOWN) {
					krad_ebml_read ( krad_ebml, krad_ebml->bsbuffer, ebml_data_size);
				}
			} else {
				if (ebml_data_size != EBML_DATA_SIZE_UNKNOWN) {
					krad_ebml_seek ( krad_ebml, ebml_data_size, SEEK_CUR);
				}
			}
			if (krad_ebml->tracks_size > 0) {
				krad_ebml->tracks_pos += ebml_data_size;
			}
		}
		known = 0;
		skip = 1;

	}
	//printf("\n");
	//printf("ret was %d\n", ret);
	
	//printf("Smallest Cluster %d Largest Cluster %d\n", krad_ebml->smallest_cluster, krad_ebml->largest_cluster);
	//printf("Track Count is %d Cluster Count %u Block Count %u\n", krad_ebml->track_count, krad_ebml->cluster_count, krad_ebml->block_count);


	return 0;


}

void krad_ebml_print_ebml_header (struct ebml_header *ebml_head) {

	printk ("EBML Header:\n");
	printk ("EBML Version: %"PRIu64"\n", ebml_head->ebml_version.v.u);
	printk ("EBML Read Version: %"PRIu64"\n", ebml_head->ebml_read_version.v.u);
	printk ("EBML Max ID Length: %"PRIu64"\n", ebml_head->ebml_max_id_length.v.u);
	printk ("EBML Max Size Length: %"PRIu64"\n", ebml_head->ebml_max_size_length.v.u);
	printk ("EBML Doctype: %s\n", ebml_head->doctype.v.s);				
	printk ("EBML Doctype Version: %"PRIu64"\n", ebml_head->doctype_version.v.u);
	printk ("EBML Doctype Read Version: %"PRIu64"\n", ebml_head->doctype_read_version.v.u);
	printk ("\n");
}

int krad_ebml_check_ebml_header (struct ebml_header *ebml_head) {

	if ((ebml_head->ebml_version.v.u == 1) &&
	    (ebml_head->ebml_read_version.v.u == 1) &&
	    (ebml_head->ebml_max_id_length.v.u == 4) &&
	    (ebml_head->ebml_max_size_length.v.u <= 8))
	{
		return 1;
	}
	
	printk ("Krad EBML can't read this EBML!");
	krad_ebml_print_ebml_header (ebml_head);
	
	return 0;
	
}


int krad_ebml_check_doctype_header (struct ebml_header *ebml_head, char *doctype, int doctype_version, int doctype_read_version ) {

	if ((strcmp(doctype, ebml_head->doctype.v.s) == 0) &&
	   (doctype_version == ebml_head->doctype_version.v.u) &&
	   (doctype_read_version == ebml_head->doctype_read_version.v.u))
	{
		return 1;
	}
	
	return 0;
	
}

int krad_ebml_read_ebml_header (krad_ebml_t *krad_ebml, struct ebml_header *ebml_head) {

	int ret;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int header_items;
	uint64_t number;
	char *string;
	unsigned char buffer[7];

	header_items = 7;
	number = 0;
	string = NULL;

	ret = krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);		

	if ((ret == 0) || (ebml_id != EBML_ID_HEADER)) {
		return 0;
	} else {

		while (header_items) {
		
			memset (buffer, '0', sizeof(buffer));
		
			ret = krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
			
			switch (ebml_id) {
				case EBML_ID_DOCTYPE:
					string = malloc(ebml_data_size + 1);
					string[ebml_data_size] = '\0';
					ret = krad_ebml_read ( krad_ebml, string, ebml_data_size );
					if (ret != ebml_data_size) {
						return 0;
					}
					break;
				case EBML_ID_EBMLVERSION:
				case EBML_ID_EBMLREADVERSION:
				case EBML_ID_EBMLMAXIDLENGTH:
				case EBML_ID_EBMLMAXSIZELENGTH:
				case EBML_ID_DOCTYPEVERSION:
				case EBML_ID_DOCTYPEREADVERSION:
					ret = krad_ebml_read ( krad_ebml, &buffer, ebml_data_size );
					if (ret != ebml_data_size) {
						return 0;
					}
					rmemcpy ( &number, &buffer, ebml_data_size);
					break;
				default:
					return 0;
			}

			switch (ebml_id) {
				case EBML_ID_DOCTYPE:
					ebml_head->doctype.v.s = string;
					ebml_head->doctype.type = TYPE_STRING;
					break;			
				case EBML_ID_EBMLVERSION:
					ebml_head->ebml_version.v.u = number;
					ebml_head->ebml_version.type = TYPE_UINT;
					break;
				case EBML_ID_EBMLREADVERSION:
					ebml_head->ebml_read_version.v.u = number;
					ebml_head->ebml_read_version.type = TYPE_UINT;
					break;
				case EBML_ID_EBMLMAXIDLENGTH:
					ebml_head->ebml_max_id_length.v.u = number;
					ebml_head->ebml_max_id_length.type = TYPE_UINT;
					break;
				case EBML_ID_EBMLMAXSIZELENGTH:
					ebml_head->ebml_max_size_length.v.u = number;
					ebml_head->ebml_max_size_length.type = TYPE_UINT;
					break;
				case EBML_ID_DOCTYPEVERSION:
					ebml_head->doctype_version.v.u = number;
					ebml_head->doctype_version.type = TYPE_UINT;
					break;
				case EBML_ID_DOCTYPEREADVERSION:
					ebml_head->doctype_read_version.v.u = number;
					ebml_head->doctype_read_version.type = TYPE_UINT;
					break;
				default:
					return 0;
			}

			header_items--;
		}
		
		return 1;

	}

}


/* IO */

int krad_ebml_write(krad_ebml_t *krad_ebml, void *buffer, size_t length) {

//	if ((length + krad_ebml->io_adapter.write_buffer_pos) >= KRADEBML_WRITE_BUFFER_SIZE) {
//		krad_ebml_write_sync(krad_ebml);
//	}

	memcpy(krad_ebml->io_adapter.write_buffer + krad_ebml->io_adapter.write_buffer_pos, buffer, length);
	krad_ebml->io_adapter.write_buffer_pos += length;

	return length;
}

int krad_ebml_write_sync(krad_ebml_t *krad_ebml) {

	uint64_t length;
	
	length = krad_ebml->io_adapter.write_buffer_pos;
	krad_ebml->io_adapter.write_buffer_pos = 0;
	krad_ebml->segment_size += length;
	return krad_ebml->io_adapter.write(&krad_ebml->io_adapter, krad_ebml->io_adapter.write_buffer, length);
}

	
void krad_ebml_enable_read_copy ( krad_ebml_t *krad_ebml ) {

	if (krad_ebml->read_copy == 0) {
		krad_ebml->read_copy_buffer = malloc (1000000);
		krad_ebml->read_copy = 1;
	}

}

void krad_ebml_disable_read_copy ( krad_ebml_t *krad_ebml ) {

	if (krad_ebml->read_copy == 1) {
		free (krad_ebml->read_copy_buffer);
		krad_ebml->read_copy_pos = 0;
		krad_ebml->read_copy = 0;
	}

}	


int krad_ebml_read (krad_ebml_t *krad_ebml, void *buffer, size_t length) {

	int ret;

	ret = krad_ebml->io_adapter.read(&krad_ebml->io_adapter, buffer, length);
	
	if (krad_ebml->read_copy == 1) {
		memcpy (krad_ebml->read_copy_buffer + krad_ebml->read_copy_pos, buffer, ret);
		krad_ebml->read_copy_pos += ret;
	}

	return ret;
}

int krad_ebml_read_copy (krad_ebml_t *krad_ebml, void *buffer ) {

	int ret;

	ret = krad_ebml->read_copy_pos;

	memcpy (buffer, krad_ebml->read_copy_buffer, krad_ebml->read_copy_pos);

	krad_ebml->read_copy_pos = 0;
	
	return ret;

}

int krad_ebml_seek(krad_ebml_t *krad_ebml, int64_t offset, int whence) {

	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) || (krad_ebml->io_adapter.mode == KRAD_EBML_IO_READWRITE)) {
		if (whence == SEEK_CUR) {
			krad_ebml->io_adapter.write_buffer_pos += offset;
		}
		if (whence == SEEK_SET) {
			krad_ebml->io_adapter.write_buffer_pos = offset;
		}
		return krad_ebml->io_adapter.write_buffer_pos;
	}

	return krad_ebml->io_adapter.seek(&krad_ebml->io_adapter, offset, whence);
}

int64_t krad_ebml_tell(krad_ebml_t *krad_ebml) {

	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) || (krad_ebml->io_adapter.mode == KRAD_EBML_IO_READWRITE)) {
		return krad_ebml->io_adapter.write_buffer_pos;
	}
  if (krad_ebml->io_adapter.tell != NULL) {
	  return krad_ebml->io_adapter.tell(&krad_ebml->io_adapter);
	} else {
	  return 0;
	}
}

int krad_ebml_fileio_open(krad_ebml_io_t *krad_ebml_io)
{

	int fd;
	
	fd = 0;
	
	if (krad_ebml_io->mode == KRAD_EBML_IO_READONLY) {
		if ((krad_ebml_io->uri == NULL) || (!strlen(krad_ebml_io->uri))) {
			fd = fileno(stdin);
		} else {
			fd = open ( krad_ebml_io->uri, O_RDONLY );
		}
	}
	
	if (krad_ebml_io->mode == KRAD_EBML_IO_WRITEONLY) {
		fd = open ( krad_ebml_io->uri, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	}

	krad_ebml_io->ptr = fd;

	return fd;
}

int krad_ebml_fileio_close(krad_ebml_io_t *krad_ebml_io) {
	return close(krad_ebml_io->ptr);
}

int krad_ebml_io_buffer_write(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length) {
	return write(krad_ebml_io->ptr, buffer, length);
}

int krad_ebml_io_buffer_read(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length) {

	if ((krad_ebml_io->buffer_io_read_pos + length) > krad_ebml_io->buffer_io_read_len) {
		return 0;
	}
	
	memcpy (buffer, krad_ebml_io->buffer_io_buffer + krad_ebml_io->buffer_io_read_pos, length);

	krad_ebml_io->buffer_io_read_pos += length;

	if (krad_ebml_io->buffer_io_read_pos == krad_ebml_io->buffer_io_read_len) {
		krad_ebml_io->buffer_io_read_pos = 0;
		krad_ebml_io->buffer_io_read_len = 0;
	}

	return length;

}

int krad_ebml_io_buffer_read_space (krad_ebml_io_t *krad_ebml_io) {

	return krad_ebml_io->buffer_io_read_len - krad_ebml_io->buffer_io_read_pos;

}

int krad_ebml_io_buffer_push(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length) {

	if ((krad_ebml_io->buffer_io_read_len + length) > 4096 * 6) {
		return 0;
	}
	
	memcpy (krad_ebml_io->buffer_io_buffer + krad_ebml_io->buffer_io_read_len, buffer, length);

	krad_ebml_io->buffer_io_read_len += length;

	return length;

}

int krad_ebml_transmissionio_write(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length) {
	if (krad_ebml_io->firstwritedone == 1) {
		return krad_transmitter_transmission_add_data (krad_ebml_io->krad_transmission, (unsigned char *)buffer, length);
	} else {
		krad_ebml_io->firstwritedone = 1;
		return krad_transmitter_transmission_set_header (krad_ebml_io->krad_transmission, (unsigned char *)buffer, length);
	}
}

int krad_ebml_fileio_write(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length) {
	return write(krad_ebml_io->ptr, buffer, length);
}

int krad_ebml_fileio_read(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length) {
	if (krad_ebml_io->ptr == 0) {
		return fread(buffer, 1, length, stdin);
	} else {
		return read(krad_ebml_io->ptr, buffer, length);
	}
}

int64_t krad_ebml_fileio_seek(krad_ebml_io_t *krad_ebml_io, int64_t offset, int whence) {

	char c;

	if (krad_ebml_io->ptr == 0) {
		while (offset--) {
			fread(&c, 1, 1, stdin);
		}
		return 0;
	} else {
		return lseek(krad_ebml_io->ptr, offset, whence);
	}
}

int64_t krad_ebml_fileio_tell(krad_ebml_io_t *krad_ebml_io) {
	return lseek(krad_ebml_io->ptr, 0, SEEK_CUR);
}

int krad_ebml_streamio_close(krad_ebml_io_t *krad_ebml_io) {
	return close(krad_ebml_io->sd);
}

int krad_ebml_streamio_write(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length) {

	int bytes;
	
	bytes = 0;

	while (bytes != length) {

		bytes += send (krad_ebml_io->sd, buffer + bytes, length - bytes, 0);

		if (bytes <= 0) {
			failfast ("Krad EBML stream io write: Got Disconnected from server. bytes ret %d len %zu", bytes, length);
		}
	}
	
	return bytes;
}


int krad_ebml_streamio_read(krad_ebml_io_t *krad_ebml_io, void *buffer, size_t length) {

	int bytes;
	
	bytes = 0;

	while (bytes != length) {

		bytes += recv (krad_ebml_io->sd, buffer + bytes, length - bytes, 0);

		if (bytes <= 0) {
			failfast ("Krad EBML Source: recv Got Disconnected from server");
		}
	}
	
	return bytes;
	
}


int krad_ebml_streamio_open(krad_ebml_io_t *krad_ebml_io) {

	struct sockaddr_in serveraddr;
	struct hostent *hostp;
	char http_string[512];
	int http_string_pos;
	char content_type[64];
	char auth[256];
	char auth_base64[256];
	int sent;

	http_string_pos = 0;

	//printf ("Krad EBML: Connecting to %s:%d\n", krad_ebml_io->host, krad_ebml_io->port);

	if ((krad_ebml_io->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		failfast ("Krad EBML Source: Socket Error");
	}

	memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(krad_ebml_io->port);
	
	if ((serveraddr.sin_addr.s_addr = inet_addr(krad_ebml_io->host)) == (unsigned long)INADDR_NONE)
	{
		// get host address 
		hostp = gethostbyname(krad_ebml_io->host);
		if(hostp == (struct hostent *)NULL)
		{
			close (krad_ebml_io->sd);
			failfast ("Krad EBML: Mount problem");
		}
		memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
	}

	// connect() to server. 
	if((sent = connect(krad_ebml_io->sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
	{
		failfast ("Krad EBML Source: Connect Error");
	} else {


		if (krad_ebml_io->mode == KRAD_EBML_IO_READONLY) {
	
			sprintf(http_string, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", krad_ebml_io->mount, krad_ebml_io->host);
			//printf("%s\n", http_string);
			krad_ebml_streamio_write(krad_ebml_io, http_string, strlen(http_string));
	
			int end_http_headers = 0;
			char buf[8];
			while (end_http_headers != 4) {
	
				krad_ebml_streamio_read(krad_ebml_io, buf, 1);
	
				//printf("%c", buf[0]);
	
				if ((buf[0] == '\n') || (buf[0] == '\r')) {
					end_http_headers++;
				} else {
					end_http_headers = 0;
				}
	
			}
		} 
		
		if (krad_ebml_io->mode == KRAD_EBML_IO_WRITEONLY) {
		
			strcpy(content_type, "video/webm");
			//strcpy(krad_mkvsource->content_type, "video/x-matroska");
			//strcpy(krad_mkvsource->content_type, "audio/mpeg");
			//strcpy(krad_mkvsource->content_type, "application/ogg");
			sprintf(auth, "source:%s", krad_ebml_io->password );
			krad_ebml_base64_encode( auth_base64, auth );
			http_string_pos = sprintf( http_string, "SOURCE %s ICE/1.0\r\n", krad_ebml_io->mount);
			http_string_pos += sprintf( http_string + http_string_pos, "content-type: %s\r\n", content_type);
			http_string_pos += sprintf( http_string + http_string_pos, "Authorization: Basic %s\r\n", auth_base64);
			http_string_pos += sprintf( http_string + http_string_pos, "\r\n");
			//printf("%s\n", http_string);
			krad_ebml_streamio_write(krad_ebml_io, http_string, http_string_pos);
		
		}

	}

	return krad_ebml_io->sd;
}


void krad_ebml_destroy(krad_ebml_t *krad_ebml) {
	
	int t;

	if (krad_ebml->cluster != 0) {
		krad_ebml_finish_element (krad_ebml, krad_ebml->cluster);
		krad_ebml_write_sync (krad_ebml);
	} else {
		if (krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) {
			krad_ebml_write_sync (krad_ebml);
		}
	}

	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) &&
		(krad_ebml->io_adapter.write == krad_ebml_fileio_write) &&
		(krad_ebml->segment != 0)) {
		krad_ebml_finish_file_segment (krad_ebml);
	}

	if (krad_ebml->io_adapter.mode != -1) {
		if (krad_ebml->io_adapter.close != NULL) {
			krad_ebml->io_adapter.close(&krad_ebml->io_adapter);
		}
	}

	if (krad_ebml->clusters != NULL) {
		free (krad_ebml->clusters);
	}

	if (krad_ebml->io_adapter.mode == KRAD_EBML_IO_READONLY) {
		for (t = 0; t < krad_ebml->track_count; t++) {
			if (krad_ebml->tracks[t].codec_data != NULL) {
				free (krad_ebml->tracks[t].codec_data);
			}
		}
	}
		
	if (krad_ebml->io_adapter.write_buffer != NULL) {
		free (krad_ebml->io_adapter.write_buffer);
	}
	
	if (krad_ebml->read_copy == 1) {
		krad_ebml_disable_read_copy ( krad_ebml );
	}
	
	if (krad_ebml->io_adapter.buffer_io_buffer != NULL) {
		free (krad_ebml->io_adapter.buffer_io_buffer);
	}
	
	if (krad_ebml->tracks != NULL) {
		free (krad_ebml->tracks);
	}
	
	if (krad_ebml->header->doctype.v.s != NULL) {
		free (krad_ebml->header->doctype.v.s);
	}
	
	free (krad_ebml->header);
	free (krad_ebml);

}

krad_ebml_t *krad_ebml_create() {

	krad_ebml_t *krad_ebml = calloc(1, sizeof(krad_ebml_t));
	krad_ebml->current_track = 1;
	krad_ebml->ebml_level = -1;
	krad_ebml->io_adapter.mode = -1;
	krad_ebml->smallest_cluster = 10000000;
  krad_ebml->tracks = calloc(10, sizeof(krad_ebml_track_t));
	krad_ebml->header = calloc(1, sizeof(struct ebml_header));
	
	return krad_ebml;

}

krad_ebml_t *krad_ebml_open_stream(char *host, int port, char *mount, char *password) {

	krad_ebml_t *krad_ebml;
	
	krad_ebml = krad_ebml_create();

	krad_ebml->record_cluster_info = 0;
	//krad_ebml->cluster_recording_space = 5000;
	//krad_ebml->clusters = calloc(krad_ebml->cluster_recording_space, sizeof(krad_ebml_cluster_t));

	//krad_ebml->io_adapter.seek = krad_ebml_streamio_seek;
	if (password == NULL) {
		krad_ebml->io_adapter.mode = KRAD_EBML_IO_READONLY;
	} else {
		krad_ebml->io_adapter.mode = KRAD_EBML_IO_WRITEONLY;
		krad_ebml->io_adapter.write_buffer = malloc(KRADEBML_WRITE_BUFFER_SIZE);
	}
	//krad_ebml->io_adapter.seekable = 1;
	krad_ebml->io_adapter.read = krad_ebml_streamio_read;
	krad_ebml->io_adapter.write = krad_ebml_streamio_write;
	krad_ebml->io_adapter.open = krad_ebml_streamio_open;
	krad_ebml->io_adapter.close = krad_ebml_streamio_close;
	krad_ebml->io_adapter.uri = host;
	krad_ebml->io_adapter.host = host;
	krad_ebml->io_adapter.port = port;
	krad_ebml->io_adapter.mount = mount;
	krad_ebml->io_adapter.password = password;
	
	krad_ebml->stream = 1;
	
	if (strcmp("ListenSD", krad_ebml->io_adapter.uri) == 0) {
		krad_ebml->io_adapter.sd = krad_ebml->io_adapter.port;
	} else {
		krad_ebml->io_adapter.open(&krad_ebml->io_adapter);
	}
	
	if (krad_ebml->io_adapter.mode == KRAD_EBML_IO_READONLY) {
		krad_ebml_read_ebml_header (krad_ebml, krad_ebml->header);
		krad_ebml_check_ebml_header (krad_ebml->header);
		//krad_ebml_print_ebml_header (krad_ebml->header);		
		krad_ebml_read_packet (krad_ebml, NULL, NULL, NULL);
	}
		
	return krad_ebml;

}


krad_ebml_t *krad_ebml_open_transmission (krad_transmission_t *krad_transmission) {

	krad_ebml_t *krad_ebml;
	
	krad_ebml = krad_ebml_create();
	
	krad_ebml->krad_transmission = krad_transmission;
	
	krad_ebml->record_cluster_info = 0;
	krad_ebml->io_adapter.mode = KRAD_EBML_IO_WRITEONLY;
	krad_ebml->io_adapter.write_buffer = malloc(KRADEBML_WRITE_BUFFER_SIZE);
	krad_ebml->io_adapter.krad_transmission = krad_ebml->krad_transmission;
	krad_ebml->io_adapter.write = krad_ebml_transmissionio_write;
	
	return krad_ebml;
}


krad_ebml_t *krad_ebml_open_file(char *filename, krad_ebml_io_mode_t mode) {

	krad_ebml_t *krad_ebml;
	
	krad_ebml = krad_ebml_create();

	krad_ebml->io_adapter.seek = krad_ebml_fileio_seek;
	krad_ebml->io_adapter.tell = krad_ebml_fileio_tell;
	krad_ebml->io_adapter.mode = mode;
	krad_ebml->io_adapter.seekable = 1;
	krad_ebml->io_adapter.read = krad_ebml_fileio_read;
	krad_ebml->io_adapter.write = krad_ebml_fileio_write;
	krad_ebml->io_adapter.open = krad_ebml_fileio_open;
	krad_ebml->io_adapter.close = krad_ebml_fileio_close;
	krad_ebml->io_adapter.uri = filename;
	krad_ebml->io_adapter.open(&krad_ebml->io_adapter);

	if (krad_ebml->io_adapter.mode == KRAD_EBML_IO_READONLY) {
		krad_ebml->record_cluster_info = 1;
		krad_ebml->cluster_recording_space = CLUSTER_RECORDING_START_SIZE;
		krad_ebml->clusters = calloc(krad_ebml->cluster_recording_space, sizeof(krad_ebml_cluster_t));
		krad_ebml_read_ebml_header (krad_ebml, krad_ebml->header);
		krad_ebml_check_ebml_header (krad_ebml->header);
		//krad_ebml_print_ebml_header (krad_ebml->header);
		krad_ebml_read_packet (krad_ebml, NULL, NULL, NULL);
	}

	if (krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) {
		krad_ebml->io_adapter.write_buffer = malloc(KRADEBML_WRITE_BUFFER_SIZE);
		krad_ebml->record_cluster_info = 1;
		krad_ebml->cluster_recording_space = CLUSTER_RECORDING_START_SIZE;
		krad_ebml->clusters = calloc(krad_ebml->cluster_recording_space, sizeof(krad_ebml_cluster_t));		
	}
	
	return krad_ebml;

} 

krad_ebml_t *krad_ebml_open_buffer(krad_ebml_io_mode_t mode) {

	krad_ebml_t *krad_ebml;
	
	krad_ebml = krad_ebml_create();

	krad_ebml->io_adapter.mode = mode;
	krad_ebml->io_adapter.read = krad_ebml_io_buffer_read;
	krad_ebml->io_adapter.write = krad_ebml_io_buffer_write;	

	if (krad_ebml->io_adapter.mode == KRAD_EBML_IO_READONLY) {
		krad_ebml->io_adapter.buffer_io_buffer = malloc (4096 * 6);
		krad_ebml->record_cluster_info = 1;
		krad_ebml->cluster_recording_space = CLUSTER_RECORDING_START_SIZE;
		krad_ebml->clusters = calloc(krad_ebml->cluster_recording_space, sizeof(krad_ebml_cluster_t));
		krad_ebml->tracks = calloc(10, sizeof(krad_ebml_track_t));
	}

	if (krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) {
		krad_ebml->io_adapter.write_buffer = malloc(KRADEBML_WRITE_BUFFER_SIZE);
	}
	
	return krad_ebml;

}

krad_ebml_t *krad_ebml_open_active_socket (int socket, krad_ebml_io_mode_t mode) {

	krad_ebml_t *krad_ebml;
	
	krad_ebml = krad_ebml_create();

	krad_ebml->record_cluster_info = 0;
	//krad_ebml->cluster_recording_space = CLUSTER_RECORDING_START_SIZE;
	//krad_ebml->clusters = calloc(krad_ebml->cluster_recording_space, sizeof(krad_ebml_cluster_t));
	krad_ebml->io_adapter.mode = mode;
	
	if ((krad_ebml->io_adapter.mode == KRAD_EBML_IO_WRITEONLY) || (krad_ebml->io_adapter.mode == KRAD_EBML_IO_READWRITE)) {
		krad_ebml->io_adapter.write_buffer = malloc(KRADEBML_WRITE_BUFFER_SIZE);
	}
	
	//krad_ebml->io_adapter.seekable = 1;
	
	krad_ebml->io_adapter.read = krad_ebml_streamio_read;
	krad_ebml->io_adapter.write = krad_ebml_streamio_write;
	
	krad_ebml->stream = 1;
	
	krad_ebml->io_adapter.sd = socket;
		
	return krad_ebml;

}


int krad_ebml_track_count(krad_ebml_t *krad_ebml) {
	return krad_ebml->track_count;
}


krad_codec_t krad_ebml_track_codec (krad_ebml_t *krad_ebml, int track) {
	return krad_ebml->tracks[track].codec;
}

int krad_ebml_track_header_count (krad_ebml_t *krad_ebml, int track) {
	return krad_ebml->tracks[track].headers;
}

int krad_ebml_track_header_size (krad_ebml_t *krad_ebml, int track, int header) {
	return krad_ebml->tracks[track].header_len[header];
}

int krad_ebml_read_track_header(krad_ebml_t *krad_ebml, unsigned char *buffer, int track, int header) {

	//if (krad_ebml->tracks[track].codec_data_size) {
	//	memcpy(buffer, krad_ebml->tracks[track].codec_data, krad_ebml->tracks[track].codec_data_size);
	//}

	//return krad_ebml->tracks[track].codec_data_size;
	if (krad_ebml->tracks[track].codec_data_size) {
		memcpy(buffer, krad_ebml->tracks[track].header[header], krad_ebml->tracks[track].header_len[header]);
		return krad_ebml->tracks[track].header_len[header];
	}

	return 0;

}


int krad_ebml_track_active (krad_ebml_t *krad_ebml, int track) {

	if (krad_ebml->tracks[track].codec != NOCODEC) {
		return 1;
	}

	return 0;
}

int krad_ebml_track_changed (krad_ebml_t *krad_ebml, int track) {
	if (krad_ebml->tracks[track].changed == 1) {
		krad_ebml->tracks[track].changed = 0;
		return 1;
	}
	return 0;
}

