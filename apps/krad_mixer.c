#include "krad_mixer_clientlib.h"

int main (int argc, char *argv[])
{

	krad_mixer_ipc_client_t *client;

	if (argc > 2) {

		client = krad_mixer_connect(argv[1]);
	
		if (client != NULL) {
		
			char cmd[128] = "";
	
			sprintf(cmd + strlen(cmd), "%s", argv[2]);
	
			krad_mixer_cmd(client, cmd);

			if (strlen(client->krad_ipc_client->buffer) > 0) {
	
				printf("%s\n", client->krad_ipc_client->buffer);
	
			}	
	
			krad_mixer_disconnect(client);
		}
	
	} else {
	
		if (argc == 1) {
			printf("Specify a station..\n");
		}
		if (argc == 2) {
			printf("Specify a command..\n");
		}
		
	}
	
	return 0;
	
}
