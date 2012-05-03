#include "krad_ipc_client.h"
#include "krad_radio_ipc.h"

int main (int argc, char *argv[]) {

	krad_ipc_client_t *client;
	
	if (argc > 2) {

		client = krad_ipc_connect (argv[1]);
	
		if (client != NULL) {
	
			/* Krad Radio Commands */
			
			if (strncmp(argv[2], "tag", 3) == 0) {
			
				if (strncmp(argv[2], "tags", 4) == 0) {
	
					krad_ipc_get_tags (client);		
					krad_ipc_print_response (client);
				} else {
			
					if (argc == 4) {
						krad_ipc_get_tag (client, argv[3]);
						krad_ipc_print_response (client);	
					}
				
					if (argc == 5) {
						krad_ipc_set_tag (client, argv[3], argv[4]);
					}
				}
			}
			
			if (strncmp(argv[2], "webon", 5) == 0) {
				if (argc == 5) {
					krad_ipc_webon (client, atoi(argv[3]), atoi(argv[4]));
					krad_ipc_print_response (client);
				}
			}			
			
			if (strncmp(argv[2], "weboff", 5) == 0) {
				if (argc == 3) {
					krad_ipc_weboff (client);
					krad_ipc_print_response (client);
				}
			}			
			
			/* Krad Mixer Commands */
			
			if (strncmp(argv[2], "mix", 3) == 0) {
				if (argc == 3) {
					krad_ipc_get_portgroups (client);
					krad_ipc_print_response (client);
				}
			}
			
			if (strncmp(argv[2], "input", 5) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_create_portgroup (client, argv[3], "input");
					krad_ipc_print_response (client);
				}
			}			

			if (strncmp(argv[2], "output", 6) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_create_portgroup (client, argv[3], "output");
					krad_ipc_print_response (client);
				}
			}

			if (strncmp(argv[2], "unplug", 6) == 0) {
				if (argc == 4) {
					krad_ipc_mixer_remove_portgroup (client, argv[3]);
					krad_ipc_print_response (client);
				}
			}
		
			if (strncmp(argv[2], "set", 3) == 0) {
				if (argc == 6) {
					krad_ipc_set_control (client, argv[3], argv[4], atof(argv[5]));
					//krad_ipc_print_response (client);
				}
			}
			
			/* Krad Link Commands */			

			if (strncmp(argv[2], "ls", 2) == 0) {
				if (argc == 3) {
					krad_ipc_list_links (client);
					krad_ipc_print_response (client);
				}
			}
			
			if (strncmp(argv[2], "link", 4) == 0) {
				if (argc == 6) {
					krad_ipc_create_link (client, argv[3], atoi(argv[4]), argv[5]);
				}
			}		
			
			if (strncmp(argv[2], "rm", 2) == 0) {
				if (argc == 4) {
					krad_ipc_destroy_link (client, atoi(argv[3]));
				}
			}
			
			usleep(80000);
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
