#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define EBML_LENGTH_1 0x80 // 10000000
#define EBML_LENGTH_2 0x40 // 01000000
#define EBML_LENGTH_3 0x20 // 00100000
#define EBML_LENGTH_4 0x10 // 00010000
#define EBML_LENGTH_5 0x08 // 00001000
#define EBML_LENGTH_6 0x04 // 00000100
#define EBML_LENGTH_7 0x02 // 00000010
#define EBML_LENGTH_8 0x01 // 00000001

#define EBML_DATA_SIZE_UNKNOWN 0xFFFFFFFFFFFFFFFF

#define EBML_ID_EBMLVERSION        0x4286
#define EBML_ID_EBMLREADVERSION    0x42F7
#define EBML_ID_EBMLMAXIDLENGTH    0x42F2
#define EBML_ID_EBMLMAXSIZELENGTH  0x42F3
#define EBML_ID_DOCTYPE            0x4282
#define EBML_ID_DOCTYPEVERSION     0x4287
#define EBML_ID_DOCTYPEREADVERSION 0x4285

#define EBML_ID_HEADER					0x1A45DFA3
#define EBML_ID_CLUSTER					0x1F43B675
#define EBML_ID_TRACK_UID				0x73c5
#define EBML_ID_TRACK_TYPE				0x83
#define EBML_ID_LANGUAGE				0x22b59c
#define EBML_ID_SEGMENT					0x18538067
#define EBML_ID_SEGMENT_TITLE			0x7BA9
#define EBML_ID_SEGMENT_INFO			0x1549A966
#define EBML_ID_SEGMENT_TRACKS			0x1654AE6B
#define EBML_ID_TAG						0x1254C367
#define EBML_ID_TRACK					0xAE

#define EBML_ID_CLUSTER_TIMECODE		0xE7
#define EBML_ID_SIMPLEBLOCK				0xA3

#define EBML_ID_DOCTYPE					0x4282
#define EBML_ID_MUXINGAPP 				0x4D80
#define EBML_ID_WRITINGAPP 				0x5741

#define EBML_ID_CODECID					0x86

#define EBML_ID_TRACKTYPE				0x83
#define EBML_ID_TRACKNUMBER				0xD7
#define EBML_ID_VIDEOWIDTH				0xB0
#define EBML_ID_VIDEOHEIGHT				0xBA

#define EBML_ID_VIDEOSETTINGS			0xE0
#define EBML_ID_AUDIOSETTINGS			0xE1

#define EBML_ID_AUDIOCHANNELS			0x9F
#define EBML_ID_AUDIOSAMPLERATE			0xB5
#define EBML_ID_AUDIOBITDEPTH			0x6264

#define EBML_ID_3D						0x53B8

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


