#include "krad_xmms2.h"

int krad_xmms_playback_status_callback (xmmsv_t *value, void *userdata) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	int status;
	
	status = 0;

	if (!xmmsv_get_int (value, &status)) {
		failfast ("Value didn't contain the expected type!");
	}

	krad_xmms->playback_status = status;

	printk ("Got Playback Status: %d", krad_xmms->playback_status);

	return 1;

}

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

	xmmsc_result_t *result;
	
	result = NULL;

}

void krad_xmms_register_for_broadcasts (krad_xmms_t *krad_xmms) {

	xmmsc_result_t *result;
	
	result = NULL;

	result = xmmsc_playback_status (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playback_status_callback, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_broadcast_playback_status (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playback_status_callback, krad_xmms);
	xmmsc_result_unref (result);


	krad_xmms_handle (krad_xmms);

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
	
	krad_xmms_start_handler (krad_xmms);
	
}


void krad_xmms_handle (krad_xmms_t *krad_xmms) {

	int n;
	struct pollfd pollfds[1];
	
	n = 0;	
	pollfds[0].fd = krad_xmms->fd;
	
	if (xmmsc_io_want_out (krad_xmms->connection)) {
		pollfds[0].events = POLLIN | POLLOUT;
	} else {
		pollfds[0].events = POLLIN;
	}
	
	n = poll (pollfds, 1, 500);

	if (n < 0) {
		return;
	}

	if (n > 0) {

		for (n = 0; n < 1; n++) {

			if (pollfds[n].revents) {

				if ((pollfds[n].revents & POLLERR) || (pollfds[n].revents & POLLHUP)) {
					xmmsc_io_in_handle (krad_xmms->connection);
					n++;
				} else {

					if (pollfds[n].revents & POLLIN) {
						xmmsc_io_in_handle (krad_xmms->connection);
						if (xmmsc_io_want_out (krad_xmms->connection)) {
							pollfds[n].events = POLLIN | POLLOUT;
						}
					}

					if (pollfds[n].revents & POLLOUT) {
						if (xmmsc_io_want_out (krad_xmms->connection)) {
							xmmsc_io_out_handle (krad_xmms->connection);
						}
						pollfds[n].events = POLLIN;
					}
				}
			}
		}
	}
}

void *krad_xmms_handler_thread (void *arg) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *)arg;

	while (krad_xmms->handler_running == 1) {
	
		krad_xmms_handle (krad_xmms);

	}

	return NULL;

}


void krad_xmms_start_handler (krad_xmms_t *krad_xmms) {

	if (krad_xmms->handler_running == 1) {
		krad_xmms_stop_handler (krad_xmms);
	}

	krad_xmms->handler_running = 1;
	pthread_create (&krad_xmms->handler_thread, NULL, krad_xmms_handler_thread, (void *)krad_xmms);

}


void krad_xmms_stop_handler (krad_xmms_t *krad_xmms) {

	if (krad_xmms->handler_running == 1) {
		krad_xmms->handler_running = 2;
		pthread_join (krad_xmms->handler_thread, NULL);
		krad_xmms->handler_running = 0;
	}

}


void krad_xmms_destroy (krad_xmms_t *krad_xmms) {

	krad_xmms_stop_handler (krad_xmms);

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
