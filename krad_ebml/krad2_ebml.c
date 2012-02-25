#include "krad2_ebml.h"


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

void krad2_ebml_base64_encode(char *dest, char *src) {

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

void krad2_ebml_write_reversed (krad2_ebml_t *krad2_ebml, void *buffer, uint32_t len) {

	char x;
	int i;
	unsigned char buffer_rev[8];
	
	rmemcpy(buffer_rev, buffer, len);
	krad2_ebml_write(krad2_ebml, buffer_rev, len);
}

void krad2_ebml_write_data_size_update (krad2_ebml_t *krad2_ebml, uint64_t data_size) {

    data_size |= (0x000000000000080LLU << ((EBML_DATA_SIZE_UNKNOWN_LENGTH - 1) * 7));

    krad2_ebml_write_reversed(krad2_ebml, (void *)&data_size, EBML_DATA_SIZE_UNKNOWN_LENGTH);

}

void krad2_ebml_write_data_size (krad2_ebml_t *krad2_ebml, uint64_t data_size) {

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

    krad2_ebml_write_reversed(krad2_ebml, (void *)&data_size, data_size_length);
}

void krad2_ebml_write_data (krad2_ebml_t *krad2_ebml, uint64_t element, void *data, uint64_t length) {

	krad2_ebml_write_element (krad2_ebml, element);
	krad2_ebml_write_data_size (krad2_ebml, length);
    krad2_ebml_write (krad2_ebml, data, length);

}

void krad2_ebml_write_string (krad2_ebml_t *krad2_ebml, uint64_t element, char *string) {

	uint64_t size;
	
	size = strlen(string);

	krad2_ebml_write_element (krad2_ebml, element);
	krad2_ebml_write_data_size (krad2_ebml, size);
    krad2_ebml_write (krad2_ebml, string, strlen(string));
}


void krad2_ebml_write_int64 (krad2_ebml_t *krad2_ebml, uint64_t element, int64_t number) {

	krad2_ebml_write_element (krad2_ebml, element);
	krad2_ebml_write_data_size (krad2_ebml, 8);
    krad2_ebml_write_reversed (krad2_ebml, &number, 8);
}

void krad2_ebml_write_int32 (krad2_ebml_t *krad2_ebml, uint64_t element, int32_t number) {

	krad2_ebml_write_element (krad2_ebml, element);
	krad2_ebml_write_data_size (krad2_ebml, 4);
    krad2_ebml_write_reversed (krad2_ebml, &number, 4);
}

void krad2_ebml_write_int16 (krad2_ebml_t *krad2_ebml, uint64_t element, int16_t number) {

	krad2_ebml_write_element (krad2_ebml, element);
	krad2_ebml_write_data_size (krad2_ebml, 2);
    krad2_ebml_write_reversed (krad2_ebml, &number, 2);
}

void krad2_ebml_write_int8 (krad2_ebml_t *krad2_ebml, uint64_t element, int8_t number) {

	krad2_ebml_write_element (krad2_ebml, element);
	krad2_ebml_write_data_size (krad2_ebml, 1);
    krad2_ebml_write_reversed (krad2_ebml, &number, 1);
}

void krad2_ebml_write_float (krad2_ebml_t *krad2_ebml, uint64_t element, float number) {

	krad2_ebml_write_element (krad2_ebml, element);
	krad2_ebml_write_data_size (krad2_ebml, 4);
    krad2_ebml_write_reversed (krad2_ebml, &number, 4);
}

void krad2_ebml_write_double (krad2_ebml_t *krad2_ebml, uint64_t element, double number) {

	krad2_ebml_write_element (krad2_ebml, element);
	krad2_ebml_write_data_size (krad2_ebml, 8);
    krad2_ebml_write_reversed (krad2_ebml, &number, 8);
}



void krad2_ebml_write_element (krad2_ebml_t *krad2_ebml, uint32_t element) {

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

	krad2_ebml_write_reversed(krad2_ebml, (void *)&element, element_len);
}

void krad2_ebml_start_element (krad2_ebml_t *krad2_ebml, uint32_t element, uint64_t *position) {

	krad2_ebml_write_element (krad2_ebml, element);
	*position = krad2_ebml_tell(krad2_ebml);
	krad2_ebml_write_data_size (krad2_ebml, EBML_DATA_SIZE_UNKNOWN);
	
	printf("position is %zu\n", *position);
	
}

void krad2_ebml_finish_element (krad2_ebml_t *krad2_ebml, uint64_t element_position) {

	uint64_t current_position;
	uint64_t element_data_size;

	current_position = krad2_ebml_tell(krad2_ebml);
	element_data_size = current_position - element_position - EBML_DATA_SIZE_UNKNOWN_LENGTH;
	printf("position is %zu\n", krad2_ebml_tell(krad2_ebml));
	krad2_ebml_seek(krad2_ebml, element_position, SEEK_SET);
	printf("position is %zu\n", krad2_ebml_tell(krad2_ebml));
	krad2_ebml_write_data_size_update (krad2_ebml, element_data_size);
	printf("position is %zu\n", krad2_ebml_tell(krad2_ebml));
	krad2_ebml_seek(krad2_ebml, current_position, SEEK_SET);
	printf("position is %zu\n", krad2_ebml_tell(krad2_ebml));
}

void krad2_ebml_header (krad2_ebml_t *krad2_ebml, char *doctype, char *appversion) {
    
    
	uint64_t ebml_header;

	krad2_ebml_start_element (krad2_ebml, EBML_ID_HEADER, &ebml_header);
	
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_EBMLVERSION, 1);	
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_EBMLREADVERSION, 1);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_EBMLMAXIDLENGTH, 4);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_EBMLMAXSIZELENGTH, 8);
	krad2_ebml_write_string (krad2_ebml, EBML_ID_DOCTYPE, doctype);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_DOCTYPEVERSION, 2);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_DOCTYPEREADVERSION, 2);
	krad2_ebml_finish_element (krad2_ebml, ebml_header);

	krad2_ebml_start_segment(krad2_ebml, appversion);

}

