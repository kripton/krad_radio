/**
 * @file krad_mixer_client.h
 * @brief Krad Radio Mixer Controller API
 */

/**
 * @mainpage Krad Radio Mixer Controller
 *
 * Krad Radio Mixer Controller (Kripton this is where you come in agin really need ya here)
 *
 */

#ifndef KRAD_MIXER_CLIENT_H
#define KRAD_MIXER_CLIENT_H

/** @defgroup krad_mixer_client Krad Radio Mixer Control
  @{
  */

typedef struct kr_audioport_St kr_audioport_t;

#include "krad_mixer_common.h"

/** Mixer **/

void kr_mixer_response_print (kr_response_t *kr_response);

int kr_mixer_read_portgroup ( kr_client_t *client, char *portname, float *volume, char *crossfade_name, float *crossfade, int *has_xmms2 );
int kr_mixer_read_control ( kr_client_t *client, char **portgroup_name, char **control_name, float *value );

void kr_mixer_portgroup_xmms2_cmd (kr_client_t *client, char *portgroupname, char *xmms2_cmd);
void kr_mixer_set_sample_rate (kr_client_t *client, int sample_rate);
void kr_mixer_sample_rate (kr_client_t *client);
void kr_mixer_plug_portgroup (kr_client_t *client, char *name, char *remote_name);
void kr_mixer_unplug_portgroup (kr_client_t *client, char *name, char *remote_name);
void kr_mixer_update_portgroup (kr_client_t *client, char *portgroupname, uint64_t update_command, char *string);
void kr_mixer_update_portgroup_map_channel (kr_client_t *client, char *portgroupname, int in_channel, int out_channel);
void kr_mixer_update_portgroup_mixmap_channel (kr_client_t *client, char *portgroupname, int in_channel, int out_channel);
void kr_mixer_push_tone (kr_client_t *client, char *tone);
void kr_mixer_bind_portgroup_xmms2 (kr_client_t *client, char *portgroupname, char *ipc_path);
void kr_mixer_unbind_portgroup_xmms2 (kr_client_t *client, char *portgroupname);
// FIXME creation is functionally incomplete
void kr_mixer_create_portgroup (kr_client_t *client, char *name, char *direction, int channels);
void kr_mixer_remove_portgroup (kr_client_t *client, char *portgroupname);
void kr_mixer_portgroups_list (kr_client_t *client);
void kr_mixer_set_control (kr_client_t *client, char *portgroup_name, char *control_name, float control_value);
void kr_mixer_add_effect (kr_client_t *client, char *portgroup_name, char *effect_name);
void kr_mixer_remove_effect (kr_client_t *client, char *portgroup_name, int effect_num);
void kr_mixer_set_effect_control (kr_client_t *client, char *portgroup_name, int effect_num, 
                                  char *control_name, int subunit, float control_value);

void kr_mixer_jack_running (kr_client_t *client);

/* Mixer Local Audio Ports */

float *kr_audioport_get_buffer (kr_audioport_t *kr_audioport, int channel);
void kr_audioport_set_callback (kr_audioport_t *kr_audioport, int callback (uint32_t, void *), void *pointer);
void kr_audioport_activate (kr_audioport_t *kr_audioport);
void kr_audioport_deactivate (kr_audioport_t *kr_audioport);
kr_audioport_t *kr_audioport_create (kr_client_t *client, krad_mixer_portgroup_direction_t direction);
void kr_audioport_destroy (kr_audioport_t *kr_audioport);

/**@}*/
#endif
