#include "krad_mixer.h"

void krad_mixer_crossfade_group_create (krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *portgroup1, krad_mixer_portgroup_t *portgroup2) {

	int x;
	krad_mixer_crossfade_group_t *crossfade_group;

	if (!(((portgroup1->direction == INPUT) || (portgroup1->direction == MIX)) &&
		  ((portgroup1->direction == INPUT) || (portgroup2->direction == MIX)) &&
		  (((portgroup1->io_type == MIXBUS) && (portgroup2->io_type == MIXBUS)) ||
		   ((portgroup1->io_type != MIXBUS) && (portgroup2->io_type != MIXBUS))))) {
		failfast ("Invalid crossfade group!");
	}
		
	if (portgroup1->crossfade_group != NULL) {
		krad_mixer_crossfade_group_destroy (krad_mixer, portgroup1->crossfade_group);
	}

	if (portgroup2->crossfade_group != NULL) {
		krad_mixer_crossfade_group_destroy (krad_mixer, portgroup2->crossfade_group);
	}

	for (x = 0; x < KRAD_MIXER_MAX_PORTGROUPS / 2; x++) {
		crossfade_group = &krad_mixer->crossfade_group[x];
		if ((crossfade_group != NULL) && ((crossfade_group->portgroup[0] == NULL) && (crossfade_group->portgroup[1] == NULL))) {
			break;
		}	
	}

	crossfade_group->portgroup[0] = portgroup1;
	crossfade_group->portgroup[1] = portgroup2;
	
	portgroup1->crossfade_group = crossfade_group;
	portgroup2->crossfade_group = crossfade_group;
	
	crossfade_group_set_crossfade (crossfade_group, -100.0f);
	
}

void krad_mixer_crossfade_group_destroy (krad_mixer_t *krad_mixer, krad_mixer_crossfade_group_t *crossfade_group) {

	crossfade_group->portgroup[0]->crossfade_group = NULL;
	crossfade_group->portgroup[1]->crossfade_group = NULL;

	crossfade_group->portgroup[0] = NULL;
	crossfade_group->portgroup[1] = NULL;
	crossfade_group->fade = -100.0f;

}

float get_fade_out (float crossfade_value) {

	float fade_out;

	fade_out = cos (3.14159f*0.5f*((crossfade_value + 100.0f) + 0.5f)/200.0f);
	fade_out = fade_out * fade_out;
		
	return fade_out;
		
}

float get_fade_in (float crossfade_value) {
	
	return 1.0f - get_fade_out (crossfade_value);
	
}

float portgroup_get_crossfade (krad_mixer_portgroup_t *portgroup) {

	if (portgroup->crossfade_group->portgroup[0] == portgroup) {
		return get_fade_out (portgroup->crossfade_group->fade);
	}

	if (portgroup->crossfade_group->portgroup[1] == portgroup) {
		return get_fade_in (portgroup->crossfade_group->fade);
	}
	
	failfast ("failed to get portgroup for crossfade!");

	return 0;
	
}

void portgroup_apply_effects (krad_mixer_portgroup_t *portgroup, int nframes) {

  if (portgroup->effects->sample_rate != portgroup->krad_mixer->sample_rate) {
    kr_effects_set_sample_rate (portgroup->effects, portgroup->krad_mixer->sample_rate);
  }

  kr_effects_process (portgroup->effects, portgroup->samples, portgroup->samples, nframes);

}

void portgroup_apply_volume (krad_mixer_portgroup_t *portgroup, int nframes) {

	int c, s, sign;
	
	sign = 0;

	for (c = 0; c < portgroup->channels; c++) {

		if (portgroup->new_volume_actual[c] == portgroup->volume_actual[c]) {
			for (s = 0; s < nframes; s++) {
				portgroup->samples[c][s] = portgroup->samples[c][s] * portgroup->volume_actual[c];
			}
		} else {
			
			/* The way the volume change is set up here, the volume can only change once per callback, but thats 
			   allways plenty of times per second */
			
			/* last_sign: 0 = unset, -1 neg, +1 pos */
				
			if (portgroup->last_sign[c] == 0) {
				if (portgroup->samples[c][0] > 0.0f) {
					portgroup->last_sign[c] = 1;
				} else {
					/* Zero counts as negative here, but its moot */
					portgroup->last_sign[c] = -1;
				}
			}
			
			for (s = 0; s < nframes; s++) {
				if (portgroup->last_sign[c] != 0) {
					if (portgroup->samples[c][s] > 0.0f) {
						sign = 1;
					} else {
						sign = -1;
					}
				
					if ((sign != portgroup->last_sign[c]) || (portgroup->samples[c][s] == 0.0f)) {
						portgroup->volume_actual[c] = portgroup->new_volume_actual[c];
						portgroup->last_sign[c] = 0;
					}
				}
				portgroup->samples[c][s] = (portgroup->samples[c][s] * portgroup->volume_actual[c]);
			}

			if (portgroup->last_sign[c] != 0) {
				portgroup->last_sign[c] = sign;
			}
		}
	}
}

