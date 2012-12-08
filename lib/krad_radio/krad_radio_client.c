#include "krad_radio_client_internal.h"
#include "krad_radio_client.h"

kr_client_t *kr_connect (char *sysname) {

  kr_client_t *kr_client;
  
  kr_client = calloc (1, sizeof(kr_client_t));
  kr_client->krad_ipc_client = krad_ipc_connect (sysname);
  if (kr_client->krad_ipc_client != NULL) {
    kr_client->krad_ebml = kr_client->krad_ipc_client->krad_ebml;
    return kr_client;
  } else {
    free (kr_client);
  }
  
  return NULL;
}

void kr_disconnect (kr_client_t **kr_client) {
  if ((kr_client != NULL) && (*kr_client != NULL)) {
    krad_ipc_disconnect ((*kr_client)->krad_ipc_client);
    free (*kr_client);
    *kr_client = NULL;
  }
}

krad_ebml_t *kr_client_get_ebml (kr_client_t *kr_client) {
  return kr_client->krad_ipc_client->krad_ebml;
}

int kr_client_local (kr_client_t *client) {
  if (client->krad_ipc_client->tcp_port == 0) {
    return 1;
  }
  return 0;
}

void kr_shm_destroy (kr_shm_t *kr_shm) {

	if (kr_shm != NULL) {
		if (kr_shm->buffer != NULL) {
			munmap (kr_shm->buffer, kr_shm->size);
			kr_shm->buffer = NULL;
		}
		if (kr_shm->fd != 0) {
			close (kr_shm->fd);
		}
		free(kr_shm);
	}
}

kr_shm_t *kr_shm_create (kr_client_t *client) {

	char filename[] = "/tmp/krad-shm-XXXXXX";
	kr_shm_t *kr_shm;

	kr_shm = calloc (1, sizeof(kr_shm_t));

	if (kr_shm == NULL) {
		return NULL;
	}

	kr_shm->size = 960 * 540 * 4 * 2;

	kr_shm->fd = mkstemp (filename);
	if (kr_shm->fd < 0) {
		fprintf(stderr, "open %s failed: %m\n", filename);
		kr_shm_destroy (kr_shm);
		return NULL;
	}

	if (ftruncate (kr_shm->fd, kr_shm->size) < 0) {
		fprintf (stderr, "ftruncate failed: %m\n");
		kr_shm_destroy (kr_shm);
		return NULL;
	}

	kr_shm->buffer = mmap (NULL, kr_shm->size, PROT_READ | PROT_WRITE, MAP_SHARED, kr_shm->fd, 0);
	unlink (filename);

	if (kr_shm->buffer == MAP_FAILED) {
		fprintf (stderr, "mmap failed: %m\n");
		kr_shm_destroy (kr_shm);
		return NULL;
	}

	return kr_shm;

}

int kr_send_fd (kr_client_t *client, int fd) {
  return krad_ipc_client_send_fd (client->krad_ipc_client, fd);
}

