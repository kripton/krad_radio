#include "kradtone.h"

kradtone_t *kradtone_create(float sample_rate) {

	kradtone_t *kradtone = calloc(1, sizeof(kradtone_t));

	kradtone->sample_rate = sample_rate;

	kradtone_volume(kradtone, KRADTONE_DEFAULT_VOLUME);

	return kradtone;

}

void kradtone_volume(kradtone_t *kradtone, int volume) {

	kradtone->volume = volume;
	kradtone->volume_actual = (float)(kradtone->volume/100.0f);
	kradtone->volume_actual *= kradtone->volume_actual;

}

void kradtone_add_preset(kradtone_t *kradtone, char *preset) {

	if ((strstr(preset, "dialtone") != NULL) || (strstr(preset, "dialtone_us") != NULL)){
		kradtone_add(kradtone, 350.0);
		kradtone_add(kradtone, 440.0);
	}
	
	if (strstr(preset, "dialtone_eu") != NULL) {
		kradtone_add(kradtone, 425.0);
	}
	
	if (strstr(preset, "dialtone_uk") != NULL) {
		kradtone_add(kradtone, 350.0);
		kradtone_add(kradtone, 450.0);
	}
	
	if (strstr(preset, "1") != NULL) {
		kradtone_add(kradtone, 697.0);
		kradtone_add(kradtone, 1209.0);
	}
	
	if (strstr(preset, "2") != NULL) {
		kradtone_add(kradtone, 697.0);
		kradtone_add(kradtone, 1336.0);		
	}

	if (strstr(preset, "3") != NULL) {
		kradtone_add(kradtone, 697.0);
		kradtone_add(kradtone, 1477.0);
	}

	if (strstr(preset, "4") != NULL) {
		kradtone_add(kradtone, 770.0);
		kradtone_add(kradtone, 1209.0);
	}
	
	if (strstr(preset, "5") != NULL) {
		kradtone_add(kradtone, 770.0);
		kradtone_add(kradtone, 1336.0);
	}

	if (strstr(preset, "6") != NULL) {
		kradtone_add(kradtone, 770.0);
		kradtone_add(kradtone, 1477.0);
	}

	if (strstr(preset, "7") != NULL) {
		kradtone_add(kradtone, 852.0);
		kradtone_add(kradtone, 1209.0);
	}
	
	if (strstr(preset, "8") != NULL) {
		kradtone_add(kradtone, 852.0);
		kradtone_add(kradtone, 1336.0);
	}

	if (strstr(preset, "9") != NULL) {
		kradtone_add(kradtone, 852.0);
		kradtone_add(kradtone, 1477.0);
	}
	
	if (strstr(preset, "0") != NULL) {
		kradtone_add(kradtone, 941.0);
		kradtone_add(kradtone, 1336.0);
	}
	
	if (strstr(preset, "*") != NULL) {
		kradtone_add(kradtone, 941.0);
		kradtone_add(kradtone, 1209.0);
	}

	if (strstr(preset, "#") != NULL) {
		kradtone_add(kradtone, 941.0);
		kradtone_add(kradtone, 1477.0);
	}
	
	if (strstr(preset, "A") != NULL) {
		kradtone_add(kradtone, 697.0);
		kradtone_add(kradtone, 1633.0);
	}
	
	if (strstr(preset, "B") != NULL) {
		kradtone_add(kradtone, 770.0);
		kradtone_add(kradtone, 1633.0);
	}

	if (strstr(preset, "C") != NULL) {
		kradtone_add(kradtone, 852.0);
		kradtone_add(kradtone, 1633.0);
	}
	
	if (strstr(preset, "D") != NULL) {
		kradtone_add(kradtone, 941.0);
		kradtone_add(kradtone, 1633.0);
	}
}

void kradtone_add(kradtone_t *kradtone, float frequency) {

	int i;
	
	kradtone->active_tones++;
	
	for (i = 0; i < MAX_TONES; i++) {
		if (kradtone->tones[i].active == 0) {
			kradtone->tones[i].frequency = frequency;
			kradtone->tones[i].delta = (2.0f * M_PI * kradtone->tones[i].frequency) / kradtone->sample_rate;
			kradtone->tones[i].angle = 0.0f;
			kradtone->tones[i].active = 1;
			break;
		}
	}
}	

void kradtone_remove(kradtone_t *kradtone, float frequency) {

	int i;
	
	kradtone->active_tones--;
	
	for (i = 0; i < MAX_TONES; i++) {
		if (kradtone->tones[i].active == 1) {
			if (kradtone->tones[i].frequency == frequency) {
				kradtone->tones[i].active = 0;
				break;
			}
		}
	}
}

void kradtone_clear(kradtone_t *kradtone) {

	int i;
	
	for (i = 0; i < MAX_TONES; i++) {
		if (kradtone->tones[i].active == 1) {
			kradtone_remove(kradtone, kradtone->tones[i].frequency);
		}
	}
}

void kradtone_run(kradtone_t *kradtone, float *buffer, int numsamples) {

	int i;

	for(kradtone->s = 0; kradtone->s < numsamples; kradtone->s++) {

		buffer[kradtone->s] = 0.0f;

		for (i = 0; i < kradtone->active_tones; i++) {

			buffer[kradtone->s] += kradtone->volume_actual * sin(kradtone->tones[i].angle);

			kradtone->tones[i].angle += kradtone->tones[i].delta;

			if (kradtone->tones[i].angle > M_PI) {
				kradtone->tones[i].angle -= 2.0f * M_PI;
			}
		}
	}


}


void kradtone_destroy(kradtone_t *kradtone) {

	free(kradtone);
	
}
