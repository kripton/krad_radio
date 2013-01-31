#ifndef KRAD_SFX_COMMON_H
#define KRAD_SFX_COMMON_H

#ifndef LIMIT
#define LIMIT(v,l,u) ((v)<(l)?(l):((v)>(u)?(u):(v)))
#endif

#define KRAD_EFFECTS_MAX 8
#define KRAD_EFFECTS_MAX_CHANNELS 8

typedef enum {
  KRAD_NOFX,
  KRAD_EQ,
  KRAD_PASS,
  KRAD_TAPETUBE,
//  KRAD_RUSHLIMIT,
} kr_effect_type_t;


#endif
