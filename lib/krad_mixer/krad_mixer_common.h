#ifndef KRAD_MIXER_COMMON_H
#define KRAD_MIXER_COMMON_H

typedef struct krad_mixer_rep_St krad_mixer_rep_t;
typedef struct krad_mixer_portgroup_rep_St kr_mixer_portgroup_t;
typedef struct krad_mixer_portgroup_rep_St krad_mixer_portgroup_rep_t;
typedef struct krad_mixer_portgroup_rep_St krad_mixer_mixbus_rep_t;
typedef struct krad_mixer_crossfade_group_rep_St krad_mixer_crossfade_group_rep_t;
typedef struct kr_mixer_portgroup_control_rep_St kr_mixer_portgroup_control_rep_t;

#define KRAD_MIXER_MAX_PORTGROUPS 12
#define KRAD_MIXER_MAX_CHANNELS 8
#define KRAD_MIXER_DEFAULT_SAMPLE_RATE 48000
#define KRAD_MIXER_DEFAULT_TICKER_PERIOD 1024
#define DEFAULT_MASTERBUS_LEVEL 75.0f
#define KRAD_MIXER_RMS_WINDOW_SIZE_MS 125

#include "krad_sfx_common.h"
#include "krad_ebml.h"
#include "krad_radio_ipc.h"

typedef enum {
	KRAD_TONE,
	KLOCALSHM,
	KRAD_AUDIO, /* i.e local audio i/o */
	KRAD_LINK, /* i.e. remote audio i/o */
	MIXBUS,	/* i.e. mixer internal i/o */
} krad_mixer_portgroup_io_t;

typedef enum {
  NOTOUTPUT,
	DIRECT,
	AUX,
} krad_mixer_output_t;

typedef enum {
	NIL,
	MONO,
	STEREO,
	THREE,
	QUAD,
	FIVE,
	SIX,
	SEVEN,
	EIGHT,
} channels_t;

typedef enum {
	OUTPUT,
	INPUT,
	MIX,
} krad_mixer_portgroup_direction_t;

struct krad_mixer_portgroup_rep_St {

  char sysname[256];
  channels_t channels;
  int io_type;
  
  krad_mixer_mixbus_rep_t *mixbus_rep;
  krad_mixer_crossfade_group_rep_t *crossfade_group_rep;
  
  float volume[KRAD_MIXER_MAX_CHANNELS];
  
  int map[KRAD_MIXER_MAX_CHANNELS];
  int mixmap[KRAD_MIXER_MAX_CHANNELS];	
  
  float rms[KRAD_MIXER_MAX_CHANNELS];
	float peak[KRAD_MIXER_MAX_CHANNELS];
  int delay;

  int has_xmms2;

};

struct krad_mixer_rep_St {
  int sample_rate;
  
  uint64_t frames;
  uint64_t timecode;
  
  krad_mixer_portgroup_rep_t *portgroup_rep[KRAD_MIXER_MAX_PORTGROUPS];
  krad_mixer_crossfade_group_rep_t *crossfade_group;
  
};

struct krad_mixer_crossfade_group_rep_St {
  krad_mixer_portgroup_rep_t *portgroup_rep[2];
  float fade;
};

struct krad_effects_rep_St {
  kr_effect_type_t effect_typ;
  void *effect[KRAD_EFFECTS_MAX_CHANNELS];
};

struct kr_mixer_portgroup_control_rep_St {
  char name[64];
  char control[64];
  float value;
};

void kr_mixer_portgroup_control_rep_destroy (kr_mixer_portgroup_control_rep_t *kr_mixer_portgroup_control_rep);
kr_mixer_portgroup_control_rep_t *kr_mixer_portgroup_control_rep_create ();

krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep_create ();
void krad_mixer_portgroup_rep_destroy (krad_mixer_portgroup_rep_t *portgroup_rep);
void krad_mixer_portgroup_rep_to_ebml (krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep, krad_ebml_t *krad_ebml);
krad_mixer_portgroup_rep_t *krad_mixer_ebml_to_krad_mixer_portgroup_rep (krad_ebml_t *krad_ebml, uint64_t *ebml_data_size, krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep);

krad_mixer_mixbus_rep_t *krad_mixer_mixbus_rep_create ();
void krad_mixer_mixbus_rep_destroy (krad_mixer_mixbus_rep_t *mixbus_rep);

krad_mixer_crossfade_group_rep_t *krad_mixer_crossfade_rep_create (krad_mixer_portgroup_rep_t *portgroup_rep1, krad_mixer_portgroup_rep_t *portgroup_rep2);
void krad_mixer_crossfade_group_rep_destroy (krad_mixer_crossfade_group_rep_t *crossfade_group_rep);

krad_mixer_rep_t *krad_mixer_rep_create ();
void krad_mixer_rep_destroy (krad_mixer_rep_t *mixer_rep);

#endif
