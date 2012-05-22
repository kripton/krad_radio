#include "krad_ipc_client.h"

krad_ipc_client_t *krad_ipc_connect (char *sysname) {
	
	krad_ipc_client_t *client = calloc (1, sizeof (krad_ipc_client_t));
	
	if (client == NULL) {
		fprintf(stderr, "Krad IPC Client mem alloc fail\n");
		return NULL;
	}
	
	if ((client->buffer = calloc (1, KRAD_IPC_BUFFER_SIZE)) == NULL) {
		fprintf(stderr, "Krad IPC Client buffer alloc fail\n");
		return NULL;
	}
	
	krad_system_init ();
	
	uname(&client->unixname);

	if (krad_valid_host_and_port (sysname)) {
	
		krad_get_host_and_port (sysname, client->host, &client->tcp_port);
	
	} else {

		if (strncmp(client->unixname.sysname, "Linux", 5) == 0) {
			client->on_linux = 1;
			client->ipc_path_pos = sprintf(client->ipc_path, "@krad_radio_%s_ipc", sysname);
		} else {
			client->ipc_path_pos = sprintf(client->ipc_path, "%s/krad_radio_%s_ipc", getenv ("HOME"), sysname);
		}
	
		if (!client->on_linux) {
			if(stat(client->ipc_path, &client->info) != 0) {
				krad_ipc_disconnect(client);
				printf ("Krad IPC Client: IPC PATH Failure\n");
				return NULL;
			}
		}
		
	}
	
	if (krad_ipc_client_init (client) == 0) {
		printf ("Krad IPC Client: Failed to init!\n");
		krad_ipc_disconnect (client);
		return NULL;
	}


	return client;
}


