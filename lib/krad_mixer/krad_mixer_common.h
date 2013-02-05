#ifndef KRAD_MIXER_COMMON_H
#define KRAD_MIXER_COMMON_H

typedef struct krad_mixer_rep_St krad_mixer_rep_t;
typedef struct krad_mixer_rep_St kr_mixer_t;
typedef struct krad_mixer_portgroup_rep_St kr_portgroup_t;
typedef struct krad_mixer_portgroup_rep_St kr_mixer_portgroup_t;
typedef struct krad_mixer_portgroup_rep_St krad_mixer_portgroup_rep_t;

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
  KR_VOLUME = 1,
  KR_CROSSFADE,
} kr_mixer_portgroup_control_t;

typedef enum {
  DB = 1,
  BANDWIDTH,
  HZ,  
  TYPE,
  DRIVE,
  BLEND,
} kr_mixer_effect_control_t;

typedef enum {
	KRAD_TONE = 1,
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

  char sysname[64];
  channels_t channels;
  int io_type;
  
  char mixbus[64];
  char crossfade_group[64];
  float fade;
  
  float volume[KRAD_MIXER_MAX_CHANNELS];
  
  int map[KRAD_MIXER_MAX_CHANNELS];
  int mixmap[KRAD_MIXER_MAX_CHANNELS];	
  
  float rms[KRAD_MIXER_MAX_CHANNELS];
	float peak[KRAD_MIXER_MAX_CHANNELS];
  int delay;

  kr_eq_rep_t eq;
  kr_lowpass_rep_t lowpass;
  kr_highpass_rep_t highpass;
  kr_analog_rep_t analog;

  int has_xmms2;
  char xmms2_ipc_path[128];

};

struct krad_mixer_rep_St {
  uint32_t sample_rate;
//  uint64_t frames;
//  uint64_t timecode;
};

struct krad_effects_rep_St {
  kr_effect_type_t effect_typ;
  void *effect[KRAD_EFFECTS_MAX_CHANNELS];
};

char *krad_mixer_channel_number_to_string (int channel);
char *effect_control_to_string (kr_mixer_effect_control_t effect_control);

void krad_mixer_portgroup_rep_to_ebml (krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep, krad_ebml_t *krad_ebml);

kr_portgroup_t *kr_portgroup_rep_create ();
void kr_portgroup_rep_destroy (kr_portgroup_t *portgroup_rep);
kr_mixer_t *kr_mixer_rep_create ();
void kr_mixer_rep_destroy (kr_mixer_t *mixer_rep);

#endif