float krad_mixer_peak_scale (float value) {
	
	float db;
	float def;

	db = 20.0f * log10f (value * 1.0f);

	if (db < -70.0f) {
		def = 0.0f;
	} else if (db < -60.0f) {
		def = (db + 70.0f) * 0.25f;
	} else if (db < -50.0f) {
		def = (db + 60.0f) * 0.5f + 2.5f;
	} else if (db < -40.0f) {
		def = (db + 50.0f) * 0.75f + 7.5;
	} else if (db < -30.0f) {
		def = (db + 40.0f) * 1.5f + 15.0f;
	} else if (db < -20.0f) {
		def = (db + 30.0f) * 2.0f + 30.0f;
	} else if (db < 0.0f) {
		def = (db + 20.0f) * 2.5f + 50.0f;
	} else {
		def = 100.0f;
	}

	return def;
}

float krad_mixer_portgroup_read_channel_peak (krad_mixer_portgroup_t *portgroup, int channel) {

	float peak;
	
	peak = portgroup->peak[channel];
	portgroup->peak[channel] = 0.0f;

	return peak;
}

float krad_mixer_portgroup_read_peak (krad_mixer_portgroup_t *portgroup) {

	//FIXME N channels

	float tmp = portgroup->peak[0];
	portgroup->peak[0] = 0.0f;

	float tmp2 = portgroup->peak[1];
	portgroup->peak[1] = 0.0f;
	if (tmp > tmp2) {
		return tmp;
	} else {
		return tmp2;
	}
}

void krad_mixer_portgroup_compute_channel_peak (krad_mixer_portgroup_t *portgroup, int channel, uint32_t nframes) {

	int s;
	float sample;

	for(s = 0; s < nframes; s++) {
		sample = fabs(portgroup->samples[channel][s]);
		if (sample > portgroup->peak[channel]) {
			portgroup->peak[channel] = sample;
		}
	}
}


void krad_mixer_portgroup_compute_peaks (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {
		krad_mixer_portgroup_compute_channel_peak (portgroup, c, nframes);
	}
}
/*
void krad_mixer_portgroup_compute_channel_rms (krad_mixer_portgroup_t *portgroup, int channel, uint32_t nframes) {

	int s;
	float sample;
	float total;

  total = 0;

	for (s = 0; s < nframes; s++) {
    sample = fabs(portgroup->samples[channel][s]);
		total += sample * sample;
	}

  portgroup->rms = (portgroup->rms + total) / portgroup->krad_mixer->rms_window_size;

}
*/
void krad_mixer_compute_levels (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {
		//krad_mixer_portgroup_compute_channel_rms (portgroup, c, nframes);
		krad_mixer_portgroup_compute_channel_peak (portgroup, c, nframes);
	}
}

void portgroup_clear_samples (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	int c;
	int s;

	for (c = 0; c < portgroup->channels; c++) {
		for (s = 0; s < nframes; s++) {
			portgroup->samples[c][s] = 0.0f;
		}
	}
}

void krad_mixer_portgroup_map_channel (krad_mixer_portgroup_t *portgroup, int in_channel, int out_channel) {

	portgroup->map[in_channel] = out_channel;

	portgroup->mapped_samples[in_channel] = &portgroup->samples[out_channel];

}

void krad_mixer_portgroup_mixmap_channel (krad_mixer_portgroup_t *portgroup, int in_channel, int out_channel) {

	portgroup->mixmap[out_channel] = in_channel;

}

int portgroup_handle_delay (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

  int delay_frames;

  if (portgroup->delay > portgroup->delay_actual) {
    delay_frames = portgroup->delay - portgroup->delay_actual;
    if (delay_frames > nframes) {
      delay_frames = nframes;
    }
    portgroup_clear_samples (portgroup, delay_frames);
    portgroup->delay += delay_frames;
    return nframes - delay_frames;
  }

  return nframes;
}

void portgroup_get_samples (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	int c;
	float *samples[KRAD_MIXER_MAX_CHANNELS];

	switch ( portgroup->io_type ) {
		case KRAD_TONE:
			krad_tone_run (portgroup->io_ptr, *portgroup->mapped_samples[0], nframes);
			break;
		case MIXBUS:
			break;
		case KRAD_AUDIO:
			krad_audio_portgroup_samples_callback (nframes, portgroup->io_ptr, samples);
			
			for (c = 0; c < KRAD_MIXER_MAX_CHANNELS; c++ ) {
				portgroup->samples[c] = samples[portgroup->map[c]];
			}
			break;
		case KRAD_LINK:
			if (portgroup->direction == INPUT) {
				krad_link_audio_samples_callback (nframes, portgroup->io_ptr, portgroup->mapped_samples[0]);
			}
			break;
		case KLOCALSHM:
			if (portgroup->direction == INPUT) {
				krad_mixer_local_audio_samples_callback (nframes, portgroup->io_ptr, portgroup->mapped_samples[0]);
			}
			break;
	}
}

void portgroup_update_samples (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

  if (portgroup->delay != portgroup->delay_actual) {
    nframes = portgroup_handle_delay (portgroup, nframes);
  }
  portgroup_get_samples (portgroup, nframes);
}

