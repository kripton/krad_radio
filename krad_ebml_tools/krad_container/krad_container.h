

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



krad_codec_t krad_container_get_track_codec (krad_container_t *krad_container, int tracknumber);
int krad_container_get_track_codec_data(krad_container_t *krad_container, int tracknumber, unsigned char *buffer);
		if (krad_container_track_changed(krad_link->krad_container, current_track)) {
			printf("track %d changed! status is %d header count is %d\n", current_track, krad_container_track_status(krad_link->krad_container, current_track), krad_container_get_track_codec_header_count(krad_link->krad_container, current_track));


int krad_container_get_track_count(krad_container_t *krad_container);
int krad_container_read_packet (krad_container_t *krad_container, int *tracknumber, unsigned char *buffer);
krad_container_t *krad_container_open_stream(char *host, int port, char *mount, char *password);
krad_container_t *krad_container_open_file(char *filename, krad_ebml_io_mode_t mode);
void krad_container_destroy(krad_container_t *krad_container);
