#include "krad2_ebml.h"

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

void rmemcpy(void *dst, void *src, int len) {

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

int krad_ebml_read_simpleblock( krad_ebml_t *krad_ebml, int len , int *tracknumber, unsigned char *buffer) {

	int tracknum;
	short timecode;
	char temp[2];
	unsigned char byte;
	unsigned char flags;

	timecode = 0;
	tracknum = 0;

	krad_ebml_read ( krad_ebml, &byte, 1 );

	tracknum = (byte - 0x80);	
	*tracknumber = tracknum;
	//printf("tracknum is %d\n", tracknum);
	
	krad_ebml_read ( krad_ebml, &temp, 2 );
	rmemcpy ( &timecode, &temp, 2);
	
	krad_ebml->last_timecode = timecode;
	
//	printf("timecode is %6.3f\r", ((krad_ebml->current_cluster_timecode + (int64_t)timecode)/1000.0));
//	fflush(stdout);
	krad_ebml_read ( krad_ebml, &flags, 1 );
	
//	printf("\nflags is %x\n", flags);
	
//	krad_ebml_seek ( krad_ebml, len - 4, SEEK_CUR );

	return krad_ebml_read ( krad_ebml, buffer, len - 4 );

}

int krad_ebml_read_packet (krad_ebml_t *krad_ebml, int *tracknumber, unsigned char *buffer) {

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
	
	known = 0;
	number = 0;
	
	krad_ebml->smallest_cluster = 10000000;
	skip = 1;

	while ((ret = krad_ebml_read ( krad_ebml, &byte, 1 )) > 0) {
	
		ebml_id = 0;
		ebml_data_size = 0;
		
		// ID length
		ebml_id_length = ebml_length ( byte );

		//printf("id length is %u\n", ebml_id_length);
		memcpy((unsigned char *)&ebml_id + (ebml_id_length - 1), &byte, 1);

		// ID
		if (ebml_id_length > 1) {
			krad_ebml_read ( krad_ebml, &temp, ebml_id_length - 1 );
			rmemcpy ( &ebml_id, &temp, ebml_id_length - 1);
		}

		// data size length
		ret = krad_ebml_read ( krad_ebml, &byte, 1 );
		if (ret != 1) {
			printf("Failurex reading %d\n", ret);
			exit(1);
		}
		
		ebml_data_size_length = ebml_length ( byte );
		//printf("data size length is %u\n", ebml_data_size_length);

		// data size
		if (ebml_data_size_length > 1) {
			ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size_length - 1 );
			if (ret != ebml_data_size_length - 1) {
				printf("Failurey reading %d\n", ret);
				exit(1);
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
			krad_ebml->ebml_level = 0;
			skip = 0;
		}

		if (ebml_id == EBML_ID_SEGMENT) {
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
			
			if (krad_ebml->header_read == 0) {
				krad_ebml->header_read = 1;
				int toseek;
				toseek = (4 + ebml_data_size_length) * -1;
				krad_ebml_seek ( krad_ebml, toseek, SEEK_CUR);
				return 0;
			}
			
		}
		
		if (ebml_id == EBML_ID_SEGMENT_INFO) {
			krad_ebml->ebml_level = 1;
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_SEGMENT_TRACKS) {
			krad_ebml->ebml_level = 1;
			skip = 0;
		}

		if (ebml_id == EBML_ID_TRACK) {
			krad_ebml->ebml_level = 2;
			skip = 0;
			krad_ebml->track_count++;
			krad_ebml->current_track = krad_ebml->track_count;
		}
				
		if (ebml_id == EBML_ID_TAG) {
			krad_ebml->ebml_level = 1;
		}
		
		if (ebml_id == EBML_ID_SIMPLEBLOCK) {
			krad_ebml->ebml_level = 2;
			krad_ebml->block_count++;
			if (1) {
				skip = 0;
				return krad_ebml_read_simpleblock( krad_ebml, ebml_data_size, tracknumber, buffer );
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
				printf("Failurez reading %d\n", ret);
				exit(1);
			}
			
			printf("%s", string);
			
			if (strncmp(string, "A_VORBIS", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = VORBIS;
			}
					
			if (strncmp(string, "A_FLAC", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = FLAC;
			}
			
			if (strncmp(string, "A_OPUS", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = OPUS;
			}
			
			if (strncmp(string, "V_THEORA", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = THEORA;
			}
			
			if (strncmp(string, "V_DIRAC", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = DIRAC;
			}
			
			if (strncmp(string, "V_VP8", 8) == 0) {
				krad_ebml->tracks[krad_ebml->current_track].codec = VP8;
			}
				
			memset(string, '\0', sizeof(string));
			skip = 0;
			known = 1;
		}
		
		
		if (ebml_id == EBML_ID_CODECDATA) {
		
			krad_ebml->tracks[krad_ebml->current_track].codec_data = calloc(1, ebml_data_size);
			
			ret = krad_ebml_read ( krad_ebml, krad_ebml->tracks[krad_ebml->current_track].codec_data, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failurebx reading %d\n", ret);
				exit(1);
			}
			
			krad_ebml->tracks[krad_ebml->current_track].codec_data_size = ebml_data_size;
			
			skip = 0;
			
		}
		

		if ((ebml_id == EBML_ID_VIDEOWIDTH) || (ebml_id == EBML_ID_VIDEOHEIGHT) ||
			(ebml_id == EBML_ID_AUDIOCHANNELS) || (ebml_id == EBML_ID_TRACKNUMBER) ||
			(ebml_id == EBML_ID_AUDIOBITDEPTH) || (ebml_id == EBML_ID_3D) || (ebml_id == EBML_ID_CLUSTER_TIMECODE)) {
			ret = krad_ebml_read ( krad_ebml, &temp, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failurea reading %d\n", ret);
				exit(1);
			}
			
			rmemcpy ( &number, &temp, ebml_data_size);

			if (krad_ebml->header_read == 0) {
				printf("%s %d", ebml_identify(ebml_id), number);
			}
			
			if (ebml_id == EBML_ID_CLUSTER_TIMECODE) {
				krad_ebml->current_cluster_timecode = number;
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
				printf("Failureb reading %d\n", ret);
				exit(1);
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
		
		if (krad_ebml->header_read == 0) {
			if (known == 0) {
				printf("%s size %" PRIu64 " ", ebml_identify(ebml_id), ebml_data_size);
				printf("\n");
			} else {
				printf("\n");
			}
		}
			
		if (skip) {
			if (ebml_data_size != EBML_DATA_SIZE_UNKNOWN) {
				krad_ebml_seek ( krad_ebml, ebml_data_size, SEEK_CUR);
			}
		}
		known = 0;
		skip = 1;

	}
	printf("\n");
	//printf("ret was %d\n", ret);
	
	printf("Smallest Cluster %d Largest Cluster %d\n", krad_ebml->smallest_cluster, krad_ebml->largest_cluster);
	printf("Track Count is %d Cluster Count %u Block Count %u\n", krad_ebml->track_count, krad_ebml->cluster_count, krad_ebml->block_count);


	return 0;


}

int krad_ebml_write(krad_ebml_t *krad_ebml, void *buffer, size_t length)
{
  return krad_ebml->io_adapter.write(buffer, length, &krad_ebml->io_adapter);
}

int krad_ebml_read(krad_ebml_t *krad_ebml, void *buffer, size_t length)
{
  return krad_ebml->io_adapter.read(buffer, length, &krad_ebml->io_adapter);
}

int krad_ebml_seek(krad_ebml_t *krad_ebml, int64_t offset, int whence)
{
  return krad_ebml->io_adapter.seek(offset, whence, &krad_ebml->io_adapter);
}

int krad_ebml_fileio_open(krad_ebml_io_t *krad_ebml_io)
{

	int fd;
	
	if (krad_ebml_io->mode == KRAD_EBML_IO_READONLY) {
		fd = open ( krad_ebml_io->uri, O_RDONLY );
	}
	
	if (krad_ebml_io->mode == KRAD_EBML_IO_WRITEONLY) {
		fd = open ( krad_ebml_io->uri, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	}
	
	//printf("fd is %d\n", fd);
	
	krad_ebml_io->ptr = fd;

	return fd;
}

int krad_ebml_fileio_close(krad_ebml_io_t *krad_ebml_io)
{
	return close(krad_ebml_io->ptr);
}

int krad_ebml_fileio_write(void *buffer, size_t length, krad_ebml_io_t *krad_ebml_io)
{
	return write(krad_ebml_io->ptr, buffer, length);
}

int krad_ebml_fileio_read(void *buffer, size_t length, krad_ebml_io_t *krad_ebml_io)
{

	//printf("len is %zu\n", length);

	return read(krad_ebml_io->ptr, buffer, length);

}

int64_t krad_ebml_fileio_seek(int64_t offset, int whence, krad_ebml_io_t *krad_ebml_io)
{
	return lseek(krad_ebml_io->ptr, offset, whence);
}


void krad_ebml_destroy(krad_ebml_t *krad_ebml) {
	
	int t;

	if (krad_ebml->io_adapter.mode != -1) {
		krad_ebml->io_adapter.close(&krad_ebml->io_adapter);
	}

	if (krad_ebml->clusters != NULL) {
		free (krad_ebml->clusters);
	}
	
	for (t = 0; t < krad_ebml->track_count; t++) {
		if (krad_ebml->tracks[t].codec_data != NULL) {
			printf("freeed on track t%d\n", t);
			free (krad_ebml->tracks[t].codec_data);
		}
	}
	
	free(krad_ebml->tracks);
	
	free (krad_ebml);

}

krad_ebml_t *krad_ebml_create() {

	krad_ebml_t *krad_ebml = calloc(1, sizeof(krad_ebml_t));

	krad_ebml->ebml_level = -1;
	krad_ebml->io_adapter.mode = -1;
	
	return krad_ebml;

}


krad_ebml_t *krad_ebml_open_file(char *filename, krad_ebml_io_mode_t mode) {

	krad_ebml_t *krad_ebml;
	
	krad_ebml = krad_ebml_create();

	krad_ebml->record_cluster_info = 1;
	krad_ebml->cluster_recording_space = 5000;
	krad_ebml->clusters = calloc(krad_ebml->cluster_recording_space, sizeof(krad_ebml_cluster_t));

	krad_ebml->io_adapter.seek = krad_ebml_fileio_seek;
	krad_ebml->io_adapter.mode = mode;
	krad_ebml->io_adapter.seekable = 1;
	krad_ebml->io_adapter.read = krad_ebml_fileio_read;
	krad_ebml->io_adapter.write = krad_ebml_fileio_write;
	krad_ebml->io_adapter.open = krad_ebml_fileio_open;
	krad_ebml->io_adapter.close = krad_ebml_fileio_close;
	krad_ebml->io_adapter.uri = filename;
	krad_ebml->io_adapter.open(&krad_ebml->io_adapter);
	
	
	krad_ebml->tracks = calloc(10, sizeof(krad_ebml_track_t));
	
	krad_ebml_read_packet (krad_ebml, NULL, NULL);
	
	return krad_ebml;

} 

krad_codec_t krad_ebml_get_track_codec (krad_ebml_t *krad_ebml, int tracknumber) {

	return krad_ebml->tracks[tracknumber].codec;

}

int krad_ebml_get_track_codec_data(krad_ebml_t *krad_ebml, int tracknumber, unsigned char *buffer) {

	if (krad_ebml->tracks[tracknumber].codec_data_size) {
	
		memcpy(buffer, krad_ebml->tracks[tracknumber].codec_data, krad_ebml->tracks[tracknumber].codec_data_size);
	
	}

	return krad_ebml->tracks[tracknumber].codec_data_size;

}


int krad_ebml_get_track_count(krad_ebml_t *krad_ebml) {

	return krad_ebml->track_count;

}

