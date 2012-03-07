#include "krad_container.h"

krad_container_type_t krad_link_select_container (char *string) {

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



int krad_container_track_count (krad_container_t *krad_container) {
	if (krad_container->container_type == OGG) {
		return krad_ogg_track_count ( krad_container->krad_ogg );
	} else {
		return krad_ebml_track_count ( krad_container->krad_ebml );		
	}
}

krad_codec_t krad_container_track_codec (krad_container_t *krad_container, int track) {
	if (krad_container->container_type == OGG) {
		return krad_ogg_track_codec ( krad_container->krad_ogg, track );
	} else {
		return krad_ebml_track_codec ( krad_container->krad_ebml, track );		
	}
}

int krad_container_track_header_size (krad_container_t *krad_container, int track, int header) {
	if (krad_container->container_type == OGG) {
		return krad_ogg_track_header_size ( krad_container->krad_ogg, track, header );
	} else {
		return krad_ebml_track_header_size ( krad_container->krad_ebml, track, header );		
	}
}

int krad_container_read_track_header (krad_container_t *krad_container, unsigned char *buffer, int track, int header) {
	if (krad_container->container_type == OGG) {
		return krad_ogg_read_track_header ( krad_container->krad_ogg, buffer, track, header );
	} else {
		return krad_ebml_read_track_header ( krad_container->krad_ebml, buffer, track, header );		
	}
}


int krad_container_track_changed (krad_container_t *krad_container, int track) {
	if (krad_container->container_type == OGG) {
		return krad_ogg_track_changed ( krad_container->krad_ogg, track );
	} else {
		return krad_ebml_track_changed( krad_container->krad_ebml, track );		
	}
}

int krad_container_track_active (krad_container_t *krad_container, int track) {
	if (krad_container->container_type == OGG) {
		return krad_ogg_track_active ( krad_container->krad_ogg, track );
	} else {
		return krad_ebml_track_active ( krad_container->krad_ebml, track );		
	}
}


int krad_container_track_header_count (krad_container_t *krad_container, int track) {
	if (krad_container->container_type == OGG) {
		return krad_ogg_track_header_count ( krad_container->krad_ogg, track );
	} else {
		return krad_ebml_track_header_count ( krad_container->krad_ebml, track );		
	}
}



int krad_container_read_packet (krad_container_t *krad_container, int *track, unsigned char *buffer) {

	if (krad_container->container_type == OGG) {
		return krad_ogg_read_packet ( krad_container->krad_ogg, track, buffer );
	} else {
		return krad_ebml_read_packet ( krad_container->krad_ebml, track, buffer );		
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
						
	if (krad_container->container_type == OGG) {
		krad_ogg_destroy(krad_container->krad_ogg);
	} else {
		krad_ebml_destroy(krad_container->krad_ebml);
	}

}	

