#include "krad_radio.h"
#include "krad_radio_interface.h"
#include "krad_radio_internal.h"

static void krad_radio_shutdown (krad_radio_t *krad_radio);
static krad_radio_t *krad_radio_create (char *sysname);
static void krad_radio_run (krad_radio_t *krad_radio);
static void krad_radio_shutdown_status (krad_timer_t *shutdown_timer);

static void krad_radio_shutdown_status (krad_timer_t *shutdown_timer) {
  printk ("Krad Radio shutdown timer at %"PRIu64"ms",
          krad_timer_sample_duration_ms (shutdown_timer));
}

static void krad_radio_shutdown (krad_radio_t *krad_radio) {

  krad_timer_t *shutdown_timer;

  shutdown_timer = krad_timer_create ();
  krad_timer_start (shutdown_timer);

  krad_system_monitor_cpu_off ();

  krad_radio_shutdown_status (shutdown_timer);
  
  if (krad_radio->remote.krad_ipc != NULL) {
    krad_ipc_server_disable (krad_radio->remote.krad_ipc);
  }
  krad_radio_shutdown_status (shutdown_timer);
  if (krad_radio->remote.krad_osc != NULL) {
    krad_osc_destroy (krad_radio->remote.krad_osc);
    krad_radio->remote.krad_osc = NULL;
  }
  krad_radio_shutdown_status (shutdown_timer);
  if (krad_radio->remote.krad_http != NULL) {
    krad_http_server_destroy (krad_radio->remote.krad_http);
    krad_radio->remote.krad_http = NULL;
  }
  krad_radio_shutdown_status (shutdown_timer);
  if (krad_radio->remote.krad_websocket != NULL) {
    krad_websocket_server_destroy (krad_radio->remote.krad_websocket);
    krad_radio->remote.krad_websocket = NULL;
  }
  krad_radio_shutdown_status (shutdown_timer);
  if (krad_radio->krad_transponder != NULL) {
    krad_transponder_destroy (krad_radio->krad_transponder);
    krad_radio->krad_transponder = NULL;    
  }
  krad_radio_shutdown_status (shutdown_timer);
  if (krad_radio->krad_mixer != NULL) {
    krad_mixer_destroy (krad_radio->krad_mixer);
    krad_radio->krad_mixer = NULL;    
  }
  krad_radio_shutdown_status (shutdown_timer);
  if (krad_radio->krad_compositor != NULL) {
    krad_compositor_destroy (krad_radio->krad_compositor);
    krad_radio->krad_compositor = NULL;
  }
  krad_radio_shutdown_status (shutdown_timer);
  if (krad_radio->krad_tags != NULL) {
    krad_tags_destroy (krad_radio->krad_tags);
    krad_radio->krad_tags = NULL;
  }  
  krad_radio_shutdown_status (shutdown_timer);
  if (krad_radio->remote.krad_ipc != NULL) {
    krad_ipc_server_destroy (krad_radio->remote.krad_ipc);
    krad_radio->remote.krad_ipc = NULL;
  }
  krad_radio_shutdown_status (shutdown_timer);
  if (shutdown_timer != NULL) {
    krad_timer_finish (shutdown_timer);  
    printk ("Krad Radio took %"PRIu64"ms to shutdown",
            krad_timer_duration_ms (shutdown_timer));
    krad_timer_destroy (shutdown_timer);
    shutdown_timer = NULL;
  }

  free (krad_radio);

  printk ("Krad Radio exited cleanly");
  krad_system_log_off ();

}

