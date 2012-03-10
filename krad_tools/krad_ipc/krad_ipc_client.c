#include "krad_ipc_client.h"

krad_ipc_client_t *krad_ipc_connect (char *daemon_name, char *station_name)
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
		client->linux_abstract_sockets = 1;
		client->ipc_path_pos = sprintf(client->ipc_path, "@%s_ipc", daemon_name);
	} else {
		client->ipc_path_pos = sprintf(client->ipc_path, "%s/krad/run/%s_ipc", getenv ("HOME"), daemon_name);
	}
	
	if (station_name != NULL) {
		sprintf(client->ipc_path + client->ipc_path_pos, "_%s", station_name);
	}
	
	if (!client->linux_abstract_sockets) {
		if(stat(client->ipc_path, &client->info) != 0) {
			krad_ipc_disconnect(client);
			printf ("Krad IPC Client: IPC PATH Failure\n");
			return NULL;
		}
	}
	
	if (krad_ipc_client_init (client) == 0) {
		printf ("Failed to init krad ipc client!\n");
		krad_ipc_disconnect(client);
		return NULL;
	}

	return client;
}


int krad_ipc_client_init (krad_ipc_client_t *client)
{

	client->fd = socket (AF_UNIX, SOCK_STREAM, 0);
	if (client->fd == -1) {
		printf ("Krad IPC Client: socket fail\n");
		return 0;
	}

	client->saddr.sun_family = AF_UNIX;
	snprintf (client->saddr.sun_path, sizeof(client->saddr.sun_path), "%s", client->ipc_path);

	if (client->linux_abstract_sockets) {
		client->saddr.sun_path[0] = '\0';
	}


	if (connect (client->fd, (struct sockaddr *) &client->saddr, sizeof (client->saddr)) == -1) {
		close (client->fd);
		client->fd = 0;
		printf ("Can't connect to socket %s\n", client->ipc_path);
		return 0;
	}

	client->flags = fcntl (client->fd, F_GETFL, 0);

	if (client->flags == -1) {
		close (client->fd);
		client->fd = 0;
		printf ("Krad IPC Client: socket get flag fail\n");
		return 0;
	}

	client->flags |= O_NONBLOCK;

	client->flags = fcntl (client->fd, F_SETFL, client->flags);
	if (client->flags == -1) {
		close (client->fd);
		client->fd = 0;
		printf ("Krad IPC Client: socket set flag fail\n");
		return 0;
	}

	return client->fd;
}

void krad_ipc_send (krad_ipc_client_t *client, char *cmd) {


	int len;
	fd_set set;
	
	strcat(cmd, "|");
	len = strlen(cmd);

	FD_ZERO (&set);
	FD_SET (client->fd, &set);

	select (client->fd+1, NULL, &set, NULL, NULL);
	send (client->fd, cmd, len, 0);

}


int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd) {

	int len;
	int bytes;
	fd_set set;

	strcat(cmd, "|");
	len = strlen(cmd);
	
	FD_ZERO (&set);
	FD_SET (client->fd, &set);
	
	select (client->fd+1, NULL, &set, NULL, NULL);
	send (client->fd, cmd, len, 0);

	FD_ZERO (&set);
	FD_SET (client->fd, &set);

	select (client->fd+1, &set, NULL, NULL, NULL);
	bytes = recv(client->fd, client->buffer, KRAD_IPC_BUFFER_SIZE, 0);
	client->buffer[bytes] = '\0';
	//printf("Received %d bytes of data: '%s' \n", bytes, client->buffer);
	return bytes;
}

int krad_ipc_wait (krad_ipc_client_t *client, char *buffer, int size) {

	//int len;
	int bytes;
	fd_set set;

	//strcat(cmd, "|");
	//len = strlen(cmd);
	
	//FD_ZERO (&set);
	//FD_SET (client->fd, &set);
	
	//select (client->fd+1, NULL, &set, NULL, NULL);
	//send (client->fd, cmd, len, 0);

	FD_ZERO (&set);
	FD_SET (client->fd, &set);

	select (client->fd+1, &set, NULL, NULL, NULL);
	bytes = recv(client->fd, buffer, size, 0);
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
	//FD_SET (client->fd, &set);
	
	//select (client->fd+1, NULL, &set, NULL, NULL);
	//send (client->fd, cmd, len, 0);

	//FD_ZERO (&set);
	//FD_SET (client->fd, &set);

	//select (client->fd+1, &set, NULL, NULL, NULL);
	//bytes = recv(client->fd, client->buffer, KRAD_IPC_BUFFER_SIZE, 0);

	bytes = recv(client->fd, buffer, size, 0);
	buffer[bytes] = '\0';
	//printf("Received %d bytes of data: '%s' \n", bytes, client->buffer);
	return bytes;
}

void krad_ipc_disconnect(krad_ipc_client_t *client) {

	if (client != NULL) {
		if (client->buffer != NULL) {
			free(client->buffer);
		}
		if (client->fd != 0) {
			close(client->fd);
		}
		free(client);
	}
}

