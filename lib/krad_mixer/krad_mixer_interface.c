#include "krad_mixer_interface.h"

int krad_mixer_handler ( krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc ) {

	uint32_t command;
	uint32_t ebml_id;	
	uint64_t ebml_data_size;
//	uint64_t number;
	krad_mixer_portgroup_t *portgroup;
	krad_mixer_portgroup_t *portgroup2;
	uint64_t element;
	
	uint64_t response;
//	uint64_t broadcast;
	
	char *crossfade_name;
	float crossfade_value;
	int p;
	
	int sd1;
	int sd2;
			
	sd1 = 0;
	sd2 = 0;

	ebml_id = 0;
	
	char portname[256];
	char portgroupname[256];
	char portgroupname2[256];	
	char controlname[256];	
	float floatval;

	int has_xmms2;

	char string[512];
	int direction;
	int number;
	int numbers[16];
	
	number = 0;
	direction = 0;
	crossfade_name = NULL;
	//i = 0;
	
	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {

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

				krad_mixer_set_portgroup_control (krad_mixer, portname, controlname, floatval);

				krad_ipc_server_mixer_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_CONTROL, portname, controlname, floatval);
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

      kr_effects_effect_add (portgroup->effects, kr_effects_string_to_effect (string));

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

			//printk ("List Portgroups\n");

			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_MIXER_PORTGROUP_LIST, &element);
			
			for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
				portgroup = krad_mixer->portgroup[p];
				if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
					crossfade_name = "";
					crossfade_value = 0.0f;
					has_xmms2 = 0;
					if (portgroup->krad_xmms != NULL) {
						has_xmms2 = 1;
					}
					
					if (portgroup->crossfade_group != NULL) {
					
						if (portgroup->crossfade_group->portgroup[0] == portgroup) {
							crossfade_name = portgroup->crossfade_group->portgroup[1]->sysname;
							crossfade_value = portgroup->crossfade_group->fade;
						}
					}
				
					krad_ipc_server_response_add_portgroup ( krad_ipc, portgroup->sysname, portgroup->channels,
											  				 portgroup->io_type, portgroup->volume[0],
											  				 portgroup->mixbus->sysname, crossfade_name,
											  				 crossfade_value, has_xmms2 );
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );
			return 1;
			break;
		// FIXME create / remove portgroup need broadcast	
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
			} else {
				direction = INPUT;
			}
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS) {
				printke ("w0t");
			}
			
			numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);

			portgroup = krad_mixer_portgroup_create (krad_mixer, portgroupname, direction, numbers[0],
										 krad_mixer->master_mix, KRAD_AUDIO, NULL, JACK);			

			if (portgroup != NULL) {

				krad_ipc_server_broadcast_portgroup_created ( krad_ipc, portgroup->sysname, portgroup->channels,
												  	   		  portgroup->io_type, portgroup->volume[0],
												  	   		  portgroup->mixbus->sysname, 0 );
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
		
			krad_ipc_server_simple_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_PORTGROUP_DESTROYED, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);		
		
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
							krad_ipc_server_mixer_broadcast2 ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED, portgroupname, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, "");		
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

								krad_ipc_server_mixer_broadcast2 ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED, portgroupname, EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, string);
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
			
	}

	return 0;

}
