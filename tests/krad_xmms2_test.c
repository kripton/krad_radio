#include "krad_xmms2.h"



void krad_xmms2_test (char *sysname, char *ipc_path) {

	krad_xmms_t *krad_xmms;

	krad_xmms = krad_xmms_create (sysname, ipc_path);

	usleep (1000 * 1000);

	usleep (1000 * 1000);

	krad_xmms_destroy (krad_xmms);

}


int main (int argc, char *argv[]) {

	char *ipc_path;
	
	krad_system_init ();
	
	if (argc < 2) {
		failfast ("provide an ipc path");
	} else {
		ipc_path = argv[1];
		printk ("Using IPC Path: %s\n", ipc_path);
	}


	krad_xmms2_test ("testy1", argv[1]);

	return 0;

}