void kradebml_end_segment(krad2_ebml_t *krad2_ebml) {

	krad2_ebml_finish_element (krad2_ebml, krad2_ebml->segment);

}

void krad2_ebml_start_segment(krad2_ebml_t *krad2_ebml, char *appversion) {

	uint64_t segment_info;
	char version_string[32];

	strcpy(version_string, "Krad EBML ");
	strcat(version_string, KRADEBML_VERSION);

	krad2_ebml_start_element (krad2_ebml, EBML_ID_SEGMENT, &krad2_ebml->segment);

	krad2_ebml_start_element (krad2_ebml, EBML_ID_SEGMENT_INFO, &segment_info);
	krad2_ebml_write_string (krad2_ebml, EBML_ID_SEGMENT_TITLE, "A Krad Production");
	krad2_ebml_write_int32 (krad2_ebml, EBML_ID_TIMECODESCALE, 1000000);
	krad2_ebml_write_string (krad2_ebml, EBML_ID_MUXINGAPP, version_string);
	krad2_ebml_write_string (krad2_ebml, EBML_ID_WRITINGAPP, appversion);
	krad2_ebml_finish_element (krad2_ebml, segment_info);

	krad2_ebml_start_element (krad2_ebml, EBML_ID_SEGMENT_TRACKS, &krad2_ebml->tracks_info);

}



int krad2_ebml_new_tracknumber(krad2_ebml_t *krad2_ebml) {

	int tracknumber;
	
	tracknumber = krad2_ebml->current_track;

	krad2_ebml->track_count++;
	krad2_ebml->current_track++;
	
	return tracknumber;

}

int krad2_ebml_add_video_track_with_private_data (krad2_ebml_t *krad2_ebml, char *codec_id, int frame_rate, int width, int height, unsigned char *private_data, int private_data_size) {

	int track_number;
	uint64_t track_info;
	uint64_t video_info;

	krad2_ebml->total_video_frames = -1;
	krad2_ebml->video_frame_rate = frame_rate;

	track_number = krad2_ebml_new_tracknumber(krad2_ebml);

	krad2_ebml_start_element (krad2_ebml, EBML_ID_TRACK, &track_info);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_TRACKNUMBER, track_number);
	// Track UID?
	krad2_ebml_write_string (krad2_ebml, EBML_ID_CODECID, codec_id);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_TRACKTYPE, 1);

	if (private_data_size > 0) {
		krad2_ebml_write_data (krad2_ebml, EBML_ID_CODECDATA, private_data, private_data_size);
	}

	krad2_ebml_start_element (krad2_ebml, EBML_ID_VIDEOSETTINGS, &video_info);
	krad2_ebml_write_int16 (krad2_ebml, EBML_ID_VIDEOWIDTH, width);
	krad2_ebml_write_int16 (krad2_ebml, EBML_ID_VIDEOHEIGHT, height);
	// StereoMode ?
	krad2_ebml_finish_element (krad2_ebml, video_info);
	krad2_ebml_finish_element (krad2_ebml, track_info);	

	return track_number;

}

int krad2_ebml_add_video_track(krad2_ebml_t *krad2_ebml, char *codec_id, int frame_rate, int width, int height) {
	return krad2_ebml_add_video_track_with_private_data(krad2_ebml, codec_id, frame_rate, width, height, NULL, 0);
}

int krad2_ebml_add_subtitle_track(krad2_ebml_t *krad2_ebml, char *codec_id) {

	int track_number;
	uint64_t track_info;
	
	track_number = krad2_ebml_new_tracknumber(krad2_ebml);

	krad2_ebml_start_element (krad2_ebml, EBML_ID_TRACK, &track_info);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_TRACKNUMBER, track_number);
	// Track UID?
	krad2_ebml_write_string (krad2_ebml, EBML_ID_CODECID, codec_id);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_TRACKTYPE, 0x11);
	krad2_ebml_finish_element (krad2_ebml, track_info);


	return track_number;

}

int krad2_ebml_add_audio_track(krad2_ebml_t *krad2_ebml, char *codec_id, float sample_rate, int channels, unsigned char *private_data, int private_data_size) {

	int track_number;
	uint64_t track_info;
	uint64_t audio_info;

	krad2_ebml->audio_sample_rate = sample_rate;
	//	kradebml->audio_channels = channels;

	track_number = krad2_ebml_new_tracknumber(krad2_ebml);

	krad2_ebml_start_element (krad2_ebml, EBML_ID_TRACK, &track_info);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_TRACKNUMBER, track_number);
	// Track UID?
	krad2_ebml_write_string (krad2_ebml, EBML_ID_CODECID, codec_id);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_TRACKTYPE, 2);

	krad2_ebml_start_element (krad2_ebml, EBML_ID_AUDIOSETTINGS, &audio_info);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_AUDIOCHANNELS, channels);
	krad2_ebml_write_float (krad2_ebml, EBML_ID_AUDIOSAMPLERATE, sample_rate);
	krad2_ebml_write_int8 (krad2_ebml, EBML_ID_AUDIOBITDEPTH, 16);
	krad2_ebml_finish_element (krad2_ebml, audio_info);
	
	if (private_data_size > 0) {
		krad2_ebml_write_data (krad2_ebml, EBML_ID_CODECDATA, private_data, private_data_size);
	}

	krad2_ebml_finish_element (krad2_ebml, track_info);

	return track_number;

}