void portgroup_mix_samples (krad_mixer_portgroup_t *dest_portgroup, krad_mixer_portgroup_t *src_portgroup, uint32_t nframes) {

	int c;
	int s;

	if (dest_portgroup->channels == src_portgroup->channels) {
		for (c = 0; c < dest_portgroup->channels; c++) {
			for (s = 0; s < nframes; s++) {
				dest_portgroup->samples[c][s] += src_portgroup->samples[c][s];
			}
		}
	} else {
		//up mix
		for (c = 0; c < dest_portgroup->channels; c++) {
			if (src_portgroup->mixmap[c] != -1) {
				for (s = 0; s < nframes; s++) {
					dest_portgroup->samples[c][s] += src_portgroup->samples[src_portgroup->mixmap[c]][s];
				}
			}
		}
	
	}
}

void portgroup_copy_samples (krad_mixer_portgroup_t *dest_portgroup, krad_mixer_portgroup_t *src_portgroup, uint32_t nframes) {

	int c;
	int s;

	// Do mixdown here?

	switch ( dest_portgroup->io_type ) {
		case KRAD_TONE:
			break;
		case MIXBUS:
			break;
		case KRAD_AUDIO:
			for (c = 0; c < dest_portgroup->channels; c++) {
				for (s = 0; s < nframes; s++) {
					dest_portgroup->samples[c][s] = src_portgroup->samples[c][s];
				}
			}
			break;
		case KRAD_LINK:
			if (dest_portgroup->direction == OUTPUT) {
				krad_link_audio_samples_callback (nframes, dest_portgroup->io_ptr, src_portgroup->samples);
			}
			break;
		case KLOCALSHM:
			if (dest_portgroup->direction == OUTPUT) {
				krad_mixer_local_audio_samples_callback (nframes, dest_portgroup->io_ptr, src_portgroup->samples);
			}
			break;
	}
}

void portgroup_limit (krad_mixer_portgroup_t *portgroup, uint32_t nframes) {

	int c;

	for (c = 0; c < portgroup->channels; c++) {
		//kr_rushlimiter_process (portgroup->kr_rushlimiter[c], portgroup->samples[c], portgroup->samples[c], nframes);
		krad_hardlimit (portgroup->samples[c], nframes);
	}
}

void krad_mixer_deactivate_portgroups (krad_mixer_t *krad_mixer) {

	int p;
	krad_mixer_portgroup_t *portgroup;
	
	p = 0;
  portgroup = NULL;
		
	// deactivate ports that need to be deactivated
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active)) {
			if (portgroup->active == 2) {
				portgroup->active = 0;
			}
		}
	}
}

int krad_mixer_process (uint32_t nframes, krad_mixer_t *krad_mixer) {
	
	int p, m;

	krad_mixer_portgroup_t *portgroup = NULL;
	krad_mixer_portgroup_t *mixbus = NULL;
	
	// only run if we have something going on
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) &&
			(portgroup != krad_mixer->tone_port) && (portgroup != krad_mixer->master_mix)) {
			break;
		}
	}
	if (p == KRAD_MIXER_MAX_PORTGROUPS) {
    krad_mixer_deactivate_portgroups (krad_mixer);
		return 0;
	}
		
	if (krad_mixer->push_tone != NULL) {
		krad_tone_add_preset (krad_mixer->tone_port->io_ptr, krad_mixer->push_tone);
		krad_mixer->push_tone = NULL;
	}
	
	// Gets input/output port buffers
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && ((portgroup->direction == INPUT) || (portgroup->direction == OUTPUT))) {
			portgroup_update_samples (portgroup, nframes);
		}
	}

	// apply volume, effects and calc peaks on inputs
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == INPUT)) {
			portgroup_apply_volume (portgroup, nframes);
			portgroup_apply_effects (portgroup, nframes);
			krad_mixer_portgroup_compute_peaks (portgroup, nframes);
		}
	}
	
	// Clear Mixes	
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->io_type == MIXBUS)) {
			portgroup_clear_samples (portgroup, nframes);
		}
	}

	// Mix
	for (m = 0; m < KRAD_MIXER_MAX_PORTGROUPS; m++) {
		mixbus = krad_mixer->portgroup[m];
		if ((mixbus != NULL) && (mixbus->active) && (mixbus->io_type == MIXBUS)) {
			for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
				portgroup = krad_mixer->portgroup[p];
				if ((portgroup != NULL) && (portgroup->active) && (portgroup->mixbus == mixbus) && (portgroup->direction == INPUT)) {
					portgroup_mix_samples ( mixbus, portgroup, nframes );
				}
			}
			//krad_mixer_portgroup_compute_peaks (mixbus, nframes);
		}
	}

	// copy to outputs, limit all outputs
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active) && (portgroup->direction == OUTPUT)) {
		  if (portgroup->output_type == AUX) {
			  portgroup_apply_volume (portgroup, nframes);
		  }
			portgroup_limit ( portgroup->mixbus, nframes );
			portgroup_copy_samples ( portgroup, portgroup->mixbus, nframes );
		}
	}
	
	if (krad_mixer->master_mix != NULL) {
	  krad_mixer_portgroup_compute_peaks (krad_mixer->master_mix, nframes);
	}
	
  krad_mixer_deactivate_portgroups (krad_mixer);

	return 0;      

}


