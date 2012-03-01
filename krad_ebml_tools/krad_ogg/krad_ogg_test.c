#include "krad_ogg.h"

#define SLICE_SIZE 4096


int main (int argc, char *argv[]) {
	
	krad_ogg_t *krad_ogg;
	char *filename;
	//int fd;
	//int ret;
	//unsigned char *buffer;
	int h;
	int len;
	int tracknumber;
	unsigned char *packet_buffer;
	
	if (argc == 2) {
		filename = argv[1];
	} else {
		printf("please specify a file\n");
		exit(1);
	}
	
	//buffer = calloc(1, SLICE_SIZE);
	packet_buffer = calloc(1, 4096 * 128);
	
	//krad_ogg = krad_ogg_create();
	
	//fd = open(filename, O_RDONLY);
	//if (fd < 1) {
	//	printf("could not open file %s\n", filename);
	//	free(buffer);
	//	exit(1);
	//}
	
//	krad_ogg = krad_ogg_open_file(filename, KRAD_IO_READONLY);
	
	krad_ogg = krad_ogg_open_stream("space.rawdod.com", 80, "/chained2.ogg", NULL);	
	
	//while ((ret = read(fd, buffer, SLICE_SIZE)) > 0) {
	//	krad_ogg_write (krad_ogg, buffer, ret);
	
		while ((len = krad_ogg_read_packet (krad_ogg, &tracknumber, packet_buffer)) > 0) {
			printf("Read %d bytes for track number %d\n", len, tracknumber);
			
			if (krad_ogg_track_changed(krad_ogg, tracknumber)) {
				printf("track %d changed! status is %d header count is %d\n", tracknumber, krad_ogg_track_status(krad_ogg, tracknumber), krad_ogg_get_track_codec_header_count(krad_ogg, tracknumber));
				
				for (h = 0; h < krad_ogg_get_track_codec_header_count(krad_ogg, tracknumber); h++) {
					printf("header %d is %d bytes\n", h, krad_ogg_get_track_codec_header_data_size(krad_ogg, tracknumber, h));
				}

				
			}
			
		}
	//}
	
	krad_ogg_destroy(krad_ogg);

	//close(fd);
	free(packet_buffer);
	//free(buffer);
	
	printf("clean exit\n");
	
	return 0;
	
}