void read_ebml(int fd) {

	int ret;
	uint32_t ebml_id;
	uint32_t ebml_id_length;
	uint64_t ebml_data_size;
	uint32_t ebml_data_size_length;
	
	unsigned char byte;
	unsigned char temp[7];

	int level;
	int skip;

	int l;
	
	char string[512];
	memset(string, '\0', sizeof(string));
	
	int track_count;
	int number;
	int known;
	
	known = 0;
	number = 0;
	
	track_count = 0;
	
	skip = 1;
	level = -1;

	while ((ret = read ( fd, &byte, 1 )) > 0) {
	
		ebml_id = 0;
		ebml_data_size = 0;
		
		// ID length
		ebml_id_length = ebml_length ( byte );

		//printf("id length is %u\n", ebml_id_length);
		memcpy((unsigned char *)&ebml_id + (ebml_id_length - 1), &byte, 1);

		// ID
		if (ebml_id_length > 1) {
			read ( fd, &temp, ebml_id_length - 1 );
			rmemcpy ( &ebml_id, &temp, ebml_id_length - 1);
		}

		// data size length
		ret = read ( fd, &byte, 1 );
		if (ret != 1) {
			printf("Failure reading\n");
			exit(1);
		}
		
		ebml_data_size_length = ebml_length ( byte );
		//printf("data size length is %u\n", ebml_data_size_length);

		// data size
		if (ebml_data_size_length > 1) {
			ret = read ( fd, &temp, ebml_data_size_length - 1 );
			if (ret != ebml_data_size_length - 1) {
				printf("Failure reading\n");
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
			level = 0;
			skip = 0;
		}

		if (ebml_id == EBML_ID_SEGMENT) {
			level = 0;
			skip = 0;
		}

		if (ebml_id == EBML_ID_CLUSTER) {
			level = 1;
			skip = 1;
		}
		
		if (ebml_id == EBML_ID_SEGMENT_INFO) {
			level = 1;
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_SEGMENT_TRACKS) {
			level = 1;
			skip = 0;
		}

		if (ebml_id == EBML_ID_TRACK) {
			level = 2;
			skip = 0;
		}
				
		if (ebml_id == EBML_ID_TAG) {
			level = 1;
		}
		
		if (ebml_id == EBML_ID_SIMPLEBLOCK) {
			level = 2;
			skip = 1;
		}

		if (ebml_id == EBML_ID_VIDEOSETTINGS) {
			level = 3;
			skip = 0;
		}
		
		if (ebml_id == EBML_ID_AUDIOSETTINGS) {
			level = 3;
			skip = 0;
		}	

		
		//for (l = level; l > 0; l--) {
		//	printf(" ");
		//}
		
		if (ebml_id == EBML_ID_TRACK) {	
			track_count++;
		}
		
		if (((ebml_id == EBML_ID_MUXINGAPP) || (ebml_id == EBML_ID_WRITINGAPP) || (ebml_id == EBML_ID_CODECID) || 
			(ebml_id == EBML_ID_DOCTYPE) || (ebml_id == EBML_ID_SEGMENT_TITLE)) 
			&& (ebml_data_size < sizeof(string))) {
			ret = read ( fd, &string, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failure reading\n");
				exit(1);
			}
			
			printf("%s", string);
			memset(string, '\0', sizeof(string));
			skip = 0;
			known = 1;
		}

		if ((ebml_id == EBML_ID_VIDEOWIDTH) || (ebml_id == EBML_ID_VIDEOHEIGHT) ||
			(ebml_id == EBML_ID_AUDIOCHANNELS) || (ebml_id == EBML_ID_TRACKNUMBER) ||
			(ebml_id == EBML_ID_AUDIOBITDEPTH) || (ebml_id == EBML_ID_3D)) {
			ret = read ( fd, &temp, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failure reading\n");
				exit(1);
			}
			
			rmemcpy ( &number, &temp, ebml_data_size);
			printf("%s %d", ebml_identify(ebml_id), number);
			
			number = 0;
			skip = 0;
			known = 1;
		}
		
		if (ebml_id == EBML_ID_AUDIOSAMPLERATE) {
		
			float srate;
			double samplerate;
			
			ret = read ( fd, &temp, ebml_data_size );
			if (ret != ebml_data_size) {
				printf("Failure reading\n");
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
		
		if (known == 0) {
			//printf("%s size %" PRIu64 " ", ebml_identify(ebml_id), ebml_data_size);
			//printf("\n");
		} else {
			printf("\n");
		}
			
		if (skip) {
			if (ebml_data_size != EBML_DATA_SIZE_UNKNOWN) {
				lseek(fd, ebml_data_size, SEEK_CUR);
			}
		}
		known = 0;
		skip = 1;

	}
	
	printf("Track Count is %d\n", track_count);

}


int main (int argc, char *argv[]) {
	
	int fd;
	char *filename;
	
	if (argc < 2) {
		printf("Please specify a file\n");
		exit(1);
	}
	
	if (argc == 2) {
		filename = argv[1];
	}
	
	fd = open ( filename, O_RDONLY );

	if (fd < 1) {
		printf("error opening file: %s\n", filename);
		exit(1);
	}
	
	read_ebml(fd);

	close (fd);

	return 0;	

}