char *krad_mixer_channel_number_to_string (int channel) {

	switch ( channel ) {
		case 0:
			return "Left";
		case 1:
			return "Right";
		case 2:
			return "RearLeft";
		case 3:
			return "RearRight";
		case 4:
			return "Center";
		case 5:
			return "Sub";
		case 6:
			return "BackLeft";
		case 7:
			return "BackRight";
		default:
			return "Unknown";
	}
}


void krad_mixer_local_audio_samples_callback (int nframes, krad_mixer_local_portgroup_t *krad_mixer_local_portgroup,
											  float **samples) {

	int ret;
	int wrote;
	char buf[1];
	
	ret = 0;
	wrote = 0;
	buf[0] = 0;
	
	if (krad_mixer_local_portgroup->direction == OUTPUT) {
		memcpy (krad_mixer_local_portgroup->local_buffer, samples[0], 2 * 4 * 1600);
	}

	wrote = write (krad_mixer_local_portgroup->msg_sd, buf, 1);

	if (wrote == 1) {
		ret = read (krad_mixer_local_portgroup->msg_sd, buf, 1);
		if (ret == 1) {

		}
	}
	
	return;

}

krad_mixer_portgroup_t *krad_mixer_local_portgroup_create (krad_mixer_t *krad_mixer, char *sysname,
														   int direction, int shm_sd, int msg_sd) {

	krad_mixer_portgroup_t *krad_mixer_portgroup;
	krad_mixer_local_portgroup_t *krad_mixer_local_portgroup;

	krad_mixer_output_t output_type;

	krad_mixer_local_portgroup = calloc(1, sizeof(krad_mixer_local_portgroup_t));

	krad_mixer_local_portgroup->local_buffer_size = 960 * 540 * 4 * 2;
	
	krad_mixer_local_portgroup->shm_sd = shm_sd;
	krad_mixer_local_portgroup->msg_sd = msg_sd;
	
	krad_mixer_local_portgroup->local_buffer = mmap (NULL, krad_mixer_local_portgroup->local_buffer_size,
											   PROT_READ | PROT_WRITE, MAP_SHARED,
											   krad_mixer_local_portgroup->shm_sd, 0);


	krad_mixer_local_portgroup->direction = direction;
	
	if (krad_mixer_local_portgroup->direction == INPUT) {
    output_type = NOTOUTPUT;
	} else {
    output_type = DIRECT;
	}

	krad_mixer_portgroup = krad_mixer_portgroup_create (krad_mixer, sysname, krad_mixer_local_portgroup->direction,
                          output_type, 2, krad_mixer->master_mix, KLOCALSHM, krad_mixer_local_portgroup, NOAUDIO);

	krad_mixer_portgroup->samples[0] = (float *)krad_mixer_local_portgroup->local_buffer;
	krad_mixer_portgroup->samples[1] = (float *)krad_mixer_local_portgroup->local_buffer + 1600;

	krad_mixer_portgroup->active = 1;

	return krad_mixer_portgroup;

}

int krad_mixer_portgroup_is_jack (krad_mixer_portgroup_t *portgroup) {

	krad_audio_portgroup_t *ka_portgroup;

	if (portgroup != NULL) {
		if (portgroup->io_type == KRAD_AUDIO) {
			ka_portgroup = portgroup->io_ptr;
			if (ka_portgroup->audio_api == JACK) {
				return 1;
			}
		}
	}
	
	return 0;
	
}

