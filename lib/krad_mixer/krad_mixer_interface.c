#include "krad_mixer_interface.h"

static krad_mixer_portgroup_rep_t *krad_mixer_portgroup_to_rep (krad_mixer_portgroup_t *krad_mixer_portgroup,
                                                         krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep);

//static int krad_mixer_broadcast_portgroup_destroyed (krad_mixer_t *krad_mixer, char *portgroupname);
static int krad_mixer_broadcast_portgroup_created ( krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *krad_mixer_portgroup );

static krad_mixer_portgroup_rep_t *krad_mixer_portgroup_to_rep (krad_mixer_portgroup_t *krad_mixer_portgroup,
                                                         krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep) {
  krad_mixer_portgroup_rep_t *portgroup_rep_ret;
  krad_mixer_portgroup_rep_t *portgroup_rep_crossfade;
  
  int i;
  
  if (krad_mixer_portgroup_rep == NULL) {
    portgroup_rep_ret = krad_mixer_portgroup_rep_create ();
  } else {
    portgroup_rep_ret = krad_mixer_portgroup_rep;
  }
  
  strcpy (portgroup_rep_ret->sysname, krad_mixer_portgroup->sysname);
  portgroup_rep_ret->channels = krad_mixer_portgroup->channels;
  portgroup_rep_ret->io_type = krad_mixer_portgroup->io_type;
  
  for (i = 0; i < KRAD_MIXER_MAX_CHANNELS; i++) {
    portgroup_rep_ret->volume[i] = krad_mixer_portgroup->volume[i];
    portgroup_rep_ret->map[i] = krad_mixer_portgroup->map[i];
    portgroup_rep_ret->mixmap[i] = krad_mixer_portgroup->mixmap[i];
    portgroup_rep_ret->rms[i] = krad_mixer_portgroup->rms[i];
    portgroup_rep_ret->peak[i] = krad_mixer_portgroup->peak[i];
  }
  
  if (krad_mixer_portgroup->krad_xmms != NULL) {
    portgroup_rep_ret->has_xmms2 = 1;
  }
  
  if ((krad_mixer_portgroup->crossfade_group != NULL) && 
      (krad_mixer_portgroup->crossfade_group->portgroup[0] == krad_mixer_portgroup)) {
     portgroup_rep_crossfade = krad_mixer_portgroup_rep_create ();
     
    strcpy (portgroup_rep_crossfade->sysname, krad_mixer_portgroup->crossfade_group->portgroup[1]->sysname);
    portgroup_rep_crossfade->channels = krad_mixer_portgroup->crossfade_group->portgroup[1]->channels;
    portgroup_rep_crossfade->io_type = krad_mixer_portgroup->crossfade_group->portgroup[1]->io_type;
     
    for (i = 0; i < KRAD_MIXER_MAX_CHANNELS; i++) {
      portgroup_rep_crossfade->volume[i] = krad_mixer_portgroup->crossfade_group->portgroup[1]->volume[i];
      portgroup_rep_crossfade->map[i] = krad_mixer_portgroup->crossfade_group->portgroup[1]->map[i];
      portgroup_rep_crossfade->mixmap[i] = krad_mixer_portgroup->crossfade_group->portgroup[1]->mixmap[i];
      portgroup_rep_crossfade->rms[i] = krad_mixer_portgroup->crossfade_group->portgroup[1]->rms[i];
      portgroup_rep_crossfade->peak[i] = krad_mixer_portgroup->crossfade_group->portgroup[1]->peak[i];
    }
     
    if (krad_mixer_portgroup->crossfade_group->portgroup[1]->krad_xmms != NULL) {
      portgroup_rep_crossfade->has_xmms2 = 1;
    }
     
    portgroup_rep_ret->crossfade_group_rep = krad_mixer_crossfade_rep_create (portgroup_rep_ret, portgroup_rep_crossfade);
    portgroup_rep_ret->crossfade_group_rep->fade = krad_mixer_portgroup->crossfade_group->fade;
  }
  
  return portgroup_rep_ret;
}