int krad_ipc_client_init (krad_ipc_client_t *client) {

	struct sockaddr_in serveraddr;
	struct hostent *hostp;
	int sent;

	if (client->tcp_port != 0) {

		printk("Krad IPC Client: Connecting to remote %s:%d\n", client->host, client->tcp_port);

		if ((client->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printke ("Krad IPC Client: Socket Error\n");
			exit(1);
		}

		memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons (client->tcp_port);
	
		if ((serveraddr.sin_addr.s_addr = inet_addr(client->host)) == (unsigned long)INADDR_NONE) {
			// get host address 
			hostp = gethostbyname(client->host);
			if (hostp == (struct hostent *)NULL) {
				printke ("Krad IPC Client: Remote Host Error\n");
				close (client->sd);
				exit (1);
			}
			memcpy (&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
		}

		// connect() to server. 
		if ((sent = connect(client->sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
			printke ("Krad IPC Client: Remote Connect Error\n");
			exit(1);
		}

	} else {

		client->sd = socket (AF_UNIX, SOCK_STREAM, 0);
		if (client->sd == -1) {
			printf ("Krad IPC Client: socket fail\n");
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
			printf ("Krad IPC Client: Can't connect to socket %s\n", client->ipc_path);
			return 0;
		}

		client->flags = fcntl (client->sd, F_GETFL, 0);

		if (client->flags == -1) {
			close (client->sd);
			client->sd = 0;
			printf ("Krad IPC Client: socket get flag fail\n");
			return 0;
		}
	
	}
	
	
/*
	client->flags |= O_NONBLOCK;

	client->flags = fcntl (client->sd, F_SETFL, client->flags);
	if (client->flags == -1) {
		close (client->sd);
		client->sd = 0;
		printf ("Krad IPC Client: socket set flag fail\n");
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
		printf("Did Not Match %s Version: %d Read Version: %d\n", KRAD_IPC_SERVER_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
	}	
	
	return client->sd;
}

void krad_ipc_get_portgroups (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t get_portgroups;
	//uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS, &get_portgroups);	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");	

	//krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, get_portgroups);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_get_tags (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t get_tags;
	//uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_LIST_TAGS, &get_tags);	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");	

	//krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, get_tags);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_get_tag (krad_ipc_client_t *client, char *tag_name) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t get_tag;
	uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_TAG, &get_tag);	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");	

	krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, get_tag);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_set_tag (krad_ipc_client_t *client, char *tag_name, char *tag_value) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t set_tag;
	uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_TAG, &set_tag);	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, tag_value);	

	krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, set_tag);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_webon (krad_ipc_client_t *client, int http_port, int websocket_port) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t webon;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE, &webon);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_HTTP_PORT, http_port);	
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_WEBSOCKET_PORT, websocket_port);	

	krad_ebml_finish_element (client->krad_ebml, webon);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_weboff (krad_ipc_client_t *client) {

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

void krad_ipc_enable_remote (krad_ipc_client_t *client, int port) {

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

void krad_ipc_disable_remote (krad_ipc_client_t *client) {

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

void krad_ipc_mixer_create_portgroup (krad_ipc_client_t *client, char *name, char *direction) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t destroy;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_CREATE_PORTGROUP, &destroy);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION, direction);
	
	krad_ebml_finish_element (client->krad_ebml, destroy);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}



void krad_ipc_mixer_update_portgroup (krad_ipc_client_t *client, char *portgroupname, uint64_t update_command, char *string) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t update;



	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP, &update);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_write_string (client->krad_ebml, update_command, string);
	
	
	krad_ebml_finish_element (client->krad_ebml, update);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_remove_portgroup (krad_ipc_client_t *client, char *name) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t destroy;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_DESTROY_PORTGROUP, &destroy);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);

	krad_ebml_finish_element (client->krad_ebml, destroy);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_set_handler_callback (krad_ipc_client_t *client, int handler (krad_ipc_client_t *, void *), void *ptr) {

	client->handler = handler;
	client->ptr = ptr;

}


void krad_ipc_set_control (krad_ipc_client_t *client, char *portgroup_name, char *control_name, float control_value) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t set_control;
	
	mixer_command = 0;
	set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_SET_CONTROL, &set_control);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, set_control);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_bug (krad_ipc_client_t *client, int x, int y, char *filename) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t bug;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_BUG, &bug);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, y);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FILENAME, filename);

	krad_ebml_finish_element (client->krad_ebml, bug);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_hex (krad_ipc_client_t *client, int x, int y, int size) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t hexdemo;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_HEX_DEMO, &hexdemo);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, y);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SIZE, size);

	krad_ebml_finish_element (client->krad_ebml, hexdemo);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_radio_uptime (krad_ipc_client_t *client) {

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

void krad_ipc_radio_info (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t info_command;
	command = 0;
	info_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_INFO, &info_command);
	krad_ebml_finish_element (client->krad_ebml, info_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);	
	
}

void krad_ipc_create_capture_link (krad_ipc_client_t *client, krad_link_video_source_t video_source) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (CAPTURE));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE, krad_link_video_source_to_string (video_source));
	
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_transmit_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode, char *host, int port, char *mount, char *password) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (TRANSMIT));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AV_MODE, krad_link_av_mode_to_string (av_mode));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_HOST, host);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PORT, port);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_MOUNT, mount);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PASSWORD, password);	
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_list_links (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t list_links;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_LIST_LINKS, &list_links);

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	//krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, list_links);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_destroy_link (krad_ipc_client_t *client, int number) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t destroy_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_DESTROY_LINK, &destroy_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_finish_element (client->krad_ebml, destroy_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_update_link_adv_num (krad_ipc_client_t *client, int number, uint32_t ebml_id, int newval) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t update_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_UPDATE_LINK, &update_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_write_int32 (client->krad_ebml, ebml_id, newval);

	krad_ebml_finish_element (client->krad_ebml, update_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_update_link_adv (krad_ipc_client_t *client, int number, uint32_t ebml_id, char *newval) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t update_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_UPDATE_LINK, &update_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_write_string (client->krad_ebml, ebml_id, newval);

	krad_ebml_finish_element (client->krad_ebml, update_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_update_link (krad_ipc_client_t *client, int number, int newval) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t update_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_UPDATE_LINK, &update_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, newval);

	krad_ebml_finish_element (client->krad_ebml, update_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

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
int krad_ipc_cmd2 (krad_ipc_client_t *client, int value) {

	fd_set set;
	
	uint32_t ebml_id;
	uint64_t ebml_data_size;	
	
	uint64_t number;		
	
//	FD_ZERO (&set);
//	FD_SET (client->sd, &set);
	
//	select (client->sd+1, NULL, &set, NULL, NULL);
	
	uint64_t ipc_command;
	uint64_t radio_command;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_CONTROL, value);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

	return 0;
}
*/

void krad_ipc_client_handle (krad_ipc_client_t *client) {
	client->handler ( client, client->ptr );
}

int krad_ipc_client_poll (krad_ipc_client_t *client) {

	int ret;
	struct pollfd sockets[1];
	//uint32_t ebml_id;
	//uint64_t ebml_data_size;	

	sockets[0].fd = client->sd;
	sockets[0].events = POLLIN;

	while ((ret = poll(sockets, 1, 0)) > 0) {
		//krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
		//*value = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
		//printf("Received number %d\n", *value);
		
		//krad_ipc_print_response (client);
		client->handler ( client, client->ptr );
	}

	return 0;
}

int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd) {
/*
	fd_set set;
	
	uint32_t ebml_id;
	uint64_t ebml_data_size;	
	
	uint64_t number;		
	
	FD_ZERO (&set);
	FD_SET (client->sd, &set);
	
	select (client->sd+1, NULL, &set, NULL, NULL);
	
	uint64_t ipc_command;
	uint64_t radio_command;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_CONTROL, 82);

	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);


	//usleep(200000);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_write_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_CONTROL);
	krad_ebml_write_data_size (client->krad_ebml, 0);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

	printf("sent\n");

	FD_ZERO (&set);
	FD_SET (client->sd, &set);

	select (client->sd+1, &set, NULL, NULL, NULL);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	printf("Received number %zu\n", number);
	
*/	
	
	return 0;
}


int krad_ipc_client_read_mixer_control ( krad_ipc_client_t *client, char **portgroup_name, char **control_name, float *value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
		printf("hrm wtf1\n");
	}
	krad_ebml_read_string (client->krad_ebml, *portgroup_name, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_NAME) {
		printf("hrm wtf2\n");
	}
	krad_ebml_read_string (client->krad_ebml, *control_name, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
		printf("hrm wtf3\n");
	}
	
	*value = krad_ebml_read_float (client->krad_ebml, ebml_data_size);

	//printf("krad_ipc_client_read_mixer_control %s %s %f\n", *portgroup_name, *control_name, *value );
		
	return 0;		
						
}						

int krad_ipc_client_read_tag ( krad_ipc_client_t *client, char **tag_name, char **tag_value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	bytes_read += ebml_data_size + 9;
	
	if (ebml_id != EBML_ID_KRAD_RADIO_TAG) {
		printf("hrm wtf\n");
	} else {
		//printf("tag size %zu\n", ebml_data_size);
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
		printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_name, ebml_data_size);
	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
		printf("hrm wtf3\n");
	} else {
		//printf("tag value size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_value, ebml_data_size);
	
	return bytes_read;
	
}


int krad_link_rep_to_string (krad_link_rep_t *krad_link, char *text) {

	int pos;
	
	pos = 0;
	
	pos = sprintf (text, "%s - %s - %s:%d%s", 
			krad_link_av_mode_to_string (krad_link->av_mode),
			krad_link_operation_mode_to_string (krad_link->operation_mode),
			krad_link->host, krad_link->tcp_port, krad_link->mount);
	
	return pos;

}


int krad_ipc_client_read_link ( krad_ipc_client_t *client, char *text) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	krad_link_rep_t *krad_link;
	float floaty;
	int textpos;
	
	int number;
	
	char string[1024];
	memset (string, '\0', 1024);
	
	krad_link = calloc (1, sizeof (krad_link_rep_t));
	
	bytes_read = 0;	
	textpos = 0;
	

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_LINK_LINK) {
		printk ("hrm wtf\n");
	} else {
		bytes_read += ebml_data_size + 9;
	}
	
	
		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_AV_MODE) {
			printk ("hrm wtf2\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
	
		if (strcmp(string, "audio only") == 0) {
			krad_link->av_mode = AUDIO_ONLY;
		}
		
		if (strcmp(string, "video only") == 0) {
			krad_link->av_mode = VIDEO_ONLY;
		}
		
		if (strcmp(string, "audio and video") == 0) {
			krad_link->av_mode = AUDIO_AND_VIDEO;
		}
		
		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_OPERATION_MODE) {
			printk ("hrm wtf2\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}
	
		krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
	
		if (strcmp(string, "capture") == 0) {
			krad_link->operation_mode = CAPTURE;
		}
		
		if (strcmp(string, "receive") == 0) {
			krad_link->operation_mode = RECEIVE;
		}
		
		if (strcmp(string, "transmit") == 0) {
			krad_link->operation_mode = TRANSMIT;
		}
		
		if (strcmp(string, "playback") == 0) {
			krad_link->operation_mode = PLAYBACK;
		}			

		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
			printk ("hrm wtf2\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (client->krad_ebml, krad_link->host, ebml_data_size);
	
		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
			printk ("hrm wtf3\n");
		} else {
			//printk ("tag value size %zu\n", ebml_data_size);
		}

		krad_link->tcp_port = krad_ebml_read_number (client->krad_ebml, ebml_data_size);

		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
			printk ("hrm wtf2\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (client->krad_ebml, krad_link->mount, ebml_data_size);


		krad_link_rep_to_string ( krad_link, text );
	

	free (krad_link);

	return bytes_read;

}


int krad_ipc_client_read_portgroup ( krad_ipc_client_t *client, char *portname, float *volume, char *crossfade_name, float *crossfade ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	char string[1024];
	float floaty;
	
	int8_t channels;
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	bytes_read += ebml_data_size + 9;
	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP) {
		printf("hrm wtf\n");
	} else {
		//printf("tag size %zu\n", ebml_data_size);
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
		printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, portname, ebml_data_size);
	
	printf("\nInput name: %s\n", portname);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS) {
		printf("hrm wtf3\n");
	} else {
		//printf("tag value size %zu\n", ebml_data_size);
	}

	channels = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	printf("Channels: %d\n", channels);

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_TYPE) {
		printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
	
	
	printf("Type: %s\n", string);	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
		printf("hrm wtf3\n");
	} else {
		//printf("VOLUME value size %zu\n", ebml_data_size);
	}

	floaty = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	*volume = floaty;
	
	printf("Volume: %f\n", floaty);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS) {
		printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);	
	
		printf("Bus: %s\n", string);	
	

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
		printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, crossfade_name, ebml_data_size);	
	
	
	if (strlen(crossfade_name)) {
		printf("Crossfade With: %s\n", crossfade_name);	
	}

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE) {
		printf("hrm wtf3\n");
	} else {
		//printf("VOLUME value size %zu\n", ebml_data_size);
	}

	floaty = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	if (strlen(crossfade_name)) {
		*crossfade = floaty;
		printf("Crossfade: %f\n", floaty);
	} else {
		*crossfade = 0.0f;
	}
	
	
	return bytes_read;
	
}