krad_mixer_portgroup_t *krad_mixer_portgroup_create (krad_mixer_t *krad_mixer, char *sysname, int direction,
                           krad_mixer_output_t output_type, int channels, 
													 krad_mixer_mixbus_t *mixbus, krad_mixer_portgroup_io_t io_type, 
													 void *io_ptr, krad_audio_api_t api) {

	int p;
	int c;
	krad_mixer_portgroup_t *portgroup;
	
	portgroup = NULL;

	/* prevent dupe names */
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if (krad_mixer->portgroup[p]->active != 0) {
			if (strcmp(sysname, krad_mixer->portgroup[p]->sysname) == 0) {
				return NULL;
			}
		}
	}
	
	//FIXME race here if portgroup being created via ipc and transponder at same moment
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if (krad_mixer->portgroup[p]->active == 0) {
			portgroup = krad_mixer->portgroup[p];
			break;
		}
	}
	
	if (portgroup == NULL) {
		return NULL;
	}
	
	/* Prevent multiple JACK direct outputs as this is redundant */
  if ((api == JACK) && (direction == OUTPUT) && (output_type == DIRECT)) {
	  for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		  if (krad_mixer->portgroup[p]->active == 1) {
        if ((krad_mixer_portgroup_is_jack(krad_mixer->portgroup[p])) &&
            (krad_mixer->portgroup[p]->direction == OUTPUT) &&
            (krad_mixer->portgroup[p]->output_type == DIRECT)) {	    
				  return NULL;
				}
		  }
	  }
  }

	portgroup->krad_mixer = krad_mixer;

	strcpy (portgroup->sysname, sysname);
	portgroup->channels = channels;
	portgroup->io_type = io_type;
	portgroup->mixbus = mixbus;
	portgroup->direction = direction;

	for (c = 0; c < KRAD_MIXER_MAX_CHANNELS; c++) {

		if (c < portgroup->channels) {
			portgroup->mixmap[c] = c;
		} else {
			portgroup->mixmap[c] = -1;
		}
		portgroup->map[c] = c;
		portgroup->mapped_samples[c] = &portgroup->samples[c];
		portgroup->volume[c] = 100;
		portgroup->volume_actual[c] = (float)(portgroup->volume[c]/100.0f);
		portgroup->volume_actual[c] *= portgroup->volume_actual[c];
		portgroup->new_volume_actual[c] = portgroup->volume_actual[c];

		switch ( portgroup->io_type ) {
			case KRAD_TONE:
				portgroup->samples[c] = calloc (1, 16384);
				break;		
			case MIXBUS:
				portgroup->samples[c] = calloc (1, 16384);
				break;
			case KRAD_AUDIO:
				break;
			case KRAD_LINK:
				portgroup->samples[c] = calloc (1, 16384);
				break;
			case KLOCALSHM:
				break;
		}
	}
	
	switch ( portgroup->io_type ) {
		case KRAD_TONE:
			portgroup->io_ptr = krad_tone_create (krad_mixer->sample_rate);
		case MIXBUS:
			break;
		case KRAD_AUDIO:
			portgroup->io_ptr = krad_audio_portgroup_create (krad_mixer->krad_audio, portgroup->sysname, 
															 portgroup->direction, portgroup->channels, api);
			break;
		case KRAD_LINK:
			portgroup->io_ptr = io_ptr;
			break;
		case KLOCALSHM:
			portgroup->io_ptr = io_ptr;		
			break;			
	}
	
	if (portgroup->io_type != KRAD_LINK) {
		portgroup->krad_tags = krad_tags_create (portgroup->sysname);
		if ((portgroup->krad_tags != NULL) && (krad_mixer->krad_ipc != NULL)) {
			krad_tags_set_set_tag_callback (portgroup->krad_tags, krad_mixer->krad_ipc, 
											(void (*)(void *, char *, char *, char *, int))krad_ipc_server_broadcast_tag);
		}
	} else {
		portgroup->krad_tags = krad_link_get_tags (portgroup->io_ptr);
	}

	if (portgroup->krad_tags == NULL) {
		failfast ("Oh I couldn't find me tags");
	}

  portgroup->effects = kr_effects_create (portgroup->channels, portgroup->krad_mixer->sample_rate);
  
	if (portgroup->effects == NULL) {
		failfast ("Oh I couldn't make effects");
	}

	for (c = 0; c < portgroup->channels; c++) {
    //portgroup->kr_rushlimiter[c] = kr_rushlimiter_create();
  }

	if (portgroup->io_type != KLOCALSHM) {
		portgroup->active = 1;
	}
	return portgroup;

}

void krad_mixer_local_portgroup_destroy (krad_mixer_local_portgroup_t *krad_mixer_local_portgroup) {

	close (krad_mixer_local_portgroup->msg_sd);
	close (krad_mixer_local_portgroup->shm_sd);
	munmap (krad_mixer_local_portgroup->local_buffer, krad_mixer_local_portgroup->local_buffer_size);
	free (krad_mixer_local_portgroup);

}


void krad_mixer_portgroup_destroy (krad_mixer_t *krad_mixer, krad_mixer_portgroup_t *portgroup) {

	int c;

	if (portgroup == NULL) {
		return;
	}

	portgroup->active = 2;

	while (portgroup->active != 0) {

		usleep(2000);
	}

	portgroup->delay = 0;
	portgroup->delay_actual = 0;

	printkd("Krad Mixer: Removing %d channel Portgroup %s", portgroup->channels, portgroup->sysname);

	for (c = 0; c < KRAD_MIXER_MAX_CHANNELS; c++) {
		switch ( portgroup->io_type ) {
			case KRAD_TONE:
				free ( portgroup->samples[c] );			
				break;
			case MIXBUS:
				free ( portgroup->samples[c] );
				break;
			case KRAD_AUDIO:
				break;
			case KRAD_LINK:
				free ( portgroup->samples[c] );			
				break;
			case KLOCALSHM:
				break;
		}
	}
	

	switch ( portgroup->io_type ) {
		case KRAD_TONE:
			krad_tone_destroy (portgroup->io_ptr);
		case MIXBUS:
			break;
		case KRAD_AUDIO:
			krad_audio_portgroup_destroy (portgroup->io_ptr);
			break;
		case KRAD_LINK:
			break;
		case KLOCALSHM:
			krad_mixer_local_portgroup_destroy (portgroup->io_ptr);
			break;
	}
	
	if (portgroup->krad_xmms != NULL) {
		krad_xmms_destroy (portgroup->krad_xmms);
		portgroup->krad_xmms = NULL;
	}
	
	if (portgroup->io_type != KRAD_LINK) {
		krad_tags_destroy (portgroup->krad_tags);	
	}	
	
	if (portgroup->effects != NULL) {
		kr_effects_destroy (portgroup->effects);
		portgroup->effects = NULL;
	}

	for (c = 0; c < portgroup->channels; c++) {
    //kr_rushlimiter_destroy (portgroup->kr_rushlimiter[c]);
    portgroup->kr_rushlimiter[c] = NULL;
  }

}

