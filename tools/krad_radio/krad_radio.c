#include "krad_radio.h"

static krad_tags_t *krad_radio_find_tags_for_item ( krad_radio_t *krad_radio_station, char *item );
static void krad_radio_set_dir ( krad_radio_t *krad_radio_station, char *dir );
static void krad_radio_destroy (krad_radio_t *krad_radio);
static krad_radio_t *krad_radio_create (char *sysname);
static void krad_radio_run (krad_radio_t *krad_radio);
static int krad_radio_handler ( void *output, int *output_len, void *ptr );

static void krad_radio_destroy (krad_radio_t *krad_radio) {

  	krad_system_monitor_cpu_off ();
	if (krad_radio->krad_osc != NULL) {
		krad_osc_destroy (krad_radio->krad_osc);
		krad_radio->krad_osc = NULL;
	}
	if (krad_radio->krad_http != NULL) {
		krad_http_server_destroy (krad_radio->krad_http);
		krad_radio->krad_http = NULL;
	}
	if (krad_radio->krad_websocket != NULL) {
		krad_websocket_server_destroy (krad_radio->krad_websocket);
		krad_radio->krad_websocket = NULL;
	}
	if (krad_radio->krad_ipc != NULL) {
		krad_ipc_server_destroy (krad_radio->krad_ipc);
		krad_radio->krad_ipc = NULL;
	}
	if (krad_radio->krad_linker != NULL) {
		krad_linker_destroy (krad_radio->krad_linker);
		krad_radio->krad_linker = NULL;		
	}
	if (krad_radio->krad_mixer != NULL) {
		krad_mixer_destroy (krad_radio->krad_mixer);
		krad_radio->krad_mixer = NULL;		
	}
	if (krad_radio->krad_compositor != NULL) {
		krad_compositor_destroy (krad_radio->krad_compositor);
		krad_radio->krad_compositor = NULL;
	}
	if (krad_radio->krad_tags != NULL) {
		krad_tags_destroy (krad_radio->krad_tags);
		krad_radio->krad_tags = NULL;
	}	
	
	free (krad_radio->sysname);
	free (krad_radio);

	printk ("Krad Radio Daemon exited cleanly");
	krad_system_log_off ();

}

