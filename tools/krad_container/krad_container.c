#include "krad_container.h"

krad_container_type_t krad_link_select_container (char *string) {

	if ((strstr(string, ".ogg")) ||
		(strstr(string, ".opus")) ||
		(strstr(string, ".Opus")) ||
		(strstr(string, ".OPUS")) ||
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

char *krad_link_select_mimetype (char *string) {

	if ((strstr(string, ".ogg")) ||
		(strstr(string, ".opus")) ||
		(strstr(string, ".Opus")) ||
		(strstr(string, ".OPUS")) ||
		(strstr(string, ".OGG")) ||
		(strstr(string, ".Ogg")) ||
		(strstr(string, ".oga")) ||
		(strstr(string, ".ogv")) ||
		(strstr(string, ".Oga")) ||		
		(strstr(string, ".OGV")))
	{
		return "application/ogg";
	}

	if (strstr(string, ".webm")) {
		return "video/webm";
	}
	
	return "video/x-matroska";
}

char *krad_container_get_container_string (krad_container_t *krad_container) {
	if (krad_container->container_type == OGG) {
		return "Ogg";
	} else {
		return "MKV";		
	}
}

int krad_container_get_container (krad_container_t *krad_container) {
	return krad_container->container_type;
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



int krad_container_read_packet (krad_container_t *krad_container, int *track, uint64_t *timecode,
								unsigned char *buffer) {

	if (krad_container->container_type == OGG) {
		return krad_ogg_read_packet ( krad_container->krad_ogg, track, timecode, buffer );
	} else {
		return krad_ebml_read_packet ( krad_container->krad_ebml, track, timecode, buffer );		
	}

}

krad_container_t *krad_container_open_stream (char *host, int port, char *mount, char *password) {

	krad_container_t *krad_container;
	
	krad_container = calloc(1, sizeof(krad_container_t));

	krad_container->container_type = krad_link_select_container (mount);

	if (krad_container->container_type == OGG) {
		krad_container->krad_ogg = krad_ogg_open_stream (host, port, mount, password);
	} else {
		krad_container->krad_ebml = krad_ebml_open_stream (host, port, mount, password);
	}

	return krad_container;

}


krad_container_t *krad_container_open_file (char *filename, krad_io_mode_t mode) {

	krad_container_t *krad_container;
	
	krad_container = calloc(1, sizeof(krad_container_t));

	krad_container->container_type = krad_link_select_container (filename);

	if (krad_container->container_type == OGG) {
		krad_container->krad_ogg = krad_ogg_open_file (filename, mode);
	} else {
	
		if (mode == KRAD_IO_WRITEONLY) {
			krad_container->krad_ebml = krad_ebml_open_file (filename, KRAD_EBML_IO_WRITEONLY);
		}

		if (mode == KRAD_IO_READONLY) {
			krad_container->krad_ebml = krad_ebml_open_file (filename, KRAD_EBML_IO_READONLY);
		}
	}

	return krad_container;

}

krad_container_t *krad_container_open_transmission (krad_transmission_t *krad_transmission) {

	krad_container_t *krad_container;
	
	krad_container = calloc(1, sizeof(krad_container_t));

	krad_container->container_type = krad_link_select_container (krad_transmission->sysname);

	if (krad_container->container_type == OGG) {
		krad_container->krad_ogg = krad_ogg_open_transmission (krad_transmission);
	} else {
		krad_container->krad_ebml = krad_ebml_open_transmission (krad_transmission);
	}

	return krad_container;

}

void krad_container_destroy (krad_container_t *krad_container) {
						
	if (krad_container->container_type == OGG) {
		krad_ogg_destroy (krad_container->krad_ogg);
	} else {
		krad_ebml_destroy (krad_container->krad_ebml);
	}

}	


int krad_container_add_video_track_with_private_data (krad_container_t *krad_container, krad_codec_t codec,
													  int fps_numerator, int fps_denominator, int width, int height,
													  krad_codec_header_t *krad_codec_header) {
									
	if (krad_container->container_type == OGG) {
		return krad_ogg_add_video_track_with_private_data (krad_container->krad_ogg, codec, fps_numerator, fps_denominator,
										 width, height, krad_codec_header->header, krad_codec_header->header_size,
										 krad_codec_header->header_count);
	} else {
		return krad_ebml_add_video_track_with_private_data (krad_container->krad_ebml, codec, fps_numerator, 
															fps_denominator, width, height,
															krad_codec_header->header_combined,
															krad_codec_header->header_combined_size);
	}									
									
}

int krad_container_add_video_track (krad_container_t *krad_container, krad_codec_t codec, 
									int fps_numerator, int fps_denominator, int width, int height) {
									
	if (krad_container->container_type == OGG) {
		return krad_ogg_add_video_track (krad_container->krad_ogg, codec, fps_numerator,
															  fps_denominator, width, height);
	} else {
		return krad_ebml_add_video_track (krad_container->krad_ebml, codec, fps_numerator, fps_denominator,
										  width, height);
	}									
									
}


int krad_container_add_audio_track (krad_container_t *krad_container, krad_codec_t codec, int sample_rate, int channels, 
									krad_codec_header_t *krad_codec_header) {

	if (krad_container->container_type == OGG) {
		return krad_ogg_add_audio_track (krad_container->krad_ogg, codec, sample_rate, channels, 
										 krad_codec_header->header, krad_codec_header->header_size,
										 krad_codec_header->header_count);
	} else {
		return krad_ebml_add_audio_track (krad_container->krad_ebml, codec, sample_rate, channels, 
										  krad_codec_header->header_combined, krad_codec_header->header_combined_size);
	}
}

void krad_container_add_video (krad_container_t *krad_container, int track, unsigned char *buffer, int buffer_size,
							   int keyframe) {

	if (krad_container->container_type == OGG) {
		krad_ogg_add_video (krad_container->krad_ogg, track, buffer, buffer_size, keyframe);
	} else {
		krad_ebml_add_video (krad_container->krad_ebml, track, buffer, buffer_size, keyframe);
	}

}


void krad_container_add_audio (krad_container_t *krad_container, int track, unsigned char *buffer, int buffer_size,
							   int frames) {

	if (krad_container->container_type == OGG) {
		krad_ogg_add_audio (krad_container->krad_ogg, track, buffer, buffer_size, frames);
	} else {
		krad_ebml_add_audio (krad_container->krad_ebml, track, buffer, buffer_size, frames);
	}

}


