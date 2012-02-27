#include "krad_mixer.h"

int main (int argc, char *argv[]) {

	krad_mixer_t *krad_mixer;

	char daemon_name[320] = "krad_mixer";

	if (argc == 2) {

		strcat(daemon_name, "_");
		strcat(daemon_name, argv[1]);
		
	} else {
		printf("No station!\n");
		exit(1);
	}
	
	printf("Starting Krad Mixer for %s\n", argv[1]);

	daemonize(daemon_name);

	krad_mixer = krad_mixer_setup(argv[1]);

	example_session(krad_mixer);
		
	if ((krad_mixer->ipc = krad_ipc_server_setup("krad_mixer", argv[1])) == NULL) {
		krad_mixer_shutdown (krad_mixer);
		exit(1);
	}

	krad_ipc_server_set_new_client_callback(krad_mixer->ipc, krad_mixer_new_client);
	krad_ipc_server_set_close_client_callback(krad_mixer->ipc, krad_mixer_close_client);
	krad_ipc_server_set_shutdown_callback(krad_mixer->ipc, krad_mixer_shutdown);
	krad_ipc_server_set_handler_callback(krad_mixer->ipc, krad_mixer_handler);
	krad_ipc_server_set_client_callback_pointer(krad_mixer->ipc, krad_mixer);
	krad_ipc_server_signals_setup(krad_mixer->ipc);
	krad_ipc_server_run(krad_mixer->ipc);


}
