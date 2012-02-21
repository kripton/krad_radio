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

int krad2_ebml_read_simpleblock( krad2_ebml_t *krad2_ebml, int len , int *tracknumber, unsigned char *buffer) {

	int tracknum;
	short timecode;
	char temp[2];
	unsigned char byte;
	unsigned char flags;

	timecode = 0;
	tracknum = 0;

	krad2_ebml_read ( krad2_ebml, &byte, 1 );

	tracknum = (byte - 0x80);	
	*tracknumber = tracknum;
	//printf("tracknum is %d\n", tracknum);
	
	krad2_ebml_read ( krad2_ebml, &temp, 2 );
	rmemcpy ( &timecode, &temp, 2);
	
	krad2_ebml->last_timecode = timecode;
	
//	printf("timecode is %6.3f\r", ((krad2_ebml->current_cluster_timecode + (int64_t)timecode)/1000.0));
//	fflush(stdout);
	krad2_ebml_read ( krad2_ebml, &flags, 1 );
	
//	printf("\nflags is %x\n", flags);
	
//	krad2_ebml_seek ( krad2_ebml, len - 4, SEEK_CUR );

	krad2_ebml->current_timecode = krad2_ebml->current_cluster_timecode + (int64_t)krad2_ebml->last_timecode;

	return krad2_ebml_read ( krad2_ebml, buffer, len - 4 );

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
			}
			if (tracks_size > 0) {
				tracks_pos += ret;
			}
			krad2_ebml->tracks[krad2_ebml->current_track].codec_data_size = ebml_data_size;
			
			skip = 0;
			
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

int krad2_ebml_write(krad2_ebml_t *krad2_ebml, void *buffer, size_t length)
{
  return krad2_ebml->io_adapter.write(buffer, length, &krad2_ebml->io_adapter);
}

int krad2_ebml_read(krad2_ebml_t *krad2_ebml, void *buffer, size_t length)
{
  return krad2_ebml->io_adapter.read(buffer, length, &krad2_ebml->io_adapter);
}

int krad2_ebml_seek(krad2_ebml_t *krad2_ebml, int64_t offset, int whence)
{
  return krad2_ebml->io_adapter.seek(offset, whence, &krad2_ebml->io_adapter);
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

int krad2_ebml_fileio_close(krad2_ebml_io_t *krad2_ebml_io)
{
	return close(krad2_ebml_io->ptr);
}

int krad2_ebml_fileio_write(void *buffer, size_t length, krad2_ebml_io_t *krad2_ebml_io)
{
	return write(krad2_ebml_io->ptr, buffer, length);
}

int krad2_ebml_fileio_read(void *buffer, size_t length, krad2_ebml_io_t *krad2_ebml_io)
{

	//printf("len is %zu\n", length);

	return read(krad2_ebml_io->ptr, buffer, length);

}

int64_t krad2_ebml_fileio_seek(int64_t offset, int whence, krad2_ebml_io_t *krad2_ebml_io)
{
	return lseek(krad2_ebml_io->ptr, offset, whence);
}


int krad2_ebml_streamio_close(krad2_ebml_io_t *krad2_ebml_io)
{
	return close(krad2_ebml_io->sd);
}

int krad2_ebml_streamio_write(void *buffer, size_t length, krad2_ebml_io_t *krad2_ebml_io)
{
	return write(krad2_ebml_io->sd, buffer, length);
}


int krad2_ebml_streamio_read(void *buffer, size_t length, krad2_ebml_io_t *krad2_ebml_io)
{

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


int krad2_ebml_streamio_open(krad2_ebml_io_t *krad2_ebml_io)
{

	struct sockaddr_in serveraddr;
	struct hostent *hostp;
	int sent;

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

		char getstring[512];
	
		sprintf(getstring, "GET %s HTTP/1.0\r\n\r\n", krad2_ebml_io->mount);
	
		krad2_ebml_streamio_write(getstring, strlen(getstring), krad2_ebml_io);
	
		int end_http_headers = 0;
		char buf[8];
		while (end_http_headers != 4) {
	
			krad2_ebml_streamio_read(buf, 1, krad2_ebml_io);
	
			printf("%c", buf[0]);
	
			if ((buf[0] == '\n') || (buf[0] == '\r')) {
				end_http_headers++;
			} else {
				end_http_headers = 0;
			}
	
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

	krad2_ebml->ebml_level = -1;
	krad2_ebml->io_adapter.mode = -1;
	krad2_ebml->smallest_cluster = 10000000;
	return krad2_ebml;

}

krad2_ebml_t *krad2_ebml_open_stream(char *host, int port, char *mount) {

	krad2_ebml_t *krad2_ebml;
	
	krad2_ebml = krad2_ebml_create();

	krad2_ebml->record_cluster_info = 0;
	//krad2_ebml->cluster_recording_space = 5000;
	//krad2_ebml->clusters = calloc(krad2_ebml->cluster_recording_space, sizeof(krad2_ebml_cluster_t));

	//krad2_ebml->io_adapter.seek = krad2_ebml_streamio_seek;
	//krad2_ebml->io_adapter.mode = mode;
	//krad2_ebml->io_adapter.seekable = 1;
	krad2_ebml->io_adapter.read = krad2_ebml_streamio_read;
	//krad2_ebml->io_adapter.write = krad2_ebml_streamio_write;
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

	krad2_ebml->record_cluster_info = 1;
	krad2_ebml->cluster_recording_space = 5000;
	krad2_ebml->clusters = calloc(krad2_ebml->cluster_recording_space, sizeof(krad2_ebml_cluster_t));

	krad2_ebml->io_adapter.seek = krad2_ebml_fileio_seek;
	krad2_ebml->io_adapter.mode = mode;
	krad2_ebml->io_adapter.seekable = 1;
	krad2_ebml->io_adapter.read = krad2_ebml_fileio_read;
	krad2_ebml->io_adapter.write = krad2_ebml_fileio_write;
	krad2_ebml->io_adapter.open = krad2_ebml_fileio_open;
	krad2_ebml->io_adapter.close = krad2_ebml_fileio_close;
	krad2_ebml->io_adapter.uri = filename;
	krad2_ebml->io_adapter.open(&krad2_ebml->io_adapter);
	
	
	krad2_ebml->tracks = calloc(10, sizeof(krad2_ebml_track_t));
	
	krad2_ebml_read_packet (krad2_ebml, NULL, NULL);
	
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

