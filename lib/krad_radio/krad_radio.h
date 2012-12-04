#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>

#ifndef KRAD_RADIO_H
#define KRAD_RADIO_H

typedef struct krad_radio_St krad_radio_t;

// FIXME option builds
// #include "krad_wayland.h"
// #include "krad_codec2.h"

#include "krad_radio_version.h"
#include "krad_system.h"
#include "krad_codec_header.h"
#include "krad_y4m.h"
#include "krad_x264.h"
#include "krad_xmms2.h"
#include "krad_timer.h"
#include "krad_easing.h"
#include "krad_ticker.h"
#include "krad_tags.h"
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
#include "krad_ebml.h"
#include "krad_ogg.h"
#include "krad_io.h"
#include "krad_theora.h"
#include "krad_vpx.h"
#include "krad_gui.h"
#include "krad_opus.h"
#include "krad_vorbis.h"
#include "krad_flac.h"
#include "krad_container.h"
#include "krad_framepool.h"
#include "krad_decklink.h"
#include "krad_compositor_subunit.h"
#include "krad_sprite.h"
#include "krad_text.h"
#include "krad_compositor.h"
#include "krad_link_common.h"
#include "krad_link.h"

#ifdef KRAD_USE_WAYLAND
#include "krad_wayland.h"
#endif

extern int verbose;
extern int do_shutdown;
extern krad_system_t krad_system;

struct krad_radio_St {

	char *sysname;
	char *dir;
	char logname[1024];	
	
	krad_ipc_server_t *krad_ipc;
	krad_osc_t *krad_osc;	
	krad_websocket_t *krad_websocket;
	krad_http_t *krad_http;
	krad_linker_t *krad_linker;	
	krad_mixer_t *krad_mixer;
	krad_compositor_t *krad_compositor;
	krad_tags_t *krad_tags;

	struct timespec start_sync_time;

	krad_timer_t *shutdown_timer;

};

void krad_radio_daemon (char *sysname);

#endif
