#include "krad_radio.h"
#include "krad_radio_interface.h"
#include "krad_radio_internal.h"

static void krad_radio_shutdown (krad_radio_t *krad_radio);
static krad_radio_t *krad_radio_create (char *sysname);
static void krad_radio_run (krad_radio_t *krad_radio);

static void krad_radio_shutdown (krad_radio_t *krad_radio) {

  krad_system_monitor_cpu_off ();
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));
	if (krad_radio->krad_ipc != NULL) {
    krad_ipc_server_disable (krad_radio->krad_ipc);
	}
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));
	if (krad_radio->krad_osc != NULL) {
		krad_osc_destroy (krad_radio->krad_osc);
		krad_radio->krad_osc = NULL;
	}
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));	
	if (krad_radio->krad_http != NULL) {
		krad_http_server_destroy (krad_radio->krad_http);
		krad_radio->krad_http = NULL;
	}
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));	
	if (krad_radio->krad_websocket != NULL) {
    krad_websocket_server_destroy (krad_radio->krad_websocket);
		krad_radio->krad_websocket = NULL;
	}
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));	
	if (krad_radio->krad_transponder != NULL) {
		krad_transponder_destroy (krad_radio->krad_transponder);
		krad_radio->krad_transponder = NULL;		
	}
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));	
	if (krad_radio->krad_mixer != NULL) {
		krad_mixer_destroy (krad_radio->krad_mixer);
		krad_radio->krad_mixer = NULL;		
	}
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));	
	if (krad_radio->krad_compositor != NULL) {
		krad_compositor_destroy (krad_radio->krad_compositor);
		krad_radio->krad_compositor = NULL;
	}
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));	
	if (krad_radio->krad_tags != NULL) {
		krad_tags_destroy (krad_radio->krad_tags);
		krad_radio->krad_tags = NULL;
	}	
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));	
	if (krad_radio->krad_ipc != NULL) {
		krad_ipc_server_destroy (krad_radio->krad_ipc);
		krad_radio->krad_ipc = NULL;
	}
  printk ("Krad Radio shutdown timer at %"PRIu64"ms", krad_timer_sample_duration_ms (krad_radio->shutdown_timer));	
	if (krad_radio->shutdown_timer != NULL) {
    krad_timer_finish (krad_radio->shutdown_timer);	
	  printk ("Krad Radio took %"PRIu64"ms to shutdown", krad_timer_duration_ms (krad_radio->shutdown_timer));
    krad_timer_destroy (krad_radio->shutdown_timer);
    krad_radio->shutdown_timer = NULL;
	}

	free (krad_radio->sysname);
	free (krad_radio);

	printk ("Krad Radio exited cleanly");
	krad_system_log_off ();

}

