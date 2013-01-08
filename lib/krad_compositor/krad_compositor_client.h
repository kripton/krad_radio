/**
 * @file krad_compositor_client.h
 * @brief Krad Radio Compositor Controller API
 */

#ifndef KRAD_COMPOSITOR_CLIENT_H
#define KRAD_COMPOSITOR_CLIENT_H


/** @defgroup krad_compositor_client Krad Radio Compositor Control
  @{
  */



typedef struct kr_videoport_St kr_videoport_t;

#include "krad_compositor_common.h"

void kr_compositor_response_print (kr_response_t *kr_response);

int kr_compositor_read_frame_rate ( kr_client_t *client, char *text, krad_compositor_rep_t **krad_compositor_rep);
int kr_compositor_read_frame_size ( kr_client_t *client, char *text, krad_compositor_rep_t **krad_compositor_rep);
int kr_compositor_read_text ( kr_client_t *client, char *text);
int kr_compositor_read_sprite ( kr_client_t *client, char *text);
int kr_compositor_read_port ( kr_client_t *client, char *text);

void kr_compositor_port_list (kr_client_t *client);

void kr_compositor_add_text (kr_client_t *client, char *text, int x, int y, int z, int tickrate, 
									float scale, float opacity, float rotation, float red, float green, float blue, char *font);

void kr_compositor_set_text (kr_client_t *client, int num, int x, int y, int z, int tickrate, 
									float scale, float opacity, float rotation, float red, float green, float blue);

void kr_compositor_remove_text (kr_client_t *client, int num);
void kr_compositor_list_texts (kr_client_t *client);

void kr_compositor_add_sprite (kr_client_t *client, char *filename, int x, int y, int z, int tickrate, 
									float scale, float opacity, float rotation);

void kr_compositor_set_sprite (kr_client_t *client, int num, int x, int y, int z,  int tickrate, 
									float scale, float opacity, float rotation);

void kr_compositor_remove_sprite (kr_client_t *client, int num);
void kr_compositor_list_sprites (kr_client_t *client);
void kr_compositor_set_frame_rate (kr_client_t *client, int numerator, int denominator);
void kr_compositor_set_resolution (kr_client_t *client, int width, int height);
void kr_compositor_get_frame_size (kr_client_t *client);
void kr_compositor_get_frame_rate (kr_client_t *client);

void kr_compositor_close_display (kr_client_t *client);
void kr_compositor_open_display (kr_client_t *client, int width, int height);
void kr_compositor_background (kr_client_t *client, char *filename);


void kr_compositor_set_port_mode (kr_client_t *client, int number, uint32_t x, uint32_t y,
										uint32_t width, uint32_t height, uint32_t crop_x, uint32_t crop_y,
										uint32_t crop_width, uint32_t crop_height, float opacity, float rotation);


void kr_compositor_info (kr_client_t *client);
void kr_compositor_snapshot (kr_client_t *client);
void kr_compositor_snapshot_jpeg (kr_client_t *client);
void kr_compositor_last_snap_name (kr_client_t *client);

/* Compositor Local Video Ports */

void kr_videoport_set_callback (kr_videoport_t *kr_videoport, int callback (void *, void *), void *pointer);
void kr_videoport_activate (kr_videoport_t *kr_videoport);
void kr_videoport_deactivate (kr_videoport_t *kr_videoport);
kr_videoport_t *kr_videoport_create (kr_client_t *client);
void kr_videoport_destroy (kr_videoport_t *kr_videoport);

/**@}*/
#endif
