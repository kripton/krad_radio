#include "krad_xmms2.h"

static int krad_xmms_playtime_callback (xmmsv_t *value, void *userdata);
static int krad_xmms_playing_id_callback (xmmsv_t *value, void *userdata);
static int krad_xmms_playback_status_callback (xmmsv_t *value, void *userdata);
static void krad_xmms_disconnect_callback (void *userdata);

static void *krad_xmms_handler_thread (void *arg);
static void krad_xmms_start_handler (krad_xmms_t *krad_xmms);
static void krad_xmms_stop_handler (krad_xmms_t *krad_xmms);
static void krad_xmms_handle (krad_xmms_t *krad_xmms);

static void krad_xmms_get_initial_state (krad_xmms_t *krad_xmms);
static void krad_xmms_register_for_broadcasts (krad_xmms_t *krad_xmms);
static void krad_xmms_unregister_for_broadcasts (krad_xmms_t *krad_xmms);

static void krad_xmms_error_state (krad_xmms_t *krad_xmms);

static void krad_xmms_connect (krad_xmms_t *krad_xmms);
static void krad_xmms_disconnect (krad_xmms_t *krad_xmms);

static int krad_xmms_media_info_callback (xmmsv_t *value, void *userdata) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	xmmsv_t *dict_entry;
	xmmsv_t *info;
	const char *val;

	info = xmmsv_propdict_to_dict (value, NULL);

	if (!xmmsv_dict_get (info, "artist", &dict_entry) || !xmmsv_get_string (dict_entry, &val)) {
		val = "Unknown Artist";
	}

	sprintf (krad_xmms->artist, "%s", val);

	if (!xmmsv_dict_get (info, "title", &dict_entry) || !xmmsv_get_string (dict_entry, &val)) {
		val = "Unknown Title";
	}

	sprintf (krad_xmms->title, "%s", val);
	sprintf (krad_xmms->now_playing, "%s - %s", krad_xmms->artist, krad_xmms->title);	

	xmmsv_unref (info);
	
	if (krad_xmms->krad_tags != NULL) {
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "artist", krad_xmms->artist);
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "title", krad_xmms->title);
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "now_playing", krad_xmms->now_playing);
	}
	
	printk ("Got now playing: %s", krad_xmms->now_playing);	
		
	return 1;
}

static int krad_xmms_playing_id_callback (xmmsv_t *value, void *userdata) {

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

static int krad_xmms_playtime_callback (xmmsv_t *value, void *userdata) {

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

static int krad_xmms_playback_status_callback (xmmsv_t *value, void *userdata) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	int status;
	
	status = 0;

	if (!xmmsv_get_int (value, &status)) {
		failfast ("Value didn't contain the expected type!");
	}

	krad_xmms->playback_status = status;

	printk ("Got Playback Status: %d", krad_xmms->playback_status);

  if ((krad_xmms->krad_tags != NULL) && (krad_xmms->playback_status == 0)) {
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "playtime", "0:00");
  }

	return 1;

}

static int krad_xmms_playlist_current_pos (xmmsv_t *value, void *userdata ) {
/*
  krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;
	const char *playlist_name;

	xmmsv_dict_entry_get_string (value, "name", &playlist_name);

	if ((strlen(krad_xmms->active_playlist) == strlen(playlist_name)) && 
      (strncmp (krad_xmms->active_playlist, playlist_name, sizeof(krad_xmms->active_playlist)) == 0)) {

		xmmsv_dict_entry_get_int (value, "position", &krad_xmms->active_playlist_position);

    // Need to use this to get the cued track when we are changing playlist pos
    // whilst stopped
    
	}
*/
	return 1;

}

static int krad_xmms_playlist_loaded (xmmsv_t *value, void *userdata) {

  krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	const char *active_playlist;

	xmmsv_get_string (value, &active_playlist);
	strncpy (krad_xmms->active_playlist, active_playlist, sizeof(krad_xmms->active_playlist));
	printk ("The active playlist is: %s", krad_xmms->active_playlist);

	return 1;

}

static void krad_xmms_error_state (krad_xmms_t *krad_xmms) {
    krad_xmms->playback_status = -1;
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "playtime", "0:00");
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "artist", "Error");
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "title", "Error");
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "now_playing", "Error - Error");
}

