#include "krad_xmms2.h"



void krad_xmms_disconnect (krad_xmms_t *krad_xmms) {

	if (krad_xmms->connected == 1) {
		krad_xmms_unregister_for_broadcasts (krad_xmms);
		xmmsc_unref (krad_xmms->connection);
		krad_xmms->connected = 0;
		krad_xmms->fd = 0;
		krad_xmms->connection = NULL;
	}
}

void krad_xmms_unregister_for_broadcasts (krad_xmms_t *krad_xmms) {


}

void krad_xmms_register_for_broadcasts (krad_xmms_t *krad_xmms) {


//	result = xmmsc_playback_status (xmms->connection);
//	xmmsc_result_notifier_set (result, playback_status, xmms);
//	xmmsc_result_unref (result);

}


void krad_xmms_connect (krad_xmms_t *krad_xmms) {

	char connection_name[256];

	sprintf (connection_name, "krad_xmms_%s", krad_xmms->sysname);

	krad_xmms->connection = xmmsc_init (connection_name);

	if (!krad_xmms->connection) {
		failfast ("Out of memory for new xmms connection");
	}

	if (!xmmsc_connect (krad_xmms->connection, krad_xmms->ipc_path)) {
		printke ("Connection failed: %s", xmmsc_get_last_error (krad_xmms->connection));
		krad_xmms->connected = 0;
		return;
	}

	printk ("Krad xmms Connected %s to %s", krad_xmms->sysname, krad_xmms->ipc_path);

	krad_xmms->fd = xmmsc_io_fd_get (krad_xmms->connection);

	krad_xmms->connected = 1;	
	
	krad_xmms_register_for_broadcasts (krad_xmms);
	
}

void krad_xmms_destroy (krad_xmms_t *krad_xmms) {

	if (krad_xmms->connected == 1) {
		krad_xmms_disconnect (krad_xmms);
	}

	free (krad_xmms);

}

krad_xmms_t *krad_xmms_create (char *sysname, char *ipc_path) {

	krad_xmms_t *krad_xmms = calloc (1, sizeof(krad_xmms_t));

	strcpy (krad_xmms->sysname, sysname);
	strcpy (krad_xmms->ipc_path, ipc_path);

	krad_xmms_connect (krad_xmms);
	
	return krad_xmms;

}
