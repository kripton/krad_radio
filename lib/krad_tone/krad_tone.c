#include "krad_tone.h"

krad_tone_t *krad_tone_create (float sample_rate) {

  krad_tone_t *krad_tone = calloc (1, sizeof(krad_tone_t));

  krad_tone_set_sample_rate (krad_tone, sample_rate);
  krad_tone_set_volume (krad_tone, KRAD_TONE_DEFAULT_VOLUME);

  return krad_tone;
}


void krad_tone_destroy (krad_tone_t *krad_tone) {
  free (krad_tone);
}

void krad_tone_set_sample_rate (krad_tone_t *krad_tone, float sample_rate) {
  krad_tone->sample_rate = sample_rate;
  krad_tone->duration = krad_tone->sample_rate / 6;
}

void krad_tone_set_volume (krad_tone_t *krad_tone, int volume) {
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

  int t;
  
  for (t = 0; t < MAX_TONES; t++) {
    if (krad_tone->tones[t].active == 0) {
      krad_tone->tones[t].frequency = frequency;
      krad_tone->tones[t].delta = (2.0f * M_PI * krad_tone->tones[t].frequency) / krad_tone->sample_rate;
      krad_tone->tones[t].angle = 0.0f;
      krad_tone->tones[t].duration = 0;
      krad_tone->tones[t].active = 1;
      break;
    }
  }
  
  krad_tone->active_tones++;
}  

void krad_tone_remove (krad_tone_t *krad_tone, float frequency) {

  int t;
  
  for (t = 0; t < MAX_TONES; t++) {
    if (krad_tone->tones[t].active != 0) {
      if (krad_tone->tones[t].frequency == frequency) {
        krad_tone->tones[t].active = 0;
        krad_tone->tones[t].duration = 0;
        krad_tone->tones[t].last_sign = 0;
        break;
      }
    }
  }
  
  krad_tone->active_tones--;
}

void krad_tone_mark_remove (krad_tone_t *krad_tone, float frequency) {

  int t;
  
  for (t = 0; t < MAX_TONES; t++) {
    if (krad_tone->tones[t].active == 1) {
      if (krad_tone->tones[t].frequency == frequency) {
        krad_tone->tones[t].active = 2;
        break;
      }
    }
  }
}

void krad_tone_clear (krad_tone_t *krad_tone) {

  int t;
  
  for (t = 0; t < MAX_TONES; t++) {
    if (krad_tone->tones[t].active != 0) {
      krad_tone_remove (krad_tone, krad_tone->tones[t].frequency);
    }
  }
}

void krad_tone_run (krad_tone_t *krad_tone, float *buffer, int numsamples) {

  int t;
  int s;
  float sample;
  int sign;
  
  sign = 0;

  memset (buffer, 0, numsamples * 4);
  
  if (krad_tone->active_tones == 0) {
    return;
  }
  
  for (t = 0; t < MAX_TONES; t++) {
    if (krad_tone->tones[t].active == 1) {
      krad_tone->tones[t].duration += numsamples;
      if (krad_tone->tones[t].duration > krad_tone->duration) {
        krad_tone_mark_remove (krad_tone, krad_tone->tones[t].frequency);
      }
    }
  }

  for (t = 0; t < MIN(MAX_TONES, krad_tone->active_tones + 1); t++) {
    if (krad_tone->tones[t].active == 1) {
      for (s = 0; s < numsamples; s++) {
        buffer[s] += krad_tone->volume_actual * sin(krad_tone->tones[t].angle);
        krad_tone->tones[t].angle += krad_tone->tones[t].delta;
        if (krad_tone->tones[t].angle > M_PI) {
          krad_tone->tones[t].angle -= 2.0f * M_PI;
        }
      }
    }
    if (krad_tone->tones[t].active == 2) {
      for (s = 0; s < numsamples; s++) {
        sample = krad_tone->volume_actual * sin(krad_tone->tones[t].angle);
        if (sample > 0.0f) {
	        sign = 1;
        } else {
	        sign = -1;
        }
        if ((krad_tone->tones[t].last_sign != 0) && (krad_tone->tones[t].last_sign != sign)) {
          krad_tone_remove (krad_tone, krad_tone->tones[t].frequency);
          break;
        }
        buffer[s] += sample;
        krad_tone->tones[t].angle += krad_tone->tones[t].delta;
        if (krad_tone->tones[t].angle > M_PI) {
          krad_tone->tones[t].angle -= 2.0f * M_PI;
        }
        krad_tone->tones[t].last_sign = sign;
      }
    }
  }
}

