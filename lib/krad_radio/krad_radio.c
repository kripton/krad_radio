#include "krad_radio.h"
#include "krad_radio_interface.h"
#include "krad_radio_internal.h"

static void krad_radio_shutdown (krad_radio_t *krad_radio);
static krad_radio_t *krad_radio_create (char *sysname);
static void krad_radio_start (krad_radio_t *krad_radio);
static void krad_radio_wait (krad_radio_t *krad_radio);

static void krad_radio_shutdown (krad_radio_t *krad_radio) {

  krad_timer_t *shutdown_timer;

  shutdown_timer = krad_timer_create_with_name ("shutdown");
  krad_timer_start (shutdown_timer);

  krad_system_monitor_cpu_off ();

  krad_timer_status (shutdown_timer);
  
  if (krad_radio->system_broadcaster != NULL) {
    krad_ipc_server_broadcaster_unregister ( &krad_radio->system_broadcaster );
  }
  
  if (krad_radio->remote.krad_ipc != NULL) {
    krad_ipc_server_disable (krad_radio->remote.krad_ipc);
  }
  krad_timer_status (shutdown_timer);
  if (krad_radio->remote.krad_osc != NULL) {
    krad_osc_destroy (krad_radio->remote.krad_osc);
    krad_radio->remote.krad_osc = NULL;
  }
  krad_timer_status (shutdown_timer);
  if (krad_radio->remote.krad_http != NULL) {
    krad_http_server_destroy (krad_radio->remote.krad_http);
    krad_radio->remote.krad_http = NULL;
  }
  krad_timer_status (shutdown_timer);
  if (krad_radio->remote.krad_websocket != NULL) {
    krad_websocket_server_destroy (krad_radio->remote.krad_websocket);
    krad_radio->remote.krad_websocket = NULL;
  }
  krad_timer_status (shutdown_timer);
  if (krad_radio->krad_transponder != NULL) {
    krad_transponder_destroy (krad_radio->krad_transponder);
    krad_radio->krad_transponder = NULL;    
  }
  krad_timer_status (shutdown_timer);
  if (krad_radio->krad_mixer != NULL) {
    krad_mixer_destroy (krad_radio->krad_mixer);
    krad_radio->krad_mixer = NULL;    
  }
  krad_timer_status (shutdown_timer);
  if (krad_radio->krad_compositor != NULL) {
    krad_compositor_destroy (krad_radio->krad_compositor);
    krad_radio->krad_compositor = NULL;
  }
  krad_timer_status (shutdown_timer);
  if (krad_radio->krad_tags != NULL) {
    krad_tags_destroy (krad_radio->krad_tags);
    krad_radio->krad_tags = NULL;
  }  

  if (krad_radio->remote.krad_ipc != NULL) {
    krad_ipc_server_destroy (krad_radio->remote.krad_ipc);
    krad_radio->remote.krad_ipc = NULL;
  }
  
  if (krad_radio->log.startup_timer != NULL) {
    krad_timer_destroy (krad_radio->log.startup_timer);
    krad_radio->log.startup_timer = NULL;
  }

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
  
  krad_radio->log.startup_timer = krad_timer_create_with_name ("startup");
  krad_timer_start (krad_radio->log.startup_timer);  
  
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
  
  krad_radio->system_broadcaster = krad_ipc_server_broadcaster_register ( krad_radio->remote.krad_ipc );
  
  if (krad_radio->system_broadcaster == NULL) {
    krad_radio_shutdown (krad_radio);
    return NULL;
  }  
  
  krad_ipc_server_broadcaster_register_broadcast ( krad_radio->system_broadcaster, EBML_ID_KRAD_SYSTEM_BROADCAST );

  krad_mixer_set_ipc (krad_radio->krad_mixer, krad_radio->remote.krad_ipc);
  //krad_tags_set_set_tag_callback (krad_radio->krad_tags, krad_radio->remote.krad_ipc, 
  //                (void (*)(void *, char *, char *, char *, int))krad_ipc_server_broadcast_tag);

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
  
  krad_system_daemonize ();

  krad_system_init ();

  krad_radio = krad_radio_create (sysname);

  if (krad_radio != NULL) {
    krad_radio_start ( krad_radio );
    krad_radio_wait ( krad_radio );
    krad_radio_shutdown ( krad_radio );
  }
}

void krad_radio_cpu_monitor_callback (krad_radio_t *krad_radio, uint32_t usage) {

  size_t size;
  unsigned char *buffer;
  krad_broadcast_msg_t *broadcast_msg;
  krad_ebml_t *krad_ebml;
  kr_address_t address;
  uint64_t message_loc;
  uint64_t payload_loc;
  
  size = 64;
  buffer = malloc (size);

  address.path.unit = KR_STATION;
  address.path.subunit.station_subunit = KR_CPU;

  krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_WRITEONLY);

  krad_radio_address_to_ebml (krad_ebml, &message_loc, &address);
  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_TYPE, EBML_ID_KRAD_SUBUNIT_INFO);
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_RADIO_MESSAGE_PAYLOAD, &payload_loc);
  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE, usage);
  krad_ebml_finish_element (krad_ebml, payload_loc);
  krad_ebml_finish_element (krad_ebml, message_loc);
  
  size = krad_ebml->io_adapter.write_buffer_pos;
  memcpy (buffer, krad_ebml->io_adapter.write_buffer, size);
  krad_ebml_destroy (krad_ebml);

  broadcast_msg = krad_broadcast_msg_create (buffer, size);

  free (buffer);

  if (broadcast_msg != NULL) {
    krad_ipc_server_broadcaster_broadcast ( krad_radio->system_broadcaster, &broadcast_msg );
  }
}

static void krad_radio_start (krad_radio_t *krad_radio) {

	struct timespec start_sync;

  krad_system_monitor_cpu_on ();
    
  krad_system_set_monitor_cpu_callback ((void *)krad_radio, 
                   (void (*)(void *, uint32_t))krad_radio_cpu_monitor_callback);
    
  clock_gettime (CLOCK_MONOTONIC, &start_sync);
  start_sync = timespec_add_ms (start_sync, 100);
    
  krad_compositor_start_ticker_at (krad_radio->krad_compositor, start_sync);
  krad_mixer_start_ticker_at (krad_radio->krad_mixer, start_sync);    

  krad_ipc_server_run (krad_radio->remote.krad_ipc);

  if (krad_radio->log.startup_timer != NULL) {
    krad_timer_finish (krad_radio->log.startup_timer);  
  }
}

static void krad_radio_wait (krad_radio_t *krad_radio) {
  krad_system_daemon_wait ();
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
  krad_system_log_on (krad_radio->log.filename);
  printk (APPVERSION);
  printk ("Station: %s", krad_radio->sysname);
  if (krad_radio->log.startup_timer != NULL) {
    printk ("Krad Radio took %"PRIu64"ms to startup",
            krad_timer_duration_ms (krad_radio->log.startup_timer));
  }
  printk ("Current Unix Time: %"PRIu64"", ktime ());
}