static void krad_xmms_disconnect_callback (void *userdata) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *) userdata;

	krad_xmms_disconnect (krad_xmms);
	
}

static void krad_xmms_disconnect (krad_xmms_t *krad_xmms) {

	if (krad_xmms->connected == 1) {
		krad_xmms_unregister_for_broadcasts (krad_xmms);
		xmmsc_unref (krad_xmms->connection);
		krad_xmms->connected = 0;
		krad_xmms->fd = 0;
		krad_xmms->connection = NULL;
    krad_xmms->playback_status = -1;
	}

	if ((krad_xmms->krad_tags != NULL) && (krad_xmms->destroying == 0)) {
    krad_xmms_error_state (krad_xmms);
	}

}

static void krad_xmms_unregister_for_broadcasts (krad_xmms_t *krad_xmms) {

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

	result = xmmsc_broadcast_playlist_loaded (krad_xmms->connection);
	xmmsc_result_disconnect (result);
	xmmsc_result_unref (result);
	xmmsc_unref (krad_xmms->connection);

	result = xmmsc_broadcast_playlist_current_pos (krad_xmms->connection);
	xmmsc_result_disconnect (result);
	xmmsc_result_unref (result);
	xmmsc_unref (krad_xmms->connection);
	
	result = xmmsc_signal_playback_playtime (krad_xmms->connection);
	xmmsc_result_disconnect (result);
	xmmsc_result_unref (result);
	xmmsc_unref (krad_xmms->connection);	

}

