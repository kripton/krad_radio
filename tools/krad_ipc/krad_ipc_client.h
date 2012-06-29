#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <pthread.h>

#include "krad_radio_ipc.h"
#include "krad_system.h"
#include "krad_ebml.h"
#include "krad_link_common.h"

#define KRAD_IPC_BUFFER_SIZE 16384
#ifndef KRAD_IPC_CLIENT
#define KRAD_IPC_CLIENT 1

#define KRAD_IPC_CLIENT_DOCTYPE "krad_ipc_client"
#define KRAD_IPC_SERVER_DOCTYPE "krad_ipc_server"
#define KRAD_IPC_DOCTYPE_VERSION 6
#define KRAD_IPC_DOCTYPE_READ_VERSION 6
#define EBML_ID_KRAD_IPC_CMD 0x4444

typedef struct krad_ipc_client_St krad_ipc_client_t;

struct krad_ipc_client_St {
	char sysname[64];
	int flags;
	struct sockaddr_un saddr;
	int sd;
	int tcp_port;
	char host[256];
	char ipc_path[256];
	int ipc_path_pos;
	int on_linux;
	struct stat info;
	struct utsname unixname;
	char *buffer;
	krad_ebml_t *krad_ebml;
	
	int (*handler)(krad_ipc_client_t *, void *);
	void *ptr;
	
	int nowait;

};

void krad_ipc_compositor_set_frame_rate (krad_ipc_client_t *client, int numerator, int denominator);
void krad_ipc_compositor_set_resolution (krad_ipc_client_t *client, int width, int height);

void krad_ipc_compositor_close_display (krad_ipc_client_t *client);
void krad_ipc_compositor_open_display (krad_ipc_client_t *client, int width, int height);
void krad_ipc_set_mixer_sample_rate (krad_ipc_client_t *client, int sample_rate);
void krad_ipc_get_mixer_sample_rate (krad_ipc_client_t *client);

void krad_ipc_client_read_tag_inner ( krad_ipc_client_t *client, char **tag_item, char **tag_name, char **tag_value );
int krad_ipc_client_read_tag ( krad_ipc_client_t *client, char **tag_item, char **tag_name, char **tag_value );

void krad_ipc_disable_linker_transmitter (krad_ipc_client_t *client);
void krad_ipc_enable_linker_transmitter (krad_ipc_client_t *client, int port);
void krad_ipc_enable_linker_listen (krad_ipc_client_t *client, int port);
void krad_ipc_disable_linker_listen (krad_ipc_client_t *client);
void krad_ipc_enable_osc (krad_ipc_client_t *client, int port);
void krad_ipc_disable_osc (krad_ipc_client_t *client);
void krad_ipc_create_playback_link (krad_ipc_client_t *client, char *path);
void krad_ipc_create_remote_playback_link (krad_ipc_client_t *client, char *host, int port, char *mount);
int krad_link_rep_to_string (krad_link_rep_t *krad_link, char *text);

void krad_ipc_mixer_update_portgroup (krad_ipc_client_t *client, char *portgroupname, uint64_t update_command, char *string);
void krad_ipc_mixer_update_portgroup_map_channel (krad_ipc_client_t *client, char *portgroupname, int in_channel, int out_channel);
void krad_ipc_mixer_update_portgroup_mixmap_channel (krad_ipc_client_t *client, char *portgroupname, int in_channel, int out_channel);

void krad_ipc_compositor_background (krad_ipc_client_t *client, char *filename);
void krad_ipc_compositor_bug (krad_ipc_client_t *client, int x, int y, char *filename);
void krad_ipc_compositor_hex (krad_ipc_client_t *client, int x, int y, int size);
void krad_ipc_compositor_vu (krad_ipc_client_t *client, int on_off);

void krad_ipc_mixer_push_tone (krad_ipc_client_t *client, char *tone);

void krad_ipc_radio_set_dir (krad_ipc_client_t *client, char *dir);

void krad_ipc_mixer_bind_portgroup_xmms2 (krad_ipc_client_t *client, char *portgroupname, char *ipc_path);
void krad_ipc_mixer_unbind_portgroup_xmms2 (krad_ipc_client_t *client, char *portgroupname);