void kr_client_print_response (kr_client_t *kr_client) {

  usleep (100000);

  printf("its a kludge!\n");

  /*

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	fd_set set;
	char tag_name_actual[256];
	char tag_item_actual[256];
	char tag_value_actual[1024];

	char string[1024];	
	krad_link_rep_t *krad_link;

  krad_compositor_rep_t *krad_compositor;
	
	tag_item_actual[0] = '\0';	
	tag_name_actual[0] = '\0';
	tag_value_actual[0] = '\0';
	
	string[0] = '\0';	
	
	char *tag_item = tag_item_actual;
	char *tag_name = tag_name_actual;
	char *tag_value = tag_value_actual;	
	
	char crossfadename_actual[1024];	
	char *crossfadename = crossfadename_actual;
	float crossfade;	
	
	int bytes_read;
	int list_size;
	int list_count;	
	
	int has_xmms2;
	
	has_xmms2 = 0;
	list_size = 0;
	bytes_read = 0;
	
	float floatval;
	int i;
	int updays, uphours, upminutes;	
	uint64_t number;
	
  	struct timeval tv;
    int ret;
    
    ret = 0;
    if (client->tcp_port) {
	    tv.tv_sec = 1;
	} else {
	    tv.tv_sec = 0;
	}
    tv.tv_usec = 500000;
	
	floatval = 0;
	i = 0;
	ebml_id = 0;
	ebml_data_size = 0;
	
	FD_ZERO (&set);
	FD_SET (client->sd, &set);	

	ret = select (client->sd+1, &set, NULL, NULL, &tv);
	
	if (ret > 0) {

		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
		switch ( ebml_id ) {
	
			case EBML_ID_KRAD_RADIO_MSG:
				//printf("Received KRAD_RADIO_MSG %zu bytes of data.\n", ebml_data_size);		
		
				krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
				switch ( ebml_id ) {
	
					case EBML_ID_KRAD_RADIO_TAG_LIST:
						//printf("Received Tag list %"PRIu64" bytes of data.\n", ebml_data_size);
						list_size = ebml_data_size;
						while ((list_size) && ((bytes_read += krad_ipc_client_read_tag ( client, &tag_item, &tag_name, &tag_value )) <= list_size)) {
							printf ("%s: %s - %s\n", tag_item, tag_name, tag_value);
							if (bytes_read == list_size) {
								break;
							}
						}
						break;
					case EBML_ID_KRAD_RADIO_TAG:
						krad_ipc_client_read_tag_inner ( client, &tag_item, &tag_name, &tag_value );
						printf ("%s: %s - %s\n", tag_item, tag_name, tag_value);
						break;

					case EBML_ID_KRAD_RADIO_UPTIME:
						number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
						printf("up ");
						updays = number / (60*60*24);
						if (updays) {
							printf ("%d day%s, ", updays, (updays != 1) ? "s" : "");
						}
						upminutes = number / 60;
						uphours = (upminutes / 60) % 24;
						upminutes %= 60;
						if (uphours) {
							printf ("%2d:%02d ", uphours, upminutes);
						} else {
							printf ("%d min ", upminutes);
						}
						printf ("\n");
					
						break;
					case EBML_ID_KRAD_RADIO_SYSTEM_INFO:
						krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
						if (string[0] != '\0') {
							printf ("%s\n", string);
						}
						break;
					case EBML_ID_KRAD_RADIO_LOGNAME:
						krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
						if (string[0] != '\0') {
							printf ("%s\n", string);
						}
						break;
					case EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE:
						number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
						printf ("%"PRIu64"%%\n", number);
						break;
						
				}
		
		
				break;
		case EBML_ID_KRAD_MIXER_MSG:
			//printf("Received KRAD_MIXER_MSG %"PRIu64" bytes of data.\n", ebml_data_size);
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			switch ( ebml_id ) {
			case EBML_ID_KRAD_MIXER_CONTROL:
				printk ("Received mixer control list %"PRIu64" bytes of data.\n", ebml_data_size);
				break;	
				
			case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
				//printf("Received PORTGROUP list %zu bytes of data.\n", ebml_data_size);
				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nAudio Inputs: \n");
				}
				while ((list_size) && ((bytes_read += kr_client_read_portgroup ( client, tag_name, &floatval, crossfadename, &crossfade, &has_xmms2 )) <= list_size)) {
					printf ("  %0.2f%%  %s", floatval, tag_name);
					if (strlen(crossfadename)) {
						printf (" < %0.2f > %s", crossfade, crossfadename);
					}
					if (has_xmms2) {
						if (!(strlen(crossfadename))) {
							printf ("\t\t");
						}							
						printf ("\t\t[XMMS2]");
					}
					printf ("\n");
					if (bytes_read == list_size) {
						break;
					}
				}	
				break;
				
			case EBML_ID_KRAD_MIXER_PORTGROUP:
				//krad_ipc_client_read_portgroup_inner ( client, &tag_name, &tag_value );
				printf("PORTGROUP %"PRIu64" bytes  \n", ebml_data_size );
				break;
					
			case EBML_ID_KRAD_MIXER_SAMPLE_RATE:
				//krad_ipc_client_read_portgroup_inner ( client, &tag_name, &tag_value );
				number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
				printf ("Krad Mixer Sample Rate: %"PRIu64"\n", number );
				break;
					
			case EBML_ID_KRAD_MIXER_JACK_RUNNING:
				//krad_ipc_client_read_portgroup_inner ( client, &tag_name, &tag_value );
				number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
				if (number > 0) {
					printf ("Yes\n");
				} else {
					printf ("Jack Server not running\n");
				}
				break;
			}
			break;
			
		case EBML_ID_KRAD_COMPOSITOR_MSG:
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			switch ( ebml_id ) {
			
			case EBML_ID_KRAD_COMPOSITOR_INFO:
				krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
				if (string[0] != '\0') {
					printf ("%s\n", string);
				}
				break;
			case EBML_ID_KRAD_COMPOSITOR_FRAME_SIZE:
				list_size = ebml_data_size;
				
				
				while ((list_size) && ((bytes_read +=  kr_compositor_read_frame_size ( 
				                            client, tag_value, &krad_compositor)) <= list_size)) {
					printf ("  %s\n", tag_value);
					
					if (bytes_read == list_size) {
						break;
					} else {
						//printf ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;
			case EBML_ID_KRAD_COMPOSITOR_FRAME_RATE:
				list_size = ebml_data_size;
				
				i = 0;
				while ((list_size) && ((bytes_read += kr_compositor_read_frame_rate ( client, tag_value, &krad_compositor)) <= list_size)) {
					printf ("  %s\n", tag_value);
					i++;
					if (bytes_read == list_size) {
						break;
					} else {
						//printf ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;
			case EBML_ID_KRAD_COMPOSITOR_LAST_SNAPSHOT_NAME:
				krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
				if (string[0] != '\0') {
					printf ("%s\n", string);
				}
				break;
			case EBML_ID_KRAD_COMPOSITOR_PORT_LIST:
				//printk ("Received LINK control list %"PRIu64" bytes of data.\n", ebml_data_size);
				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nVideo Ports:\n");
				}
				i = 0;
				while ((list_size) && ((bytes_read += kr_compositor_read_port ( client, tag_value)) <= list_size)) {
					printf ("  %d: %s\n", i, tag_value);
					i++;
					if (bytes_read == list_size) {
						break;
					} else {
						//printk ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;
			
			case EBML_ID_KRAD_COMPOSITOR_TEXT_LIST:
				printf ("Received TEXT list %"PRIu64" bytes of data, %d bytes read\n", ebml_data_size, bytes_read);

				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nTexts:\n");
				}
		
				while ((list_size) && ((bytes_read += kr_compositor_read_text ( client, tag_value)) <= list_size)) {
					printf ("  %s\n", tag_value);
					if (bytes_read == list_size) {
						break;
					}  else {
            printf ("%d: %d\n", list_size, bytes_read);
          }
        }	
        printf ("here\n");
				break;							
			
				
			case EBML_ID_KRAD_COMPOSITOR_SPRITE_LIST:
				//printf ("Received SPRITE list %"PRIu64" bytes of data, %d bytes read\n", ebml_data_size, bytes_read);

				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nSprites:\n");
				}
		
				while ((list_size) && ((bytes_read += kr_compositor_read_sprite ( client, tag_value)) <= list_size)) {
					printf ("  %s\n", tag_value);
				
					if (bytes_read == list_size) {
						break;
					} else {
						//printf ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;							
			}
			break;
		case EBML_ID_KRAD_LINK_MSG:
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			
			switch ( ebml_id ) {
			
			case EBML_ID_KRAD_LINK_DECKLINK_LIST:
				
				krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
				list_count = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
				for (i = 0; i < list_count; i++) {
					krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);						
					krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
					printf ("%d: %s\n", i, string);
				}	
				break;
				
				
			case EBML_ID_KRAD_LINK_V4L2_LIST:
				
				krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
				list_count = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
				for (i = 0; i < list_count; i++) {
					krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);						
					krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
					printf ("%s\n", string);
				}	
				break;				
				
			case EBML_ID_KRAD_LINK_LINK_LIST:
				//printf("Received LINK control list %"PRIu64" bytes of data.\n", ebml_data_size);
				
				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nLinks:\n");
				}
				i = 0;
				while ((list_size) && ((bytes_read += krad_ipc_client_read_link ( client, tag_value, &krad_link)) <= list_size)) {
					printf ("  Id: %d  %s\n", krad_link->link_num, tag_value);
					free (krad_link);
					i++;
					if (bytes_read == list_size) {
						break;
					}
				}	
				break;
			
			default:
				printf("Received KRAD_LINK_MSG %"PRIu64" bytes of data.\n", ebml_data_size);
				break;
			}
			
			break;
		}
	}
	
	*/

}

