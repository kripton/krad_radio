#include "krad_hardlimiter.h"

void krad_hardlimit (float *samples, int count) {

  int s;

  for (s = 0; s < count; s++ ) {
    if (samples[s] < -1.0f) {
      samples[s] = -1.0f;
    } else if (samples[s] > 1.0f) {
      samples[s] = 1.0f;
    }
  }
}

