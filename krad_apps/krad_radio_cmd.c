#include "krad_ipc_client.h"
#include "krad_radio_ipc.h"

int main (int argc, char *argv[]) {

	krad_ipc_client_t *client;
	char cmd[128];
	
	if (argc > 2) {

		client = krad_ipc_connect (argv[1]);
	
		if (client != NULL) {
	
			
			if (strncmp(argv[2], "tag", 3) == 0) {
			
				if (strncmp(argv[2], "tags", 4) == 0) {
	
					krad_ipc_get_tags (client, argv[3]);		
	
				} else {
			
					if (argc == 4) {
						krad_ipc_get_tag (client, argv[3]);
					}
				
					if (argc == 5) {
						krad_ipc_set_tag (client, argv[3], argv[4]);
					}
				}
			
				krad_ipc_wait (client, client->buffer, 222);
			
			} else {
	
				sprintf(cmd, "%s", argv[2]);
	
				krad_ipc_cmd(client, cmd);
			}
			
			
			if (strlen(client->buffer) > 0) {
	
				printf("%s\n", client->buffer);
	
			}
		
			krad_ipc_disconnect (client);
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