static krad_radio_t *krad_radio_create (char *sysname) {

	krad_radio_t *krad_radio = calloc (1, sizeof(krad_radio_t));

	if (!krad_valid_sysname(sysname)) {
		return NULL;
	}
	krad_radio->sysname = strdup (sysname);
	
	krad_radio->krad_tags = krad_tags_create ("station");
	
	if (krad_radio->krad_tags == NULL) {
		krad_radio_shutdown (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_mixer = krad_mixer_create (krad_radio->sysname);
	
	if (krad_radio->krad_mixer == NULL) {
		krad_radio_shutdown (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_compositor = krad_compositor_create (DEFAULT_COMPOSITOR_WIDTH,
														  DEFAULT_COMPOSITOR_HEIGHT,
														  DEFAULT_COMPOSITOR_FPS_NUMERATOR,
														  DEFAULT_COMPOSITOR_FPS_DENOMINATOR);
	
	if (krad_radio->krad_compositor == NULL) {
		krad_radio_shutdown (krad_radio);
		return NULL;
	}
	
	krad_compositor_set_krad_mixer (krad_radio->krad_compositor, krad_radio->krad_mixer);	
	
	krad_radio->krad_transponder = krad_transponder_create (krad_radio);
	
	if (krad_radio->krad_transponder == NULL) {
		krad_radio_shutdown (krad_radio);
		return NULL;
	}
	
	krad_radio->krad_osc = krad_osc_create ();
	
	if (krad_radio->krad_osc == NULL) {
		krad_radio_shutdown (krad_radio);
		return NULL;
	}	
	
	krad_radio->krad_ipc = krad_ipc_server_create ( "krad_radio", sysname, krad_radio_handler, krad_radio );
	
	if (krad_radio->krad_ipc == NULL) {
		krad_radio_shutdown (krad_radio);
		return NULL;
	}
	
	krad_ipc_server_register_broadcast ( krad_radio->krad_ipc, EBML_ID_KRAD_RADIO_GLOBAL_BROADCAST );
	
	krad_mixer_set_ipc (krad_radio->krad_mixer, krad_radio->krad_ipc);
	krad_tags_set_set_tag_callback (krad_radio->krad_tags, krad_radio->krad_ipc, 
									(void (*)(void *, char *, char *, char *, int))krad_ipc_server_broadcast_tag);
		
	return krad_radio;

}

void krad_radio_daemon (char *sysname) {

	pid_t pid;
	krad_radio_t *krad_radio_station;

	pid = fork();

	if (pid < 0) {
		exit (1);
	}

	if (pid > 0) {
		return;
	}

	krad_system_init ();

	krad_radio_station = krad_radio_create (sysname);

	if (krad_radio_station != NULL) {
		krad_radio_run ( krad_radio_station );
		krad_radio_shutdown (krad_radio_station);
	}
}

void krad_radio_cpu_monitor_callback (krad_radio_t *krad_radio_station, uint32_t usage) {

	//printk ("System CPU Usage: %d%%", usage);
	krad_ipc_server_simplest_broadcast ( krad_radio_station->krad_ipc,
										 EBML_ID_KRAD_RADIO_MSG,
										 EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE,
										 usage);

}

static void krad_radio_run (krad_radio_t *krad_radio_station) {

	int signal_caught;
	sigset_t signal_mask;

	krad_system_daemonize ();

    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGINT);
    sigaddset (&signal_mask, SIGTERM);
    sigaddset (&signal_mask, SIGHUP);
    if (pthread_sigmask (SIG_BLOCK, &signal_mask, NULL) != 0) {
		failfast ("Could not set signal mask!");
    }

  	krad_system_monitor_cpu_on ();
  	
	krad_system_set_monitor_cpu_callback ((void *)krad_radio_station, 
									 (void (*)(void *, uint32_t))krad_radio_cpu_monitor_callback);  	
  	
  clock_gettime (CLOCK_MONOTONIC, &krad_radio_station->start_sync_time);
  	
  krad_radio_station->start_sync_time = timespec_add_ms (krad_radio_station->start_sync_time, 100);
  	
	krad_compositor_start_ticker_at (krad_radio_station->krad_compositor, krad_radio_station->start_sync_time);
	krad_mixer_start_ticker_at (krad_radio_station->krad_mixer, krad_radio_station->start_sync_time);  	

	krad_ipc_server_run (krad_radio_station->krad_ipc);

  krad_radio_station->shutdown_timer = krad_timer_create();
			  	
	while (1) {

		if (sigwait (&signal_mask, &signal_caught) != 0) {
			failfast ("error on sigwait!");
		}
		switch (signal_caught) {
			case SIGHUP:
				printkd ("Got HANGUP Signal!");
				break;
			case SIGINT:
				printkd ("Got SIGINT Signal!");
			case SIGTERM:
				printkd ("Got SIGTERM Signal!");
			default:
			  krad_timer_start (krad_radio_station->shutdown_timer);
			  krad_system_monitor_cpu_off_fast ();			  
				printk ("Krad Radio Shutting down!");
				return;
		}
	}
}

krad_tags_t *krad_radio_find_tags_for_item ( krad_radio_t *krad_radio_station, char *item ) {

	krad_mixer_portgroup_t *portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_radio_station->krad_mixer, item);
	
	if (portgroup != NULL) {
		return portgroup->krad_tags;
	} else {
		return krad_transponder_get_tags_for_link (krad_radio_station->krad_transponder, item);
	}

	return NULL;

}

static int dir_exists (char *dir) {

  int err;
  struct stat s;

  err = stat (dir, &s);

  if (err == -1) {
    if(ENOENT == errno) {
      // does not exist
      return 0;
    } else {
      // another error
      return 0;
    }
  } else {
    if(S_ISDIR(s.st_mode)) {
      // it's a directory
      return 1;
    } else {
      // exists but is no dir
      return 0;
    }
  }

  return 0;
}

void krad_radio_set_dir ( krad_radio_t *krad_radio_station, char *dir ) {

  if ((dir == NULL) || (!dir_exists (dir))) {
    return;
  }

	if (krad_radio_station->dir != NULL) {
		free (krad_radio_station->dir);
	}
	
	krad_radio_station->dir = strdup (dir);

	if (krad_radio_station->krad_compositor != NULL) {
		krad_compositor_set_dir (krad_radio_station->krad_compositor, krad_radio_station->dir);
	}

	sprintf (krad_radio_station->logname, "%s/%s_%"PRIu64".log", dir, krad_radio_station->sysname, ktime ());
	verbose = 1;
	krad_system_log_on (krad_radio_station->logname);
	printk (APPVERSION);

}

