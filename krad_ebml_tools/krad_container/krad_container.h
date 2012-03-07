#include "krad_ebml.h"
#include "krad_ogg.h"

typedef enum {
	EBML = 100,
	OGG,
	NAKED,
} krad_container_type_t;


typedef struct krad_container_St krad_container_t;


struct krad_container_St {

	krad_container_type_t container_type;
	krad_ogg_t *krad_ogg;
	krad_ebml_t *krad_ebml;
	
};



int krad_container_track_count (krad_container_t *krad_container);
krad_codec_t krad_container_track_codec (krad_container_t *krad_container, int track);
int krad_container_track_header_count (krad_container_t *krad_container, int track);
int krad_container_track_header_size (krad_container_t *krad_container, int track, int header);
int krad_container_read_track_header (krad_container_t *krad_container, unsigned char *buffer, int track, int header);
int krad_container_track_active (krad_container_t *krad_container, int track);
int krad_container_track_changed (krad_container_t *krad_container, int track);


int krad_container_read_packet (krad_container_t *krad_container, int *track, uint64_t *timecode, unsigned char *buffer);
krad_container_t *krad_container_open_stream (char *host, int port, char *mount, char *password);
krad_container_t *krad_container_open_file (char *filename, krad_io_mode_t mode);
void krad_container_destroy (krad_container_t *krad_container);
