#include "krad_tone.h"

krad_tone_t *krad_tone_create (float sample_rate) {

	krad_tone_t *krad_tone = calloc (1, sizeof(krad_tone_t));

	krad_tone->sample_rate = sample_rate;

	krad_tone_volume (krad_tone, KRAD_TONE_DEFAULT_VOLUME);

	return krad_tone;

}

void krad_tone_set_sample_rate (krad_tone_t *krad_tone, float sample_rate) {
	krad_tone->sample_rate = sample_rate;
}

void krad_tone_volume (krad_tone_t *krad_tone, int volume) {

	krad_tone->volume = volume;
	krad_tone->volume_actual = (float)(krad_tone->volume/100.0f);
	krad_tone->volume_actual *= krad_tone->volume_actual;

}

void krad_tone_add_preset (krad_tone_t *krad_tone, char *preset) {

	krad_tone_clear (krad_tone);

	if (strstr(preset, "dialtone_eu") != NULL) {
		krad_tone_add (krad_tone, 425.0);
		return;		
	}
	
	if (strstr(preset, "dialtone_uk") != NULL) {
		krad_tone_add (krad_tone, 350.0);
		krad_tone_add (krad_tone, 450.0);
		return;		
	}

	if ((strstr(preset, "dialtone") != NULL) || (strstr(preset, "dialtone_us") != NULL)){
		krad_tone_add (krad_tone, 350.0);
		krad_tone_add (krad_tone, 440.0);
		return;		
	}
	
	if (strstr(preset, "1") != NULL) {
		krad_tone_add (krad_tone, 697.0);
		krad_tone_add (krad_tone, 1209.0);
		return;		
	}
	
	if (strstr(preset, "2") != NULL) {
		krad_tone_add (krad_tone, 697.0);
		krad_tone_add (krad_tone, 1336.0);		
		return;		
	}

	if (strstr(preset, "3") != NULL) {
		krad_tone_add (krad_tone, 697.0);
		krad_tone_add (krad_tone, 1477.0);
		return;		
	}

	if (strstr(preset, "4") != NULL) {
		krad_tone_add (krad_tone, 770.0);
		krad_tone_add (krad_tone, 1209.0);
		return;		
	}
	
	if (strstr(preset, "5") != NULL) {
		krad_tone_add (krad_tone, 770.0);
		krad_tone_add (krad_tone, 1336.0);
		return;		
	}

	if (strstr(preset, "6") != NULL) {
		krad_tone_add (krad_tone, 770.0);
		krad_tone_add (krad_tone, 1477.0);
		return;		
	}

	if (strstr(preset, "7") != NULL) {
		krad_tone_add (krad_tone, 852.0);
		krad_tone_add (krad_tone, 1209.0);
		return;		
	}
	
	if (strstr(preset, "8") != NULL) {
		krad_tone_add (krad_tone, 852.0);
		krad_tone_add (krad_tone, 1336.0);
		return;		
	}

	if (strstr(preset, "9") != NULL) {
		krad_tone_add (krad_tone, 852.0);
		krad_tone_add (krad_tone, 1477.0);
		return;		
	}
	
	if (strstr(preset, "0") != NULL) {
		krad_tone_add (krad_tone, 941.0);
		krad_tone_add (krad_tone, 1336.0);
		return;		
	}
	
	if (strstr(preset, "*") != NULL) {
		krad_tone_add (krad_tone, 941.0);
		krad_tone_add (krad_tone, 1209.0);
		return;		
	}

	if (strstr(preset, "#") != NULL) {
		krad_tone_add (krad_tone, 941.0);
		krad_tone_add (krad_tone, 1477.0);
		return;		
	}
	
	if (strstr(preset, "A") != NULL) {
		krad_tone_add (krad_tone, 697.0);
		krad_tone_add (krad_tone, 1633.0);
		return;		
	}
	
	if (strstr(preset, "B") != NULL) {
		krad_tone_add (krad_tone, 770.0);
		krad_tone_add (krad_tone, 1633.0);
		return;		
	}

	if (strstr(preset, "C") != NULL) {
		krad_tone_add(krad_tone, 852.0);
		krad_tone_add(krad_tone, 1633.0);
		return;		
	}
	
	if (strstr(preset, "D") != NULL) {
		krad_tone_add (krad_tone, 941.0);
		krad_tone_add (krad_tone, 1633.0);
		return;		
	}
}

void krad_tone_add (krad_tone_t *krad_tone, float frequency) {

	int i;
	
	krad_tone->active_tones++;
	
	for (i = 0; i < MAX_TONES; i++) {
		if (krad_tone->tones[i].active == 0) {
			krad_tone->tones[i].frequency = frequency;
			krad_tone->tones[i].delta = (2.0f * M_PI * krad_tone->tones[i].frequency) / krad_tone->sample_rate;
			krad_tone->tones[i].angle = 0.0f;
			krad_tone->tones[i].duration = 0;
			krad_tone->tones[i].active = 1;
			break;
		}
	}
}	

void krad_tone_remove (krad_tone_t *krad_tone, float frequency) {

	int i;
	
	krad_tone->active_tones--;
	
	for (i = 0; i < MAX_TONES; i++) {
		if (krad_tone->tones[i].active == 1) {
			if (krad_tone->tones[i].frequency == frequency) {
				krad_tone->tones[i].active = 0;
				krad_tone->tones[i].duration = 0;				
				break;
			}
		}
	}
}

void krad_tone_clear (krad_tone_t *krad_tone) {

	int i;
	
	for (i = 0; i < MAX_TONES; i++) {
		if (krad_tone->tones[i].active == 1) {
			krad_tone_remove(krad_tone, krad_tone->tones[i].frequency);
		}
	}
}

void krad_tone_run (krad_tone_t *krad_tone, float *buffer, int numsamples) {

	int i;
	
	if (krad_tone->active_tones == 0) {
		memset (buffer, '0', numsamples * 4);
	}
	
	for (i = 0; i < MAX_TONES; i++) {
		if (krad_tone->tones[i].active == 1) {
			krad_tone->tones[i].duration += numsamples;
			if (krad_tone->tones[i].duration > (krad_tone->sample_rate / 6)) {
				krad_tone_remove (krad_tone, krad_tone->tones[i].frequency);
			}
		}
	}

	for(krad_tone->s = 0; krad_tone->s < numsamples; krad_tone->s++) {

		buffer[krad_tone->s] = 0.0f;

		for (i = 0; i < krad_tone->active_tones; i++) {

			buffer[krad_tone->s] += krad_tone->volume_actual * sin(krad_tone->tones[i].angle);

			krad_tone->tones[i].angle += krad_tone->tones[i].delta;

			if (krad_tone->tones[i].angle > M_PI) {
				krad_tone->tones[i].angle -= 2.0f * M_PI;
			}
		}
	}


}


void krad_tone_destroy (krad_tone_t *krad_tone) {

	free (krad_tone);
	
}
