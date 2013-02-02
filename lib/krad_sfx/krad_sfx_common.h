#ifndef KRAD_SFX_COMMON_H
#define KRAD_SFX_COMMON_H

#ifndef LIMIT
#define LIMIT(v,l,u) ((v)<(l)?(l):((v)>(u)?(u):(v)))
#endif

#define KRAD_EFFECTS_MAX 4
#define KRAD_EFFECTS_MAX_CHANNELS 8

#define KRAD_EQ_MAX_BANDS 32

typedef enum {
  KRAD_NOFX,
  KRAD_EQ,
  KRAD_LOWPASS,
  KRAD_HIGHPASS,
  KRAD_ANALOG,
} kr_effect_type_t;

typedef struct krad_eq_rep_St krad_eq_rep_t;
typedef struct krad_eq_rep_St kr_eq_rep_t;

typedef struct krad_eq_band_rep_St krad_eq_band_rep_t;
typedef struct krad_eq_band_rep_St kr_eq_band_rep_t;

struct krad_eq_band_rep_St {
  float db;
  float bandwidth;
  float hz;
};

struct krad_eq_rep_St {
  kr_eq_band_rep_t band[KRAD_EQ_MAX_BANDS];
};

typedef struct krad_pass_rep_St krad_pass_rep_t;
typedef struct krad_pass_rep_St kr_highpass_rep_t;
typedef struct krad_pass_rep_St kr_lowpass_rep_t;

struct krad_pass_rep_St {
  float bandwidth;
  float hz;
};

typedef struct krad_analog_rep_St krad_analog_rep_t;
typedef struct krad_analog_rep_St kr_analog_rep_t;

struct krad_analog_rep_St {
  float drive;
  float blend;
};

#endif
