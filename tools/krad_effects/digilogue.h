#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//#include "ladspa.h"
//#include "util/tap_utils.h"
#define LIMIT(v,l,u) ((v)<(l)?(l):((v)>(u)?(u):(v)))
#define EPS 0.000000001f

typedef struct {
	float drive;
	float blend;
	float * input;
	float * output;

	float prev_med;
	float prev_out;

	float rdrive;
	float rbdr;
	float kpa;
	float kpb;
	float kna;
	float knb;
	float ap;
	float an;
	float imr;
	float kc;
	float srct;
	float sq;
	float pwrq;

	float prev_drive;
	float prev_blend;
	
	float med;
	float in;
	float out;
	
	unsigned long sample_index;
	unsigned long sample_rate;
	//float run_adding_gain;
} digilogue_t;


digilogue_t *digilogue_create(digilogue_t *digilogue);
void digilogue_run(digilogue_t *digilogue, float *input, float *output, int sample_count);
void digilogue_destroy(digilogue_t *digilogue);