#define EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS 0xE3
#define EBML_ID_KRAD_MIXER_PORTGROUP_TYPE 0xE4
#define EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME 0xE5
#define EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS 0xE6

void krad_ipc_client_read_tag_inner ( krad_ipc_client_t *client, char **tag_name, char **tag_value ) {

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

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
		printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_name, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
		printf("hrm wtf3\n");
	} else {
		//printf("tag value size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_value, ebml_data_size);
	
}

void krad_ipc_print_response (krad_ipc_client_t *client) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	fd_set set;
	char tag_name_actual[256];
	char tag_value_actual[1024];
	
	tag_name_actual[0] = '\0';
	tag_value_actual[0] = '\0';
	
	char *tag_name = tag_name_actual;
	char *tag_value = tag_value_actual;	
	
	char crossfadename_actual[1024];	
	char *crossfadename = crossfadename_actual;
	float crossfade;	
	
	int bytes_read;
	int list_size;
	
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
						while ((list_size) && ((bytes_read += krad_ipc_client_read_tag ( client, &tag_name, &tag_value )) <= list_size)) {
							printf("Tag %s - %s.\n", tag_name, tag_value);
							if (bytes_read == list_size) {
								break;
							}
						}	
						break;
					case EBML_ID_KRAD_RADIO_TAG:
						krad_ipc_client_read_tag_inner ( client, &tag_name, &tag_value );
						printf("Tag %"PRIu64" bytes %s - %s.\n", ebml_data_size, tag_name, tag_value);
						break;

					case EBML_ID_KRAD_RADIO_UPTIME:
						number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
						printf("up ");
						updays = number / (60*60*24);
						if (updays) {
							printf("%d day%s, ", updays, (updays != 1) ? "s" : "");
						}
						upminutes = number / 60;
						uphours = (upminutes / 60) % 24;
						upminutes %= 60;
						if (uphours) {
							printf("%2d:%02d ", uphours, upminutes);
						} else {
							printf("%d min ", upminutes);
						}
						printf("\n");
					
						break;
					case EBML_ID_KRAD_RADIO_INFO:
						krad_ebml_read_string (client->krad_ebml, tag_name, ebml_data_size);
						printf("%s\n", tag_name);
						break;

				}
		
		
				break;
			case EBML_ID_KRAD_MIXER_MSG:
				//printf("Received KRAD_MIXER_MSG %"PRIu64" bytes of data.\n", ebml_data_size);
			
				krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
				switch ( ebml_id ) {
					case EBML_ID_KRAD_MIXER_CONTROL:
						printf("Received mixer control list %"PRIu64" bytes of data.\n", ebml_data_size);


	
						break;	
					case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
						//printf("Received PORTGROUP list %zu bytes of data.\n", ebml_data_size);
						list_size = ebml_data_size;
						while ((list_size) && ((bytes_read += krad_ipc_client_read_portgroup ( client, tag_name, &floatval, crossfadename, &crossfade )) <= list_size)) {
							//printf("Tag %s - %s.\n", tag_name, tag_value);
							if (bytes_read == list_size) {
								break;
							}
						}	
						break;
					case EBML_ID_KRAD_MIXER_PORTGROUP:
						//krad_ipc_client_read_portgroup_inner ( client, &tag_name, &tag_value );
						printf("PORTGROUP %"PRIu64" bytes  \n", ebml_data_size );
						break;
				}
		
		
				break;

			case EBML_ID_KRAD_LINK_MSG:

				krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

				switch ( ebml_id ) {
					case EBML_ID_KRAD_LINK_LINK_LIST:
						//printf("Received LINK control list %"PRIu64" bytes of data.\n", ebml_data_size);
	
						list_size = ebml_data_size;
						i = 0;
						while ((list_size) && ((bytes_read += krad_ipc_client_read_link ( client, tag_value)) <= list_size)) {
							printf("%d: %s\n", i, tag_value);
							i++;
							if (bytes_read == list_size) {
								break;
							}
						}	
						break;
					
	
						break;
					default:
						printf("Received KRAD_LINK_MSG %"PRIu64" bytes of data.\n", ebml_data_size);
						break;
				}
			
				break;
		}
	}
}



