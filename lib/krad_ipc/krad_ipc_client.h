#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include <sys/wait.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/mman.h>

#include "krad_radio_version.h"
#include "krad_mixer_common.h"
#include "krad_radio_ipc.h"
#include "krad_system.h"
#include "krad_ebml.h"
#include "krad_link_common.h"
#include "krad_compositor_common.h"


#define KRAD_IPC_BUFFER_SIZE 16384
#ifndef KRAD_IPC_CLIENT
#define KRAD_IPC_CLIENT 1

#define KRAD_IPC_CLIENT_DOCTYPE "krad_ipc_client"
#define KRAD_IPC_SERVER_DOCTYPE "krad_ipc_server"
#define KRAD_IPC_DOCTYPE_VERSION KRAD_VERSION
#define KRAD_IPC_DOCTYPE_READ_VERSION KRAD_VERSION
#define EBML_ID_KRAD_IPC_CMD 0x4444

typedef struct krad_radio_watchdog_St krad_radio_watchdog_t;

struct krad_radio_watchdog_St {
	int count;
	char *stations[1024];
	char *launch_scripts[1024];
};


typedef struct kr_shm_St kr_shm_t;
typedef struct kr_videoport_St kr_videoport_t;
typedef struct kr_audioport_St kr_audioport_t;
typedef struct kr_client_St kr_client_t;
typedef struct kr_client_St krad_ipc_client_t;

struct kr_shm_St {

	int fd;
	char *buffer;
	uint64_t size;

};

struct kr_videoport_St {

	int width;
	int height;
	kr_shm_t *kr_shm;
	kr_client_t *client;
	int sd;
	
	int (*callback)(void *, void *);
	void *pointer;
	
	int active;
	
	pthread_t process_thread;	
	
};

struct kr_audioport_St {

	int samplerate;
	kr_shm_t *kr_shm;
	kr_client_t *client;
	int sd;
	
	krad_mixer_portgroup_direction_t direction;
	
	int (*callback)(uint32_t, void *);
	void *pointer;
	
	int active;
	
	pthread_t process_thread;	
	
};

struct kr_client_St {
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

/*** Krad IPC ***/

void krad_ipc_broadcast_subscribe (krad_ipc_client_t *client, uint32_t broadcast_id);
void krad_ipc_set_handler_callback (krad_ipc_client_t *client, int handler (krad_ipc_client_t *, void *), void *ptr);
int krad_ipc_client_init (krad_ipc_client_t *client);
int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd);
void krad_ipc_send (krad_ipc_client_t *client, char *cmd);
int krad_ipc_wait (krad_ipc_client_t *client, char *buffer, int size);
int krad_ipc_recv (krad_ipc_client_t *client, char *buffer, int size);
void krad_ipc_client_handle (krad_ipc_client_t *client);
int krad_ipc_client_poll (krad_ipc_client_t *client);
int krad_ipc_cmd2 (krad_ipc_client_t *client, int value);

/*** Krad Radio Control ***/

void krad_radio_launch (char *sysname);
int krad_radio_destroy (char *sysname);
void krad_radio_watchdog_launch (char *config_file);
int krad_radio_running (char *sysname);
int krad_radio_pid (char *sysname);
char *krad_radio_threads (char *sysname);
char *krad_radio_running_stations ();


/*** Krad Radio Client API ***/

kr_client_t *kr_connect (char *sysname);
void kr_disconnect (kr_client_t *client);

/** Station **/

/* Info */
void krad_ipc_radio_uptime (krad_ipc_client_t *client);
void krad_ipc_radio_get_system_info (krad_ipc_client_t *client);
void krad_ipc_radio_get_system_cpu_usage (krad_ipc_client_t *client);
void krad_ipc_radio_set_dir (krad_ipc_client_t *client, char *dir);
void krad_ipc_radio_get_logname (krad_ipc_client_t *client);

