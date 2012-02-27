#include "sidechain_comp.h"

sc2_data_t *sc2_init(sc2_data_t *sc2_data) {

	if ((sc2_data = malloc (sizeof (sc2_data_t))) == NULL) {
		return NULL;
	}

    //  unsigned int i;
	sc2_data->sample_rate = 44100.0;

	sc2_data->rms = rms_env_new();
	sc2_data->sum = 0.0f;
	sc2_data->amp = 0.0f;
	sc2_data->gain = 0.0f;
	sc2_data->gain_t = 0.0f;
	sc2_data->env = 0.0f;
	sc2_data->count = 0;

	sc2_data->as = malloc(A_TBL * sizeof(float));
	sc2_data->as[0] = 1.0f;
	for (sc2_data->i=1; sc2_data->i<A_TBL; sc2_data->i++) {
		sc2_data->as[sc2_data->i] = expf(-1.0f / (	sc2_data->sample_rate * (float)	sc2_data->i / (float)A_TBL));
	}

//  2.00,300.00,-30.00,10.00,5.00,0.00

	sc2_data->attack = 2.0f;
	sc2_data->release = 300.0f;
	sc2_data->threshold = -30.0f;
	sc2_data->knee = 10.0f;
	sc2_data->ratio = 5.0f;
	sc2_data->makeup_gain = 0.0f;


	db_init();
      
	return sc2_data;
}


void sc2_run(sc2_data_t *sc2_data, float *input, float *sidechain, float *output, int sample_count) {

	  unsigned long pos;
			


      const float ga = sc2_data->as[f_round(sc2_data->attack * 0.001f * (float)(A_TBL-1))];
      const float gr = sc2_data->as[f_round(sc2_data->release * 0.001f * (float)(A_TBL-1))];
      const float rs = (sc2_data->ratio - 1.0f) / sc2_data->ratio;
      const float mug = db2lin(sc2_data->makeup_gain);
      const float knee_min = db2lin(sc2_data->threshold - sc2_data->knee);
      const float knee_max = db2lin(sc2_data->threshold + sc2_data->knee);
      const float ef_a = ga * 0.25f;
      const float ef_ai = 1.0f - ef_a;

      for (pos = 0; pos < sample_count; pos++) {
        sc2_data->sum += sidechain[pos] * sidechain[pos];

        if (sc2_data->amp > sc2_data->env) {
          sc2_data->env = sc2_data->env * ga + sc2_data->amp * (1.0f - ga);
        } else {
          sc2_data->env = sc2_data->env * gr + sc2_data->amp * (1.0f - gr);
        }
        if (sc2_data->count++ % 4 == 3) {
          sc2_data->amp = rms_env_process(sc2_data->rms, sc2_data->sum * 0.25f);
          sc2_data->sum = 0.0f;
          if (sc2_data->env <= knee_min) {
            sc2_data->gain_t = 1.0f;
	  } else if (sc2_data->env < knee_max) {
	    const float x = -(sc2_data->threshold - sc2_data->knee - lin2db(sc2_data->env)) / sc2_data->knee;
	    sc2_data->gain_t = db2lin(-sc2_data->knee * rs * x * x * 0.25f);
          } else {
            sc2_data->gain_t = db2lin((sc2_data->threshold - lin2db(sc2_data->env)) * rs);
          }
        }
        sc2_data->gain = sc2_data->gain * ef_a + sc2_data->gain_t * ef_ai;
        //buffer_write(output[pos], input[pos] * sc2_data->gain * mug);
		output[pos] = input[pos] * sc2_data->gain * mug;
      }
      /*
      sc2_data->sum = sum;
      sc2_data->amp = amp;
      sc2_data->gain = gain;
      sc2_data->gain_t = gain_t;
      sc2_data->env = env;
      sc2_data->count = count;
      */
}