void kr_set_dir (kr_client_t *client, char *dir) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t setdir;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_DIR, &setdir);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_DIR, dir);
	
	krad_ebml_finish_element (client->krad_ebml, setdir);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void kr_uptime (kr_client_t *client) {

	uint64_t command;
	uint64_t uptime_command;
	command = 0;
	uptime_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_UPTIME, &uptime_command);
	krad_ebml_finish_element (client->krad_ebml, uptime_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_system_info (kr_client_t *client) {

	uint64_t command;
	uint64_t info_command;
	command = 0;
	info_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_INFO, &info_command);
	krad_ebml_finish_element (client->krad_ebml, info_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);	
	
}


void kr_logname (kr_client_t *client) {

	uint64_t command;
	uint64_t log_command;
	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_LOGNAME, &log_command);
	krad_ebml_finish_element (client->krad_ebml, log_command);
	krad_ebml_finish_element (client->krad_ebml, command);
	krad_ebml_write_sync (client->krad_ebml);
}

void kr_system_cpu_usage (kr_client_t *client) {

	uint64_t command;
	uint64_t cpu_command;
	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_CPU_USAGE, &cpu_command);
	krad_ebml_finish_element (client->krad_ebml, cpu_command);
	krad_ebml_finish_element (client->krad_ebml, command);
	krad_ebml_write_sync (client->krad_ebml);
}