krad_mixer_portgroup_t *krad_mixer_get_portgroup_from_sysname (krad_mixer_t *krad_mixer, char *sysname) {

	int p;
	krad_mixer_portgroup_t *portgroup;

	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		portgroup = krad_mixer->portgroup[p];
		if ((portgroup != NULL) && (portgroup->active)) {
			if (strcmp(sysname, portgroup->sysname) == 0) {
				return portgroup;
			}
		}
	}

	return NULL;
}

void portgroup_set_crossfade (krad_mixer_portgroup_t *portgroup, float value) {

	if (portgroup->crossfade_group != NULL) {

		portgroup->crossfade_group->fade = value;	

		portgroup_update_volume (portgroup->crossfade_group->portgroup[0]);
		portgroup_update_volume (portgroup->crossfade_group->portgroup[1]);	
	}
}

void crossfade_group_set_crossfade (krad_mixer_crossfade_group_t *crossfade_group, float value) {

	crossfade_group->fade = value;	

	if ((crossfade_group->portgroup[0] != NULL) && (crossfade_group->portgroup[1] != NULL)) {
		portgroup_update_volume (crossfade_group->portgroup[0]);
		portgroup_update_volume (crossfade_group->portgroup[1]);	
	}
}

void portgroup_set_volume (krad_mixer_portgroup_t *portgroup, float value) {

	int c;
	float volume_temp;

	volume_temp = (value/100.0f);
	volume_temp *= volume_temp;

	if (portgroup->direction == INPUT) {
		volume_temp = volume_temp * portgroup->mixbus->volume_actual[0];

  	if (portgroup->crossfade_group != NULL) {
  		volume_temp = volume_temp * portgroup_get_crossfade (portgroup);
  	}
  }

	for (c = 0; c < portgroup->channels; c++) {
		portgroup->volume[c] = value;
    if (portgroup->direction == MIX) {
      portgroup->mixbus->volume_actual[0] = volume_temp;
      break;
    } else {
		  portgroup->new_volume_actual[c] = volume_temp;
		}
	}
	
  if (portgroup->direction == MIX) {
  
	  int p;
	  krad_mixer_portgroup_t *pg;

	  for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		  pg = portgroup->krad_mixer->portgroup[p];
		  if ((pg != NULL) && (pg->active) && (pg->direction == INPUT)) {
			  if (pg->mixbus == portgroup) {
				  portgroup_update_volume (pg);
			  }
		  }
	  }
    printk ("Mixbus %s volume set to %f", portgroup->sysname, portgroup->volume[0]);
	}
}

void portgroup_set_channel_volume (krad_mixer_portgroup_t *portgroup, int channel, float value) {

	float volume_temp;

	if (portgroup->direction == INPUT) {

	  portgroup->volume[channel] = value;
	  volume_temp = (portgroup->volume[channel]/100.0f);
	  volume_temp *= volume_temp;
		volume_temp = volume_temp * portgroup->mixbus->volume_actual[0];
	  if (portgroup->crossfade_group != NULL) {
		  volume_temp = volume_temp * portgroup_get_crossfade (portgroup);
	  }

	  portgroup->new_volume_actual[channel] = volume_temp;
  }
}

void portgroup_update_volume (krad_mixer_portgroup_t *portgroup) {

	int c;
	
	for (c = 0; c < portgroup->channels; c++) {
		portgroup_set_channel_volume (portgroup, c, portgroup->volume[c]);
	}	
}		

int krad_mixer_set_portgroup_control (krad_mixer_t *krad_mixer, char *sysname, char *control, float value) {

	krad_mixer_portgroup_t *portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, sysname);
	
	if (portgroup != NULL) {
			
    if ((portgroup->direction == OUTPUT) && (portgroup->output_type == DIRECT)) {
      return 1;
    }
			
    if ((strncmp(control, "volume", 6) == 0) && (strlen(control) == 6)) {
      portgroup_set_volume (portgroup, value);
      return 1;
    }

    if ((portgroup->direction == INPUT) && (portgroup->io_type != MIXBUS)) {

      if ((strncmp(control, "crossfade", 9) == 0) && (strlen(control) == 9)) {
        portgroup_set_crossfade (portgroup, value);
        return 1;
      }				

      if (strncmp(control, "volume_left", 11) == 0) {
        portgroup_set_channel_volume (portgroup, 0, value);
        return 1;	
      }

      if (strncmp(control, "volume_right", 12) == 0) {
        portgroup_set_channel_volume (portgroup, 1, value);
        return 1;
      }
    }
  }
		
	return 0;
}

