#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>

#ifndef KRAD_RADIO
#define KRAD_RADIO 1
#endif

#ifndef KRAD_RADIO_H
#define KRAD_RADIO_H

typedef struct krad_radio_St krad_radio_t;
typedef struct krad_log_St krad_log_t;
typedef struct krad_remote_control_St krad_remote_control_t;

#include "krad_radio_version.h"
#include "krad_system.h"
#include "krad_ebml.h"
///#include "krad_radio_interface.h"
#include "krad_ogg.h"
#include "krad_io.h"
#include "krad_codec_header.h"
#include "krad_y4m.h"
#include "krad_xmms2.h"
#include "krad_timer.h"
#include "krad_easing.h"
#include "krad_ticker.h"
#include "krad_tags.h"
#include "krad_container.h"
#include "krad_ebml.h"
#include "krad_ipc_server.h"
#include "krad_radio_ipc.h"
#include "krad_transmitter.h"
#include "krad_osc.h"
#include "krad_ring.h"
#include "krad_resample_ring.h"
#include "krad_tone.h"
#include "krad_audio.h"
#include "krad_jack.h"
#include "krad_vhs.h"
#ifndef __MACH__
#include "krad_v4l2.h"
#include "krad_alsa.h"
#endif
#include "krad_mixer.h"
#include "krad_mixer_common.h"
#include "krad_websocket.h"
#include "krad_http.h"
#include "krad_udp.h"
#include "krad_x11.h"
#include "krad_theora.h"
#include "krad_vpx.h"
#include "krad_opus.h"
#include "krad_vorbis.h"
#include "krad_flac.h"
#include "krad_framepool.h"
#include "krad_decklink.h"
#include "krad_compositor_subunit.h"
#include "krad_sprite.h"
#include "krad_text.h"
#include "krad_compositor.h"
#include "krad_transponder_common.h"
#include "krad_transponder_graph.h"
#include "krad_transponder.h"
#include "krad_receiver.h"

#ifdef KRAD_USE_WAYLAND
#include "krad_wayland.h"
#endif

extern int verbose;
extern krad_system_t krad_system;

struct krad_remote_control_St {
	krad_ipc_server_t *krad_ipc;
	krad_osc_t *krad_osc;	
	krad_websocket_t *krad_websocket;
	krad_http_t *krad_http;
};

struct krad_log_St {
  krad_timer_t *startup_timer;
	char filename[512];
};

struct krad_radio_St {

	char sysname[KRAD_SYSNAME_MAX + 1];
	krad_log_t log;
	krad_tags_t *krad_tags;
	krad_remote_control_t remote;
	krad_transponder_t *krad_transponder;	
	krad_mixer_t *krad_mixer;
	krad_compositor_t *krad_compositor;

};

void krad_radio_daemon (char *sysname);

#endif