void krad2_ebml_add_video(krad2_ebml_t *krad2_ebml, int track_num, unsigned char *buffer, int buffer_len, int keyframe) {

    uint32_t block_length;
    unsigned char track_number;
    unsigned short block_timecode;
    unsigned char flags;
	int64_t timecode;

	flags = 0;
	block_timecode = 0;
	krad2_ebml->total_video_frames += 1;

    block_length = buffer_len + 4;
    block_length |= 0x10000000;

    track_number = track_num;
    track_number |= 0x80;

    if (keyframe) {
		flags |= 0x80;
	}
	
	timecode = krad2_ebml->total_video_frames * 1000 * (uint64_t)1 / krad2_ebml->video_frame_rate;
	block_timecode = timecode - krad2_ebml->cluster_timecode;	

	if (keyframe) {
		krad2_ebml_cluster(krad2_ebml, timecode);
	}

    krad2_ebml_write_element (krad2_ebml, EBML_ID_SIMPLEBLOCK);
    
	krad2_ebml_write_reversed(krad2_ebml, &block_length, 4);

	krad2_ebml_write(krad2_ebml, &track_number, 1);
	krad2_ebml_write_reversed(krad2_ebml, &block_timecode, 2);
	
	krad2_ebml_write(krad2_ebml, &flags, 1);
	krad2_ebml_write(krad2_ebml, buffer, buffer_len);
}

void krad2_ebml_add_audio(krad2_ebml_t *krad2_ebml, int track_num, unsigned char *buffer, int buffer_len, int frames) {

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

	if (krad2_ebml->total_audio_frames > 0) {
		timecode = (krad2_ebml->total_audio_frames)/ krad2_ebml->audio_sample_rate * 1000;
	} else {
		timecode = 0;
	}
	
	krad2_ebml->total_audio_frames += frames;
	krad2_ebml->audio_frames_since_cluster += frames;

	if ((timecode == 0) && (krad2_ebml->track_count == 1)) {
			krad2_ebml_cluster(krad2_ebml, timecode);
	}

	if ((krad2_ebml->audio_frames_since_cluster >= krad2_ebml->audio_sample_rate) && (krad2_ebml->track_count == 1)) {
			krad2_ebml_cluster(krad2_ebml, timecode);
			krad2_ebml->audio_frames_since_cluster = 0;
	}

	block_timecode = timecode - krad2_ebml->cluster_timecode;

    krad2_ebml_write_element (krad2_ebml, EBML_ID_SIMPLEBLOCK);

	krad2_ebml_write_reversed(krad2_ebml, &block_length, 4);

	krad2_ebml_write(krad2_ebml, &track_number, 1);
	krad2_ebml_write_reversed(krad2_ebml, &block_timecode, 2);
	
	krad2_ebml_write(krad2_ebml, &flags, 1);
	krad2_ebml_write(krad2_ebml, buffer, buffer_len);
}

void krad2_ebml_cluster(krad2_ebml_t *krad2_ebml, int64_t timecode) {

	if (krad2_ebml->tracks_info != 0) {
		krad2_ebml_finish_element (krad2_ebml, krad2_ebml->tracks_info);
		krad2_ebml->tracks_info = 0;
		krad2_ebml_sync (krad2_ebml);
	}

	if (krad2_ebml->cluster != 0) {
		krad2_ebml_finish_element (krad2_ebml, krad2_ebml->cluster);
		krad2_ebml_sync (krad2_ebml);
	}

	krad2_ebml->cluster_timecode = timecode;

	krad2_ebml_start_element (krad2_ebml, EBML_ID_CLUSTER, &krad2_ebml->cluster);
	krad2_ebml_write_int32 (krad2_ebml, EBML_ID_CLUSTER_TIMECODE, krad2_ebml->cluster_timecode);
}