int krad_ipc_wait (krad_ipc_client_t *client, char *buffer, int size) {

	//int len;
	int bytes;
	fd_set set;

	//strcat(cmd, "|");
	//len = strlen(cmd);
	
	//FD_ZERO (&set);
	//FD_SET (client->sd, &set);
	
	//select (client->sd+1, NULL, &set, NULL, NULL);
	//send (client->sd, cmd, len, 0);

	FD_ZERO (&set);
	FD_SET (client->sd, &set);

	select (client->sd+1, &set, NULL, NULL, NULL);
	bytes = recv(client->sd, buffer, size, 0);
	buffer[bytes] = '\0';
	//printf("Received %d bytes of data: '%s' \n", bytes, client->buffer);
	return bytes;
}

int krad_ipc_recv (krad_ipc_client_t *client, char *buffer, int size) {

	//int len;
	int bytes;
	//fd_set set;

	//strcat(cmd, "|");
	//len = strlen(cmd);
	
	//FD_ZERO (&set);
	//FD_SET (client->sd, &set);
	
	//select (client->sd+1, NULL, &set, NULL, NULL);
	//send (client->sd, cmd, len, 0);

	//FD_ZERO (&set);
	//FD_SET (client->sd, &set);

	//select (client->sd+1, &set, NULL, NULL, NULL);
	//bytes = recv(client->sd, client->buffer, KRAD_IPC_BUFFER_SIZE, 0);

	bytes = recv(client->sd, buffer, size, 0);
	buffer[bytes] = '\0';
	//printf("Received %d bytes of data: '%s' \n", bytes, client->buffer);
	return bytes;
}

void krad_ipc_disconnect(krad_ipc_client_t *client) {

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