void krad_mixer_portgroup_xmms2_cmd (krad_mixer_t *krad_mixer, char *portgroupname, char *xmms2_cmd) {

	krad_mixer_portgroup_t *portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);

	if ((portgroup != NULL) && (portgroup->krad_xmms != NULL)) {
		if (strncmp(xmms2_cmd, "play", 4) == 0) {
			krad_xmms_playback_cmd (portgroup->krad_xmms, PLAY);
			return;
		}
		if (strncmp(xmms2_cmd, "pause", 5) == 0) {
			krad_xmms_playback_cmd (portgroup->krad_xmms, PAUSE);
			return;
		}
		if (strncmp(xmms2_cmd, "stop", 4) == 0) {
			krad_xmms_playback_cmd (portgroup->krad_xmms, STOP);
			return;
		}
		if (strncmp(xmms2_cmd, "next", 4) == 0) {
			krad_xmms_playback_cmd (portgroup->krad_xmms, NEXT);
			return;
		}
		if (strncmp(xmms2_cmd, "prev", 4) == 0) {
			krad_xmms_playback_cmd (portgroup->krad_xmms, PREV);
			return;
		}
	}
}

void krad_mixer_plug_portgroup (krad_mixer_t *krad_mixer, char *name, char *remote_name) {

	krad_mixer_portgroup_t *portgroup;
	krad_audio_portgroup_t *ka_portgroup;
	krad_jack_portgroup_t *krad_jack_portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, name);

	printk ("mixer want to plug %s to %s", name, remote_name);

	if (portgroup != NULL) {
		if (portgroup->io_type == KRAD_AUDIO) {
			ka_portgroup = portgroup->io_ptr;
			if (ka_portgroup->audio_api == JACK) {
				krad_jack_portgroup = ka_portgroup->api_portgroup;
				krad_jack_portgroup_plug (krad_jack_portgroup, remote_name);
			}
		}
	}
}


void krad_mixer_unplug_portgroup (krad_mixer_t *krad_mixer, char *name, char *remote_name) {

	krad_mixer_portgroup_t *portgroup;
	krad_audio_portgroup_t *ka_portgroup;
	krad_jack_portgroup_t *krad_jack_portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, name);

	if (portgroup != NULL) {
		if (portgroup->io_type == KRAD_AUDIO) {
			ka_portgroup = portgroup->io_ptr;
			if (ka_portgroup->audio_api == JACK) {
				krad_jack_portgroup = ka_portgroup->api_portgroup;
				krad_jack_portgroup_unplug (krad_jack_portgroup, remote_name);
			}
		}
	}
}

void krad_mixer_bind_portgroup_xmms2 (krad_mixer_t *krad_mixer, char *portgroupname, char *ipc_path) {

	krad_mixer_portgroup_t *portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);

	if (portgroup->krad_xmms != NULL) {
		krad_xmms_destroy (portgroup->krad_xmms);	
		portgroup->krad_xmms = NULL;
	}

	portgroup->krad_xmms = krad_xmms_create (krad_mixer->name, ipc_path, portgroup->krad_tags);

}

void krad_mixer_unbind_portgroup_xmms2 (krad_mixer_t *krad_mixer, char *portgroupname) {

	krad_mixer_portgroup_t *portgroup;

	portgroup = krad_mixer_get_portgroup_from_sysname (krad_mixer, portgroupname);

	if (portgroup->krad_xmms != NULL) {
	
		krad_xmms_destroy (portgroup->krad_xmms);
	
		portgroup->krad_xmms = NULL;
	}

}


void *krad_mixer_ticker_thread (void *arg) {

	krad_mixer_t *krad_mixer = (krad_mixer_t *)arg;

	krad_system_set_thread_name ("kr_mixer");

	krad_mixer->krad_ticker = krad_ticker_create (krad_mixer->sample_rate, krad_mixer->ticker_period);

	krad_ticker_start_at (krad_mixer->krad_ticker, krad_mixer->start_time);

	while (krad_mixer->ticker_running == 1) {
	
		krad_mixer_process (krad_mixer->ticker_period, krad_mixer);
	
		krad_ticker_wait (krad_mixer->krad_ticker);

	}

	krad_ticker_destroy (krad_mixer->krad_ticker);


	return NULL;

}


void krad_mixer_start_ticker (krad_mixer_t *krad_mixer) {

	if (krad_mixer->ticker_running == 1) {
		krad_mixer_stop_ticker (krad_mixer);
	}
  clock_gettime (CLOCK_MONOTONIC, &krad_mixer->start_time);
	krad_mixer->ticker_running = 1;
	pthread_create (&krad_mixer->ticker_thread, NULL, krad_mixer_ticker_thread, (void *)krad_mixer);

}

void krad_mixer_start_ticker_at (krad_mixer_t *krad_mixer, struct timespec start_time) {

	if (krad_mixer->ticker_running == 1) {
		krad_mixer_stop_ticker (krad_mixer);
	}
	memcpy (&krad_mixer->start_time, &start_time, sizeof(struct timespec));
	krad_mixer->ticker_running = 1;
	pthread_create (&krad_mixer->ticker_thread, NULL, krad_mixer_ticker_thread, (void *)krad_mixer);

}