void krad2_ebml_sync (krad2_ebml_t *krad2_ebml) { 

	

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
		case EBML_ID_TRACKNUMBER:
			return "Track Number";
		case EBML_ID_AUDIOBITDEPTH:
			return "Bit Depth";		
		case EBML_ID_BLOCKGROUP:
			return "Block Group";
		case EBML_ID_3D:
			return "3D Mode";			
		default:
			printf("%x", element_id);
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

int krad2_ebml_read_xiph_lace_value ( unsigned char *bytes, int *bytes_read ) {

	unsigned char byte;
	unsigned int value;
	int maxlen;
	int offset;
	
	offset = 0;
	value = 0;
	maxlen = 10;
	
	while (maxlen--) {
		*bytes_read += 1;
		byte = bytes[0] + offset;
		if (byte != 255) {
	
			value += byte;
		
			printf("xiph lace value is %d\n", value);
			return value;
	
		} else {
			value += 255;
			offset += 1;
		}
	}
}


int krad2_ebml_read_simpleblock( krad2_ebml_t *krad2_ebml, int len , int *tracknumber, unsigned char *buffer) {

	int tracknum;
	short timecode;
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
	timecode = 0;
	tracknum = 0;

	krad2_ebml_read ( krad2_ebml, &byte, 1 );

	tracknum = (byte - 0x80);	
	*tracknumber = tracknum;
	//printf("tracknum is %d\n", tracknum);
	
	krad2_ebml_read ( krad2_ebml, &temp, 2 );
	rmemcpy ( &timecode, &temp, 2);
	
	krad2_ebml->last_timecode = timecode;
	
	printf("timecode is %6.3f\n", ((krad2_ebml->current_cluster_timecode + (int64_t)timecode)/1000.0));
//	fflush(stdout);
	krad2_ebml_read ( krad2_ebml, &flags, 1 );
	
//	printf("flags is %x\n", flags);
	
	if ((flags & 0x04) && (flags & 0x02)) {
	//	printf("EBML Lacing\n");
		lacing = 1;
	}

	if ((!(flags & 0x04)) && (flags & 0x02)) {
	///	printf("Xiph Lacing\n");
		lacing = 2;
	}
	
	if ((!(flags & 0x04)) && (!(flags & 0x02))) {
	//	printf("No Lacing\n");
	}
	
	if ((flags & 0x04) && (!(flags & 0x02))) {
	//	printf("Fixed Lacing\n");
		lacing = 3;
	}

//	krad2_ebml_seek ( krad2_ebml, len - 4, SEEK_CUR );

	krad2_ebml->current_timecode = krad2_ebml->current_cluster_timecode + (int64_t)krad2_ebml->last_timecode;

	if (lacing == 0) {
		return krad2_ebml_read ( krad2_ebml, buffer, len - 4 );
	} else {
	
		krad2_ebml_read ( krad2_ebml, &laced_frames, 1 );
		
		block_bytes_read = 5;
		
		laced_frames += 1;
		
		framecount = 0;
		
		//printf("%u laced frames\n", laced_frames);
							
			if (lacing == 1) {
			

				while (framecount != laced_frames - 1) {
					block_bytes_read += krad2_ebml_read ( krad2_ebml, &byte, 1 );
					frame_size_len = ebml_length(byte);
			
					//printf("length of frame size one is %u\n", frame_size_len);
					
					
					memset(temp, 0, 8);
					
					// data size
					if (frame_size_len > 1) {
						ret = krad2_ebml_read ( krad2_ebml, &temp, frame_size_len - 1 );
						if (ret != frame_size_len - 1) {
							printf("Failurey reading %d\n", ret);
							exit(1);
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
					
					krad2_ebml->frame_sizes[framecount] = frame_size;
					
					total_size_of_frames += frame_size;

					framecount++;
				}
				
				
				
				frame_size = len - block_bytes_read - total_size_of_frames;
				//frame_size = last_frame_size + frame_size;
				//printf("last frame size is %u\n", frame_size);
				
				krad2_ebml->frame_sizes[framecount] = frame_size;
				
				krad2_ebml->read_laced_frames = framecount;
				
				krad2_ebml->current_laced_frame = 0;
				
				//printf("reading first ebml laced frame %u bytes\n", krad2_ebml->frame_sizes[krad2_ebml->current_laced_frame]);
				
				return krad2_ebml_read ( krad2_ebml, buffer, krad2_ebml->frame_sizes[krad2_ebml->current_laced_frame] );
			}
			
			if (lacing == 2) {
			
			
				while (framecount != laced_frames - 1) {
				
					int bytes = 0;
					
					block_bytes_read += krad2_ebml_read ( krad2_ebml, &byte, 1 );
					if (byte != 255) {
					
						bytes += byte;
						
						
						//printf("frame size is %d\n", bytes);
						krad2_ebml->frame_sizes[framecount] = bytes;
						total_size_of_frames += bytes;
						framecount++;
					
					} else {
						bytes += 255;
					}
				
				
				}
			
			
				frame_size = len - block_bytes_read - total_size_of_frames;
				//frame_size = last_frame_size + frame_size;
				//printf("last frame size is %u\n", frame_size);
				
				krad2_ebml->frame_sizes[framecount] = frame_size;
				
				krad2_ebml->read_laced_frames = framecount;
				
				krad2_ebml->current_laced_frame = 0;
				
				//printf("reading first xiph laced frame %u bytes\n", krad2_ebml->frame_sizes[krad2_ebml->current_laced_frame]);
				
				return krad2_ebml_read ( krad2_ebml, buffer, krad2_ebml->frame_sizes[krad2_ebml->current_laced_frame] );
			
			
			}
			
			if (lacing == 3) {
			
			

				
				krad2_ebml->read_laced_frames = laced_frames - 1;
				framecount = laced_frames;
				while (framecount ) {
					framecount--;
					krad2_ebml->frame_sizes[framecount] = (len - 5) / laced_frames;
				}
				
							
				krad2_ebml->current_laced_frame = 0;
				
				//printf("reading first fixed laced frame %u bytes\n", krad2_ebml->frame_sizes[krad2_ebml->current_laced_frame]);
				
				return krad2_ebml_read ( krad2_ebml, buffer, krad2_ebml->frame_sizes[krad2_ebml->current_laced_frame] );
			
			
			}

	
	
	
	
	
	}
	
	
}

int krad2_ebml_read_packet (krad2_ebml_t *krad2_ebml, int *tracknumber, unsigned char *buffer) {

	int ret;
	uint32_t ebml_id;
	uint32_t ebml_id_length;
	uint64_t ebml_data_size;
	uint32_t ebml_data_size_length;
	
	unsigned char byte;
	unsigned char temp[7];

	int skip;
	
	char string[512];
	memset(string, '\0', sizeof(string));
	
	int number;
	int known;
	int tracks_size;
	int tracks_pos;
	tracks_size = 0;
	tracks_pos = 0;
	known = 0;
	number = 0;
	
	skip = 1;

	while (1) {
	
		if ((krad2_ebml->header_read == 0) && (tracks_size > 0) && (tracks_pos == tracks_size)) {
				krad2_ebml->header_read = 1;
				return 0;
		}
	
	
		if (krad2_ebml->read_laced_frames) {

			krad2_ebml->current_laced_frame += 1;
			krad2_ebml->read_laced_frames -= 1;
			printf("reading %d laced frame %zu bytes\n", krad2_ebml->current_laced_frame, krad2_ebml->frame_sizes[krad2_ebml->current_laced_frame]);
			return krad2_ebml_read ( krad2_ebml, buffer, krad2_ebml->frame_sizes[krad2_ebml->current_laced_frame] );
		}
	
	
		if (!(ret = krad2_ebml_read ( krad2_ebml, &byte, 1 )) > 0) {
			printf("ebml read ret was %d\n", ret);
			break;
		}
		
		if (tracks_size > 0) {
			tracks_pos += ret;
		}
	
		ebml_id = 0;
		ebml_data_size = 0;
		
		// ID length
		ebml_id_length = ebml_length ( byte );

		//printf("id length is %u\n", ebml_id_length);
		memcpy((unsigned char *)&ebml_id + (ebml_id_length - 1), &byte, 1);

		// ID
		if (ebml_id_length > 1) {
			ret = krad2_ebml_read ( krad2_ebml, &temp, ebml_id_length - 1 );
			if (tracks_size > 0) {
				tracks_pos += ret;
			}
			rmemcpy ( &ebml_id, &temp, ebml_id_length - 1);
		}

		// data size length
		ret = krad2_ebml_read ( krad2_ebml, &byte, 1 );
		if (ret != 1) {
			printf("Failurex reading %d\n", ret);
			exit(1);
		}
		if (tracks_size > 0) {
			tracks_pos += ret;
		}
		ebml_data_size_length = ebml_length ( byte );
		//printf("data size length is %u\n", ebml_data_size_length);

		// data size
		if (ebml_data_size_length > 1) {
			ret = krad2_ebml_read ( krad2_ebml, &temp, ebml_data_size_length - 1 );
			if (ret != ebml_data_size_length - 1) {
				printf("Failurey reading %d\n", ret);
				exit(1);
			}
			if (tracks_size > 0) {
				tracks_pos += ret;
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

		//printf("data size is %" PRIu64 "\n", ebml_data_size);

		if (ebml_id == EBML_ID_HEADER) {
			krad2_ebml->ebml_level = 0;
			skip = 0;
		}

		if (ebml_id == EBML_ID_SEGMENT) {
			krad2_ebml->ebml_level = 0;
			skip = 0;
		}

		if (ebml_id == EBML_ID_CLUSTER) {
			krad2_ebml->ebml_level = 1;
			skip = 0;
			krad2_ebml->cluster_count++;
			if (ebml_data_size > krad2_ebml->largest_cluster) {
				krad2_ebml->largest_cluster = ebml_data_size;
			}
			
			if (ebml_data_size < krad2_ebml->smallest_cluster) {
				krad2_ebml->smallest_cluster = ebml_data_size;
			}
			/*
			if (krad2_ebml->header_read == 0) {
				krad2_ebml->header_read = 1;
				int toseek;
				toseek = (4 + ebml_data_size_length) * -1;
				krad2_ebml_seek ( krad2_ebml, toseek, SEEK_CUR);
				return 0;
			}
			*/
		}
		
		if (ebml_id == EBML_ID_SEGMENT_INFO) {
			krad2_ebml->ebml_level = 1;
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_SEGMENT_TRACKS) {
			krad2_ebml->ebml_level = 1;
			skip = 0;
			tracks_size = ebml_data_size;
		}

		if (ebml_id == EBML_ID_TRACK) {
			krad2_ebml->ebml_level = 2;
			skip = 0;
			krad2_ebml->track_count++;
			krad2_ebml->current_track = krad2_ebml->track_count;
		}
				
		if (ebml_id == EBML_ID_TAG) {
			krad2_ebml->ebml_level = 1;
		}
		
		if (ebml_id == EBML_ID_SIMPLEBLOCK) {
			krad2_ebml->ebml_level = 2;
			krad2_ebml->block_count++;
			if (1) {
				skip = 0;
				return krad2_ebml_read_simpleblock( krad2_ebml, ebml_data_size, tracknumber, buffer );
			} else {
				skip = 1;
			}
		}

		if (ebml_id == EBML_ID_VIDEOSETTINGS) {
			krad2_ebml->ebml_level = 3;
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_AUDIOSETTINGS) {
			krad2_ebml->ebml_level = 3;
			skip = 0;
		}
		
		if (((ebml_id == EBML_ID_MUXINGAPP) || (ebml_id == EBML_ID_WRITINGAPP) || (ebml_id == EBML_ID_CODECID) || 
			(ebml_id == EBML_ID_DOCTYPE) || (ebml_id == EBML_ID_SEGMENT_TITLE)) 
			&& (ebml_data_size < sizeof(string))) {
			ret = krad2_ebml_read ( krad2_ebml, &string, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failurez reading %d\n", ret);
				exit(1);
			}
			if (tracks_size > 0) {
				tracks_pos += ret;
			}
			printf("%s", string);
			
			if (strncmp(string, "A_VORBIS", 8) == 0) {
				krad2_ebml->tracks[krad2_ebml->current_track].codec = VORBIS;
			}
					
			if (strncmp(string, "A_FLAC", 8) == 0) {
				krad2_ebml->tracks[krad2_ebml->current_track].codec = FLAC;
			}
			
			if (strncmp(string, "A_OPUS", 8) == 0) {
				krad2_ebml->tracks[krad2_ebml->current_track].codec = OPUS;
			}
			
			if (strncmp(string, "V_THEORA", 8) == 0) {
				krad2_ebml->tracks[krad2_ebml->current_track].codec = THEORA;
			}
			
			if (strncmp(string, "V_DIRAC", 8) == 0) {
				krad2_ebml->tracks[krad2_ebml->current_track].codec = DIRAC;
			}
			
			if (strncmp(string, "V_VP8", 8) == 0) {
				krad2_ebml->tracks[krad2_ebml->current_track].codec = VP8;
			}
				
			memset(string, '\0', sizeof(string));
			skip = 0;
			known = 1;
		}
		
		
		if (ebml_id == EBML_ID_CODECDATA) {
		
			krad2_ebml->tracks[krad2_ebml->current_track].codec_data = calloc(1, ebml_data_size);
			
			
			
			ret = krad2_ebml_read ( krad2_ebml, krad2_ebml->tracks[krad2_ebml->current_track].codec_data, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failurebx reading %d\n", ret);
				exit(1);
			} else {
				printf("read %d\n", ret);
			}
			if (tracks_size > 0) {
				tracks_pos += ret;
			}
			krad2_ebml->tracks[krad2_ebml->current_track].codec_data_size = ebml_data_size;
			
			skip = 0;
			
			if ((krad2_ebml->tracks[krad2_ebml->current_track].codec == THEORA) || (krad2_ebml->tracks[krad2_ebml->current_track].codec == VORBIS)) {
			
				int bytes_read;
			
				bytes_read = 0;
			
				// first byte is the number of elements
				bytes_read = 1;

			
				krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[0] = krad2_ebml_read_xiph_lace_value ( krad2_ebml->tracks[krad2_ebml->current_track].codec_data + bytes_read, &bytes_read );
				krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[1] = krad2_ebml_read_xiph_lace_value ( krad2_ebml->tracks[krad2_ebml->current_track].codec_data + bytes_read, &bytes_read );
				krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[2] = krad2_ebml->tracks[krad2_ebml->current_track].codec_data_size - bytes_read - (krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[0] + krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[1]);
				
				krad2_ebml->tracks[krad2_ebml->current_track].xiph_header[0] = krad2_ebml->tracks[krad2_ebml->current_track].codec_data + bytes_read;
				krad2_ebml->tracks[krad2_ebml->current_track].xiph_header[1] = krad2_ebml->tracks[krad2_ebml->current_track].codec_data + bytes_read + krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[0];
				krad2_ebml->tracks[krad2_ebml->current_track].xiph_header[2] = krad2_ebml->tracks[krad2_ebml->current_track].codec_data + bytes_read + krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[0] + krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[1];



				printf("got xiph headers codec data size %zu -- %d %d %d\n",
						ebml_data_size,
						krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[0],
						krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[1],
						krad2_ebml->tracks[krad2_ebml->current_track].xiph_header_len[2]);
			
			}
			
		}
		

		if ((ebml_id == EBML_ID_VIDEOWIDTH) || (ebml_id == EBML_ID_VIDEOHEIGHT) ||
			(ebml_id == EBML_ID_AUDIOCHANNELS) || (ebml_id == EBML_ID_TRACKNUMBER) ||
			(ebml_id == EBML_ID_AUDIOBITDEPTH) || (ebml_id == EBML_ID_3D) || (ebml_id == EBML_ID_CLUSTER_TIMECODE)) {
			ret = krad2_ebml_read ( krad2_ebml, &temp, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failurea reading %d\n", ret);
				exit(1);
			}
			if (tracks_size > 0) {
				tracks_pos += ret;
			}
			rmemcpy ( &number, &temp, ebml_data_size);

			if (krad2_ebml->header_read == 0) {
				printf("%s %d", ebml_identify(ebml_id), number);
			}
			
			if (ebml_id == EBML_ID_CLUSTER_TIMECODE) {
				krad2_ebml->current_cluster_timecode = number;
			}
			
			if (ebml_id == EBML_ID_VIDEOWIDTH) {
				krad2_ebml->width = number;
			}
			
			if (ebml_id == EBML_ID_VIDEOHEIGHT) {
				krad2_ebml->height = number;
			}
			
			number = 0;
			skip = 0;
			known = 1;
		}
		
		if (ebml_id == EBML_ID_AUDIOSAMPLERATE) {
		
			float srate;
			double samplerate;
			
			ret = krad2_ebml_read ( krad2_ebml, &temp, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failureb reading %d\n", ret);
				exit(1);
			}
			if (tracks_size > 0) {
				tracks_pos += ret;
			}
			int adj = 0;
			if (ebml_data_size == 4) {
				rmemcpy ( &srate, &temp, ebml_data_size );
				printf("Sample Rate %f", srate);
			} else {
				rmemcpy ( &samplerate, &temp, ebml_data_size - adj);
				printf("Sample Rate %f", samplerate);
			}
	
			skip = 0;
			known = 1;
		}
		
		if (krad2_ebml->header_read == 0) {
			if (known == 0) {
				printf("%s size %" PRIu64 " ", ebml_identify(ebml_id), ebml_data_size);
				printf("\n");
			} else {
				printf("\n");
			}
		}
			
		if (skip) {
			if (krad2_ebml->stream == 1) {
				if (ebml_data_size != EBML_DATA_SIZE_UNKNOWN) {
					krad2_ebml_read ( krad2_ebml, krad2_ebml->bsbuffer, ebml_data_size);
				}
			} else {
				if (ebml_data_size != EBML_DATA_SIZE_UNKNOWN) {
					krad2_ebml_seek ( krad2_ebml, ebml_data_size, SEEK_CUR);
				}
			}
			if (tracks_size > 0) {
				tracks_pos += ebml_data_size;
			}
		}
		known = 0;
		skip = 1;

	}
	printf("\n");
	//printf("ret was %d\n", ret);
	
	printf("Smallest Cluster %d Largest Cluster %d\n", krad2_ebml->smallest_cluster, krad2_ebml->largest_cluster);
	printf("Track Count is %d Cluster Count %u Block Count %u\n", krad2_ebml->track_count, krad2_ebml->cluster_count, krad2_ebml->block_count);


	return 0;


}

int krad2_ebml_write(krad2_ebml_t *krad2_ebml, void *buffer, size_t length) {
	return krad2_ebml->io_adapter.write(&krad2_ebml->io_adapter, buffer, length);
}

int krad2_ebml_read(krad2_ebml_t *krad2_ebml, void *buffer, size_t length) {
	return krad2_ebml->io_adapter.read(&krad2_ebml->io_adapter, buffer, length);
}

int krad2_ebml_seek(krad2_ebml_t *krad2_ebml, int64_t offset, int whence) {
	return krad2_ebml->io_adapter.seek(&krad2_ebml->io_adapter, offset, whence);
}

int64_t krad2_ebml_tell(krad2_ebml_t *krad2_ebml) {
	return krad2_ebml->io_adapter.tell(&krad2_ebml->io_adapter);
}

int krad2_ebml_fileio_open(krad2_ebml_io_t *krad2_ebml_io)
{

	int fd;
	
	if (krad2_ebml_io->mode == KRAD2_EBML_IO_READONLY) {
		fd = open ( krad2_ebml_io->uri, O_RDONLY );
	}
	
	if (krad2_ebml_io->mode == KRAD2_EBML_IO_WRITEONLY) {
		fd = open ( krad2_ebml_io->uri, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	}
	
	//printf("fd is %d\n", fd);
	
	krad2_ebml_io->ptr = fd;

	return fd;
}

int krad2_ebml_fileio_close(krad2_ebml_io_t *krad2_ebml_io) {
	return close(krad2_ebml_io->ptr);
}

int krad2_ebml_fileio_write(krad2_ebml_io_t *krad2_ebml_io, void *buffer, size_t length) {
	return write(krad2_ebml_io->ptr, buffer, length);
}

int krad2_ebml_fileio_read(krad2_ebml_io_t *krad2_ebml_io, void *buffer, size_t length) {

	//printf("len is %zu\n", length);

	return read(krad2_ebml_io->ptr, buffer, length);

}

int64_t krad2_ebml_fileio_seek(krad2_ebml_io_t *krad2_ebml_io, int64_t offset, int whence) {
	return lseek(krad2_ebml_io->ptr, offset, whence);
}

int64_t krad2_ebml_fileio_tell(krad2_ebml_io_t *krad2_ebml_io) {
	return lseek(krad2_ebml_io->ptr, 0, SEEK_CUR);
}

int krad2_ebml_streamio_close(krad2_ebml_io_t *krad2_ebml_io) {
	return close(krad2_ebml_io->sd);
}

int krad2_ebml_streamio_write(krad2_ebml_io_t *krad2_ebml_io, void *buffer, size_t length) {

	int bytes;
	
	bytes = 0;

	while (bytes != length) {

		bytes += send (krad2_ebml_io->sd, buffer + bytes, length - bytes, 0);

		if (bytes <= 0) {
			fprintf(stderr, "Krad EBML Source: send Got Disconnected from server\n");
			exit(1);
		}
	}
	
	return bytes;
}


int krad2_ebml_streamio_read(krad2_ebml_io_t *krad2_ebml_io, void *buffer, size_t length) {

	int bytes;
	
	bytes = 0;

	while (bytes != length) {

		bytes += recv (krad2_ebml_io->sd, buffer + bytes, length - bytes, 0);

		if (bytes <= 0) {
			fprintf(stderr, "Krad EBML Source: recv Got Disconnected from server\n");
			exit(1);
		}
	}
	
	return bytes;
	
}


int krad2_ebml_streamio_open(krad2_ebml_io_t *krad2_ebml_io) {

	struct sockaddr_in serveraddr;
	struct hostent *hostp;
	char http_string[512];
	int http_string_pos;
	char content_type[64];
	char auth[256];
	char auth_base64[256];
	int sent;

	http_string_pos = 0;

	printf("Krad EBML: Connecting to %s:%d\n", krad2_ebml_io->host, krad2_ebml_io->port);

	if ((krad2_ebml_io->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Krad EBML Source: Socket Error\n");
		exit(1);
	}

	memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(krad2_ebml_io->port);
	
	if ((serveraddr.sin_addr.s_addr = inet_addr(krad2_ebml_io->host)) == (unsigned long)INADDR_NONE)
	{
		// get host address 
		hostp = gethostbyname(krad2_ebml_io->host);
		if(hostp == (struct hostent *)NULL)
		{
			printf("Krad EBML: Mount problem\n");
			close(krad2_ebml_io->sd);
			exit(1);
		}
		memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
	}

	// connect() to server. 
	if((sent = connect(krad2_ebml_io->sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
	{
		printf("Krad EBML Source: Connect Error\n");
		exit(1);
	} else {


		if (krad2_ebml_io->mode == KRAD2_EBML_IO_READONLY) {
	
			sprintf(http_string, "GET %s HTTP/1.0\r\n\r\n", krad2_ebml_io->mount);
	
			krad2_ebml_streamio_write(krad2_ebml_io, http_string, strlen(http_string));
	
			int end_http_headers = 0;
			char buf[8];
			while (end_http_headers != 4) {
	
				krad2_ebml_streamio_read(krad2_ebml_io, buf, 1);
	
				printf("%c", buf[0]);
	
				if ((buf[0] == '\n') || (buf[0] == '\r')) {
					end_http_headers++;
				} else {
					end_http_headers = 0;
				}
	
			}
		} 
		
		if (krad2_ebml_io->mode == KRAD2_EBML_IO_WRITEONLY) {
		
			strcpy(content_type, "video/webm");
			//strcpy(krad_mkvsource->content_type, "video/x-matroska");
			//strcpy(krad_mkvsource->content_type, "audio/mpeg");
			//strcpy(krad_mkvsource->content_type, "application/ogg");
			sprintf(auth, "source:%s", krad2_ebml_io->password );
			krad2_ebml_base64_encode( auth_base64, auth );
			http_string_pos = sprintf( http_string, "SOURCE /%s ICE/1.0\r\n", krad2_ebml_io->mount);
			http_string_pos += sprintf( http_string + http_string_pos, "content-type: %s\r\n", content_type);
			http_string_pos += sprintf( http_string + http_string_pos, "Authorization: Basic %s\r\n", auth_base64);
			http_string_pos += sprintf( http_string + http_string_pos, "\r\n");
		
			krad2_ebml_streamio_write(krad2_ebml_io, http_string, http_string_pos);
		
		}

	}

	return krad2_ebml_io->sd;
}


void krad2_ebml_destroy(krad2_ebml_t *krad2_ebml) {
	
	int t;

	if (krad2_ebml->io_adapter.mode != -1) {
		krad2_ebml->io_adapter.close(&krad2_ebml->io_adapter);
	}

	if (krad2_ebml->clusters != NULL) {
		free (krad2_ebml->clusters);
	}
	
	for (t = 0; t < krad2_ebml->track_count; t++) {
		if (krad2_ebml->tracks[t].codec_data != NULL) {
			printf("freeed on track t%d\n", t);
			free (krad2_ebml->tracks[t].codec_data);
		}
	}
	
	free(krad2_ebml->tracks);
	
	free (krad2_ebml);

}

krad2_ebml_t *krad2_ebml_create() {

	krad2_ebml_t *krad2_ebml = calloc(1, sizeof(krad2_ebml_t));
	krad2_ebml->current_track = 1;
	krad2_ebml->ebml_level = -1;
	krad2_ebml->io_adapter.mode = -1;
	krad2_ebml->smallest_cluster = 10000000;
	return krad2_ebml;

}

krad2_ebml_t *krad2_ebml_open_stream(char *host, int port, char *mount, char *password) {

	krad2_ebml_t *krad2_ebml;
	
	krad2_ebml = krad2_ebml_create();

	krad2_ebml->record_cluster_info = 0;
	//krad2_ebml->cluster_recording_space = 5000;
	//krad2_ebml->clusters = calloc(krad2_ebml->cluster_recording_space, sizeof(krad2_ebml_cluster_t));

	//krad2_ebml->io_adapter.seek = krad2_ebml_streamio_seek;
	if (password == NULL) {
		krad2_ebml->io_adapter.mode = KRAD2_EBML_IO_READONLY;
	} else {
		krad2_ebml->io_adapter.mode = KRAD2_EBML_IO_WRITEONLY;
	}
	//krad2_ebml->io_adapter.seekable = 1;
	krad2_ebml->io_adapter.read = krad2_ebml_streamio_read;
	krad2_ebml->io_adapter.write = krad2_ebml_streamio_write;
	krad2_ebml->io_adapter.open = krad2_ebml_streamio_open;
	krad2_ebml->io_adapter.close = krad2_ebml_streamio_close;
	krad2_ebml->io_adapter.uri = host;
	krad2_ebml->io_adapter.host = host;
	krad2_ebml->io_adapter.port = port;
	krad2_ebml->io_adapter.mount = mount;
	
	krad2_ebml->stream = 1;
	
	krad2_ebml->io_adapter.open(&krad2_ebml->io_adapter);
	
	krad2_ebml->tracks = calloc(10, sizeof(krad2_ebml_track_t));
	
	krad2_ebml_read_packet (krad2_ebml, NULL, NULL);
	
	return krad2_ebml;

}

krad2_ebml_t *krad2_ebml_open_file(char *filename, krad2_ebml_io_mode_t mode) {

	krad2_ebml_t *krad2_ebml;
	
	krad2_ebml = krad2_ebml_create();

	krad2_ebml->io_adapter.seek = krad2_ebml_fileio_seek;
	krad2_ebml->io_adapter.tell = krad2_ebml_fileio_tell;
	krad2_ebml->io_adapter.mode = mode;
	krad2_ebml->io_adapter.seekable = 1;
	krad2_ebml->io_adapter.read = krad2_ebml_fileio_read;
	krad2_ebml->io_adapter.write = krad2_ebml_fileio_write;
	krad2_ebml->io_adapter.open = krad2_ebml_fileio_open;
	krad2_ebml->io_adapter.close = krad2_ebml_fileio_close;
	krad2_ebml->io_adapter.uri = filename;
	krad2_ebml->io_adapter.open(&krad2_ebml->io_adapter);
	

	if (krad2_ebml->io_adapter.mode == KRAD2_EBML_IO_READONLY) {
		krad2_ebml->record_cluster_info = 1;
		krad2_ebml->cluster_recording_space = 5000;
		krad2_ebml->clusters = calloc(krad2_ebml->cluster_recording_space, sizeof(krad2_ebml_cluster_t));
	
		krad2_ebml->tracks = calloc(10, sizeof(krad2_ebml_track_t));
	
		krad2_ebml_read_packet (krad2_ebml, NULL, NULL);
	}
	
	return krad2_ebml;

} 

krad_codec_t krad2_ebml_get_track_codec (krad2_ebml_t *krad2_ebml, int tracknumber) {

	return krad2_ebml->tracks[tracknumber].codec;

}

int krad2_ebml_get_track_codec_data(krad2_ebml_t *krad2_ebml, int tracknumber, unsigned char *buffer) {

	if (krad2_ebml->tracks[tracknumber].codec_data_size) {
	
		memcpy(buffer, krad2_ebml->tracks[tracknumber].codec_data, krad2_ebml->tracks[tracknumber].codec_data_size);
	
	}

	return krad2_ebml->tracks[tracknumber].codec_data_size;

}


int krad2_ebml_get_track_count(krad2_ebml_t *krad2_ebml) {

	return krad2_ebml->track_count;

}