static int krad_mixer_broadcast_portgroup_created ( krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *portgroup ) {

  size_t size;
  unsigned char *buffer;
  krad_broadcast_msg_t *broadcast_msg;
  krad_mixer_portgroup_rep_t *portgroup_rep;

  size = 2048;
  buffer = malloc (size);

  uint64_t response;
  uint64_t created;
  krad_ebml_t *krad_ebml;

  krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_WRITEONLY);
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_MIXER_MSG, &response);
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CREATED, &created);

  portgroup_rep = krad_mixer_portgroup_to_rep (portgroup, NULL);
  krad_mixer_portgroup_rep_to_ebml (portgroup_rep, krad_ebml);
  krad_mixer_portgroup_rep_destroy (portgroup_rep);

  krad_ebml_finish_element (krad_ebml, created);
  krad_ebml_finish_element (krad_ebml, response);

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
  
  size = 256;
  buffer = malloc (size);


  krad_ebml_t *krad_ebml;
  uint64_t element_loc;
  uint64_t subelement_loc;

  krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_WRITEONLY);
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_MIXER_MSG, &element_loc);  
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_MIXER_CONTROL, &subelement_loc);  
  krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
  krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, controlname);
  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, value);
  krad_ebml_finish_element (krad_ebml, subelement_loc);
  krad_ebml_finish_element (krad_ebml, element_loc);
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