static void krad_xmms_register_for_broadcasts (krad_xmms_t *krad_xmms) {

	xmmsc_result_t *result;
	
	result = NULL;

	result = xmmsc_broadcast_playback_status (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playback_status_callback, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_broadcast_playback_current_id (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playing_id_callback, krad_xmms);
	xmmsc_result_unref (result);	

	result = xmmsc_broadcast_playlist_loaded (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playlist_loaded, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_broadcast_playlist_current_pos (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playlist_current_pos, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_signal_playback_playtime (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playtime_callback, krad_xmms);
	xmmsc_result_unref (result);

}


static void krad_xmms_get_initial_state (krad_xmms_t *krad_xmms) {

	xmmsc_result_t *result;
	
	result = NULL;

	result = xmmsc_playback_status (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playback_status_callback, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_playback_current_id (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playing_id_callback, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_playlist_current_active (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playlist_loaded, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_playlist_current_pos (krad_xmms->connection, NULL);
	xmmsc_result_notifier_set (result, krad_xmms_playlist_current_pos, krad_xmms);
	xmmsc_result_unref (result);

	result = xmmsc_playback_playtime (krad_xmms->connection);
	xmmsc_result_notifier_set (result, krad_xmms_playtime_callback, krad_xmms);
	xmmsc_result_unref (result);

}

static void krad_xmms_lock (void *arg) {

  krad_xmms_t *krad_xmms = (krad_xmms_t *)arg;

  pthread_mutex_lock (&krad_xmms->lock);	
}

static void krad_xmms_unlock (void *arg) {

  krad_xmms_t *krad_xmms = (krad_xmms_t *)arg;

  pthread_mutex_unlock (&krad_xmms->lock);	
}

static void krad_xmms_connect (krad_xmms_t *krad_xmms) {

	char connection_name[256];

	sprintf (connection_name, "krad_xmms_%s", krad_xmms->sysname);

	krad_xmms->connection = xmmsc_init (connection_name);

	if (!krad_xmms->connection) {
		failfast ("Out of memory for new xmms connection");
	}

  xmmsc_lock_set ( krad_xmms->connection, krad_xmms, krad_xmms_lock, krad_xmms_unlock);

	if (!xmmsc_connect (krad_xmms->connection, krad_xmms->ipc_path)) {
		printke ("Connection failed: %s", xmmsc_get_last_error (krad_xmms->connection));
    krad_xmms_error_state (krad_xmms);
		krad_xmms->connected = 0;
		return;
	}

	printk ("Krad xmms Connected %s to %s", krad_xmms->sysname, krad_xmms->ipc_path);

	krad_xmms->fd = xmmsc_io_fd_get (krad_xmms->connection);

	krad_xmms->connected = 1;	
	
	xmmsc_disconnect_callback_set (krad_xmms->connection, krad_xmms_disconnect_callback, krad_xmms);
	
	krad_xmms_register_for_broadcasts (krad_xmms);
	
	krad_xmms_get_initial_state (krad_xmms);

}


static void krad_xmms_handle (krad_xmms_t *krad_xmms) {

  char in;
	int n;
  int nfds;
  int ret;
	struct pollfd pollfds[2];
	
	n = 0;

  if (krad_xmms->fd > 0) {
	  pollfds[n].fd = krad_xmms->fd;

	  if (xmmsc_io_want_out (krad_xmms->connection)) {
		  pollfds[n].events = POLLIN | POLLOUT;
	  } else {
		  pollfds[n].events = POLLIN;
	  }
    n++;
  }

  pollfds[n].fd = krad_xmms->handler_thread_socketpair[1];
  pollfds[n].events = POLLIN;
  n++;

  nfds = n;

	ret = poll (pollfds, nfds, 1000);
	if (ret < 0) {
    krad_xmms->handler_running = 2;
		return;
	}

	if (ret > 0) {

		for (n = 0; n < nfds; n++) {

			if (pollfds[n].revents) {
        if (pollfds[n].fd == krad_xmms->handler_thread_socketpair[1]) {
				  if (pollfds[n].revents & POLLIN) {
            read (krad_xmms->handler_thread_socketpair[1], &in, 1);            
            if (strncmp(&in, "1", 1) == 0) {
                return;
            }
          }
          krad_xmms->handler_running = 2;
          return;
        } else {
				  if ((pollfds[n].revents & POLLERR) || (pollfds[n].revents & POLLHUP)) {
					  xmmsc_io_in_handle (krad_xmms->connection);
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
}

static void *krad_xmms_handler_thread (void *arg) {

	krad_xmms_t *krad_xmms = (krad_xmms_t *)arg;

	krad_system_set_thread_name ("kr_xmms2");

	while (krad_xmms->handler_running == 1) {
		if (krad_xmms->connected == 0) {
			krad_xmms_connect (krad_xmms);
		}
		krad_xmms_handle (krad_xmms);
	}

	if (krad_xmms->connected == 1) {
		krad_xmms_disconnect (krad_xmms);
	}

	return NULL;

}


static void krad_xmms_start_handler (krad_xmms_t *krad_xmms) {

  char error_string[64];

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, krad_xmms->handler_thread_socketpair)) {
    strerror_r (errno, error_string, sizeof(error_string));
    printk("krad_xmms: could not create socketpair: %s", error_string);
		return;
	}

	krad_xmms->handler_running = 1;
	pthread_create (&krad_xmms->handler_thread, NULL, krad_xmms_handler_thread, (void *)krad_xmms);

}


static void krad_xmms_stop_handler (krad_xmms_t *krad_xmms) {
	if (krad_xmms->handler_running == 1) {
    write (krad_xmms->handler_thread_socketpair[0], "0", 1);
		pthread_join (krad_xmms->handler_thread, NULL);
		close (krad_xmms->handler_thread_socketpair[0]);
    close (krad_xmms->handler_thread_socketpair[1]);
		krad_xmms->handler_running = 0;
	}
}


/* The following are called from Krad IPC */

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

  write (krad_xmms->handler_thread_socketpair[0], "1", 1);

}

void krad_xmms_destroy (krad_xmms_t *krad_xmms) {
  krad_xmms->destroying = 1;
  krad_xmms_stop_handler (krad_xmms);

	if (krad_xmms->krad_tags != NULL) {
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "playtime", "");
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "artist", "");
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "title", "");
		krad_tags_set_tag_internal (krad_xmms->krad_tags, "now_playing", "");
	}
	pthread_mutex_destroy (&krad_xmms->lock);
  free (krad_xmms);
}

krad_xmms_t *krad_xmms_create (char *sysname, char *ipc_path, krad_tags_t *krad_tags) {

  krad_xmms_t *krad_xmms = calloc (1, sizeof(krad_xmms_t));

  strcpy (krad_xmms->sysname, sysname);
  strcpy (krad_xmms->ipc_path, ipc_path);
  krad_xmms->krad_tags = krad_tags;
	pthread_mutex_init (&krad_xmms->lock, NULL);
  krad_xmms->playback_status = -1;
  krad_xmms_start_handler (krad_xmms);

  return krad_xmms;

}
