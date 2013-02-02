#ifndef KRAD_SFX_COMMON_H
#define KRAD_SFX_COMMON_H

#ifndef LIMIT
#define LIMIT(v,l,u) ((v)<(l)?(l):((v)>(u)?(u):(v)))
#endif

#define KRAD_EFFECTS_MAX 4
#define KRAD_EFFECTS_MAX_CHANNELS 8

typedef enum {
  KRAD_NOFX,
  KRAD_EQ,
  KRAD_LOWPASS,
  KRAD_HIGHPASS,
  KRAD_ANALOG,
} kr_effect_type_t;


#endif
