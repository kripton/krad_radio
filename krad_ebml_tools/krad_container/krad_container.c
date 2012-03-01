#include "krad_container.h"

krad_container_t krad_link_select_container(char *string) {

	if ((strstr(string, ".ogg")) ||
		(strstr(string, ".OGG")) ||
		(strstr(string, ".Ogg")) ||
		(strstr(string, ".oga")) ||
		(strstr(string, ".ogv")) ||
		(strstr(string, ".Oga")) ||		
		(strstr(string, ".OGV")))
	{
		return OGG;
	}
	
	return EBML;
}

/*

		if (krad_ogg_track_changed(krad_link->krad_ogg, current_track)) {
			track_codecs[current_track] = krad_ogg_get_track_codec(krad_link->krad_ogg, current_track);
			for (h = 0; h < krad_ogg_get_track_codec_header_count(krad_link->krad_ogg, current_track); h++) {
				printf("header %d is %d bytes\n", h, krad_ogg_get_track_codec_header_data_size(krad_link->krad_ogg, current_track, h));
				total_header_size += krad_ogg_get_track_codec_header_data_size(krad_link->krad_ogg, current_track, h);
					for (h = 0; h < krad_ogg_get_track_codec_header_count(krad_link->krad_ogg, current_track); h++) {
						header_size = krad_ogg_get_track_codec_header_data_size(krad_link->krad_ogg, current_track, h);
						krad_ogg_get_track_codec_header_data(krad_link->krad_ogg, current_track, header_buffer, h);


*/

krad_codec_t krad_container_get_track_codec (krad_container_t *krad_container, int tracknumber);
int krad_container_get_track_codec_data(krad_container_t *krad_container, int tracknumber, unsigned char *buffer);
int krad_container_get_track_count(krad_container_t *krad_container);


int krad_container_read_packet (krad_container_t *krad_container, int *tracknumber, unsigned char *buffer) {

	if (krad_container->container_type == OGG) {
		return krad_ogg_read_packet ( krad_container->krad_ogg, tracknumber, buffer);
	} else {
		return krad_ebml_read_packet ( krad_container->krad_ebml, tracknumber, buffer);		
	}

}

krad_container_t *krad_container_open_stream(char *host, int port, char *mount, char *password) {

	krad_container_t *krad_container;
	
	krad_container = calloc(1, sizeof(krad_container_t));

	krad_container->container_type = krad_link_select_container(host);

	if (krad_container->container_type == OGG) {
		krad_container->krad_ogg = krad_ogg_open_stream(host, port, mount, NULL);
	} else {
		krad_container->krad_ebml = krad_ebml_open_stream(host, port, mount, NULL);
	}

	return krad_container;

}


krad_container_t *krad_container_open_file(char *filename, krad_io_mode_t mode) {

	krad_container_t *krad_container;
	
	krad_container = calloc(1, sizeof(krad_container_t));

	krad_container->container_type = krad_link_select_container(filename);

	if (krad_container->container_type == OGG) {
		krad_container->krad_ogg = krad_ogg_open_file(filename, KRAD_IO_READONLY);
	} else {
		krad_container->krad_ebml = krad_ebml_open_file(filename, KRAD_EBML_IO_READONLY);
	}

	return krad_container;

}



void krad_container_destroy(krad_container_t *krad_container) {
						
	if (container == OGG) {
		krad_ogg_destroy(krad_link->krad_ogg);
	} else {
		krad_ebml_destroy(krad_link->krad_ebml);
	}
}	

