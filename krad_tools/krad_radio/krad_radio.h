#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>

typedef struct krad_radio_St krad_radio_t;

#ifndef KRAD_RADIO_H
#define KRAD_RADIO_H
#include "krad_tags.h"
#include "krad_ipc_server.h"
#include "krad_radio_ipc.h"
#include "krad_ring.h"
#include "krad_audio.h"
#include "krad_jack.h"
#include "krad_alsa.h"
#include "krad_pulse.h"
#include "krad_mixer.h"
#include "krad_websocket.h"
#include "krad_http.h"
#include "krad_udp.h"
#include "krad_x11.h"
#include "krad_ebml.h"
#include "krad_ogg.h"
#include "krad_io.h"
#include "krad_dirac.h"
#include "krad_theora.h"
#include "krad_vpx.h"
#include "krad_v4l2.h"
#include "krad_gui.h"
#include "krad_codec2.h"
#include "krad_opus.h"
#include "krad_vorbis.h"
#include "krad_flac.h"
#include "krad_container.h"
#include "krad_decklink.h"
#include "krad_link.h"
#include "krad_compositor.h"

struct krad_radio_St {

	char *sysname;
	
	krad_ipc_server_t *krad_ipc;
	krad_websocket_t *krad_websocket;
	krad_http_t *krad_http;
	krad_linker_t *krad_linker;
	krad_mixer_t *krad_mixer;
	krad_compositor_t *krad_compositor;
	krad_tags_t *krad_tags;

};



void krad_radio_destroy (krad_radio_t *krad_radio);
krad_radio_t *krad_radio_create (char *sysname);
void krad_radio_daemonize ();
void krad_radio_run (krad_radio_t *krad_radio);
void krad_radio (char *sysname);

int krad_radio_handler ( void *output, int *output_len, void *ptr );

#endif