static krad_radio_t *krad_radio_create (char *sysname) {

	krad_radio_t *krad_radio = calloc (1, sizeof(krad_radio_t));

	if (!krad_valid_sysname(sysname)) {
		return NULL;
	}
	krad_radio->sysname = strdup (sysname);
	
	krad_radio->krad_tags = krad_tags_create ("station");
	
	if (krad_radio->krad_tags == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_mixer = krad_mixer_create (krad_radio->sysname);
	
	if (krad_radio->krad_mixer == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_compositor = krad_compositor_create (DEFAULT_COMPOSITOR_WIDTH,
														  DEFAULT_COMPOSITOR_HEIGHT,
														  DEFAULT_COMPOSITOR_FPS_NUMERATOR,
														  DEFAULT_COMPOSITOR_FPS_DENOMINATOR);
	
	if (krad_radio->krad_compositor == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
	
	krad_compositor_set_krad_mixer (krad_radio->krad_compositor, krad_radio->krad_mixer);	
	
	krad_radio->krad_linker = krad_linker_create (krad_radio);
	
	if (krad_radio->krad_linker == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_osc = krad_osc_create ();
	
	if (krad_radio->krad_osc == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}	
	
	krad_radio->krad_ipc = krad_ipc_server_create ( sysname, krad_radio_handler, krad_radio );
	
	if (krad_radio->krad_ipc == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
	
	krad_ipc_server_register_broadcast ( krad_radio->krad_ipc, EBML_ID_KRAD_RADIO_GLOBAL_BROADCAST );
	
	krad_mixer_set_ipc (krad_radio->krad_mixer, krad_radio->krad_ipc);
	krad_tags_set_set_tag_callback (krad_radio->krad_tags, krad_radio->krad_ipc, 
									(void (*)(void *, char *, char *, char *, int))krad_ipc_server_broadcast_tag);
		
	return krad_radio;

}

void krad_radio_daemon (char *sysname) {

	pid_t pid;
	krad_radio_t *krad_radio_station;

	pid = fork();

	if (pid < 0) {
		exit (1);
	}

	if (pid > 0) {
		return;
	}

	krad_system_init ();

	krad_radio_station = krad_radio_create (sysname);

	if (krad_radio_station != NULL) {
		krad_radio_run ( krad_radio_station );
		krad_radio_destroy (krad_radio_station);
	}
}

void krad_radio_cpu_monitor_callback (krad_radio_t *krad_radio_station, uint32_t usage) {

	//printk ("System CPU Usage: %d%%", usage);
	krad_ipc_server_simplest_broadcast ( krad_radio_station->krad_ipc,
										 EBML_ID_KRAD_RADIO_MSG,
										 EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE,
										 usage);

}

static void krad_radio_run (krad_radio_t *krad_radio_station) {

	int signal_caught;
	sigset_t signal_mask;

	krad_system_daemonize ();

    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGINT);
    sigaddset (&signal_mask, SIGTERM);
    sigaddset (&signal_mask, SIGHUP);
    if (pthread_sigmask (SIG_BLOCK, &signal_mask, NULL) != 0) {
		failfast ("Could not set signal mask!");
    }

  	krad_system_monitor_cpu_on ();
  	
	krad_system_set_monitor_cpu_callback ((void *)krad_radio_station, 
									 (void (*)(void *, uint32_t))krad_radio_cpu_monitor_callback);  	
  	
	krad_compositor_start_ticker (krad_radio_station->krad_compositor);
	krad_mixer_start_ticker (krad_radio_station->krad_mixer);  	

	krad_ipc_server_run (krad_radio_station->krad_ipc);
	
	while (1) {

		if (sigwait (&signal_mask, &signal_caught) != 0) {
			failfast ("error on sigwait!");
		}
		switch (signal_caught) {
			case SIGHUP:
				printkd ("Got HANGUP Signal!");
				break;
			case SIGINT:
				printkd ("Got SIGINT Signal!");			
				return;
			case SIGTERM:
				printkd ("Got SIGTERM Signal!");
				krad_system_monitor_cpu_off_fast ();			
				return;
			default:
				break;
		}
	}
}

static krad_tags_t *krad_radio_find_tags_for_item ( krad_radio_t *krad_radio_station, char *item ) {

	krad_mixer_portgroup_t *portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_radio_station->krad_mixer, item);
	
	if (portgroup != NULL) {
		return portgroup->krad_tags;
	} else {
		return krad_linker_get_tags_for_link (krad_radio_station->krad_linker, item);
	}

	return NULL;

}

static void krad_radio_set_dir ( krad_radio_t *krad_radio_station, char *dir ) {

	if (krad_radio_station->dir != NULL) {
		free (krad_radio_station->dir);
	}
	
	krad_radio_station->dir = strdup (dir);

	if (krad_radio_station->krad_compositor != NULL) {
		krad_compositor_set_dir (krad_radio_station->krad_compositor, krad_radio_station->dir);
	}

	sprintf (krad_radio_station->logname, "%s/%s_%"PRIu64".log", dir, krad_radio_station->sysname, ktime ());
	verbose = 1;
	krad_system_log_on (krad_radio_station->logname);

}


static int krad_radio_handler ( void *output, int *output_len, void *ptr ) {

	krad_radio_t *krad_radio_station = (krad_radio_t *)ptr;
	
	uint32_t ebml_id;	
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
	
	krad_ipc_server_read_command ( krad_radio_station->krad_ipc, &command, &ebml_data_size);

	switch ( command ) {
		
		/* Handoffs */
		case EBML_ID_KRAD_MIXER_CMD:
			//printk ("Krad Mixer Command");
			return krad_mixer_handler ( krad_radio_station->krad_mixer, krad_radio_station->krad_ipc );
		case EBML_ID_KRAD_COMPOSITOR_CMD:
			//printk ("Krad Compositor Command");
			return krad_compositor_handler ( krad_radio_station->krad_compositor, krad_radio_station->krad_ipc );			
		case EBML_ID_KRAD_LINK_CMD:
			//printk ("Krad Link Command");
			return krad_linker_handler ( krad_radio_station->krad_linker, krad_radio_station->krad_ipc );

		/* Krad Radio Commands */
		case EBML_ID_KRAD_RADIO_CMD:
			//printk ("Krad Radio Command");
			return krad_radio_handler ( output, output_len, ptr );
		case EBML_ID_KRAD_RADIO_CMD_LIST_TAGS:
			//printk ("LIST_TAGS");
			
			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (krad_radio_station->krad_ipc->current_client->krad_ebml, tag_item, ebml_data_size);
			
			printk ("Get Tags for %s", tag_item);

			if (strncmp("station", tag_item, 7) == 0) {
				krad_tags = krad_radio_station->krad_tags;
			} else {
				krad_tags = krad_radio_find_tags_for_item ( krad_radio_station, tag_item );
			}
			
			if (krad_tags != NULL) {
				printk ("Got Tags for %s", tag_item);
		
				krad_ipc_server_response_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
				krad_ipc_server_response_list_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_TAG_LIST, &element);
			
				while (krad_tags_get_next_tag ( krad_tags, &i, &tag_name, &tag_value)) {
					krad_ipc_server_response_add_tag ( krad_radio_station->krad_ipc, tag_item, tag_name, tag_value);
					printk ("Tag %d: %s - %s", i, tag_name, tag_value);
				}
			
				krad_ipc_server_response_list_finish ( krad_radio_station->krad_ipc, element );
				krad_ipc_server_response_finish ( krad_radio_station->krad_ipc, response );

			} else {
				printke ("Could not find %s", tag_item);
			}

			return 1;

		case EBML_ID_KRAD_RADIO_CMD_SET_TAG:
			
			krad_ipc_server_read_tag ( krad_radio_station->krad_ipc, &tag_item, &tag_name, &tag_value );
			
			if (strncmp("station", tag_item, 7) == 0) {
				krad_tags_set_tag ( krad_radio_station->krad_tags, tag_name, tag_value);
			} else {
				krad_tags = krad_radio_find_tags_for_item ( krad_radio_station, tag_item );
				if (krad_tags != NULL) {
					krad_tags_set_tag ( krad_tags, tag_name, tag_value);
					printk ("Set Tag %s on %s to %s", tag_name, tag_item, tag_value);
				} else {
					printke ("Could not find %s", tag_item);
				}
			}
			
			//krad_ipc_server_broadcast_tag ( krad_radio_station->krad_ipc, tag_item, tag_name, tag_value);

			return 2;

		case EBML_ID_KRAD_RADIO_CMD_GET_TAG:
			krad_ipc_server_read_tag ( krad_radio_station->krad_ipc, &tag_item, &tag_name, &tag_value );
			printk ("Get Tag %s - %s - %s", tag_item, tag_name, tag_value);
			if (strncmp("station", tag_item, 7) == 0) {
				tag_value = krad_tags_get_tag (krad_radio_station->krad_tags, tag_name);
			} else {
				krad_tags = krad_radio_find_tags_for_item ( krad_radio_station, tag_item );
				if (krad_tags != NULL) {
					tag_value = krad_tags_get_tag ( krad_tags, tag_name );
					printk ("Got Tag %s on %s - %s", tag_name, tag_item, tag_value);
				} else {
					printke ("Could not find %s", tag_item);
				}
			}
			
			if (strlen(tag_value)) {
				krad_ipc_server_response_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
				krad_ipc_server_response_add_tag ( krad_radio_station->krad_ipc, tag_item, tag_name, tag_value);
				krad_ipc_server_response_finish ( krad_radio_station->krad_ipc, response);
			}
			return 1;
			
		case EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE:
		
			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_HTTP_PORT) {
				printke ("hrm wtf5");
			}
			
			numbers[0] = krad_ebml_read_number ( krad_radio_station->krad_ipc->current_client->krad_ebml, ebml_data_size);	

			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEBSOCKET_PORT) {
				printke ("hrm wtf6");
			}
			
			numbers[1] = krad_ebml_read_number ( krad_radio_station->krad_ipc->current_client->krad_ebml, ebml_data_size);
		
			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEB_HEADCODE) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (krad_radio_station->krad_ipc->current_client->krad_ebml, string1, ebml_data_size);
		
			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEB_HEADER) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (krad_radio_station->krad_ipc->current_client->krad_ebml, string2, ebml_data_size);
			
			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_WEB_FOOTER) {
				failfast ("missing item");
			}
			
			krad_ebml_read_string (krad_radio_station->krad_ipc->current_client->krad_ebml, string3, ebml_data_size);
		
		
			// Remove existing if existing (ie. I am changing the port)
			if (krad_radio_station->krad_http != NULL) {
				krad_http_server_destroy (krad_radio_station->krad_http);
			}
			if (krad_radio_station->krad_websocket != NULL) {
				krad_websocket_server_destroy (krad_radio_station->krad_websocket);
			}
		
			krad_radio_station->krad_http = krad_http_server_create ( krad_radio_station, numbers[0], numbers[1],
																	  string1, string2, string3 );
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
				printke ("hrm wtf6");
			}
			
			numbers[0] = krad_ebml_read_number ( krad_radio_station->krad_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_ipc_server_enable_remote (krad_radio_station->krad_ipc, numbers[0]);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE:
			
			krad_ipc_server_disable_remote (krad_radio_station->krad_ipc);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_OSC_ENABLE:
			
			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_UDP_PORT) {
				printke ("hrm wtf6");
			}
			
			numbers[0] = krad_ebml_read_number ( krad_radio_station->krad_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_osc_listen (krad_radio_station->krad_osc, numbers[0]);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_OSC_DISABLE:
			
			krad_osc_stop_listening (krad_radio_station->krad_osc);
			
			return 0;
			
		case EBML_ID_KRAD_RADIO_CMD_UPTIME:
			krad_ipc_server_response_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_respond_number ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_UPTIME, krad_system_daemon_uptime());
			krad_ipc_server_response_finish ( krad_radio_station->krad_ipc, response);
		
			return 1;
		case EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_INFO:
			krad_ipc_server_response_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_respond_string ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_SYSTEM_INFO, krad_system_info());
			krad_ipc_server_response_finish ( krad_radio_station->krad_ipc, response);
			return 1;
			
		case EBML_ID_KRAD_RADIO_CMD_SET_DIR:

			krad_ebml_read_element ( krad_radio_station->krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_RADIO_DIR) {
				printke ("hrm wtf6");
			}
			
			krad_ebml_read_string (krad_radio_station->krad_ipc->current_client->krad_ebml, tag_value_actual, ebml_data_size);
			
			if (strlen(tag_value_actual)) {
				krad_radio_set_dir ( krad_radio_station, tag_value_actual );
			}
			
			return 0;
	
		case EBML_ID_KRAD_RADIO_CMD_GET_LOGNAME:

			krad_ipc_server_response_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_respond_string ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_LOGNAME, krad_radio_station->logname);
			krad_ipc_server_response_finish ( krad_radio_station->krad_ipc, response);
			
			return 0;

		case EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_CPU_USAGE:

			krad_ipc_server_response_start ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_MSG, &response);
			krad_ipc_server_respond_number ( krad_radio_station->krad_ipc, EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE, krad_system_get_cpu_usage());
			krad_ipc_server_response_finish ( krad_radio_station->krad_ipc, response);
			
			return 0;
			
		case EBML_ID_KRAD_RADIO_CMD_BROADCAST_SUBSCRIBE:
		
			numbers[0] = krad_ebml_read_number ( krad_radio_station->krad_ipc->current_client->krad_ebml, ebml_data_size);		
		
			krad_ipc_server_add_client_to_broadcast ( krad_radio_station->krad_ipc, numbers[0] );
		
			return 0;

		default:
			printke ("Krad Radio Command Unknown! %u", command);
			return 0;
	}

	return 0;

}