void krad_ipc_mixer_jack_running (krad_ipc_client_t *client);
void krad_ipc_list_v4l2 (krad_ipc_client_t *client);
void krad_ipc_list_decklink (krad_ipc_client_t *client);

void krad_ipc_print_response (krad_ipc_client_t *client);

/* Remote Control */
void krad_ipc_enable_osc (krad_ipc_client_t *client, int port);
void krad_ipc_disable_osc (krad_ipc_client_t *client);
void krad_ipc_disable_remote (krad_ipc_client_t *client);
void krad_ipc_enable_remote (krad_ipc_client_t *client, int port);
void krad_ipc_weboff (krad_ipc_client_t *client);
void krad_ipc_webon (krad_ipc_client_t *client, int http_port, int websocket_port, char *headcode, char *header, char *footer);

/* Tagging */
void krad_ipc_get_tags (krad_ipc_client_t *client, char *item);
void krad_ipc_client_read_tag_inner ( krad_ipc_client_t *client, char **tag_item, char **tag_name, char **tag_value );
int krad_ipc_client_read_tag ( krad_ipc_client_t *client, char **tag_item, char **tag_name, char **tag_value );
void krad_ipc_get_tag (krad_ipc_client_t *client, char *item, char *tag_name);
void krad_ipc_set_tag (krad_ipc_client_t *client, char *item, char *tag_name, char *tag_value);

/** SHM for local a/v ports **/

kr_shm_t *kr_shm_create (krad_ipc_client_t *client);
void kr_shm_destroy (kr_shm_t *kr_shm);

/** Mixer **/

void krad_ipc_mixer_portgroup_xmms2_cmd (krad_ipc_client_t *client, char *portgroupname, char *xmms2_cmd);
void krad_ipc_set_mixer_sample_rate (krad_ipc_client_t *client, int sample_rate);
void krad_ipc_get_mixer_sample_rate (krad_ipc_client_t *client);
void kr_mixer_plug_portgroup (krad_ipc_client_t *client, char *name, char *remote_name);
void kr_mixer_unplug_portgroup (krad_ipc_client_t *client, char *name, char *remote_name);
void krad_ipc_mixer_update_portgroup (krad_ipc_client_t *client, char *portgroupname, uint64_t update_command, char *string);
void krad_ipc_mixer_update_portgroup_map_channel (krad_ipc_client_t *client, char *portgroupname, int in_channel, int out_channel);
void krad_ipc_mixer_update_portgroup_mixmap_channel (krad_ipc_client_t *client, char *portgroupname, int in_channel, int out_channel);
void krad_ipc_mixer_push_tone (krad_ipc_client_t *client, char *tone);
void krad_ipc_mixer_bind_portgroup_xmms2 (krad_ipc_client_t *client, char *portgroupname, char *ipc_path);
void krad_ipc_mixer_unbind_portgroup_xmms2 (krad_ipc_client_t *client, char *portgroupname);
// FIXME creation is functionally incomplete
void krad_ipc_mixer_create_portgroup (krad_ipc_client_t *client, char *name, char *direction, int channels);
void krad_ipc_mixer_remove_portgroup (krad_ipc_client_t *client, char *portgroupname);
int krad_ipc_client_read_portgroup ( krad_ipc_client_t *client, char *portname, float *volume, char *crossfade_name, float *crossfade, int *has_xmms2 );
int krad_ipc_client_read_mixer_control ( krad_ipc_client_t *client, char **portgroup_name, char **control_name, float *value );
void krad_ipc_get_portgroups (krad_ipc_client_t *client);
void krad_ipc_set_control (krad_ipc_client_t *client, char *portgroup_name, char *control_name, float control_value);
void kr_mixer_add_effect (krad_ipc_client_t *client, char *portgroup_name, char *effect_name);
void kr_mixer_remove_effect (krad_ipc_client_t *client, char *portgroup_name, int effect_num);
void krad_ipc_set_effect_control (krad_ipc_client_t *client, char *portgroup_name, int effect_num, 
                                  char *control_name, int subunit, float control_value);


