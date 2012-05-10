#include "krad_radio.h"

void krad_radio_destroy (krad_radio_t *krad_radio) {

	krad_http_server_destroy (krad_radio->krad_http);
	krad_websocket_server_destroy (krad_radio->krad_websocket);
	krad_ipc_server_destroy (krad_radio->krad_ipc);
	krad_linker_destroy (krad_radio->krad_linker);
	krad_mixer_destroy (krad_radio->krad_mixer);
	krad_compositor_destroy (krad_radio->krad_compositor);
	krad_tags_destroy (krad_radio->krad_tags);	
	free (krad_radio->sysname);
	free (krad_radio);

}

krad_radio_t *krad_radio_create (char *sysname) {

	krad_radio_t *krad_radio = calloc(1, sizeof(krad_radio_t));


	if (!krad_valid_sysname(sysname)) {
		return NULL;
	}
	krad_radio->sysname = strdup (sysname);
	
	krad_radio->krad_tags = krad_tags_create ();
	
	if (krad_radio->krad_tags == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_mixer = krad_mixer_create (krad_radio);
	
	if (krad_radio->krad_mixer == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_compositor = krad_compositor_create (640, 360);
	
	if (krad_radio->krad_compositor == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}	
	
	krad_radio->krad_linker = krad_linker_create (krad_radio);
	
	if (krad_radio->krad_linker == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}	
	
	krad_radio->krad_ipc = krad_ipc_server ( sysname, krad_radio_handler, krad_radio );
	
	if (krad_radio->krad_ipc == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
		
	return krad_radio;

}


void krad_radio (char *sysname) {

	krad_radio_t *krad_radio_station;

	krad_system_init ();

	krad_radio_station = krad_radio_create (sysname);

	if (krad_radio_station != NULL) {
	
		printf ("Krad Radio Station %s Daemonizing..\n", krad_radio_station->sysname);
		
		//krad_system_daemonize ();

		krad_radio_run ( krad_radio_station );

		krad_radio_destroy (krad_radio_station);

	}
}


void krad_radio_run (krad_radio_t *krad_radio) {

	while (1) {
		sleep (25);
	}
}


int krad_radio_handler ( void *output, int *output_len, void *ptr ) {

	krad_radio_t *krad_radio_station = (krad_radio_t *)ptr;
	
	uint32_t ebml_id;	
	uint32_t command;
	uint64_t ebml_data_size;
	uint64_t element;
	uint64_t response;
//	uint64_t broadcast;
	uint64_t numbers[10];
	
	char tag_name_actual[256];
	char tag_value_actual[1024];
	
	tag_name_actual[0] = '\0';
	tag_value_actual[0] = '\0';
	
	char *tag_name = tag_name_actual;
	char *tag_value = tag_value_actual;
	
	int i;
	
	i = 0;
	command = 0;
	ebml_id = 0;
	ebml_data_size = 0;
	element = 0;
	response = 0;

	//printf("handler! \n");
	
	krad_ipc_server_read_command ( krad_radio_station->krad_ipc, &command, &ebml_data_size);

	switch ( command ) {
		
		/* Handoffs */
		case EBML_ID_KRAD_MIXER_CMD:
			printf("Krad Mixer Command\n");
			return krad_mixer_handler ( krad_radio_station->krad_mixer, krad_radio_station->krad_ipc );
		case EBML_ID_KRAD_COMPOSITOR_CMD:
			printf("Krad Compositor Command\n");
			return krad_compositor_handler ( krad_radio_station->krad_compositor, krad_radio_station->krad_ipc );			
		case EBML_ID_KRAD_LINK_CMD:
			printf("Krad Link Command\n");
			return krad_linker_handler ( krad_radio_station->krad_linker, krad_radio_station->krad_ipc );

		/* Krad Radio Commands */
		case EBML_ID_KRAD_RADIO_CMD:
			printf("Krad Radio Command\n");
			return krad_radio_handler ( output, output_len, ptr );
		case EBML_ID_KRAD_RADIO_CMD_LIST_TAGS:
			printf("LIST_TAGS\n");
			krad_ipc_server_response_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_response_list_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_TAG_LIST, &element);
			
			while (krad_tags_get_next_tag ( krad_radio_station->krad_tags, &i, &tag_name, &tag_value)) {
				krad_ipc_server_response_add_tag ( krad_radio_station->krad_ipc, tag_name, tag_value);
				printf("Tag %d: %s - %s\n", i, tag_name, tag_value);
			}
			
			krad_ipc_server_response_list_finish ( krad_radio_station->krad_ipc, element);
			krad_ipc_server_response_finish ( krad_radio_station->krad_ipc, response);
			return 1;

		case EBML_ID_KRAD_RADIO_CMD_SET_TAG:
			
			krad_ipc_server_read_tag ( krad_radio_station->krad_ipc, &tag_name, &tag_value );
			
			krad_tags_set_tag ( krad_radio_station->krad_tags, tag_name, tag_value);

			printf("Set Tag %s to %s\n", tag_name, tag_value);
			

			*output_len = 666;
			return 2;

		case EBML_ID_KRAD_RADIO_CMD_GET_TAG:
			krad_ipc_server_read_tag ( krad_radio_station->krad_ipc, &tag_name, &tag_value );
			
			tag_value = krad_tags_get_tag (krad_radio_station->krad_tags, tag_name);
			
			printf("Get Tag %s - %s\n", tag_name, tag_value);
			
			krad_ipc_server_response_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_response_add_tag ( krad_radio_station->krad_ipc, tag_name, tag_value);
			krad_ipc_server_response_finish ( krad_radio_station->krad_ipc, response);

			return 1;
			
		case EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE:
		
			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_HTTP_PORT) {
				printf("hrm wtf5\n");
			}
			
			numbers[0] = krad_ebml_read_number ( krad_radio_station->krad_ipc->current_client->krad_ebml, ebml_data_size);	

			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEBSOCKET_PORT) {
				printf("hrm wtf6\n");
			}
			
			numbers[1] = krad_ebml_read_number ( krad_radio_station->krad_ipc->current_client->krad_ebml, ebml_data_size);
		
			// Remove existing if existing (ie. I am changing the port)
			if (krad_radio_station->krad_http != NULL) {
				krad_http_server_destroy (krad_radio_station->krad_http);
			}
			if (krad_radio_station->krad_websocket != NULL) {
				krad_websocket_server_destroy (krad_radio_station->krad_websocket);
			}		
		
			krad_radio_station->krad_http = krad_http_server_create ( numbers[0], numbers[1] );
			krad_radio_station->krad_websocket = krad_websocket_server_create ( krad_radio_station->sysname, numbers[1] );
		
			return 0;
		
		case EBML_ID_KRAD_RADIO_CMD_WEB_DISABLE:
			
			krad_http_server_destroy (krad_radio_station->krad_http);
			krad_websocket_server_destroy (krad_radio_station->krad_websocket);			
			
			krad_radio_station->krad_http = NULL;
			krad_radio_station->krad_websocket = NULL;
			
			return 0;
			
		case EBML_ID_KRAD_RADIO_CMD_REMOTE_ENABLE:
			
			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TCP_PORT) {
				printk ("hrm wtf6\n");
			}
			
			numbers[0] = krad_ebml_read_number ( krad_radio_station->krad_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_ipc_server_enable_remote (krad_radio_station->krad_ipc, numbers[0]);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE:
			
			krad_ipc_server_disable_remote (krad_radio_station->krad_ipc);
			
			return 0;
			
		default:
			printf("Krad Radio Command Unknown! %u\n", command);
			return 0;
	}

	return 0;

}