void krad_mixer_stop_ticker (krad_mixer_t *krad_mixer) {

	if (krad_mixer->ticker_running == 1) {
		krad_mixer->ticker_running = 2;
		pthread_join (krad_mixer->ticker_thread, NULL);
		krad_mixer->ticker_running = 0;
	}

}


void krad_mixer_destroy (krad_mixer_t *krad_mixer) {

	int p;
	
  printk ("Krad Mixer shutdown started");
	
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if ((krad_mixer->portgroup[p]->active == 1) && (krad_mixer->portgroup[p]->io_type != MIXBUS)) {
			krad_mixer_portgroup_destroy (krad_mixer, krad_mixer->portgroup[p]);
		}
	}
	
	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		if (krad_mixer->portgroup[p]->active == 1) {
			krad_mixer_portgroup_destroy (krad_mixer, krad_mixer->portgroup[p]);
		}
	}
	
  krad_mixer_stop_ticker (krad_mixer);
	
	free ( krad_mixer->crossfade_group );

	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		free ( krad_mixer->portgroup[p] );
	}
	
	free ( krad_mixer->name );

	free ( krad_mixer );
	
  printk ("Krad Mixer shutdown complete");
	
}

void krad_mixer_unset_pusher (krad_mixer_t *krad_mixer) {
	if (krad_mixer->ticker_running == 1) {
		krad_mixer_stop_ticker (krad_mixer);
	}
	krad_mixer_start_ticker (krad_mixer);
	krad_mixer->pusher = 0;
}

void krad_mixer_set_pusher (krad_mixer_t *krad_mixer, krad_audio_api_t pusher) {
	if (krad_mixer->ticker_running == 1) {
		krad_mixer_stop_ticker (krad_mixer);
	}	
	krad_mixer->pusher = pusher;
}

int krad_mixer_has_pusher (krad_mixer_t *krad_mixer) {
	if (krad_mixer->pusher == 0) {
		return 0;
	} else {
		return 1;
	}
}

krad_audio_api_t krad_mixer_get_pusher (krad_mixer_t *krad_mixer) {
	return krad_mixer->pusher;
}

int krad_mixer_get_sample_rate (krad_mixer_t *krad_mixer) {
	return krad_mixer->sample_rate;
}

void krad_mixer_set_sample_rate (krad_mixer_t *krad_mixer, int sample_rate) {
	krad_mixer->sample_rate = sample_rate;
	krad_tone_set_sample_rate (krad_mixer->tone_port->io_ptr, krad_mixer->sample_rate);
	
	if (krad_mixer->ticker_running == 1) {
		krad_mixer_stop_ticker (krad_mixer);
    krad_mixer_start_ticker (krad_mixer);
	}
	
}

krad_mixer_t *krad_mixer_create (char *name) {

	int p;

	krad_mixer_t *krad_mixer;

	if ((krad_mixer = calloc (1, sizeof (krad_mixer_t))) == NULL) {
		failfast ("Krad Mixer memory alloc failure");
	}
	
	krad_mixer->name = strdup (name);
	krad_mixer->sample_rate = KRAD_MIXER_DEFAULT_SAMPLE_RATE;
	krad_mixer->rms_window_size = (krad_mixer->sample_rate / 1000) * KRAD_MIXER_RMS_WINDOW_SIZE_MS;
	krad_mixer->ticker_period = KRAD_MIXER_DEFAULT_TICKER_PERIOD;
	
	krad_mixer->crossfade_group = calloc (KRAD_MIXER_MAX_PORTGROUPS / 2, sizeof (krad_mixer_crossfade_group_t));

	for (p = 0; p < KRAD_MIXER_MAX_PORTGROUPS; p++) {
		krad_mixer->portgroup[p] = calloc (1, sizeof (krad_mixer_portgroup_t));
	}
	
	krad_mixer->krad_audio = krad_audio_create (krad_mixer);
	
	krad_mixer->master_mix = krad_mixer_portgroup_create (krad_mixer, "MasterBUS", MIX, NOTOUTPUT, 8, NULL, MIXBUS, NULL, 0);
	
	krad_mixer->tone_port = krad_mixer_portgroup_create (krad_mixer, "DTMF", INPUT, NOTOUTPUT, 1,
										 				 krad_mixer->master_mix, KRAD_TONE, NULL, 0);
	
	krad_mixer_portgroup_mixmap_channel (krad_mixer->tone_port, 0, 1);

  krad_mixer_set_portgroup_control (krad_mixer, "MasterBUS", "volume", 80.0f);
  krad_mixer_set_portgroup_control (krad_mixer, "DTMF", "volume", 70.0f);
  

	return krad_mixer;
	
}

void krad_mixer_set_ipc (krad_mixer_t *krad_mixer, krad_ipc_server_t *krad_ipc) {
	krad_mixer->krad_ipc = krad_ipc;
}

