#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#include "krad_system.h"

typedef struct {

  float gain;
  int hold;
      
} kr_rushlimiter_t;


kr_rushlimiter_t *kr_rushlimiter_create ();
void kr_rushlimiter_destroy (kr_rushlimiter_t *kr_rushlimiter);

void kr_rushlimiter_process (kr_rushlimiter_t *kr_rushlimiter, float *input, float *output, int num_samples);