/* Mixer Local Audio Ports */

float *kr_audioport_get_buffer (kr_audioport_t *kr_audioport, int channel);
void kr_audioport_set_callback (kr_audioport_t *kr_audioport, int callback (uint32_t, void *), void *pointer);
void kr_audioport_activate (kr_audioport_t *kr_audioport);
void kr_audioport_deactivate (kr_audioport_t *kr_audioport);
kr_audioport_t *kr_audioport_create (krad_ipc_client_t *client, krad_mixer_portgroup_direction_t direction);
void kr_audioport_destroy (kr_audioport_t *kr_audioport);

/** Compositor **/

void krad_ipc_compositor_add_text (krad_ipc_client_t *client, char *text, int x, int y, int tickrate, 
									float scale, float opacity, float rotation, int red, int green, int blue, char *font);

void krad_ipc_compositor_set_text (krad_ipc_client_t *client, int num, int x, int y, int tickrate, 
									float scale, float opacity, float rotation, int red, int green, int blue);

void krad_ipc_compositor_remove_text (krad_ipc_client_t *client, int num);
void krad_ipc_compositor_list_texts (krad_ipc_client_t *client);

void krad_ipc_compositor_add_sprite (krad_ipc_client_t *client, char *filename, int x, int y, int tickrate, 
									float scale, float opacity, float rotation);

void krad_ipc_compositor_set_sprite (krad_ipc_client_t *client, int num, int x, int y, int tickrate, 
									float scale, float opacity, float rotation);

void krad_ipc_compositor_remove_sprite (krad_ipc_client_t *client, int num);
void krad_ipc_compositor_list_sprites (krad_ipc_client_t *client);
void krad_ipc_compositor_set_frame_rate (krad_ipc_client_t *client, int numerator, int denominator);
void krad_ipc_compositor_set_resolution (krad_ipc_client_t *client, int width, int height);
void krad_ipc_compositor_get_frame_size (krad_ipc_client_t *client);
void krad_ipc_compositor_get_frame_rate (krad_ipc_client_t *client);

void krad_ipc_compositor_close_display (krad_ipc_client_t *client);
void krad_ipc_compositor_open_display (krad_ipc_client_t *client, int width, int height);
void krad_ipc_compositor_background (krad_ipc_client_t *client, char *filename);
void krad_ipc_compositor_bug (krad_ipc_client_t *client, int x, int y, char *filename);
void krad_ipc_compositor_hex (krad_ipc_client_t *client, int x, int y, int size);
void krad_ipc_compositor_vu (krad_ipc_client_t *client, int on_off);
void krad_ipc_compositor_set_port_mode (krad_ipc_client_t *client, int number, uint32_t x, uint32_t y,
										uint32_t width, uint32_t height, uint32_t crop_x, uint32_t crop_y,
										uint32_t crop_width, uint32_t crop_height, float opacity, float rotation);

void krad_ipc_compositor_list_ports (krad_ipc_client_t *client);
void krad_ipc_compositor_info (krad_ipc_client_t *client);
void krad_ipc_compositor_snapshot (krad_ipc_client_t *client);
void krad_ipc_compositor_snapshot_jpeg (krad_ipc_client_t *client);
void krad_ipc_radio_get_last_snap_name (krad_ipc_client_t *client);

/* Compositor Local Video Ports */

void kr_videoport_set_callback (kr_videoport_t *kr_videoport, int callback (void *, void *), void *pointer);
void kr_videoport_activate (kr_videoport_t *kr_videoport);
void kr_videoport_deactivate (kr_videoport_t *kr_videoport);
kr_videoport_t *kr_videoport_create (krad_ipc_client_t *client);
void kr_videoport_destroy (kr_videoport_t *kr_videoport);

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


#endif