/* Remote Control */

void kr_remote_enable (kr_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t enable_remote;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_ENABLE, &enable_remote);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_remote);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_remote_disable (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t disable_remote;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE, &disable_remote);

	krad_ebml_finish_element (client->krad_ebml, disable_remote);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_web_enable (kr_client_t *client, int http_port, int websocket_port,
					char *headcode, char *header, char *footer) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t webon;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE, &webon);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_HTTP_PORT, http_port);	
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_WEBSOCKET_PORT, websocket_port);	

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_HEADCODE, headcode);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_HEADER, header);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_FOOTER, footer);
	
	krad_ebml_finish_element (client->krad_ebml, webon);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_web_disable (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t weboff;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_DISABLE, &weboff);

	krad_ebml_finish_element (client->krad_ebml, weboff);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_osc_enable (kr_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t enable_osc;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_OSC_ENABLE, &enable_osc);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_UDP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_osc);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_osc_disable (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t disable_osc;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_OSC_DISABLE, &disable_osc);

	krad_ebml_finish_element (client->krad_ebml, disable_osc);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


/* Tagging */

void kr_read_tag_inner ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
/*
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_RADIO_TAG) {
		printf("hrm wtf\n");
	} else {
		//printf("tag size %zu\n", ebml_data_size);
	}
*/

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
		//printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_item, ebml_data_size);

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
		//printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_name, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
		//printf("hrm wtf3\n");
	} else {
		//printf("tag value size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_value, ebml_data_size);
	
}

int kr_read_tag ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	bytes_read += ebml_data_size + 9;
	
	if (ebml_id != EBML_ID_KRAD_RADIO_TAG) {
		//printf("hrm wtf\n");
	} else {
		//printf("tag size %zu\n", ebml_data_size);
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
		//printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_item, ebml_data_size);	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
		//printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_name, ebml_data_size);
	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
		//printf("hrm wtf3\n");
	} else {
		//printf("tag value size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_value, ebml_data_size);
	
	return bytes_read;
	
}

void kr_tags (kr_client_t *client, char *item) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t get_tags;
	//uint64_t tag;

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_LIST_TAGS, &get_tags);	

	if (item == NULL) {
		item = "station";
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);

	krad_ebml_finish_element (client->krad_ebml, get_tags);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_tag (kr_client_t *client, char *item, char *tag_name) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t get_tag;
	uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_TAG, &get_tag);	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	if (item == NULL) {
		item = "station";
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");	

	krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, get_tag);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void kr_set_tag (kr_client_t *client, char *item, char *tag_name, char *tag_value) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t set_tag;
	uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_TAG, &set_tag);	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	if (item == NULL) {
		item = "station";
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, tag_value);	

	krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, set_tag);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}
