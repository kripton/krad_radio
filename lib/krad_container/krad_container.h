#include "krad_ebml.h"
#include "krad_ogg.h"
#include "krad_codec_header.h"

typedef enum {
	EBML = 100,
	OGG,
	RAW,
} krad_container_type_t;


typedef struct krad_container_St krad_container_t;

struct krad_container_St {

	krad_container_type_t container_type;
	krad_ogg_t *krad_ogg;
	krad_ebml_t *krad_ebml;
	krad_io_t *krad_io;
	krad_transmission_t *krad_transmission;	
};

int krad_container_get_container (krad_container_t *krad_container);
char *krad_container_get_container_string (krad_container_t *krad_container);

int krad_container_track_count (krad_container_t *krad_container);
krad_codec_t krad_container_track_codec (krad_container_t *krad_container, int track);
int krad_container_track_header_count (krad_container_t *krad_container, int track);
int krad_container_track_header_size (krad_container_t *krad_container, int track, int header);
int krad_container_read_track_header (krad_container_t *krad_container, unsigned char *buffer, int track, int header);
int krad_container_track_active (krad_container_t *krad_container, int track);
int krad_container_track_changed (krad_container_t *krad_container, int track);
char *krad_link_select_mimetype (char *string);

int krad_container_read_packet (krad_container_t *krad_container, int *track, uint64_t *timecode, 
								unsigned char *buffer);
krad_container_t *krad_container_open_stream (char *host, int port, char *mount, char *password);
krad_container_t *krad_container_open_file (char *filename, krad_io_mode_t mode);
krad_container_t *krad_container_open_transmission (krad_transmission_t *krad_transmission);
void krad_container_destroy (krad_container_t *krad_container);


int krad_container_add_video_track_with_private_data (krad_container_t *krad_container, krad_codec_t codec,
													  int fps_numerator, int fps_denominator, int width, int height, 
													  krad_codec_header_t *krad_codec_header);

int krad_container_add_video_track (krad_container_t *krad_container, krad_codec_t codec, int fps_numerator, 
									int fps_denominator, int width, int height);

int krad_container_add_audio_track (krad_container_t *krad_container, krad_codec_t codec, int sample_rate, int channels, 
									krad_codec_header_t *krad_codec_header);

void krad_container_add_video (krad_container_t *krad_container, int track, unsigned char *buffer, int buffer_size,
							   int keyframe);

void krad_container_add_audio (krad_container_t *krad_container, int track, unsigned char *buffer, int buffer_size,
							   int frames);

