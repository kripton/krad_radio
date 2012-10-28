#include "krad_eq.h"
#include "krad_pass.h"
#include "krad_rushlimiter.h"

#include "krad_system.h"

#define KRAD_EFFECTS_MAX 8
#define KRAD_EFFECTS_MAX_CHANNELS 8

typedef struct kr_effect_St kr_effect_t;
typedef struct kr_effects_St kr_effects_t;

typedef enum {
  KRAD_NOFX,
  KRAD_EQ,
  KRAD_PASS,
//  KRAD_RUSHLIMIT,
} kr_effect_type_t;


struct kr_effect_St {

	kr_effect_type_t effect_type;
	void *effect[KRAD_EFFECTS_MAX_CHANNELS];
  int active;

};

struct kr_effects_St {

  float sample_rate;
  int channels;

	kr_effect_t *effect;

};


kr_effects_t *kr_effects_create (int channels, int sample_rate);
void kr_effects_destroy (kr_effects_t *kr_effects);

void kr_effects_set_sample_rate (kr_effects_t *kr_effects, int sample_rate);
void kr_effects_process (kr_effects_t *kr_effects, float **input, float **output, int num_samples);

/* OpControls */
void kr_effects_effect_add (kr_effects_t *kr_effects, kr_effect_type_t effect);
void kr_effects_effect_remove (kr_effects_t *kr_effects, int effect_num);

/* Controls */
void kr_effects_effect_set_control (kr_effects_t *kr_effects, int effect_num, int control, int subunit, float value);

/* Utils */

kr_effect_type_t kr_effects_string_to_effect (char *string);
int kr_effects_string_to_effect_control (kr_effect_type_t effect_type, char *string);
