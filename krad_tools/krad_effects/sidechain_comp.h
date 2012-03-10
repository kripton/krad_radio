#include <malloc.h>
#include <math.h>
#include "util/db.h"
#include "util/rms.h"

#define A_TBL 256

typedef struct {

	unsigned int i;
	float sample_rate;

	void *rms;
	float sum;
	float amp;
	float gain;
	float gain_t;
	float env;
	int count;
	float *as;
	
	
	float attack;
	float release;
	float threshold;
	float knee;
	float ratio;
	float makeup_gain;

} sc2_data_t;


sc2_data_t *sc2_init(sc2_data_t *sc2_data);
void sc2_run(sc2_data_t *sc2_data, float *input, float *sidechain, float *output, int sample_count);
void sc2_destroy(sc2_data_t *sc2_data);
