#include <krad_radio.h>


void krad_radio_daemonize () {

	pid_t pid, sid;

	close (STDIN_FILENO);
	close (STDOUT_FILENO);
	close (STDERR_FILENO);

	pid = fork();

	if (pid < 0) {
		exit (EXIT_FAILURE);
	}

	if (pid > 0) {
		exit (EXIT_SUCCESS);
	}
	
	umask(0);
 
	sid = setsid();
	
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}
        
}

int krad_radio_valid_callsign(char *callsign) {
	
	int i = 0;
	char j;
	
	char requirements[] = "Callsigns must be atleast 4 numbers or lowercase letters, start with a letter, and be no longer than 40 characters.";
	
	
	if (strlen(callsign) < 4) {
		fprintf (stderr, "Callsign Name %s is invalid, too short!\n", callsign);
		fprintf (stderr, "%s\n", requirements);
		return 0;
	}
	
	if (strlen(callsign) > 40) {
		fprintf (stderr, "Callsign %s is invalid, too long!\n", callsign);
		fprintf (stderr, "%s\n", requirements);
		return 0;
	}
	
	j = callsign[i];
	if (!((isalpha (j)) && (islower (j)))) {
		fprintf (stderr, "Callsign %s is invalid, must start with a lowercase letter!\n", callsign);
		fprintf (stderr, "%s\n", requirements);
		return 0;
	}
	i++;

	while (callsign[i]) {
		j = callsign[i];
			if (!isalnum (j)) {
				fprintf (stderr, "Callsign %s is invalid, alphanumeric only!\n", callsign);
				fprintf (stderr, "%s\n", requirements);
				return 0;
			}
			if (isalpha (j)) {
				if (!islower (j)) {
					fprintf (stderr, "Callsign %s is invalid lowercase letters only!\n", callsign);
					fprintf (stderr, "%s\n", requirements);
					return 0;
				}
			}
		i++;
	}

	return 1;

}

void krad_radio_destroy (krad_radio_t *krad_radio) {

	krad_http_server_destroy (krad_radio->krad_http);
	krad_websocket_server_destroy (krad_radio->krad_websocket);
	krad_ipc_server_destroy (krad_radio->ipc);
	free (krad_radio->name);	
	free (krad_radio->callsign);
	free (krad_radio);

}

krad_radio_t *krad_radio_create (char *callsign_or_config) {

	krad_radio_t *krad_radio = calloc(1, sizeof(krad_radio_t));

	// add config file stuff here
	if (1) {
		if (!krad_radio_valid_callsign(callsign_or_config)) {
			return NULL;
		}
		krad_radio->callsign = strdup (callsign_or_config);
		krad_radio->name = strdup (callsign_or_config);
	}
	
	krad_radio->ipc = krad_ipc_server ( callsign_or_config, krad_radio_handler, krad_radio );
	
	if (krad_radio->ipc == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_websocket = krad_websocket_server_create (callsign_or_config, 41222);
	
	if (krad_radio->krad_websocket == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}	
	
	krad_radio->krad_http = krad_http_server_create (41221);
	
	if (krad_radio->krad_http == NULL) {
		krad_radio_destroy (krad_radio);
		return NULL;
	}	
	
	return krad_radio;

}


void krad_radio (char *callsign_or_config) {

	krad_radio_t *krad_radio_station;

	krad_radio_station = krad_radio_create (callsign_or_config);

	if (krad_radio_station != NULL) {
		printf("Krad Radio Station %s Daemonizing..\n", krad_radio_station->name);
		//krad_radio_daemonize ();

		krad_radio_run ( krad_radio_station );

		krad_radio_destroy (krad_radio_station);
	}
}


void krad_radio_run (krad_radio_t *krad_radio) {

	while (1) {
		sleep (25);
	}
}


int krad_radio_handler ( void *output, int *output_len, void *ptr ) {

	krad_radio_t *krad_radio_station = (krad_radio_t *)ptr;
	
	uint32_t command;
	uint64_t ebml_data_size;
	uint64_t number;
	
	//printf("handler! \n");	
	
	krad_ipc_server_read_command ( krad_radio_station->ipc, &command, &ebml_data_size);

	switch ( command ) {
	
		case EBML_ID_KRAD_RADIO_CMD:
			return krad_radio_handler ( output, output_len, ptr );
		case EBML_ID_KRAD_RADIO_CMD_SETCONTROL:
			number = krad_ipc_server_read_number ( krad_radio_station->ipc, ebml_data_size );
			krad_radio_station->test_value = number;
			//printf("SET CONTROL to %d!\n", krad_radio_station->test_value);
			krad_ipc_server_broadcast_number ( krad_radio_station->ipc, EBML_ID_KRAD_RADIO_RESPONSE_GETCONTROL, krad_radio_station->test_value);
			*output_len = 666;
			return 2;
			break;	
		case EBML_ID_KRAD_RADIO_CMD_GETCONTROL:
			//printf("GET CONTROL! %d\n", krad_radio_station->test_value);
			krad_ipc_server_respond_number ( krad_radio_station->ipc, EBML_ID_KRAD_RADIO_RESPONSE_GETCONTROL, krad_radio_station->test_value);
			return 1;
			break;
		default:
			printf("unknown command! %u\n", command);
			return 0;
			break;
	}

	return 0;

}