// FIXME creation is functionally incomplete
void krad_ipc_mixer_create_portgroup (krad_ipc_client_t *client, char *name, char *direction, int channels);
void krad_ipc_mixer_remove_portgroup (krad_ipc_client_t *client, char *portgroupname);

void krad_ipc_disable_remote (krad_ipc_client_t *client);
void krad_ipc_enable_remote (krad_ipc_client_t *client, int port);

void krad_ipc_weboff (krad_ipc_client_t *client);
void krad_ipc_webon (krad_ipc_client_t *client, int http_port, int websocket_port);

void krad_ipc_create_receive_link (krad_ipc_client_t *client, int port);
//void krad_ipc_create_playback_link (krad_ipc_client_t *client, char *host, int port, char *mount, char *password);
void krad_ipc_create_record_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode, char *filename, char *codecs,
								  int video_width, int video_height, int video_bitrate, int audio_bitrate);

void krad_ipc_create_capture_link (krad_ipc_client_t *client, krad_link_video_source_t video_source);

void krad_ipc_create_transmit_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode, char *host, int port,
									char *mount, char *password, char *codecs,
									int video_width, int video_height, int video_bitrate, int audio_bitrate);

void krad_ipc_list_links (krad_ipc_client_t *client);
void krad_ipc_destroy_link (krad_ipc_client_t *client, int number);
//void krad_ipc_update_link (krad_ipc_client_t *client, int number, int newval);
void krad_ipc_update_link_adv (krad_ipc_client_t *client, int number, uint32_t ebml_id, char *newval);
void krad_ipc_update_link_adv_num (krad_ipc_client_t *client, int number, uint32_t ebml_id, int newval);

void krad_ipc_radio_uptime (krad_ipc_client_t *client);
void krad_ipc_radio_info (krad_ipc_client_t *client);

int krad_ipc_client_read_link ( krad_ipc_client_t *client, char *text, krad_link_rep_t **krad_link_rep);

int krad_ipc_client_read_portgroup ( krad_ipc_client_t *client, char *portname, float *volume, char *crossfade_name, float *crossfade );

int krad_ipc_client_read_mixer_control ( krad_ipc_client_t *client, char **portgroup_name, char **control_name, float *value );
void krad_ipc_set_handler_callback (krad_ipc_client_t *client, int handler (krad_ipc_client_t *, void *), void *ptr);

void krad_ipc_compositor_set_port_mode (krad_ipc_client_t *client, int number, uint32_t x, uint32_t y,
										uint32_t width, uint32_t height, float opacity, float rotation);

void krad_ipc_compositor_list_ports (krad_ipc_client_t *client);
void krad_ipc_compositor_info (krad_ipc_client_t *client);
void krad_ipc_compositor_snapshot (krad_ipc_client_t *client);

void krad_ipc_get_portgroups (krad_ipc_client_t *client);
void krad_ipc_set_control (krad_ipc_client_t *client, char *portgroup_name, char *control_name, float control_value);
void krad_ipc_print_response (krad_ipc_client_t *client);
void krad_ipc_get_tags (krad_ipc_client_t *client, char *item);

void krad_ipc_get_tag (krad_ipc_client_t *client, char *item, char *tag_name);
void krad_ipc_set_tag (krad_ipc_client_t *client, char *item, char *tag_name, char *tag_value);

void krad_ipc_client_handle (krad_ipc_client_t *client);
int krad_ipc_client_poll (krad_ipc_client_t *client);
int krad_ipc_cmd2 (krad_ipc_client_t *client, int value);


krad_ipc_client_t *krad_ipc_connect ();
int krad_ipc_client_init (krad_ipc_client_t *client);
void krad_ipc_disconnect (krad_ipc_client_t *client);
int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd);
void krad_ipc_send (krad_ipc_client_t *client, char *cmd);
int krad_ipc_wait (krad_ipc_client_t *client, char *buffer, int size);
int krad_ipc_recv (krad_ipc_client_t *client, char *buffer, int size);
#endif
