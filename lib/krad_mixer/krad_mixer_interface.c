#include "krad_mixer_interface.h"

static kr_portgroup_t *krad_mixer_portgroup_to_rep (krad_mixer_portgroup_t *portgroup,
                                                    kr_portgroup_t *portgroup_rep) {
  int i;
  
  if (portgroup_rep == NULL) {
    portgroup_rep = kr_portgroup_rep_create ();
  }
  
  strcpy (portgroup_rep->sysname, portgroup->sysname);
  portgroup_rep->channels = portgroup->channels;
  portgroup_rep->io_type = portgroup->io_type;
  
  strncpy (portgroup_rep->mixbus, portgroup->mixbus->sysname, sizeof(portgroup_rep->mixbus));
  
  for (i = 0; i < KRAD_MIXER_MAX_CHANNELS; i++) {
    portgroup_rep->volume[i] = portgroup->volume[i];
    portgroup_rep->map[i] = portgroup->map[i];
    portgroup_rep->mixmap[i] = portgroup->mixmap[i];
    portgroup_rep->rms[i] = portgroup->rms[i];
    portgroup_rep->peak[i] = portgroup->peak[i];
  }
  
  if (portgroup->krad_xmms != NULL) {
    portgroup_rep->has_xmms2 = 1;
    strncpy (portgroup_rep->xmms2_ipc_path,
             portgroup->krad_xmms->ipc_path,
             sizeof(portgroup_rep->xmms2_ipc_path));
  }
  
  kr_eq_t *eq;
  kr_pass_t *lowpass;
  kr_pass_t *highpass;
  kr_analog_t *analog;
  
  eq = (kr_eq_t *)portgroup->effects->effect[0].effect[0];
  lowpass = (kr_lowpass_t *)portgroup->effects->effect[1].effect[0];
  highpass = (kr_highpass_t *)portgroup->effects->effect[2].effect[0];
  analog = (kr_analog_t *)portgroup->effects->effect[3].effect[0];

  for (i = 0; i < KRAD_EQ_MAX_BANDS; i++) {
    portgroup_rep->eq.band[i].db = eq->band[i].db;
    portgroup_rep->eq.band[i].bandwidth = eq->band[i].bandwidth;
    portgroup_rep->eq.band[i].hz = eq->band[i].hz;
  }
  
  portgroup_rep->lowpass.hz = lowpass->hz;
  portgroup_rep->lowpass.bandwidth = lowpass->bandwidth;
  portgroup_rep->highpass.hz = highpass->hz;
  portgroup_rep->highpass.bandwidth = highpass->bandwidth;
  
  portgroup_rep->analog.drive = analog->drive;
  portgroup_rep->analog.blend = analog->blend;
  
  if ((portgroup->crossfade_group != NULL) && (portgroup->crossfade_group->portgroup[0] == portgroup)) {
    portgroup_rep->fade = portgroup->crossfade_group->fade;
    strncpy (portgroup_rep->crossfade_group,
             portgroup->crossfade_group->portgroup[1]->sysname,
             sizeof(portgroup_rep->crossfade_group));
  }
  
  return portgroup_rep;
}

static int krad_mixer_broadcast_portgroup_created ( krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *portgroup ) {

  size_t size;
  unsigned char *buffer;
  krad_broadcast_msg_t *broadcast_msg;
  krad_mixer_portgroup_rep_t *portgroup_rep;

  size = 2048;
  buffer = malloc (size);

  uint64_t message_loc;
  uint64_t payload_loc;
  krad_ebml_t *krad_ebml;

  payload_loc = 0;

  krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_WRITEONLY);
  
  krad_radio_address_to_ebml (krad_ebml, &message_loc, &portgroup->address);
  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_TYPE, EBML_ID_KRAD_SUBUNIT_CREATED);
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_PAYLOAD, &payload_loc);
  portgroup_rep = krad_mixer_portgroup_to_rep (portgroup, NULL);
  krad_mixer_portgroup_rep_to_ebml (portgroup_rep, krad_ebml);
  kr_portgroup_rep_destroy (portgroup_rep);
  krad_ebml_finish_element (krad_ebml, payload_loc);
  krad_ebml_finish_element (krad_ebml, message_loc);

  size = krad_ebml->io_adapter.write_buffer_pos;
  memcpy (buffer, krad_ebml->io_adapter.write_buffer, size);
  krad_ebml_destroy (krad_ebml);

  broadcast_msg = krad_broadcast_msg_create (buffer, size);

  free (buffer);

  if (broadcast_msg != NULL) {
    return krad_ipc_server_broadcaster_broadcast ( krad_mixer->broadcaster, &broadcast_msg );
  }

  return -1;
}

