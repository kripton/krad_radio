#include "krad_mixer_clientlib.h"

krad_mixer_ipc_client_t *krad_mixer_connect (char *station_name)
{

	krad_mixer_ipc_client_t *client = NULL;

	if ((client = calloc (1, sizeof (krad_mixer_ipc_client_t))) == NULL) {
				fprintf(stderr, "krad_mixer IPC Client Mem ALLOC FAIL\n");
				exit (1);
	}
	
	client->krad_ipc_client = krad_ipc_connect("krad_mixer", station_name);
	
	if (client->krad_ipc_client == NULL) {
		client = krad_mixer_disconnect(client);
	}

	return client;
}

int krad_mixer_cmd (krad_mixer_ipc_client_t *client, char *cmd) {

	return krad_ipc_cmd (client->krad_ipc_client, cmd);

}

int krad_mixer_wait (krad_mixer_ipc_client_t *client, char *buffer, int size) {

	return krad_ipc_wait (client->krad_ipc_client, buffer, size);

}

int krad_mixer_recv (krad_mixer_ipc_client_t *client, char *buffer, int size) {

	return krad_ipc_recv (client->krad_ipc_client, buffer, size);

}


void krad_mixer_send (krad_mixer_ipc_client_t *client, char *cmd) {

	krad_ipc_send (client->krad_ipc_client, cmd);

}

krad_mixer_ipc_client_t *krad_mixer_disconnect(krad_mixer_ipc_client_t *client) {

	if (client != NULL) {
		krad_ipc_disconnect(client->krad_ipc_client);
		free(client);
		client = NULL;
	}
	
	return NULL;
}

/* re-entrant */
int krad_mixer_destroy_portgroup_advanced(krad_mixer_ipc_client_t *client, char *cmd, char *name, char *codec, char *type) {
	
	sprintf(cmd, "destroy_advanced_portgroup,%s,%s,%s", name, codec, type);
	//return krad_mixer_cmd(client, cmd);
	krad_mixer_send(client, cmd);
	return 0;
}

int krad_mixer_create_portgroup_advanced(krad_mixer_ipc_client_t *client, char *cmd, char *name, char *codec, char *type) {
	
	sprintf(cmd, "create_advanced_portgroup,%s,%s,%s", name, codec, type);
	//return krad_mixer_cmd(client, cmd);
	krad_mixer_send(client, cmd);
	
	usleep(400000); //kludge, prolly long enuf to create the ports?
	
	return 0;
}

int krad_mixer_create_portgroup(krad_mixer_ipc_client_t *client, char *name) {

	char cmd[256];
	
	sprintf(cmd, "create_portgroup,%s", name);
	//return krad_mixer_cmd(client, cmd);
	krad_mixer_send(client, cmd);
	
	usleep(400000); //kludge, prolly long enuf to create the ports?
	
	return 0;
}

int krad_mixer_destroy_portgroup(krad_mixer_ipc_client_t *client, char *name) {

	char cmd[256];

	sprintf(cmd, "destroy_portgroup,%s", name);
	//return krad_mixer_cmd(client, cmd);
	krad_mixer_send(client, cmd);
	return 0;
}

