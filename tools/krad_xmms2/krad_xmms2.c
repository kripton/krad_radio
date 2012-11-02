#include "krad_xmms2.h"

void krad_xmms_playback_cmd (krad_xmms_t *krad_xmms, krad_xmms_playback_cmd_t cmd) {

	switch (cmd) {
		case PREV:
			xmmsc_result_unref ( xmmsc_playlist_set_next_rel (krad_xmms->connection, -1) );
			xmmsc_result_unref ( xmmsc_playback_tickle (krad_xmms->connection) );
			break;
		case NEXT:
			xmmsc_result_unref ( xmmsc_playlist_set_next_rel (krad_xmms->connection, 1) );
			xmmsc_result_unref ( xmmsc_playback_tickle (krad_xmms->connection) );
			break;
		case PAUSE:
			xmmsc_result_unref ( xmmsc_playback_pause (krad_xmms->connection) );
			break;
		case PLAY:
			xmmsc_result_unref ( xmmsc_playback_start (krad_xmms->connection) );
			break;
		case STOP:
			xmmsc_result_unref ( xmmsc_playback_stop (krad_xmms->connection) );		
			break;
	}

}

void krad_xmms_disconnect_callback (void *userdata) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	krad_xmms_disconnect (krad_xmms);
	
}

int krad_xmms_media_info_callback (xmmsv_t *value, void *userdata) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	xmmsv_t *dict_entry;
	xmmsv_t *info;
	const char *val;

	info = xmmsv_propdict_to_dict (value, NULL);

	if (!xmmsv_dict_get (info, "artist", &dict_entry) || !xmmsv_get_string (dict_entry, &val)) {
		val = "No artist";
	}

	sprintf (krad_xmms->artist, "%s", val);

	if (!xmmsv_dict_get (info, "title", &dict_entry) || !xmmsv_get_string (dict_entry, &val)) {
		val = "No Title";
	}

	sprintf (krad_xmms->title, "%s", val);
	sprintf (krad_xmms->now_playing, "%s - %s", krad_xmms->artist, krad_xmms->title);	

	/*
	if (!xmmsv_dict_get (infos, "picture_front", &dict_entry) || !xmmsv_get_string (dict_entry, &val)) {
		if (!xmmsv_dict_get (infos, "kcoverhash", &dict_entry) || !xmmsv_get_string (dict_entry, &val)) {
			// Neither image source
			strcpy(xmms->current_image, "");
			strcpy(xmms->current_image_url, "");				
		} else {
			//found kcover
			xmms->url_start = "http://kradradio.com/covers";
			sprintf (xmms->current_image, "%s", val);
			sprintf(xmms->current_image_url, "%s/%s", xmms->url_start, xmms->current_image);
		}
	} else {
		// found embedded art
		xmms->url_start = "http://kradradio.com/bindata";
		sprintf (xmms->current_image, "%s", val);
		sprintf(xmms->current_image_url, "%s/%s/%s", xmms->url_start, xmms->kradstation->station_shortname, xmms->current_image);
	}
	*/
	xmmsv_unref (info);
	
	if (krad_xmms->krad_tags != NULL) {
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "artist", krad_xmms->artist);
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "title", krad_xmms->title);
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "now_playing", krad_xmms->now_playing);
	}
	
	printk ("Got now playing: %s", krad_xmms->now_playing);	
		
	return 1;
}

int krad_xmms_playing_id_callback (xmmsv_t *value, void *userdata) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	xmmsc_result_t *result;	
	int id;
	
	result = NULL;
	id = 0;

	if (!xmmsv_get_int (value, &id)) {
		failfast ("Value didn't contain the expected type!");
	}
	
	krad_xmms->playing_id = id;

	//printk ("Got playback id: %d", krad_xmms->playing_id);
	
	result = xmmsc_medialib_get_info (krad_xmms->connection, krad_xmms->playing_id);
	xmmsc_result_notifier_set (result, krad_xmms_media_info_callback, krad_xmms);
	xmmsc_result_unref (result);
	
	return 1;
	
}

int krad_xmms_playtime_callback (xmmsv_t *value, void *userdata) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	int playtime;
	
	playtime = 0;

	if (!xmmsv_get_int (value, &playtime)) {
		failfast ("Value didn't contain the expected type!");
	}

	if (krad_xmms->playtime != (playtime / 1000)) {
		krad_xmms->playtime = (playtime / 1000);
		sprintf (krad_xmms->playtime_string, "%d:%02d", krad_xmms->playtime / 60, krad_xmms->playtime % 60);
		//printk ("Got playtime: %s", krad_xmms->playtime_string);
		if (krad_xmms->krad_tags != NULL) {
			krad_tags_set_tag_internal (krad_xmms->krad_tags, "playtime", krad_xmms->playtime_string);
		}			
	}

	return 1;

}

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
		krad_xmms->handler_running = 0;
	}
}

void krad_xmms_unregister_for_broadcasts (krad_xmms_t *krad_xmms) {

	xmmsc_result_t *result;
	
	result = NULL;
	
	result = xmmsc_broadcast_playback_status (krad_xmms->connection);
	xmmsc_result_disconnect (result);
	xmmsc_result_unref (result);
	xmmsc_unref (krad_xmms->connection);
	
	result = xmmsc_broadcast_playback_current_id (krad_xmms->connection);
	xmmsc_result_disconnect (result);
	xmmsc_result_unref (result);
	xmmsc_unref (krad_xmms->connection);
	
	result = xmmsc_signal_playback_playtime (krad_xmms->connection);
	xmmsc_result_disconnect (result);
	xmmsc_result_unref (result);
	xmmsc_unref (krad_xmms->connection);	

}

void krad_xmms_register_for_broadcasts (krad_xmms_t *krad_xmms) {

	xmmsc_result_t *result;
	
	result = NULL;

	result = xmmsc_playback_status (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playback_status_callback, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_playback_current_id (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playing_id_callback, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_broadcast_playback_status (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playback_status_callback, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_signal_playback_playtime (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playtime_callback, krad_xmms);
	xmmsc_result_unref (result);
	
	result = xmmsc_broadcast_playback_current_id (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playing_id_callback, krad_xmms);
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
	
	xmmsc_disconnect_callback_set (krad_xmms->connection, krad_xmms_disconnect_callback, krad_xmms);
	
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

	krad_system_set_thread_name ("kr_xmms2");

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

krad_xmms_t *krad_xmms_create (char *sysname, char *ipc_path, krad_tags_t *krad_tags) {

	krad_xmms_t *krad_xmms = calloc (1, sizeof(krad_xmms_t));

	strcpy (krad_xmms->sysname, sysname);
	strcpy (krad_xmms->ipc_path, ipc_path);

	krad_xmms_connect (krad_xmms);
	
	if (krad_xmms->connected == 0) {
		krad_xmms_destroy (krad_xmms);
		return NULL;
	}
	
	krad_xmms->krad_tags = krad_tags;
	
	return krad_xmms;

}
