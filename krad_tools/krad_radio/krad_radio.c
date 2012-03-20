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

void krad_radio_destroy(krad_radio_t *krad_radio) {

	free (krad_radio->name);	
	free (krad_radio->callsign);
	free (krad_radio);

}

krad_radio_t *krad_radio_create(char *station_or_config) {

	krad_radio_t *krad_radio = calloc(1, sizeof(krad_radio_t));

	// add config file stuff here
	if (1) {
		krad_radio->callsign = strdup (station_or_config);
		krad_radio->name = strdup (station_or_config);
	}
	
	return krad_radio;

}


void krad_radio (char *station_or_config) {

	krad_radio_t *krad_radio_station;

	krad_radio_station = krad_radio_create (station_or_config);

	printf("Krad Radio Station %s Daemonizing..\n", krad_radio_station->name);
	krad_radio_daemonize ();

	sleep (10);

	krad_radio_destroy (krad_radio_station);

}
