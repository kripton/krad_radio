/* hrm? */

void krad_ipc_list_v4l2 (kr_client_t *client);
void krad_ipc_list_decklink (kr_client_t *client);

/** Transponder **/

void krad_ipc_disable_linker_transmitter (krad_ipc_client_t *client);
void krad_ipc_enable_linker_transmitter (krad_ipc_client_t *client, int port);
void krad_ipc_enable_linker_listen (krad_ipc_client_t *client, int port);
void krad_ipc_disable_linker_listen (krad_ipc_client_t *client);
void krad_ipc_create_playback_link (krad_ipc_client_t *client, char *path);
void krad_ipc_create_remote_playback_link (krad_ipc_client_t *client, char *host, int port, char *mount);
int krad_link_rep_to_string (krad_link_rep_t *krad_link, char *text);
void krad_ipc_create_receive_link (krad_ipc_client_t *client, int port);
void krad_ipc_create_record_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode, char *filename, char *codecs,
								  int video_width, int video_height, int video_bitrate, char *audio_bitrate);

void krad_ipc_create_capture_link (krad_ipc_client_t *client, krad_link_video_source_t video_source, char *device,
								   int width, int height, int fps_numerator, int fps_denominator,
								   krad_link_av_mode_t av_mode, char *audio_input, char *passthru_codec);

void krad_ipc_create_transmit_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode, char *host, int port,
									char *mount, char *password, char *codecs,
									int video_width, int video_height, int video_bitrate, char *audio_bitrate);

void krad_ipc_list_links (krad_ipc_client_t *client);
void krad_ipc_destroy_link (krad_ipc_client_t *client, int number);
void krad_ipc_update_link_adv (krad_ipc_client_t *client, int number, uint32_t ebml_id, char *newval);
void krad_ipc_update_link_adv_num (krad_ipc_client_t *client, int number, uint32_t ebml_id, int newval);
int krad_ipc_client_read_link ( krad_ipc_client_t *client, char *text, krad_link_rep_t **krad_link_rep);