static krad_radio_t *krad_radio_create (char *sysname) {

  krad_radio_t *krad_radio = calloc (1, sizeof(krad_radio_t));

  if (!krad_valid_sysname(sysname)) {
    return NULL;
  }
  
  strncpy (krad_radio->sysname, strdup (sysname), sizeof(krad_radio->sysname));
  
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
  
  krad_radio->remote.krad_osc = krad_osc_create ();
  
  if (krad_radio->remote.krad_osc == NULL) {
    krad_radio_shutdown (krad_radio);
    return NULL;
  }  
  
  krad_radio->remote.krad_ipc = krad_ipc_server_create ( "krad_radio", sysname, krad_radio_handler, krad_radio );
  
  if (krad_radio->remote.krad_ipc == NULL) {
    krad_radio_shutdown (krad_radio);
    return NULL;
  }
  
  krad_ipc_server_register_broadcast ( krad_radio->remote.krad_ipc, EBML_ID_KRAD_RADIO_GLOBAL_BROADCAST );
  
  krad_mixer_set_ipc (krad_radio->krad_mixer, krad_radio->remote.krad_ipc);
  krad_tags_set_set_tag_callback (krad_radio->krad_tags, krad_radio->remote.krad_ipc, 
                  (void (*)(void *, char *, char *, char *, int))krad_ipc_server_broadcast_tag);
    
  return krad_radio;

}

void krad_radio_daemon (char *sysname) {

  pid_t pid;
  krad_radio_t *krad_radio;

  pid = fork();

  if (pid < 0) {
    exit (1);
  }

  if (pid > 0) {
    return;
  }

  krad_system_init ();

  krad_radio = krad_radio_create (sysname);

  if (krad_radio != NULL) {
    krad_radio_run ( krad_radio );
    krad_radio_shutdown (krad_radio);
  }
}

void krad_radio_cpu_monitor_callback (krad_radio_t *krad_radio, uint32_t usage) {

  //printk ("System CPU Usage: %d%%", usage);
  krad_ipc_server_simplest_broadcast ( krad_radio->remote.krad_ipc,
                     EBML_ID_KRAD_RADIO_MSG,
                     EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE,
                     usage);

}

static void krad_radio_run (krad_radio_t *krad_radio) {

  int signal_caught;
  sigset_t signal_mask;
	struct timespec start_sync;

  krad_system_daemonize ();

  sigemptyset (&signal_mask);
  sigaddset (&signal_mask, SIGINT);
  sigaddset (&signal_mask, SIGTERM);
  sigaddset (&signal_mask, SIGHUP);
  if (pthread_sigmask (SIG_BLOCK, &signal_mask, NULL) != 0) {
    failfast ("Could not set signal mask!");
  }

  krad_system_monitor_cpu_on ();
    
  krad_system_set_monitor_cpu_callback ((void *)krad_radio, 
                   (void (*)(void *, uint32_t))krad_radio_cpu_monitor_callback);    
    
  clock_gettime (CLOCK_MONOTONIC, &start_sync);
  start_sync = timespec_add_ms (start_sync, 100);
    
  krad_compositor_start_ticker_at (krad_radio->krad_compositor, start_sync);
  krad_mixer_start_ticker_at (krad_radio->krad_mixer, start_sync);    

  krad_ipc_server_run (krad_radio->remote.krad_ipc);

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
        printk ("Krad Radio Shutting down!");
        return;
    }
  }
}

krad_tags_t *krad_radio_find_tags_for_item ( krad_radio_t *krad_radio, char *item ) {

  krad_mixer_portgroup_t *portgroup;

  portgroup = krad_mixer_get_portgroup_from_sysname (krad_radio->krad_mixer, item);
  
  if (portgroup != NULL) {
    return portgroup->krad_tags;
  } else {
    return krad_transponder_get_tags_for_link (krad_radio->krad_transponder, item);
  }

  return NULL;

}

void krad_radio_set_dir ( krad_radio_t *krad_radio, char *dir ) {

  if ((dir == NULL) || (!dir_exists (dir))) {
    return;
  }

  if (krad_radio->krad_compositor != NULL) {
    krad_compositor_set_dir (krad_radio->krad_compositor, dir);
  }

  sprintf (krad_radio->log.filename, "%s/%s_%"PRIu64".log", dir, krad_radio->sysname, ktime ());
  verbose = 1;
  krad_system_log_on (krad_radio->log.filename);
  printk (APPVERSION);
  printk ("Station: %s", krad_radio->sysname);
  printk ("Time: %"PRIu64"", ktime ());
}

