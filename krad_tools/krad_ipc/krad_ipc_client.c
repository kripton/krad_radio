#include "krad_ipc_client.h"

#include "krad_radio_ipc.h"

krad_ipc_client_t *krad_ipc_connect (char *callsign_or_ipc_path_or_port)
{
	
	krad_ipc_client_t *client = calloc (1, sizeof (krad_ipc_client_t));
	
	if (client == NULL) {
		fprintf(stderr, "Krad IPC Client mem alloc fail\n");
		return NULL;
	}
	
	if ((client->buffer = calloc (1, KRAD_IPC_BUFFER_SIZE)) == NULL) {
		fprintf(stderr, "Krad IPC Client buffer alloc fail\n");
		return NULL;
	}
	
	uname(&client->unixname);

	if (strncmp(client->unixname.sysname, "Linux", 5) == 0) {
		client->on_linux = 1;
		client->ipc_path_pos = sprintf(client->ipc_path, "@%s_ipc", callsign_or_ipc_path_or_port);
	} else {
		client->ipc_path_pos = sprintf(client->ipc_path, "%s/%s_ipc", getenv ("HOME"), callsign_or_ipc_path_or_port);
	}
	
	if (!client->on_linux) {
		if(stat(client->ipc_path, &client->info) != 0) {
			krad_ipc_disconnect(client);
			printf ("Krad IPC Client: IPC PATH Failure\n");
			return NULL;
		}
	}
	
	if (krad_ipc_client_init (client) == 0) {
		printf ("Failed to init krad ipc client!\n");
		krad_ipc_disconnect (client);
		return NULL;
	}

	return client;
}


int krad_ipc_client_init (krad_ipc_client_t *client)
{

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
		printf ("Can't connect to socket %s\n", client->ipc_path);
		return 0;
	}

	client->flags = fcntl (client->sd, F_GETFL, 0);

	if (client->flags == -1) {
		close (client->sd);
		client->sd = 0;
		printf ("Krad IPC Client: socket get flag fail\n");
		return 0;
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

int krad_ipc_client_check (krad_ipc_client_t *client, int *value) {

	int ret;
	struct pollfd sockets[1];
	uint32_t ebml_id;
	uint64_t ebml_data_size;	

	sockets[0].fd = client->sd;
	sockets[0].events = POLLIN;

	while ((ret = poll(sockets, 1, 0)) > 0) {
		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
		*value = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
		//printf("Received number %d\n", *value);
	}

	return 0;
}

int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd) {

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

	int ret;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	fd_set set;
	char tag_name_actual[256];
	char tag_value_actual[1024];
	
	tag_name_actual[0] = '\0';
	tag_value_actual[0] = '\0';
	
	char *tag_name = tag_name_actual;
	char *tag_value = tag_value_actual;	
	
	int bytes_read;
	int list_size;
	
	list_size;
	bytes_read = 0;

	FD_ZERO (&set);
	FD_SET (client->sd, &set);	

	select (client->sd+1, &set, NULL, NULL, NULL);

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	switch ( ebml_id ) {
	
		case EBML_ID_KRAD_RADIO_MSG:
			printf("Received KRAD_RADIO_MSG %zu bytes of data.\n", ebml_data_size);		
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			switch ( ebml_id ) {
	
				case EBML_ID_KRAD_RADIO_TAG_LIST:
					printf("Received Tag list %zu bytes of data.\n", ebml_data_size);
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
					printf("Tag %zu bytes %s - %s.\n", ebml_data_size, tag_name, tag_value);
					break;
			}
		
		
			break;
		case EBML_ID_KRAD_MIXER_MSG:
			printf("Received KRAD_MIXER_MSG %zu bytes of data.\n", ebml_data_size);				
			break;
		case EBML_ID_KRAD_LINK_MSG:
			printf("Received KRAD_LINK_MSG %zu bytes of data.\n", ebml_data_size);				
			break;
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
			free(client->buffer);
		}
		if (client->sd != 0) {
			close(client->sd);
		}
		if (client->krad_ebml != NULL) {
			krad_ebml_destroy (client->krad_ebml);
		}
		free(client);
	}
}