/*
static int krad_mixer_broadcast_portgroup_destroyed (krad_mixer_t *krad_mixer, char *portgroupname) {

  size_t size;
  unsigned char *buffer;
  krad_broadcast_msg_t *broadcast_msg;
  
  size = 256;
  buffer = malloc (size);


  krad_ebml_t *krad_ebml;
  uint64_t element_loc;
  uint64_t subelement_loc;

  krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_WRITEONLY);
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_MIXER_MSG, &element_loc);  
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_DESTROYED, &subelement_loc);  
  krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
  krad_ebml_finish_element (krad_ebml, subelement_loc);
  krad_ebml_finish_element (krad_ebml, element_loc);
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
*/
int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc ) {

  uint32_t command;
  uint32_t ebml_id;  
  uint64_t ebml_data_size;

  krad_mixer_portgroup_t *portgroup;
  krad_mixer_portgroup_t *portgroup2;
  
  krad_mixer_portgroup_rep_t *portgroup_rep;
  
  uint64_t element;
  uint64_t response;

  krad_mixer_output_t output_type;

  int p;
  
  int sd1;
  int sd2;
  
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
  
  krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size );

  switch ( command ) {

    case EBML_ID_KRAD_MIXER_CMD_GET_CONTROL:
      //printk ("Get Control\n");
      return 1;
      break;  
    case EBML_ID_KRAD_MIXER_CMD_SET_CONTROL:
      //printf("krad mixer handler! got set control\n");      

    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
        printke ("hrm wtf2\n");
      } else {
        //printf("tag name size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portname, ebml_data_size);
  
      //printk ("Set Control: %s / ", portname);
  
  
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_NAME) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, controlname, ebml_data_size);
  
      //printk ("%s = ", controlname);

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id == EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
        floatval = krad_ebml_read_float (krad_ipc->current_client->krad_ebml, ebml_data_size);
  
        //printk ("%f\n", floatval);

        krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
        if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_DURATION) {
          printke ("krm wtf3\n");
        }
        number = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
        
        krad_mixer_set_portgroup_control ( krad_mixer, portname, controlname, floatval, number );
        //krad_mixer_broadcast_portgroup_control ( krad_mixer, portname, controlname, floatval );

      } else {

      }

      return 2;

    case EBML_ID_KRAD_MIXER_CMD_ADD_EFFECT:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
        printke ("hrm wtf");
      } else {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      }
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (ebml_id != EBML_ID_KRAD_EFFECT_NAME) {
        printke ("hrm wtf");
      } else {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
      }

      portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);

      if (portgroup->direction == INPUT) {
        kr_effects_effect_add (portgroup->effects, kr_effects_string_to_effect (string));
      }

      break;
    case EBML_ID_KRAD_MIXER_CMD_REMOVE_EFFECT:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
        printke ("hrm wtf");
      } else {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      }

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_EFFECT_NUM) {
        printke ("hrm wtf2\n");
      } else {
        number = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);

      kr_effects_effect_remove (portgroup->effects, number);

      break;
    case EBML_ID_KRAD_MIXER_CMD_SET_EFFECT_CONTROL:
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
        printke ("hrm wtf");
      } else {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      }

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_EFFECT_NUM) {
        printke ("hrm wtf2\n");
      } else {
        number = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  
      if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_NAME) {
        printke ("hrm wtf");
      } else {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, controlname, ebml_data_size);
      }

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_SUBUNIT) {
        printke ("hrm wtf2\n");
      } else {
        numbers[5] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
        floatval = krad_ebml_read_float (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }

      portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);

      kr_effects_effect_set_control (portgroup->effects, number,
                                     kr_effects_string_to_effect_control(portgroup->effects[number].effect->effect_type, controlname),
                                     numbers[5], floatval);

      break;
    case EBML_ID_KRAD_MIXER_CMD_PUSH_TONE:
    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_TONE_NAME) {
        printke ("hrm wtf2\n");
      } else {
        //printf("tag name size %zu\n", ebml_data_size);
      }
      if (krad_mixer->push_tone == NULL) {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, krad_mixer->push_tone_value, ebml_data_size);
        krad_mixer->push_tone = krad_mixer->push_tone_value;
      } else {
        krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);
      }
      break;

    case EBML_ID_KRAD_MIXER_CMD_PORTGROUP_XMMS2_CMD:


      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);    
    
    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_XMMS2_CMD) {
        printke ("hrm wtf2\n");
      } else {
        //printf("tag name size %zu\n", ebml_data_size);
      }
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

      krad_mixer_portgroup_xmms2_cmd (krad_mixer, portgroupname, string);

      break;
      
    case EBML_ID_KRAD_MIXER_CMD_PLUG_PORTGROUP:
    
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

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);    
    
    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_XMMS2_IPC_PATH) {
        printke ("hrm wtf2\n");
      } else {
        //printf("tag name size %zu\n", ebml_data_size);
      }
      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

      krad_mixer_bind_portgroup_xmms2 (krad_mixer, portgroupname, string);

      break;

    case EBML_ID_KRAD_MIXER_CMD_UNBIND_PORTGROUP_XMMS2:
    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);    
    
      krad_mixer_unbind_portgroup_xmms2 (krad_mixer, portgroupname);
    
      break;
            
      
    case EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS:

      krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, &response);
      krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_MIXER_PORTGROUP_LIST, &element);

      for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
        portgroup = krad_mixer->portgroup[p];
        if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {

          portgroup_rep = krad_mixer_portgroup_to_rep(portgroup, NULL);
          krad_mixer_portgroup_rep_to_ebml (portgroup_rep, krad_ipc->current_client->krad_ebml2);
          krad_mixer_portgroup_rep_destroy (portgroup_rep);
          
        }
      }

      krad_ipc_server_response_list_finish ( krad_ipc, element );
      krad_ipc_server_response_finish ( krad_ipc, response );
      return 1;

    case EBML_ID_KRAD_MIXER_CMD_CREATE_PORTGROUP:
    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION ) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

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
      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS) {
        printke ("w0t");
      }
      
      numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);

      portgroup = krad_mixer_portgroup_create (krad_mixer, portgroupname, direction, output_type, numbers[0],
                     krad_mixer->master_mix, KRAD_AUDIO, NULL, JACK);      

      if (portgroup != NULL) {
        //if ((portgroup->direction == INPUT) || ((portgroup->direction == OUTPUT) && (portgroup->output_type == AUX))) {
        if (portgroup->direction == INPUT) {
          krad_mixer_broadcast_portgroup_created ( krad_mixer, portgroup );
        }
      }

      break;
    case EBML_ID_KRAD_MIXER_CMD_DESTROY_PORTGROUP:  
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      
      krad_mixer_portgroup_destroy (krad_mixer, krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname));
    
      krad_radio_broadcast_subunit_destroyed (krad_mixer->broadcaster, KR_MIXER, KR_PORTGROUP, portgroupname);
      //krad_mixer_broadcast_portgroup_destroyed ( krad_mixer, portgroupname );
  
      break;
      
    case EBML_ID_KRAD_MIXER_CMD_PORTGROUP_INFO:  
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME ) {
        printke ("hrm wtf3\n");
      } else {
        //printf("tag value size %zu\n", ebml_data_size);
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, portgroupname, ebml_data_size);
      
      portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);

      if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
        krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, &response);
        portgroup_rep = krad_mixer_portgroup_to_rep(portgroup, NULL);
        krad_mixer_portgroup_rep_to_ebml (portgroup_rep, krad_ipc->current_client->krad_ebml2);
        krad_mixer_portgroup_rep_destroy (portgroup_rep);
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
  
    case EBML_ID_KRAD_MIXER_CMD_GET_SAMPLE_RATE:
    
      krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, &response);
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_MIXER_SAMPLE_RATE, 
                       krad_mixer_get_sample_rate (krad_mixer));
      krad_ipc_server_response_finish ( krad_ipc, response);
    
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
