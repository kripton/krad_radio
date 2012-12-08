#include "krad_ipc_client.h"

static int krad_ipc_client_init (krad_ipc_client_t *client);

void krad_ipc_disconnect (krad_ipc_client_t *client) {
  if (client != NULL) {
    if (client->buffer != NULL) {
      free (client->buffer);
    }
    if (client->sd != 0) {
      close (client->sd);
    }
    if (client->krad_ebml != NULL) {
      krad_ebml_destroy (client->krad_ebml);
    }
    free(client);
  }
}

krad_ipc_client_t *krad_ipc_connect (char *sysname) {
	
	krad_ipc_client_t *client = calloc (1, sizeof (krad_ipc_client_t));
	
	if (client == NULL) {
		failfast ("Krad IPC Client mem alloc fail");
		return NULL;
	}
	
	if ((client->buffer = calloc (1, KRAD_IPC_BUFFER_SIZE)) == NULL) {
		failfast ("Krad IPC Client buffer alloc fail");
		return NULL;
	}
	
	krad_system_init ();
	
	uname(&client->unixname);

	if (krad_valid_host_and_port (sysname)) {
	
		krad_get_host_and_port (sysname, client->host, &client->tcp_port);
	
	} else {
	
		strncpy (client->sysname, sysname, sizeof (client->sysname));

		if (strncmp(client->unixname.sysname, "Linux", 5) == 0) {
			client->on_linux = 1;
			client->ipc_path_pos = sprintf(client->ipc_path, "@krad_radio_%s_ipc", sysname);
		} else {
			client->ipc_path_pos = sprintf(client->ipc_path, "%s/krad_radio_%s_ipc", "/tmp", sysname);
		}
	
		if (!client->on_linux) {
			if(stat(client->ipc_path, &client->info) != 0) {
				krad_ipc_disconnect(client);
				failfast ("Krad IPC Client: IPC PATH Failure\n");
				return NULL;
			}
		}
		
	}
	
	if (krad_ipc_client_init (client) == 0) {
		printke ("Krad IPC Client: Failed to init!");
		krad_ipc_disconnect (client);
		return NULL;
	}

	return client;
}

static int krad_ipc_client_init (krad_ipc_client_t *client) {

	struct sockaddr_in serveraddr;
	struct hostent *hostp;
	int sent;

	if (client->tcp_port != 0) {

		printkd ("Krad IPC Client: Connecting to remote %s:%d", client->host, client->tcp_port);

		if ((client->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			failfast ("Krad IPC Client: Socket Error");
		}

		memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons (client->tcp_port);
	
		if ((serveraddr.sin_addr.s_addr = inet_addr(client->host)) == (unsigned long)INADDR_NONE) {
			// get host address 
			hostp = gethostbyname(client->host);
			if (hostp == (struct hostent *)NULL) {
				printke ("Krad IPC Client: Remote Host Error");
				close (client->sd);
			}
			memcpy (&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
		}

		// connect() to server. 
		if ((sent = connect(client->sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
			printke ("Krad IPC Client: Remote Connect Error");
		}

	} else {

		client->sd = socket (AF_UNIX, SOCK_STREAM, 0);
		if (client->sd == -1) {
			failfast ("Krad IPC Client: socket fail");
			return 0;
		}

		client->saddr.sun_family = AF_UNIX;
		snprintf (client->saddr.sun_path, sizeof(client->saddr.sun_path), "%s", client->ipc_path);

		if (client->on_linux) {
			client->saddr.sun_path[0] = '\0';
		}


		if (connect (client->sd, (struct sockaddr *) &client->saddr, sizeof (client->saddr)) == -1) {
			close (client->sd);
			client->sd = 0;
			printke ("Krad IPC Client: Can't connect to socket %s", client->ipc_path);
			return 0;
		}

		client->flags = fcntl (client->sd, F_GETFL, 0);

		if (client->flags == -1) {
			close (client->sd);
			client->sd = 0;
			printke ("Krad IPC Client: socket get flag fail");
			return 0;
		}
	
	}
	
	
/*
	client->flags |= O_NONBLOCK;

	client->flags = fcntl (client->sd, F_SETFL, client->flags);
	if (client->flags == -1) {
		close (client->sd);
		client->sd = 0;
		printke ("Krad IPC Client: socket set flag fail\n");
		return 0;
	}
*/

	client->krad_ebml = krad_ebml_open_active_socket (client->sd, KRAD_EBML_IO_READWRITE);

	krad_ebml_header_advanced (client->krad_ebml, KRAD_IPC_CLIENT_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
	krad_ebml_write_sync (client->krad_ebml);
	
	krad_ebml_read_ebml_header (client->krad_ebml, client->krad_ebml->header);
	krad_ebml_check_ebml_header (client->krad_ebml->header);
	//krad_ebml_print_ebml_header (client->krad_ebml->header);
	
	if (krad_ebml_check_doctype_header (client->krad_ebml->header, KRAD_IPC_SERVER_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION)) {
		//printf("Matched %s Version: %d Read Version: %d\n", KRAD_IPC_SERVER_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
	} else {
		printke ("Did Not Match %s Version: %d Read Version: %d", KRAD_IPC_SERVER_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
	}	
	
	return client->sd;
}

void krad_ipc_broadcast_subscribe (krad_ipc_client_t *client, uint32_t broadcast_id) {

	uint64_t radio_command;

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_BROADCAST_SUBSCRIBE, broadcast_id);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_client_handle (krad_ipc_client_t *client) {
	client->handler ( client, client->ptr );
}

void krad_ipc_set_handler_callback (krad_ipc_client_t *client, int handler (krad_ipc_client_t *, void *), void *ptr) {
	client->handler = handler;
	client->ptr = ptr;
}


void krad_ipc_send (krad_ipc_client_t *client, char *cmd) {


	int len;
	fd_set set;
	
	strcat(cmd, "|");
	len = strlen(cmd);

	FD_ZERO (&set);
	FD_SET (client->sd, &set);

	select (client->sd+1, NULL, &set, NULL, NULL);
	send (client->sd, cmd, len, 0);

}








/*
void krad_ipc_print_response (krad_ipc_client_t *client) {

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
}
*/


int krad_ipc_client_sendfd (krad_ipc_client_t *client, int fd) {
	char buf[1];
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	int n;
	char cms[CMSG_SPACE(sizeof(int))];

	buf[0] = 0;
	iov.iov_base = buf;
	iov.iov_len = 1;

	memset(&msg, 0, sizeof msg);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (caddr_t)cms;
	msg.msg_controllen = CMSG_LEN(sizeof(int));

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	memmove(CMSG_DATA(cmsg), &fd, sizeof(int));

	if((n=sendmsg(client->sd, &msg, 0)) != iov.iov_len)
		    return 0;
	return 1;
}



