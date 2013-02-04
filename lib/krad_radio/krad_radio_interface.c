#include "krad_radio_interface.h"
#include "krad_radio_internal.h"

int krad_radio_broadcast_subunit_destroyed (krad_ipc_broadcaster_t *broadcaster, kr_address_t *address) {

  size_t size;
  unsigned char *buffer;
  krad_broadcast_msg_t *broadcast_msg;
  
  size = 256;
  buffer = malloc (size);

  krad_ebml_t *krad_ebml;
  uint64_t message_loc;

  krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_WRITEONLY);
  
  krad_radio_address_to_ebml (krad_ebml, &message_loc, address);
  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_TYPE, EBML_ID_KRAD_RADIO_UNIT_DESTROYED);
  krad_ebml_finish_element (krad_ebml, message_loc);

  size = krad_ebml->io_adapter.write_buffer_pos;
  memcpy (buffer, krad_ebml->io_adapter.write_buffer, size);
  krad_ebml_destroy (krad_ebml);

  broadcast_msg = krad_broadcast_msg_create (buffer, size);

  free (buffer);

  if (broadcast_msg != NULL) {
    return krad_ipc_server_broadcaster_broadcast ( broadcaster, &broadcast_msg );
  }

  return -1;
}

int krad_radio_handler ( void *output, int *output_len, void *ptr ) {

	krad_radio_t *krad_radio = (krad_radio_t *)ptr;
	krad_ipc_server_t *kr_ipc;
	
	kr_ipc = krad_radio->remote.krad_ipc;
	
	uint32_t ebml_id;	
	uint64_t list;	
	uint32_t command;
	uint64_t ebml_data_size;
	uint64_t element;
	uint64_t response;
//	uint64_t broadcast;
	uint64_t numbers[10];
	krad_tags_t *krad_tags;
	
	char tag_item_actual[256];	
	char tag_name_actual[256];
	char tag_value_actual[1024];
	
	tag_item_actual[0] = '\0';	
	tag_name_actual[0] = '\0';
	tag_value_actual[0] = '\0';
	
	char *tag_item = tag_item_actual;	
	char *tag_name = tag_name_actual;
	char *tag_value = tag_value_actual;
	
	int i;
	
	char string1[512];	
	char string2[512];
	char string3[512];	
	
	string1[0] = '\0';
	string2[0] = '\0';
	string3[0] = '\0';	
	
	krad_tags = NULL;
	i = 0;
	command = 0;
	ebml_id = 0;
	ebml_data_size = 0;
	element = 0;
	response = 0;

	//printf("handler! ");
	
	krad_ipc_server_read_command ( kr_ipc, &command, &ebml_data_size);

	switch ( command ) {
		
		/* Handoffs */
		case EBML_ID_KRAD_MIXER_CMD:
			//printk ("Krad Mixer Command");
			return krad_mixer_handler ( krad_radio->krad_mixer, kr_ipc );
		case EBML_ID_KRAD_COMPOSITOR_CMD:
			//printk ("Krad Compositor Command");
			return krad_compositor_handler ( krad_radio->krad_compositor, kr_ipc );			
		case EBML_ID_KRAD_TRANSPONDER_CMD:
			//printk ("Krad Link Command");
			return krad_transponder_handler ( krad_radio->krad_transponder, kr_ipc );

		/* Krad Radio Commands */
		case EBML_ID_KRAD_RADIO_CMD:
			//printk ("Krad Radio Command");
			return krad_radio_handler ( output, output_len, ptr );
		case EBML_ID_KRAD_RADIO_CMD_LIST_TAGS:
			//printk ("LIST_TAGS");
			
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (kr_ipc->current_client->krad_ebml, tag_item, ebml_data_size);
			
			//printk ("Get Tags for %s", tag_item);

			if (strncmp("station", tag_item, 7) == 0) {
				krad_tags = krad_radio->krad_tags;
			} else {
				krad_tags = krad_radio_find_tags_for_item ( krad_radio, tag_item );
			}
			
			if (krad_tags != NULL) {
				//printk ("Got Tags for %s", tag_item);
		
				krad_ipc_server_response_start ( kr_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
				krad_ipc_server_response_list_start ( kr_ipc, EBML_ID_KRAD_RADIO_TAG_LIST, &element);
			
				while (krad_tags_get_next_tag ( krad_tags, &i, &tag_name, &tag_value)) {
					krad_ipc_server_response_add_tag ( kr_ipc, tag_item, tag_name, tag_value);
					//printk ("Tag %d: %s - %s", i, tag_name, tag_value);
				}
			
				krad_ipc_server_response_list_finish ( kr_ipc, element );
				krad_ipc_server_response_finish ( kr_ipc, response );

			} else {
				printke ("Could not find %s", tag_item);
			}

			return 1;

		case EBML_ID_KRAD_RADIO_CMD_SET_TAG:
			
			krad_ipc_server_read_tag ( kr_ipc, &tag_item, &tag_name, &tag_value );
			
			if (strncmp("station", tag_item, 7) == 0) {
				krad_tags_set_tag ( krad_radio->krad_tags, tag_name, tag_value);
			} else {
				krad_tags = krad_radio_find_tags_for_item ( krad_radio, tag_item );
				if (krad_tags != NULL) {
					krad_tags_set_tag ( krad_tags, tag_name, tag_value);
					printk ("Set Tag %s on %s to %s", tag_name, tag_item, tag_value);
				} else {
					printke ("Could not find %s", tag_item);
				}
			}
			
			//krad_ipc_server_broadcast_tag ( kr_ipc, tag_item, tag_name, tag_value);

			return 2;

		case EBML_ID_KRAD_RADIO_CMD_GET_TAG:
			krad_ipc_server_read_tag ( kr_ipc, &tag_item, &tag_name, &tag_value );
			//printk ("Get Tag %s - %s - %s", tag_item, tag_name, tag_value);
			if (strncmp("station", tag_item, 7) == 0) {
				tag_value = krad_tags_get_tag (krad_radio->krad_tags, tag_name);
			} else {
				krad_tags = krad_radio_find_tags_for_item ( krad_radio, tag_item );
				if (krad_tags != NULL) {
					tag_value = krad_tags_get_tag ( krad_tags, tag_name );
					//printk ("Got Tag %s on %s - %s", tag_name, tag_item, tag_value);
				} else {
					printke ("Could not find %s", tag_item);
				}
			}
			
			if (strlen(tag_value)) {
				krad_ipc_server_response_start ( kr_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
				krad_ipc_server_response_add_tag ( kr_ipc, tag_item, tag_name, tag_value);
				krad_ipc_server_response_finish ( kr_ipc, response);
			}
			return 1;
			
		case EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE:
		
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_HTTP_PORT) {
				printke ("hrm wtf5");
			}
			
			numbers[0] = krad_ebml_read_number ( kr_ipc->current_client->krad_ebml, ebml_data_size);	

			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEBSOCKET_PORT) {
				printke ("hrm wtf6");
			}
			
			numbers[1] = krad_ebml_read_number ( kr_ipc->current_client->krad_ebml, ebml_data_size);
		
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEB_HEADCODE) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (kr_ipc->current_client->krad_ebml, string1, ebml_data_size);
		
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEB_HEADER) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (kr_ipc->current_client->krad_ebml, string2, ebml_data_size);
			
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEB_FOOTER) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (kr_ipc->current_client->krad_ebml, string3, ebml_data_size);
		
		
			// Remove existing if existing (ie. I am changing the port)
			if (krad_radio->remote.krad_http != NULL) {
				krad_http_server_destroy (krad_radio->remote.krad_http);
			}
			if (krad_radio->remote.krad_websocket != NULL) {
				krad_websocket_server_destroy (krad_radio->remote.krad_websocket);
			}
		
			krad_radio->remote.krad_http = krad_http_server_create ( krad_radio, numbers[0], numbers[1],
																	  string1, string2, string3 );
      krad_radio->remote.krad_websocket = krad_websocket_server_create ( krad_radio->sysname, numbers[1] );
		
			return 0;
		
		case EBML_ID_KRAD_RADIO_CMD_WEB_DISABLE:
			
			krad_http_server_destroy (krad_radio->remote.krad_http);
      krad_websocket_server_destroy (krad_radio->remote.krad_websocket);			
			
			krad_radio->remote.krad_http = NULL;
			krad_radio->remote.krad_websocket = NULL;
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_GET_REMOTE_STATUS:

			krad_ipc_server_response_start ( kr_ipc, EBML_ID_KRAD_RADIO_MSG, &response);

      krad_ipc_server_response_list_start ( kr_ipc, EBML_ID_KRAD_RADIO_REMOTE_STATUS_LIST, &list);

      for (i = 0; i < MAX_REMOTES; i++) {
        if (kr_ipc->tcp_port[i]) {
	        krad_ebml_start_element (kr_ipc->current_client->krad_ebml2, EBML_ID_KRAD_RADIO_REMOTE_STATUS, &element);
			    krad_ipc_server_respond_string ( kr_ipc, EBML_ID_KRAD_RADIO_REMOTE_INTERFACE, kr_ipc->tcp_interface[i]);
			    krad_ipc_server_respond_number ( kr_ipc, EBML_ID_KRAD_RADIO_REMOTE_PORT, kr_ipc->tcp_port[i]);
	        krad_ebml_finish_element (kr_ipc->current_client->krad_ebml2, element);
        }      
      }

      krad_ipc_server_response_list_finish ( kr_ipc, list );
		
			krad_ipc_server_response_finish ( kr_ipc, response);
			
			return 0;
			
		case EBML_ID_KRAD_RADIO_CMD_REMOTE_ENABLE:

			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_REMOTE_INTERFACE) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (kr_ipc->current_client->krad_ebml, string1, ebml_data_size);
			
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TCP_PORT) {
				printke ("hrm wtf6");
			}
			
			numbers[0] = krad_ebml_read_number ( kr_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_ipc_server_enable_remote (kr_ipc, string1, numbers[0]);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE:
			
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_REMOTE_INTERFACE) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (kr_ipc->current_client->krad_ebml, string1, ebml_data_size);
			
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TCP_PORT) {
				printke ("hrm wtf6");
			}
			
			numbers[0] = krad_ebml_read_number ( kr_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_ipc_server_disable_remote (kr_ipc, string1, numbers[0]);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_OSC_ENABLE:
			
			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_UDP_PORT) {
				printke ("hrm wtf6");
			}
			
			numbers[0] = krad_ebml_read_number ( kr_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_osc_listen (krad_radio->remote.krad_osc, numbers[0]);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_OSC_DISABLE:
			
			krad_osc_stop_listening (krad_radio->remote.krad_osc);
			
			return 0;
			
		case EBML_ID_KRAD_RADIO_CMD_UPTIME:
			krad_ipc_server_response_start ( kr_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_respond_number ( kr_ipc, EBML_ID_KRAD_RADIO_UPTIME, krad_system_daemon_uptime());
			krad_ipc_server_response_finish ( kr_ipc, response);
		
			return 1;
		case EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_INFO:
			krad_ipc_server_response_start ( kr_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_respond_string ( kr_ipc, EBML_ID_KRAD_RADIO_SYSTEM_INFO, krad_system_info());
			krad_ipc_server_response_finish ( kr_ipc, response);
			return 1;
			
		case EBML_ID_KRAD_RADIO_CMD_SET_DIR:

			krad_ebml_read_element ( kr_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_DIR) {
				printke ("hrm wtf6");
			}
			
			krad_ebml_read_string (kr_ipc->current_client->krad_ebml, tag_value_actual, ebml_data_size);
			
			if (strlen(tag_value_actual)) {
				krad_radio_set_dir ( krad_radio, tag_value_actual );
			}
			
			return 0;
	
		case EBML_ID_KRAD_RADIO_CMD_GET_LOGNAME:

			krad_ipc_server_response_start ( kr_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_respond_string ( kr_ipc, EBML_ID_KRAD_RADIO_LOGNAME, krad_radio->log.filename);
			krad_ipc_server_response_finish ( kr_ipc, response);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_CPU_USAGE:

			krad_ipc_server_response_start ( kr_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_respond_number ( kr_ipc, EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE, krad_system_get_cpu_usage());
			krad_ipc_server_response_finish ( kr_ipc, response);
			
			return 0;
			
		case EBML_ID_KRAD_RADIO_CMD_BROADCAST_SUBSCRIBE:
		
			numbers[0] = krad_ebml_read_number ( kr_ipc->current_client->krad_ebml, ebml_data_size);		
		
			krad_ipc_server_add_client_to_broadcast ( kr_ipc, numbers[0] );
		
			return 0;

		default:
			printke ("Krad Radio Command Unknown! %u", command);
			return 0;
	}

	return 0;

}
