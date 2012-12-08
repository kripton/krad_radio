#ifndef KRAD_COMPOSITOR_CLIENT_H
#define KRAD_COMPOSITOR_CLIENT_H

#include "krad_ipc_client.h"
#include "krad_compositor_common.h"

int kr_compositor_read_frame_rate ( krad_ipc_client_t *client, char *text, krad_compositor_rep_t **krad_compositor_rep);
int kr_compositor_read_frame_size ( krad_ipc_client_t *client, char *text, krad_compositor_rep_t **krad_compositor_rep);
int kr_compositor_read_text ( krad_ipc_client_t *client, char *text);
int kr_compositor_read_sprite ( krad_ipc_client_t *client, char *text);
int kr_compositor_read_port ( krad_ipc_client_t *client, char *text);

void kr_compositor_port_list (krad_ipc_client_t *client);

void kr_compositor_add_text (krad_ipc_client_t *client, char *text, int x, int y, int z, int tickrate, 
									float scale, float opacity, float rotation, float red, float green, float blue, char *font);

void kr_compositor_set_text (krad_ipc_client_t *client, int num, int x, int y, int z, int tickrate, 
									float scale, float opacity, float rotation, float red, float green, float blue);

void kr_compositor_remove_text (krad_ipc_client_t *client, int num);
void kr_compositor_list_texts (krad_ipc_client_t *client);

void kr_compositor_add_sprite (krad_ipc_client_t *client, char *filename, int x, int y, int z, int tickrate, 
									float scale, float opacity, float rotation);

void kr_compositor_set_sprite (krad_ipc_client_t *client, int num, int x, int y, int z,  int tickrate, 
									float scale, float opacity, float rotation);

void kr_compositor_remove_sprite (krad_ipc_client_t *client, int num);
void kr_compositor_list_sprites (krad_ipc_client_t *client);
void kr_compositor_set_frame_rate (krad_ipc_client_t *client, int numerator, int denominator);
void kr_compositor_set_resolution (krad_ipc_client_t *client, int width, int height);
void kr_compositor_get_frame_size (krad_ipc_client_t *client);
void kr_compositor_get_frame_rate (krad_ipc_client_t *client);

void kr_compositor_close_display (krad_ipc_client_t *client);
void kr_compositor_open_display (krad_ipc_client_t *client, int width, int height);
void kr_compositor_background (krad_ipc_client_t *client, char *filename);


void kr_compositor_set_port_mode (krad_ipc_client_t *client, int number, uint32_t x, uint32_t y,
										uint32_t width, uint32_t height, uint32_t crop_x, uint32_t crop_y,
										uint32_t crop_width, uint32_t crop_height, float opacity, float rotation);


void kr_compositor_info (krad_ipc_client_t *client);
void kr_compositor_snapshot (krad_ipc_client_t *client);
void kr_compositor_snapshot_jpeg (krad_ipc_client_t *client);
void krad_ipc_radio_get_last_snap_name (krad_ipc_client_t *client);

/* Compositor Local Video Ports */

void kr_videoport_set_callback (kr_videoport_t *kr_videoport, int callback (void *, void *), void *pointer);
void kr_videoport_activate (kr_videoport_t *kr_videoport);
void kr_videoport_deactivate (kr_videoport_t *kr_videoport);
kr_videoport_t *kr_videoport_create (krad_ipc_client_t *client);
void kr_videoport_destroy (kr_videoport_t *kr_videoport);


#endif