int krad_mixer_broadcast_portgroup_effect_control (krad_mixer_t *krad_mixer, char *portgroupname, int effect_num, int effect_param_num, kr_mixer_effect_control_t effect_control, float value) {

  size_t size;
  unsigned char *buffer;
  krad_broadcast_msg_t *broadcast_msg;
  krad_ebml_t *krad_ebml;
  kr_address_t address;
  uint64_t message_loc;
  uint64_t payload_loc;
  
  size = 256;
  buffer = malloc (size);

  krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_WRITEONLY);
  
  address.path.unit = KR_MIXER;
  address.path.subunit.mixer_subunit = KR_EFFECT;
  strncpy (address.id.name, portgroupname, sizeof (address.id.name));
  address.sub_id = effect_num;
  address.sub_id2 = effect_param_num;
  address.control.effect_control = effect_control;

  krad_radio_address_to_ebml (krad_ebml, &message_loc, &address);
  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_TYPE, EBML_ID_KRAD_SUBUNIT_CONTROL);
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_PAYLOAD, &payload_loc);
  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_SUBUNIT_CONTROL, value);
  krad_ebml_finish_element (krad_ebml, payload_loc);
  krad_ebml_finish_element (krad_ebml, message_loc);

  size = krad_ebml->io_adapter.write_buffer_pos;
  memcpy (buffer, krad_ebml->io_adapter.write_buffer, size);
  krad_ebml_destroy (krad_ebml);



  broadcast_msg = krad_broadcast_msg_create (buffer, size);

  free (buffer);

  if (broadcast_msg != NULL) {
    return krad_ipc_server_broadcaster_broadcast ( krad_mixer->broadcaster, &broadcast_msg );
  }

  return -1;
}

int krad_mixer_broadcast_portgroup_control (krad_mixer_t *krad_mixer, char *portgroupname, char *controlname, float value) {

  size_t size;
  unsigned char *buffer;
  krad_broadcast_msg_t *broadcast_msg;
  krad_ebml_t *krad_ebml;
  kr_address_t address;
  uint64_t message_loc;
  uint64_t payload_loc;
  
  size = 256;
  buffer = malloc (size);

  krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_WRITEONLY);
  
  address.path.unit = KR_MIXER;
  address.path.subunit.mixer_subunit = KR_PORTGROUP;
  strncpy (address.id.name, portgroupname, sizeof (address.id.name));

  if (controlname[0] == 'c') {
    address.control.portgroup_control = KR_CROSSFADE;
  } 
  if (controlname[0] == 'v') {
    address.control.portgroup_control = KR_VOLUME;
  }

  krad_radio_address_to_ebml (krad_ebml, &message_loc, &address);

  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_TYPE, EBML_ID_KRAD_SUBUNIT_CONTROL);

  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_PAYLOAD, &payload_loc);

  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_SUBUNIT_CONTROL, value);

  krad_ebml_finish_element (krad_ebml, payload_loc);

  krad_ebml_finish_element (krad_ebml, message_loc);

  size = krad_ebml->io_adapter.write_buffer_pos;
  memcpy (buffer, krad_ebml->io_adapter.write_buffer, size);
  krad_ebml_destroy (krad_ebml);



  broadcast_msg = krad_broadcast_msg_create (buffer, size);

  free (buffer);

  if (broadcast_msg != NULL) {
    return krad_ipc_server_broadcaster_broadcast ( krad_mixer->broadcaster, &broadcast_msg );
  }

  return -1;
}

