#include "hardlimiter.h"

void hardlimit (float *samples, int count) {

	int i;

	for (i = 0; i < count; i++ ) {
		if (samples[i] < -1.0f) {
			samples[i] = -1.0f;
		} else if (samples[i] > 1.0f) {
			samples[i] = 1.0f;
		}
	}
	

}