int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc ) {

  uint32_t command;
  uint32_t ebml_id;
  uint64_t payload_loc;
  uint64_t ebml_data_size;

  krad_mixer_portgroup_t *portgroup;
  krad_mixer_portgroup_t *portgroup2;
  
  krad_mixer_portgroup_rep_t *portgroup_rep;
  
  //kr_mixer_t *kr_mixer;
  uint64_t info_loc;
  uint64_t response;

  krad_mixer_output_t output_type;

  int p;
  
  int sd1;
  int sd2;
  
  kr_address_t address;
  
  char portname[256];
  char portgroupname[256];
  char portgroupname2[256];  
  char controlname[256];  
  float floatval;

  char string[512];
  int direction;
  int number;
  int numbers[16];
      
  sd1 = 0;
  sd2 = 0;
  ebml_id = 0;
  number = 0;
  direction = 0;
  
  payload_loc = 0;
  
  krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size );

  switch ( command ) {
  
    case EBML_ID_KRAD_MIXER_CMD_SET_CONTROL:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portname, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, controlname, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (ebml_id == EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
        floatval = krad_ebml_read_float (krad_ipc->current_client->krad_ebml, ebml_data_size);
        krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
        number = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);  
        krad_mixer_set_portgroup_control ( krad_mixer, portname, controlname, floatval, number );
        //krad_mixer_broadcast_portgroup_control ( krad_mixer, portname, controlname, floatval );
      }
      return 2;

    case EBML_ID_KRAD_MIXER_CMD_SET_EFFECT_CONTROL:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      number = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      numbers[5] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, controlname, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
        floatval = krad_ebml_read_float (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_MIXER_CONTROL_DURATION) {
        numbers[6] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_MIXER_CONTROL_DURATION) {
        numbers[7] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);
      if (portgroup != NULL) {
        kr_effects_effect_set_control (portgroup->effects, number, numbers[5],
                   kr_effects_string_to_effect_control(portgroup->effects->effect[number].effect_type,
                                                        controlname),
                                       floatval, numbers[6], numbers[7]);
      }
      break;
    case EBML_ID_KRAD_MIXER_CMD_PUSH_TONE:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (krad_mixer->push_tone == NULL) {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, krad_mixer->push_tone_value, ebml_data_size);
        krad_mixer->push_tone = krad_mixer->push_tone_value;
      } else {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
      }
      break;
    case EBML_ID_KRAD_MIXER_CMD_PORTGROUP_XMMS2_CMD:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
      krad_mixer_portgroup_xmms2_cmd (krad_mixer, portgroupname, string);
      break;
      
    case EBML_ID_KRAD_MIXER_CMD_PLUG_PORTGROUP:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname2, ebml_data_size);
      krad_mixer_plug_portgroup (krad_mixer, portgroupname, portgroupname2);
      break;
    case EBML_ID_KRAD_MIXER_CMD_UNPLUG_PORTGROUP:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      }
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);    
  
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      }
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname2, ebml_data_size);
      
      krad_mixer_unplug_portgroup (krad_mixer, portgroupname, portgroupname2);
    
      break;
      
    case EBML_ID_KRAD_MIXER_CMD_BIND_PORTGROUP_XMMS2:  
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
      krad_mixer_bind_portgroup_xmms2 (krad_mixer, portgroupname, string);
      break;
    case EBML_ID_KRAD_MIXER_CMD_UNBIND_PORTGROUP_XMMS2:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      krad_mixer_unbind_portgroup_xmms2 (krad_mixer, portgroupname);
      break;
    case EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS:
      for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
        portgroup = krad_mixer->portgroup[p];
        if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
          portgroup_rep = krad_mixer_portgroup_to_rep (portgroup, NULL);
          krad_ipc_server_response_start_with_address_and_type ( krad_ipc,
                                                                 &portgroup->address,
                                                                 EBML_ID_KRAD_SUBUNIT_INFO,
                                                                 &response);
          krad_ipc_server_payload_start ( krad_ipc, &payload_loc);
          krad_mixer_portgroup_rep_to_ebml (portgroup_rep, krad_ipc->current_client->krad_ebml2);
          krad_ipc_server_payload_finish ( krad_ipc, payload_loc );
          krad_ipc_server_response_finish ( krad_ipc, response );
          kr_portgroup_rep_destroy (portgroup_rep);
        }
      }
      return 1;
    case EBML_ID_KRAD_MIXER_CMD_CREATE_PORTGROUP:
    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

      if (strncmp(string, "output", 6) == 0) {
        direction = OUTPUT;
        output_type = DIRECT;
      } else {
        if (strncmp(string, "auxout", 6) == 0) {
          direction = OUTPUT;
          output_type = AUX;
        } else {
          direction = INPUT;
          output_type = NOTOUTPUT;
        }
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      portgroup = krad_mixer_portgroup_create (krad_mixer, portgroupname, direction, output_type, numbers[0],
                  0.0f, krad_mixer->master_mix, KRAD_AUDIO, NULL, JACK);      

      if (portgroup != NULL) {
        if (portgroup->direction == INPUT) {
          krad_mixer_broadcast_portgroup_created ( krad_mixer, portgroup );
          //krad_radio_broadcast_subunit_created (krad_mixer->broadcaster, &address, rep);
        }
      }

      krad_mixer_set_portgroup_control (krad_mixer, portgroupname, "volume", 100.0f, 500);

      break;
    case EBML_ID_KRAD_MIXER_CMD_DESTROY_PORTGROUP:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);
      if (portgroup != NULL) {
        krad_mixer_portgroup_destroy (krad_mixer, portgroup);
        address.path.unit = KR_MIXER;
        address.path.subunit.mixer_subunit = KR_PORTGROUP;
        strncpy (address.id.name, portgroupname, sizeof (address.id.name));
        krad_radio_broadcast_subunit_destroyed (krad_mixer->broadcaster, &address);
      }
      break;
    case EBML_ID_KRAD_MIXER_CMD_PORTGROUP_INFO:  

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);
      if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
        
        krad_ipc_server_response_start_with_address_and_type ( krad_ipc,
                                                               &portgroup->address,
                                                               EBML_ID_KRAD_SUBUNIT_INFO,
                                                               &response);

        krad_ipc_server_payload_start ( krad_ipc, &payload_loc);
        
        portgroup_rep = krad_mixer_portgroup_to_rep(portgroup, NULL);
        krad_mixer_portgroup_rep_to_ebml (portgroup_rep, krad_ipc->current_client->krad_ebml2);
        kr_portgroup_rep_destroy (portgroup_rep);
        
        krad_ipc_server_payload_finish ( krad_ipc, payload_loc );
        krad_ipc_server_response_finish ( krad_ipc, response );
      }

      break;
    case EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP:      
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);

      // set tag / add/remove effect / set/rm crossfade group partner

      if (ebml_id == EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
      
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
        
        portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);
        
        if (portgroup != NULL) {
          if (portgroup->crossfade_group != NULL) {
        
            krad_mixer_crossfade_group_destroy (krad_mixer, portgroup->crossfade_group);
        
            if (strlen(string) == 0) {
              //krad_ipc_server_mixer_broadcast2 ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED, portgroupname, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, "");    
              return 0;
            }
          }

          if (strlen(string) > 0) {
        
            portgroup2 = krad_mixer_get_portgroup_from_sysname (krad_mixer, string);
        
            if (portgroup2 != NULL) {
              if (portgroup2->crossfade_group != NULL) {
                krad_mixer_crossfade_group_destroy (krad_mixer, portgroup2->crossfade_group);
              }
            
              if (portgroup != portgroup2) {
            
                krad_mixer_crossfade_group_create (krad_mixer, portgroup, portgroup2);

                //krad_ipc_server_mixer_broadcast2 ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED, portgroupname, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, string);
              }
            }
          }
        }
      }
      
      
      if (ebml_id == EBML_ID_KRAD_MIXER_MAP_CHANNEL) {

        krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
        if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL) {
          printke ("w0t");
        }
        numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      

        krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
        if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL) {
          printke ("w0t");
        }
        numbers[1] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);      
      
      
        krad_mixer_portgroup_map_channel (krad_mixer_get_portgroup_from_sysname (krad_mixer,
                                             portgroupname),
                                             numbers[0],
                                             numbers[1]);      
      
      
      }

      if (ebml_id == EBML_ID_KRAD_MIXER_MIXMAP_CHANNEL) {

        krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
        if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL) {
          printke ("w0t");
        }
        numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      

        krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
        if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL) {
          printke ("w0t");
        }
        numbers[1] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);      
      
      
        krad_mixer_portgroup_mixmap_channel (krad_mixer_get_portgroup_from_sysname (krad_mixer,
                                             portgroupname),
                                             numbers[0],
                                             numbers[1]);      
      
      
      }

      break;
      
    case EBML_ID_KRAD_MIXER_CMD_LOCAL_AUDIOPORT_DESTROY:

      for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
        portgroup = krad_mixer->portgroup[p];
        if (portgroup->io_type == KLOCALSHM) {
          krad_mixer_portgroup_destroy (krad_mixer, portgroup);
        }
      }
        
      break;

    case EBML_ID_KRAD_MIXER_CMD_LOCAL_AUDIOPORT_CREATE:
    
      sd1 = 0;
      sd2 = 0;

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION ) {
        printke ("hrm wtf3");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

      if (strncmp(string, "output", 6) == 0) {
        direction = OUTPUT;
      } else {
        direction = INPUT;
      }

      sd1 = krad_ipc_server_recvfd (krad_ipc->current_client);
      sd2 = krad_ipc_server_recvfd (krad_ipc->current_client);
        
      printk ("AUDIOPORT_CREATE Got FD's %d and %d\n", sd1, sd2);
        
      krad_mixer_local_portgroup_create (krad_mixer, "localport", direction, sd1, sd2);
        
      break;
      
    case EBML_ID_KRAD_MIXER_CMD_JACK_RUNNING:
    
      krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, &response);
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_MIXER_JACK_RUNNING,
                       krad_jack_detect ());
      krad_ipc_server_response_finish ( krad_ipc, response);
    
      return 1;
  
    case EBML_ID_KRAD_MIXER_CMD_GET_INFO:

      krad_ipc_server_response_start_with_address_and_type ( krad_ipc,
                                                             &krad_mixer->address,
                                                             EBML_ID_KRAD_UNIT_INFO,
                                                             &response);

      krad_ipc_server_payload_start ( krad_ipc, &payload_loc);

      krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER, &info_loc);
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_MIXER_SAMPLE_RATE,
                       krad_mixer_get_sample_rate (krad_mixer));
      krad_ipc_server_response_finish ( krad_ipc, info_loc);
      
      krad_ipc_server_payload_finish ( krad_ipc, payload_loc );
      krad_ipc_server_response_finish ( krad_ipc, response );
      return 1;
    case EBML_ID_KRAD_MIXER_CMD_SET_SAMPLE_RATE:

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_SAMPLE_RATE) {
        printke ("hrm wtf2\n");
      } else {
        //printf("tag name size %zu\n", ebml_data_size);
      }

      number = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);

      if (krad_mixer_has_pusher (krad_mixer) == 0) {
        krad_mixer_set_sample_rate (krad_mixer, number);
      }

      break;
  }

  return 0;
}
