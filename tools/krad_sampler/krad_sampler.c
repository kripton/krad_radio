#include "krad_sampler.h"

void set_start_offset(krad_sample_t *sample, float offset) {

	sample->startoffset = offset;
	
	if (sample->play == FALSE) {
		sample->ringbuffer[0]->read_ptr = sample->startoffset;
		sample->ringbuffer[1]->read_ptr = sample->startoffset;
	}

}

void sample_playback_reset(krad_sample_t *sample) {

	if (sample->loop != TRUE) {
		sample->play = FALSE;
		sample->fade = 1;
	}

	sample->ringbuffer[0]->read_ptr = sample->startoffset;
	sample->ringbuffer[1]->read_ptr = sample->startoffset;

	sample->current_sample[0] = -1;
	sample->current_sample[1] = -1;
							
}


void fade(krad_sample_t *sample, int nframes) {

	int i, j;
	int sign[2];
	
	i = 0;
	
	if (sample->current_sample[0] == -1) {
		printf("fade start\n");
		for (j = 0; j < 2; j++) {
		
			sample->current_sample[j]++;
		
			if (sample->fade != 1) {
				sample->current_fade_amount[j] = 1.0f;
			} else {
				sample->current_fade_amount[j] = 0.0f;
			}
			
			if (sample->samples[j][i] >= 0) {
				sample->lastsign[j] = 1;
			} else {
				sample->lastsign[j] = 0;
			}
		}
		
	}
	

	for (i = 0; i < nframes; i++) {
		for (j = 0; j < 2; j++) {

			if (sample->samples[j][i] >= 0) {
				sign[j] = 1;
			} else {
				sign[j] = 0;
			}
			
			if ((sign[j] != sample->lastsign[j]) && (sample->current_sample[j] < sample->fade_frame_duration + 1)) {
				if (sample->fade != 1) {
					sample->current_fade_amount[j] = ((sample->fade_frame_duration - (sample->current_sample[j])) * 100.0) / sample->fade_frame_duration;
				} else {
					sample->current_fade_amount[j] = ((sample->current_sample[j]) * 100.0) / sample->fade_frame_duration;
				}
				sample->current_fade_amount[j] /= 100.0;
				sample->current_fade_amount[j] *= sample->current_fade_amount[j];
							
				if (sample->current_sample[j] == sample->fade_frame_duration) {
					// This is the silly way we know we are done
					sample->current_sample[j]++;
				}
							
			}

			sample->samples[j][i] = sample->samples[j][i] * sample->current_fade_amount[j];
			sample->lastsign[j] = sign[j];

			if (sample->current_sample[j] < sample->fade_frame_duration) {
				sample->current_sample[j]++;
			}
					
					
		}
	}
	
	
	if ((sample->current_sample[0] == sample->fade_frame_duration + 1) && (sample->current_sample[1] == sample->fade_frame_duration + 1)) {
		sample->current_sample[0] = -1;
		sample->current_sample[1] = -1;
	
		if (sample->fade == 1) {
			sample->fade = 0;
		} else {
			if (sample->fade == 3) {
				sample_playback_reset(sample);
			} else {
				sample->fade = 1;
				sample->play = FALSE;
			}
		}
	}
		

}

void apply_volume(portgroup_t *portgroup, int nframes) {

	int c, s, sign;

	if (portgroup->type == MIC) {
		
		for (s = 0; s < nframes; s++) {
			// Turn into stereo, right channel first so the left volume doensnt control both
			portgroup->samples[1][s] = portgroup->samples[0][s] * portgroup->volume_actual[1];
			portgroup->samples[0][s] = portgroup->samples[0][s] * portgroup->volume_actual[0];

		}
	
	
	} else {	// 2 channel
		for (c = 0; c < 2; c++) {
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

}


float read_peak(portgroup_t *portgroup, int channel)
{
	float tmp = portgroup->peak[channel];
	portgroup->peak[channel] = 0.0f;

	return tmp;
}

void compute_peaks(portgroup_t *portgroup, int sample_count) {

	int i;
	
	for (i = 0; i < portgroup->channels; i++) {
		compute_peak(portgroup, i, sample_count);
	}
}

void compute_peak(portgroup_t *portgroup, int channel, int sample_count) {

	int i;
	

	for(i = 0; i < sample_count; i++) {
		const float s = fabs(portgroup->samples[channel][i]);
		if (s > portgroup->peak[channel]) {
			portgroup->peak[channel] = s;
		}
	}
}

void silent_buffer(float *buffer, int samples) {

	int i;

	for (i=0; i < samples; i++) {
		buffer[i] = 0.0f;
	}

}

void mix(float *mixbuffer, float *buffer, int samples) {

	int i;

	for (i=0; i < samples; i++) {
		mixbuffer[i] += buffer[i];
	}

}


int
crossfade_chunk(void *sample_buffer_from, void *sample_buffer_to, void *faded_sample_buffer, int sample_start_number, int samples_in_chunk, int total_samples, int sample_size, int number_of_channels) {

	/* ok lets handle those void * and hard code it to float for now, is it possible to cast without a new var?? */
	
	float *samples_from = (float*)sample_buffer_from;
	float *samples_to = (float*)sample_buffer_to;
	float *faded_samples = (float*)faded_sample_buffer;

	int current_sample_number, n;
	float crossfade_amount_per_sample, current_crossfade_amount, crossfade_starting_amount, coef_from, coef_to;
	int i, j; /* , bytes; */
	float next_fade_amount, next_in_fade_amount;
	float current_fade_amount[2][number_of_channels];
	crossfade_starting_amount = -1.0f;
	int sign[2][number_of_channels], lastsign[2][number_of_channels];
	current_sample_number = sample_start_number;

	for (n = 0; n < 2; n++) {
		for (j = 0; j < number_of_channels; j++) {
			current_fade_amount[n][j] = 0;
			sign[n][j] = 0;
			lastsign[n][j] = -1;
		}
	}
	
	//total_samples = total_samples - 1;


	for (i = 0; i < samples_in_chunk; i++) {
		for (j = 0; j < number_of_channels; j++) {




	next_in_fade_amount = cos(3.14159*0.5*(((float)(i))  + 0.5)/(float)total_samples);

	next_in_fade_amount = next_in_fade_amount * next_in_fade_amount;
	next_in_fade_amount = 1 - next_in_fade_amount;
				
	next_fade_amount = cos(3.14159*0.5*(((float)(i)) + 0.5)/(float)total_samples);

	next_fade_amount = next_fade_amount * next_fade_amount;
	
		if (current_fade_amount[0][j] == 0) {
				current_fade_amount[1][j] = next_in_fade_amount;
				current_fade_amount[0][j] = next_fade_amount;
			}
		
	
		if (samples_from[i*number_of_channels + j] >= 0) {
				sign[0][j] = 1;
			} else {
				sign[0][j] = 0;
			}
		

			if (samples_to[i*number_of_channels + j] >= 0) {
				sign[1][j] = 1;
			} else {
				sign[1][j] = 0;
			}



	for (n = 0; n < 2; n++) {	
			
			if(lastsign[n][j] == -1)
				lastsign[n][j] = sign[n][j];
			
			if (sign[n][j] != lastsign[n][j]) {
			 if(n == 1) {
			 	//XMMS_DBG("fade amount in is: %f ", next_in_fade_amount);
				current_fade_amount[n][j] = next_in_fade_amount;
			} else {
				current_fade_amount[n][j] = next_fade_amount;
			}
				//XMMS_DBG ("Zero Crossing at %d", current_sample_number);
				if(current_fade_amount[n][j] < 0)
					current_fade_amount[n][j] = 0;
				if(current_fade_amount[n][j] > 100)
					current_fade_amount[n][j] = 100;
			}

		}

			
			faded_samples[i*number_of_channels + j] = ((((samples_from[i*number_of_channels + j] * current_fade_amount[0][j]) ) + ((samples_to[i*number_of_channels + j] * current_fade_amount[1][j]) )) );
			//faded_samples[i*number_of_channels + j] = samples_from[i*number_of_channels + j] * coef_from + samples_to[i*number_of_channels + j] * coef_to;
		
			lastsign[0][j] = sign[0][j];
			lastsign[1][j] = sign[1][j];
			
			// += 1;
		
		}
	}

	return 0;

}


int jack_callback (jack_nframes_t nframes, void *arg)
{

	kraddsp_t *kraddsp = (kraddsp_t *)arg;
	
	int x, j, ret, cycle, s, i2, i, pg;

	portgroup_t *portgroup = NULL;
	portgroup_t *portgroup2 = NULL;
	
	krad_sample_t *sample = NULL;
	krad_sinewave_t *sinewave = NULL;
	
	/*
	
	cur_sample = cur_sample + nframes;

	if (cur_sample == 4096) {
		kiss_fft( cfg , inf , outf );
		cur_sample = 0;
	}
	
	*/
	


	// Gets input/output port buffers and hardlimits inputs
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = kraddsp->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if (portgroup->type == SYNTH) {
			portgroup->samples[0] = jack_port_get_buffer (portgroup->output_ports[0], nframes);
			portgroup->samples[1] = jack_port_get_buffer (portgroup->output_ports[1], nframes);
	
			for (i = 0; i < MAX_SINES; i++) {
				
				if (portgroup->synth->sinewave[i]->active == 1) {

					sinewave = portgroup->synth->sinewave[i];

					for(s=0; s<nframes; s++) {

						sinewave->ramp += sinewave->note_frqs[sinewave->freq];
				
						if (sinewave->ramp > 1.0) {
							sinewave->ramp = sinewave->ramp - 2.0;
							if (sinewave->new_freq != sinewave->freq) {
								sinewave->freq = sinewave->new_freq;
							}
						}
				
						if (sinewave->active != sinewave->new_state) {

						
							if (1.0*sin(2*M_PI*sinewave->ramp) > 0.0f) {
								sinewave->sign = 1;
							} else {
								sinewave->sign = -1;
							}

							if ((sinewave->last_sign != 0) && (sinewave->sign != sinewave->last_sign)) {
								sinewave->active = sinewave->new_state;
								sinewave->last_sign = 0;
								sinewave->sign = 0;
								if (sinewave->active == 2) {
									for (; s<nframes; s++) {
										sinewave->samples[0][s] = 0.0f;
										sinewave->samples[1][s] = 0.0f;
									}
									break;
								}
							} else {
								sinewave->last_sign = sinewave->sign;
							}
						}

						sinewave->samples[0][s] = 1.0*sin(2*M_PI*sinewave->ramp);
						sinewave->samples[1][s] = 1.0*sin(2*M_PI*sinewave->ramp);
				
					}
					
					if (i > 0) {
						mix(portgroup->samples[0], sinewave->samples[0], nframes);
						mix(portgroup->samples[1], sinewave->samples[1], nframes);
					} else {
						memcpy((char *)portgroup->samples[0], (char *)sinewave->samples[0], nframes * 4);
						memcpy((char *)portgroup->samples[1], (char *)sinewave->samples[1], nframes * 4);
					}
					
					if (sinewave->active == 2) {
						sinewave->active = 0;
					}
					
				}
			
			}
			
			//apply_volume(portgroup, nframes);
		}
		
		if ((portgroup->type == SAMPLER) && (portgroup->direction == INPUT)) {

			portgroup->samples[0] = jack_port_get_buffer (portgroup->input_ports[0], nframes);
			portgroup->samples[1] = jack_port_get_buffer (portgroup->input_ports[1], nframes);
			
			for (i = 0; i < MAX_SAMPLES; i++) {
			
				if (portgroup->sampler->sample[i] != NULL) {
				
				if (portgroup->sampler->sample[i]->active == 1) {
				
					sample = portgroup->sampler->sample[i];
				
					if (sample->record == TRUE) {
						
						//sample->length = 0;
						sample->length = sample->ringbuffer[1]->write_ptr / 4;
						
						if (sample->ringbuffer[1]->read_ptr > sample->ringbuffer[1]->write_ptr) {
							if (sample->ringbuffer[0]->write_ptr - 8 > -1) {
								sample->ringbuffer[0]->read_ptr = sample->ringbuffer[0]->write_ptr - 8;
								sample->ringbuffer[1]->read_ptr = sample->ringbuffer[1]->write_ptr - 8;
							} else {
								sample->ringbuffer[0]->read_ptr = 0;
								sample->ringbuffer[1]->read_ptr = 0;
							}
						}
			
						if (sample->write_seek != -1) {
				
							if ((sample->write_seek > -1) && (sample->write_seek < SAMPLE_RINGBUF_SIZE / 4)) {
								sample->ringbuffer[0]->write_ptr = sample->write_seek * 4;
								sample->ringbuffer[1]->write_ptr = sample->write_seek * 4;
							}
							sample->write_seek = -1;
						}
			
		
						if (jack_ringbuffer_write_space (sample->ringbuffer[1]) >= nframes * 4) {
							jack_ringbuffer_write (sample->ringbuffer[0], (char *)portgroup->samples[0], nframes * 4);
							jack_ringbuffer_write (sample->ringbuffer[1], (char *)portgroup->samples[1], nframes * 4);
						} else {
							// loopr here
							if (FALSE) { 
							} else {
								sample->record = FALSE;
								//portgroup->sample->ringbuffer[0]->read_ptr = portgroup->sample->ringbuffer[0]->write_ptr;
								//portgroup->sample->ringbuffer[1]->read_ptr = portgroup->sample->ringbuffer[1]->write_ptr;
							}
							//silent_buffer(portgroup->output_ports[0], nframes);
							//silent_buffer(portgroup->output_ports[1], nframes);
						}
						//apply_volume(portgroup, nframes);
					} else {
					//silent_buffer(portgroup->output_ports[0], nframes);
					//silent_buffer(portgroup->output_ports[1], nframes);
					}
				}
				}
			}
		}
		
		if ((portgroup->type == SAMPLER) && (portgroup->direction == OUTPUT)) {
			portgroup->samples[0] = jack_port_get_buffer (portgroup->output_ports[0], nframes);
			portgroup->samples[1] = jack_port_get_buffer (portgroup->output_ports[1], nframes);
			
			silent_buffer(portgroup->samples[0], nframes);
			silent_buffer(portgroup->samples[1], nframes);
			
			for (i = 0; i < MAX_SAMPLES; i++) {
			
				if (portgroup->sampler->sample[i] != NULL) {
				if (portgroup->sampler->sample[i]->active == 1) {
				
					sample = portgroup->sampler->sample[i];
			
					if (sample->play == TRUE) {
			
						//if (sample->length == 0) {
						//	sample->length = jack_ringbuffer_read_space (sample->ringbuffer[1]) / 4;
						//}
			
			
						if (sample->seek != -1) {
				
							if ((sample->seek > -1) && (sample->seek < sample->length)) {
							
								if (sample->seek < sample->startoffset) {
									sample->seek = sample->startoffset;
								}
								
								if (sample->seek > sample->stopoffset) {
									sample->seek = sample->stopoffset;
								}
							
								sample->ringbuffer[0]->read_ptr = sample->seek * 4;
								sample->ringbuffer[1]->read_ptr = sample->seek * 4;
							}
							sample->seek = -1;
						}
						
						
						if (sample->crossfade_sample_count > 0) {
								
							memcpy((char *)sample->samples[0], (char *)sample->crossfaded_samples[0] + ((sample->crossfade_sample_count - sample->crossfaded_samples_left) * 4), nframes * 4);
							memcpy((char *)sample->samples[1], (char *)sample->crossfaded_samples[1] + ((sample->crossfade_sample_count - sample->crossfaded_samples_left) * 4), nframes * 4);
							
							mix(portgroup->samples[0], sample->samples[0], nframes);
							mix(portgroup->samples[1], sample->samples[1], nframes);
							
							sample->crossfaded_samples_left = sample->crossfaded_samples_left - nframes;
								
							if (sample->crossfaded_samples_left == 0) {
								sample->crossfade_sample_count = 0;
							}
								
						} else {
						
		
						if (jack_ringbuffer_read_space (sample->ringbuffer[1]) < nframes * 4) {
							sample_playback_reset(sample);							
						}
						
						if ((sample->play) && (jack_ringbuffer_read_space (sample->ringbuffer[1]) >= nframes * 4)) {
						
							if ((sample->loop == 1) && ((jack_ringbuffer_read_space (sample->ringbuffer[1]) / 4) <= sample->stopoffset + (sample->crossfade_frame_duration / 2))) {
							
								if (sample->crossfade_sample_count == 0) {
								
								jack_ringbuffer_read (sample->ringbuffer[0], (char *)sample->crossfade_samples1[0], jack_ringbuffer_read_space (sample->ringbuffer[0]) - (sample->stopoffset * 4));
								sample->crossfade_sample_count = jack_ringbuffer_read (sample->ringbuffer[1], (char *)sample->crossfade_samples1[1], jack_ringbuffer_read_space (sample->ringbuffer[1]) - (sample->stopoffset * 4))  / 4;

								sample_playback_reset(sample);
								
								jack_ringbuffer_read (sample->ringbuffer[0], (char *)sample->crossfade_samples2[0], sample->crossfade_sample_count * 4);
								sample->crossfade_sample_count2 = jack_ringbuffer_read (sample->ringbuffer[1], (char *)sample->crossfade_samples2[1], sample->crossfade_sample_count * 4)  / 4;
					
								crossfade_chunk(sample->crossfade_samples1[0], sample->crossfade_samples2[0], sample->crossfaded_samples[0], 0, sample->crossfade_sample_count, sample->crossfade_sample_count, 4, 1);
								crossfade_chunk(sample->crossfade_samples1[1], sample->crossfade_samples2[1], sample->crossfaded_samples[1], 0, sample->crossfade_sample_count, sample->crossfade_sample_count, 4, 1);
					
								sample->crossfaded_samples_left = sample->crossfade_sample_count - nframes;
								
								printf("situation is: 1 %d - 2 %d ==== left: %d count %d - nframes %d \n", sample->crossfade_sample_count, sample->crossfade_sample_count2, sample->crossfaded_samples_left, sample->crossfade_sample_count, nframes);
								
								memcpy((char *)sample->samples[0], (char *)sample->crossfaded_samples[0], nframes * 4);
								memcpy((char *)sample->samples[1], (char *)sample->crossfaded_samples[1], nframes * 4);
					
								}
				
								
							
							} else {
							
								jack_ringbuffer_read (sample->ringbuffer[0], (char *)sample->samples[0], nframes * 4);
								jack_ringbuffer_read (sample->ringbuffer[1], (char *)sample->samples[1], nframes * 4);
						
								if (sample->loop == 0) {
							
								if ((sample->fade == 0) && ((jack_ringbuffer_read_space (sample->ringbuffer[1]) / 4) <= sample->stopoffset + sample->fade_frame_duration)) {
									sample->fade = 3;
									// fade
									sample->current_sample[0] = -1;
									sample->current_sample[1] = -1;
								}
								
								}
							
								if (sample->fade != 0) {
									fade(sample, nframes);
								}
								
								
							
							}

							mix(portgroup->samples[0], sample->samples[0], nframes);
							mix(portgroup->samples[1], sample->samples[1], nframes);
						}	
					}
					}
				} else {
					if (portgroup->sampler->sample[i]->active == 2) {
						portgroup->sampler->sample[i]->active = 0;
					}
				}
				}
			}
			
			//apply_volume(portgroup, nframes);
			
		}
		
		if (portgroup->active == 2) {
			portgroup->active = 0;
		}
		
		}
	}

	return 0;      

}


void *krad_sample_encoding_thread(void *arg) { 

	krad_sample_t *sample = (krad_sample_t *)arg;
	
	
	sample->saving = 1;
	
	
	sample->flac_encoder = FLAC__stream_encoder_new();
	

	FLAC__stream_encoder_set_channels (sample->flac_encoder, sample->sampler->output_portgroup->channels);
	FLAC__stream_encoder_set_bits_per_sample (sample->flac_encoder, 24);
	FLAC__stream_encoder_set_sample_rate (sample->flac_encoder, 44100);
	FLAC__stream_encoder_set_compression_level	(sample->flac_encoder, 5);

	char *homedir = getenv ("HOME");
	
	time_t     now;
    struct tm  *ts;
    char       buf[80];
 
    /* Get the current time */
    now = time(0);
 
    /* Format and print the time, "ddd yyyy-mm-dd hh:mm:ss zzz" */
    ts = localtime(&now);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    printf("kraddsp sample Saving sample at: %s\n", buf);
    strftime(buf, sizeof(buf), "%a_%Y-%m-%d_%H.%M.%S_%Z", ts);
	sprintf(sample->filename, "%s/krad/recordings/sampler/%s_%s.flac", homedir, sample->sampler->kraddsp->station_name, buf);
    printf("kraddsp sample using filename: %s\n", sample->filename);
	FLAC__StreamEncoderInitStatus ret = FLAC__stream_encoder_init_file(sample->flac_encoder, sample->filename, NULL, sample);

	if (ret == FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		//printf("Flac is good\n");
	}
	
	int ringcount;
	int numsamples = 2048; // per channel...	
	int processing_size = numsamples * 4;
	int *int32_samples[8];
	float *float_samples;
	int i, c;


	float_samples = malloc(processing_size);
	

	
	
		for (c = 0; c < sample->sampler->input_portgroup->channels; c++) {
			int32_samples[c] = malloc(processing_size);
			printf("kraddsp sample alloc %d bytes\n", processing_size);
		}
	
		//printf("kraddsp sample Started saving at: %ju\n", kradrecorder->recording_time);
	

		while (jack_ringbuffer_read_space (sample->ringbuffer[sample->sampler->input_portgroup->channels - 1]) > 0 ) {




			int bitshifter = 8;

			double scaled_value;



			for (c = 0; c < sample->sampler->input_portgroup->channels; c++) {
	
				ringcount = jack_ringbuffer_read (sample->ringbuffer[c], (char *)float_samples, processing_size );

				if (ringcount != processing_size) {
					printf("kraddsp sample jack ringbuffer final samples\n");
					//exit(1);
				}
	
				for (i = 0; i < numsamples; i++) {
					scaled_value = (float_samples[i] * (8.0 * 0x10000000));
					int32_samples[c][i] = (int) (lrint (scaled_value) >> bitshifter) ;
				}
			}

			//printf("lala %d %d %5zu %d %f\n", c, i, jack_ringbuffer_read_space (portgroup->ringbuf[portgroup->channels - 1]), int32_samples[1][444], float_samples[444]);



			FLAC__stream_encoder_process(sample->flac_encoder, (const FLAC__int32 * const *)int32_samples, numsamples);

		}

		FLAC__stream_encoder_finish(sample->flac_encoder);
		FLAC__stream_encoder_delete	(sample->flac_encoder);

		free(float_samples);
		printf("kraddsp sample freeeed %d bytes\n", processing_size);
		
		for (c = 0; c < sample->sampler->input_portgroup->channels; c++) {
			free(int32_samples[c]);
			printf("kraddsp sample freed %d bytes\n", processing_size);
		}

		sample->saved = 1;
		sample->saving = 0;
	//}

}

static void
flac_decoder_callback_error (const FLAC__StreamDecoder *flacdecoder,
							 FLAC__StreamDecoderErrorStatus status,
							 void *client_data)
{
	printf ("%s\n", FLAC__StreamDecoderErrorStatusString[status]);
}
 
void
int24_to_float_array (const int *in, float *out, int len)
{
	while (len)
	{	len -- ;
		out [len] = (float) ((in [len] << 8) / (8.0 * 0x10000000)) ;
		} ;

	return ;
}


static FLAC__StreamDecoderWriteStatus
flac_decoder_callback_write (const FLAC__StreamDecoder *flacdecoder,
                     const FLAC__Frame *frame,
                     const FLAC__int32 * const fbuffer[],
                     void *client_data)
{

	krad_sample_t *sample = (krad_sample_t *)client_data;

	float float_samples[2][10000];
	
	int i, j;
	
	for (i = 0; i < 2; i++) {
		int24_to_float_array ((const int *)fbuffer[i], float_samples[i], frame->header.blocksize);
	}
	

	if (jack_ringbuffer_write_space (sample->ringbuffer[1]) < frame->header.blocksize * 4) {
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	} else {
		for (i = 0; i < 2; i++) {
			jack_ringbuffer_write (sample->ringbuffer[i], (char *)float_samples[i], frame->header.blocksize * 4);
		}
		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

}


void *krad_sample_decoding_thread(void *arg) { 

	krad_sample_t *sample = (krad_sample_t *)arg;

	int i;
	FLAC__StreamDecoderInitStatus ret;



	sample->flac_decoder = FLAC__stream_decoder_new();

	ret = FLAC__stream_decoder_init_file (sample->flac_decoder, sample->filename,
											flac_decoder_callback_write,
											NULL, flac_decoder_callback_error, sample );


	if (ret == FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		FLAC__stream_decoder_process_until_end_of_stream (sample->flac_decoder);
	} else {
		fprintf(stderr, "FLAC Decoder init error\n");
	}

	FLAC__stream_decoder_process_until_end_of_stream (sample->flac_decoder);

	FLAC__stream_decoder_delete	(sample->flac_decoder);

}


void sample_load(krad_sample_t *sample, char *filename) {

	if (true) { //(sampler->loading == 0) { //&& (sampler->saved == 0)) {
	
		if (sample->play == TRUE) {
			sample->play = FALSE;
			usleep(50000);
		}
		
		if (sample->record == TRUE) {
			sample->record = FALSE;
			usleep(50000);
		}
		
		if (sample->write_seek != -1) {
				
			if ((sample->write_seek > -1) && (sample->write_seek < SAMPLE_RINGBUF_SIZE / 4)) {
				sample->ringbuffer[0]->write_ptr = sample->write_seek * 4;
				sample->ringbuffer[1]->write_ptr = sample->write_seek * 4;
			}
			sample->write_seek = -1;
		}
	
		sample->ringbuffer[0]->read_ptr = 0;
		sample->ringbuffer[1]->read_ptr = 0;
		strcpy(sample->filename, filename);
		pthread_create(&sample->decoder_thread, NULL, krad_sample_decoding_thread, (void *)sample);
		pthread_detach(sample->decoder_thread);
	}
}

void sample_save(krad_sample_t *sample) {

	if ((sample->saving == 0) && (sample->saved == 0)) {
	
		if (sample->play == TRUE) {
			sample->play = FALSE;
			usleep(50000);
		}
	
		sample->ringbuffer[0]->read_ptr = 0;
		sample->ringbuffer[1]->read_ptr = 0;
	
		pthread_create(&sample->encoder_thread, NULL, krad_sample_encoding_thread, (void *)sample);
		pthread_detach(sample->encoder_thread);
	}
}

void set_crossfade_duration(krad_sample_t *sample, int duration) {

	sample->crossfade_frame_duration = duration;
	
	free(sample->crossfade_samples1[0]);
	free(sample->crossfade_samples1[1]);
	
	free(sample->crossfade_samples2[0]);
	free(sample->crossfade_samples2[1]);
	
	free(sample->crossfaded_samples[0]);
	free(sample->crossfaded_samples[1]);
	
	sample->crossfade_samples1[0] = malloc(4 * sample->crossfade_frame_duration);
	sample->crossfade_samples1[1] = malloc(4 * sample->crossfade_frame_duration);
	
	sample->crossfade_samples2[0] = malloc(4 * sample->crossfade_frame_duration);
	sample->crossfade_samples2[1] = malloc(4 * sample->crossfade_frame_duration);
	
	sample->crossfaded_samples[0] = malloc(4 * sample->crossfade_frame_duration);
	sample->crossfaded_samples[1] = malloc(4 * sample->crossfade_frame_duration);

}

reset_sample(krad_sample_t *sample) {

	jack_ringbuffer_reset(sample->ringbuffer[0]);
	jack_ringbuffer_reset(sample->ringbuffer[1]);
	sample->fade = 1;
	sample->fade_frame_duration = 7000;
	sample->crossfade_frame_duration = 10240;
	sample->seek = -1;
	sample->startoffset = 0;
	sample->stopoffset = 0;
	sample->write_seek = -1;
}

int add_sample(krad_sampler_t *sampler) {

	int i;
	
	for (i = 0; i < MAX_SAMPLES; i++) {
		if (sampler->sample[i] == NULL) {
			break;
		}
	}

	sampler->sample[i] = calloc(1, sizeof(krad_sample_t));
	sampler->sample[i]->fade = 1;
	sampler->sample[i]->fade_frame_duration = 7000;
	sampler->sample[i]->crossfade_frame_duration = 10240;
	sampler->sample[i]->seek = -1;
	sampler->sample[i]->startoffset = 0;
	sampler->sample[i]->stopoffset = 0;
	sampler->sample[i]->write_seek = -1;
	sampler->sample[i]->sampler = sampler;
	
	sampler->sample[i]->current_sample[0] = -1;
	sampler->sample[i]->current_sample[1] = -1;
	
	sampler->sample[i]->samples[0] = malloc(4 * 1024);
	sampler->sample[i]->samples[1] = malloc(4 * 1024);
	
	sampler->sample[i]->crossfade_samples1[0] = malloc(4 * sampler->sample[i]->crossfade_frame_duration);
	sampler->sample[i]->crossfade_samples1[1] = malloc(4 * sampler->sample[i]->crossfade_frame_duration);
	
	sampler->sample[i]->crossfade_samples2[0] = malloc(4 * sampler->sample[i]->crossfade_frame_duration);
	sampler->sample[i]->crossfade_samples2[1] = malloc(4 * sampler->sample[i]->crossfade_frame_duration);
	
	sampler->sample[i]->crossfaded_samples[0] = malloc(4 * sampler->sample[i]->crossfade_frame_duration);
	sampler->sample[i]->crossfaded_samples[1] = malloc(4 * sampler->sample[i]->crossfade_frame_duration);
	
	sampler->sample[i]->ringbuffer[0] = jack_ringbuffer_create (SAMPLE_RINGBUF_SIZE);
	sampler->sample[i]->ringbuffer[1] = jack_ringbuffer_create (SAMPLE_RINGBUF_SIZE);

	if (jack_ringbuffer_mlock(sampler->sample[i]->ringbuffer[0])) {
		printf("kraddsp sample ringbuffer mlock failure\n");
	}
	
	if (jack_ringbuffer_mlock(sampler->sample[i]->ringbuffer[1])) {
		printf("kraddsp sample ringbuffer mlock failure\n");
	}

	sampler->sample[i]->active = 1;
	
	return i;

}

void destroy_sample(krad_sample_t *sample) {

	sample->active = 2;

	while (sample->active != 0) {
		usleep(20000);
	}

	free(sample->samples[0]);
	free(sample->samples[1]);
	
	free(sample->crossfade_samples1[0]);
	free(sample->crossfade_samples1[1]);
	
	free(sample->crossfade_samples2[0]);
	free(sample->crossfade_samples2[1]);
	
	free(sample->crossfaded_samples[0]);
	free(sample->crossfaded_samples[1]);

	jack_ringbuffer_free (sample->ringbuffer[0]);
	jack_ringbuffer_free (sample->ringbuffer[1]);
	free(sample);
	
	sample = NULL;
	
}


krad_sampler_t *create_sampler(kraddsp_t *kraddsp, const char *name) {

	krad_sampler_t *sampler = calloc(1, sizeof(krad_sampler_t));
	
	sampler->output_portgroup = create_portgroup(kraddsp, "samplerout", OUTPUT, STEREO, SAMPLER, 0);
	sampler->output_portgroup->sampler = sampler;
	
	sampler->input_portgroup = create_portgroup(kraddsp, "samplerin", INPUT, STEREO, SAMPLER, 0);
	sampler->input_portgroup->sampler = sampler;
	
	sampler->kraddsp = kraddsp;
	
	int i;
	
	for (i = 0; i < MAX_SAMPLES; i++) {
		sampler->sample[i] = NULL;
	}

	add_sample(sampler);
	request_mixer_portgroup(sampler->kraddsp, "sampler");
	
	char portname1[256];
	char portname2[256];
	
	sprintf(portname1, "%s:samplerout_right", kraddsp->client_name);
	sprintf(portname2, "kradmixer_%s:sampler_right", kraddsp->station_name);
	connect_port(kraddsp->jack_client, portname1, portname2);
	
	sprintf(portname1, "%s:samplerout_left", kraddsp->client_name);
	sprintf(portname2, "kradmixer_%s:sampler_left", kraddsp->station_name);
	connect_port(kraddsp->jack_client, portname1, portname2);
	
	sprintf(portname1, "kradmixer_%s:main_right", kraddsp->station_name);
	sprintf(portname2, "%s:samplerin_right", kraddsp->client_name);
	connect_port(kraddsp->jack_client, portname1, portname2);
	
	sprintf(portname1, "kradmixer_%s:main_left", kraddsp->station_name);
	sprintf(portname2, "%s:samplerin_left", kraddsp->client_name);
	connect_port(kraddsp->jack_client, portname1, portname2);
	
	sampler->input_portgroup->active = 1;
	sampler->output_portgroup->active = 1;
	
	return sampler;

}


void destroy_sampler(krad_sampler_t *sampler) {


	int i;
	
	for (i = 0; i < MAX_SAMPLES; i++) {
		if (sampler->sample[i] != NULL) {
			destroy_sample(sampler->sample[i]);
		}
	}
	destroy_mixer_portgroup(sampler->kraddsp, "sampler");
	destroy_portgroup (sampler->kraddsp, sampler->input_portgroup);
	destroy_portgroup (sampler->kraddsp, sampler->output_portgroup);
	free(sampler);

}


void request_mixer_portgroup(kraddsp_t *kraddsp, char *name) {

	kradmixer_ipc_client_t *mixer = NULL;
	
	mixer = kradmixer_connect(kraddsp->station_name);
	if (mixer != NULL) {
		kradmixer_create_portgroup(mixer, name);
		kradmixer_disconnect(mixer);
	}
	mixer = NULL;


}

void destroy_mixer_portgroup(kraddsp_t *kraddsp, char *name) {

	kradmixer_ipc_client_t *mixer = NULL;
	
	mixer = kradmixer_connect(kraddsp->station_name);
	if (mixer != NULL) {
		kradmixer_destroy_portgroup(mixer, name);
		kradmixer_disconnect(mixer);
	}
	mixer = NULL;


}


void destroy_portgroup (kraddsp_t *kraddsp, portgroup_t *portgroup) {


	portgroup->active = 2;
	
	while (portgroup->active != 0) {
		usleep(20000);
	}

	if (portgroup->direction == INPUT) {

		if (portgroup->channels == MONO) {

			printf("Krad DSP Removing Mono Input Portgroup %s\n", portgroup->name);

			printf("Krad DSP Disconnecting Input Port\n");
			jack_port_disconnect(kraddsp->jack_client, portgroup->input_port);
			printf("Krad DSP Unregistering Input port\n");
			jack_port_unregister(kraddsp->jack_client, portgroup->input_port);
	
		} else {
	
			printf("Krad DSP Removing Stereo Input Portgroup %s\n", portgroup->name);

			printf("Krad DSP Disconnecting Left Input Port\n");
			jack_port_disconnect(kraddsp->jack_client, portgroup->input_ports[0]);
			printf("Krad DSP Unregistering Left Input port\n");
			jack_port_unregister(kraddsp->jack_client, portgroup->input_ports[0]);
			
			printf("Krad DSP Disconnecting Right Input Port\n");
			jack_port_disconnect(kraddsp->jack_client, portgroup->input_ports[1]);
			printf("Krad DSP Unregistering Right Input port\n");
			jack_port_unregister(kraddsp->jack_client, portgroup->input_ports[1]);
	
		}
	
	
	} else {
	
		printf("Krad DSP Removing Output Portgroup %s\n", portgroup->name);
		
		printf("Krad DSP Disconnecting Left Output Port\n");
		jack_port_disconnect(kraddsp->jack_client, portgroup->output_ports[0]);
		printf("Krad DSP Unregistering Left Output port\n");
		jack_port_unregister(kraddsp->jack_client, portgroup->output_ports[0]);
			
		printf("Krad DSP Disconnecting Right Output Port\n");
		jack_port_disconnect(kraddsp->jack_client, portgroup->output_ports[1]);
		printf("Krad DSP Unregistering Right Output port\n");
		jack_port_unregister(kraddsp->jack_client, portgroup->output_ports[1]);	

	
	}
	int i;
	
	for(i = 0; i < PORTGROUP_MAX; i++) {
		if (kraddsp->portgroup[i] == portgroup) {
			free(kraddsp->portgroup[i]);
			kraddsp->portgroup[i] = NULL;
			break;
		}
	}
	
	portgroup = NULL;
}


portgroup_t *create_portgroup(kraddsp_t *kraddsp, const char *name, int direction, int channels, int type, int group) {

	portgroup_t *portgroup;
	
		
	int i;
	
	for(i = 0; i < PORTGROUP_MAX; i++) {
		if (kraddsp->portgroup[i] == NULL) {
			kraddsp->portgroup[i] = calloc (1, sizeof (portgroup_t));
			portgroup = kraddsp->portgroup[i];
			if (i < PORTGROUP_MAX - 1) { portgroup->next = kraddsp->portgroup[i + 1]; }
			break;
		}
	}
	
	char portname[256] = "";
	
	strcpy(portname, name);

	strcpy(portgroup->name, name);
	portgroup->channels = channels;
	portgroup->type = type;
	portgroup->group = group;
	portgroup->direction = direction;

	portgroup->monitor = 0;
	

	

	for(i = 0; i< 8; i++) {
	
		// temp default volume
		portgroup->volume[i] = 100;
	
	
		portgroup->volume_actual[i] = (float)(portgroup->volume[i]/100.0f);
		portgroup->volume_actual[i] *= portgroup->volume_actual[i];
		portgroup->new_volume_actual[i] = portgroup->volume_actual[i];
	}

	strcpy(portname, name);


	if (portgroup->direction == INPUT) {


		

		if (portgroup->channels == MONO) {
			portgroup->input_port = jack_port_register (kraddsp->jack_client, portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsInput, 0);

			portgroup->input_ports[0] = portgroup->input_port;

			if (portgroup->input_port == NULL) {
				fprintf(stderr, "no more JACK ports available\n");
				exit (1);
			}
			
			//Allocate second buffer for turning mono into stereo since jack will give us one per port
			
			portgroup->samples[1] = malloc(2048 * sizeof(float));
			
		} else {
		
			strcat(portname, "_left");

			portgroup->input_ports[0] = jack_port_register (kraddsp->jack_client, portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsInput, 0);
			if (portgroup->input_ports[0] == NULL) {
				fprintf(stderr, "no more JACK ports availabl\n");
				exit (1);
			}
			
			strcpy(portname, name);
			strcat(portname, "_right");

			portgroup->input_ports[1] = jack_port_register (kraddsp->jack_client, portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsInput, 0);
			if (portgroup->input_ports[1] == NULL) {
				fprintf(stderr, "no more JACK ports available\n");
				exit (1);
			}
																					  
		
		}

	} else {
	
	
		strcat(portname, "_left");

		portgroup->output_ports[0] = jack_port_register (kraddsp->jack_client, portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsOutput, 0);

		if (portgroup->output_ports[0] == NULL) {
			fprintf(stderr, "no more JACK ports available\n");
			exit (1);
		}

		strcpy(portname, name);
		strcat(portname, "_right");

		portgroup->output_ports[1] = jack_port_register (kraddsp->jack_client, portname,
																					  JACK_DEFAULT_AUDIO_TYPE,
																					  JackPortIsOutput, 0);
																					  
		if (portgroup->output_ports[1] == NULL) {
			fprintf(stderr, "no more JACK ports available\n");
			exit (1);
		}


}


	//portgroup->active = 1;

	return portgroup;

}


kraddsp_t *kraddsp_setup(char *jack_name_suffix) {

	// Setup our vars

	//char jack_client_name_string[48] = "";

	kraddsp_t *kraddsp;

	if ((kraddsp = calloc (1, sizeof (kraddsp_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}
	
//	if ((kraddsp->portgroup = calloc (PORTGROUP_MAX, sizeof (portgroup_t))) == NULL) {
//			fprintf(stderr, "mem alloc fail\n");
//				exit (1);
//	}

	
	if (jack_name_suffix != NULL) {
		strcat(kraddsp->client_name, "kraddsp_");
		strcat(kraddsp->client_name, jack_name_suffix);
		strcpy(kraddsp->station_name, jack_name_suffix);
	} else {
		strcpy(kraddsp->client_name, "kraddsp");
	}
	
	kraddsp->server_name = NULL;
	kraddsp->options = JackNoStartServer;
	
	kraddsp->userdata = kraddsp;

	// Connect up to the JACK server 

	kraddsp->jack_client = jack_client_open (kraddsp->client_name, kraddsp->options, &kraddsp->status, kraddsp->server_name);
	if (kraddsp->jack_client == NULL) {
			fprintf(stderr, "jack_client_open() failed, "
			"status = 0x%2.0x\n", kraddsp->status);
		if (kraddsp->status & JackServerFailed) {
			fprintf(stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	
	if (kraddsp->status & JackServerStarted) {
		fprintf(stderr, "JACK server started\n");
	}

	if (kraddsp->status & JackNameNotUnique) {
		strcpy(kraddsp->client_name, jack_get_client_name(kraddsp->jack_client));
		fprintf(stderr, "unique name `%s' assigned\n", kraddsp->client_name);
	}

	// Set up Callbacks

	jack_set_process_callback (kraddsp->jack_client, jack_callback, kraddsp->userdata);
	jack_on_shutdown (kraddsp->jack_client, jack_shutdown, kraddsp->userdata);

	jack_set_xrun_callback (kraddsp->jack_client, xrun_callback, kraddsp->userdata);

	// Activate

	if (jack_activate (kraddsp->jack_client)) {
		printf("cannot activate client\n");
		exit (1);
	}
	
	
	return kraddsp;
	
}



void example_session(kraddsp_t *kraddsp) {


	// Set up port groups.

	//create_portgroup(kraddsp, "sinewave", OUTPUT, STEREO, SYNTH, 0);
	create_synth(kraddsp, "synth");
	create_sampler(kraddsp, "sampler");
	
}

void listcontrols(kraddsp_client_t *client, char *data) {

	int i, x, pg;
	
	portgroup_t *portgroup = NULL;
	
	x = 0;
	
	int sampler_count, synth_count;
	sampler_count = 0;
	synth_count = 0;
	
	x += sprintf(data + x, "kraddsp_state:");
	
	//for (portgroup = client->kraddsp->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = client->kraddsp->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		if (portgroup->type == SYNTH) {
			synth_count++;
		}
		if (portgroup->type == SAMPLER) {
			sampler_count++; // this comes out as two because it has both input and output ports
		}
	}
	}
	
	x += sprintf(data + x, "/dsp/master/add_synth|%d|%d|%d*", 0, 1, synth_count);
	x += sprintf(data + x, "/dsp/master/add_sampler|%d|%d|%d*", 0, 1, sampler_count);
	

	//for (portgroup = client->kraddsp->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = client->kraddsp->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
		
		if ((portgroup->type == SYNTH) && (portgroup->direction == OUTPUT)) {

			//x += sprintf(data + x, "/synth/master/add_sinewave|%d|%d|%d*", 0, 1, 0);

			for (i = 0; i < MAX_SINES; i++) {
			
				if (portgroup->synth->sinewave[i] != NULL) {
					x += sprintf(data + x, "/synth/%d/freq|%d|%d|%d*", i, 0, 127,portgroup->synth->sinewave[i]->freq);
					x += sprintf(data + x, "/synth/%d/active|%d|%d|%d*", i, 0, 1, portgroup->synth->sinewave[i]->active);
					//x += sprintf(data + x, "/synth/%d/remove|%d|%d|%d*", i, 0, 1, 0);
				}
			}
			
			x += sprintf(data + x, "/synth/master/remove|%d|%d|%d*", 0, 1, 0);
		}
		
		if ((portgroup->type == SAMPLER) && (portgroup->direction == OUTPUT)) { 

			x += sprintf(data + x, "/sampler/master/add_sample|%d|%d|%d*", 0, 1, 0);
			x += sprintf(data + x, "/sampler/master/list_samples|%d|%d|%d*", 0, 1, 0);

			for (i = 0; i < MAX_SAMPLES; i++) {
			
				if (portgroup->sampler->sample[i] != NULL) {
					x += sprintf(data + x, "/sampler/%d/crossfadelength|%d|%d|%d*", i, 0, 999, portgroup->sampler->sample[i]->crossfade_frame_duration);
					x += sprintf(data + x, "/sampler/%d/startoffset|%d|%d|%d*", i, 0, 999, portgroup->sampler->sample[i]->startoffset);
					x += sprintf(data + x, "/sampler/%d/stopoffset|%d|%d|%d*", i, 0, 999, portgroup->sampler->sample[i]->stopoffset);
					x += sprintf(data + x, "/sampler/%d/length|%d|%d|%d*", i, 0, 999, portgroup->sampler->sample[i]->length);
					x += sprintf(data + x, "/sampler/%d/play|%d|%d|%d*", i, 0, 1, portgroup->sampler->sample[i]->play);
					x += sprintf(data + x, "/sampler/%d/loop|%d|%d|%d*", i, 0, 1, portgroup->sampler->sample[i]->loop);
					x += sprintf(data + x, "/sampler/%d/record|%d|%d|%d*", i, 0, 1, portgroup->sampler->sample[i]->record);
					x += sprintf(data + x, "/sampler/%d/writeseek|%d|%d|%d*", i, 0, 999, portgroup->sampler->sample[i]->write_seek);
					x += sprintf(data + x, "/sampler/%d/seek|%d|%d|%d*", i, 0, 999, portgroup->sampler->sample[i]->seek);
					x += sprintf(data + x, "/sampler/%d/load|%d|%d|%d*", i, 0, 1, 0);
					x += sprintf(data + x, "/sampler/%d/save|%d|%d|%d*", i, 0, 1, 0);
					x += sprintf(data + x, "/sampler/%d/remove|%d|%d|%d*", i, 0, 1, 0);
				}
			}
			
			x += sprintf(data + x, "/sampler/master/remove|%d|%d|%d*", 0, 1, 0);
		}

	}
	}

}


int setcontrol(kraddsp_client_t *client, char *data, float value) {

	char buffer[128];
	char *unit, *subunit, *control;
	portgroup_t *portgroup = NULL;

	float volume_temp;
	krad_sample_t *asample;
	int sample;
	int sine;
	int i;
	int pos = 0;
	int value_int;
	int pg;
	
	value_int = value;

	strcpy(buffer, client->path);

	client->temp = NULL;

	unit  = strtok_r(buffer, "/", &client->temp);
	subunit = strtok_r(NULL, "/", &client->temp);
	control = strtok_r(NULL, "/", &client->temp);


	printf("Unit: %s Subunit: %s Control: %s Value: %f\n", unit, subunit, control, value);
	
	int a_count;
	a_count = 0;
	
	if (((strcmp(unit, "dsp")) == 0) && ((strcmp(subunit, "master")) == 0)) {
		
		if (strcmp(control, "add_synth") == 0) {
			//for (portgroup = client->kraddsp->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = client->kraddsp->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
				if (portgroup->type == SYNTH) {
					a_count++;
				}
			}
			}
			
			if (a_count == 0) {
				printf("Adding a synth..");
				create_synth(client->kraddsp, "synth");
			} else {
				printf("sorry limit one synth at this time..");
			}
		}

		if (strcmp(control, "add_sampler") == 0) {
			//for (portgroup = client->kraddsp->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = client->kraddsp->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
				if (portgroup->type == SAMPLER) {
					a_count++;
				}
			}
			}
			
			if (a_count == 0) {
				printf("Adding a sampler..");
				create_sampler(client->kraddsp, "sampler");
			} else {
				printf("sorry limit one sampler at this time..");
			}
		}
			
		return 0;	

	}
	
	
	//for (portgroup = client->kraddsp->portgroup; portgroup != NULL && portgroup->active; portgroup = portgroup->next) {
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
	portgroup = client->kraddsp->portgroup[pg];
	if ((portgroup != NULL) && (portgroup->active)) {
	
		printf("I'm on %s\n",  portgroup->name);

		if (((strcmp(unit, "sampler")) == 0) && (portgroup->type == SAMPLER)) {
		
			if ((strcmp(subunit, "master")) == 0) {
			
				if (strcmp(control, "add_sample") == 0) {
					printf("Adding another sample..");
					value = add_sample(portgroup->sampler);
				}
			
				if (strcmp(control, "remove") == 0) {
					printf("Removing sampler\n");
				
					destroy_sampler(portgroup->sampler);
				}
				
				if (strcmp(control, "list_samples") == 0) {
					printf("Listing Samples..");
					pos = sprintf(data + pos, "samplerinfo*");
					for (i = 0; i < MAX_SAMPLES; i++) {
						if (portgroup->sampler->sample[i] != NULL) {
							asample = portgroup->sampler->sample[i];
							// number, length, startoffset, stopoffset, recording, playing, looping, crossfadelength, position
							pos = sprintf(data + pos, "%d,%d,%d,%d,%d,%d,%d,%d,%lu*", i, 
										  asample->length, asample->startoffset, asample->stopoffset, asample->record, 
										  asample->play, asample->loop, asample->crossfade_frame_duration, (asample->ringbuffer[1]->read_ptr / 4)  );
						}
					}
					
				}
				
				break;
				
			}
		
			sample = atoi(subunit);
			
			printf("I'm on sample %d\n",  sample);
			
			if (strcmp(control, "record") == 0) {
				if (portgroup->sampler->sample[sample]->saving) {
					printf("Cant record while saving..");
				} else {
					printf("Set record to %f\n", value);
					portgroup->sampler->sample[sample]->record = value;
					if (portgroup->sampler->sample[sample]->record == 1) {
						portgroup->sampler->sample[sample]->saved = 0;
					}
				}
			
			}
			
			if (strcmp(control, "play") == 0) {
				printf("Set play to %f\n", value);
				//portgroup->sampler->sample[sample]->play = value;
				if ((value == 0.0f) && (portgroup->sampler->sample[sample]->play)){
					portgroup->sampler->sample[sample]->fade = 2;
				} else {
					portgroup->sampler->sample[sample]->play = value;
				}
			
			}
			
			if (strcmp(control, "reset") == 0) {
				printf("reset sample %d\n", sample);
				reset_sample(portgroup->sampler->sample[sample]);			
			}
			
			if (strcmp(control, "loop") == 0) {
				printf("Set loop to %f\n", value);
				portgroup->sampler->sample[sample]->loop = value;
			
			}
			
			if (strcmp(control, "seek") == 0) {
				printf("Set seek to %f\n", value);
				portgroup->sampler->sample[sample]->seek = value;
			}
			
			if (strcmp(control, "writeseek") == 0) {
				printf("Set write seek to %f\n", value);
				portgroup->sampler->sample[sample]->write_seek = value;
			}
		
			if (strcmp(control, "save") == 0) {
				printf("Saving sample\n");
				sample_save(portgroup->sampler->sample[sample]);
			}	
			
			if (strcmp(control, "startoffset") == 0) {
				printf("setting start offset\n");
				set_start_offset(portgroup->sampler->sample[sample], value);  
			}
			
			if (strcmp(control, "crossfadelength") == 0) {
				printf("setting crossfade length\n");
				set_crossfade_duration(portgroup->sampler->sample[sample], value_int);
			}
			
			if (strcmp(control, "stopoffset") == 0) {
				printf("setting stop offset\n");
				portgroup->sampler->sample[sample]->stopoffset = value;
			}	
			
			if (strcmp(control, "load") == 0) {
				printf("Loading sample\n");
				sample_load(portgroup->sampler->sample[sample], "/home/oneman/krad/recordings/sampler/radio1_Mon_2011-07-25_06.20.36_EDT.flac");
			}
			
			if (strcmp(control, "remove") == 0) {
				printf("Removing sample %d\n", sample);
				destroy_sample(portgroup->sampler->sample[sample]);
			}
			
			break;
			
		}

		if (((strcmp(unit, "synth")) == 0) && (portgroup->type == SYNTH)) {
		
			if (strcmp(subunit, "master") == 0) {
				//if (strcmp(control, "add_sinewave") == 0) {
				//	printf("Adding a sine wave.\n");
				
				//	add_sine(portgroup->synth);
				
				//}
			
				if (strcmp(control, "remove") == 0) {
					printf("Removing synth\n");
					destroy_synth(portgroup->synth);
				}

				break;

			}
			
			sine = atoi(subunit);
			
			if (strcmp(control, "freq") == 0) {
				printf("Set sine %d freq to %f\n", sine, value);
				portgroup->synth->sinewave[sine]->new_freq = value;
			}
			
			if (strcmp(control, "active") == 0) {
				printf("setting sine wave %d %f\n", sine, value);
				if (value != 0.0f) {
					activate_sine(portgroup->synth->sinewave[sine]);
				} else {
					deactivate_sine(portgroup->synth->sinewave[sine]);
				}
			}
			
			break;	

		}
	
	}
	}

}




void kraddsp_want_shutdown() {

	krad_ipc_server_want_shutdown();

}

void kraddsp_shutdown(void *arg) {

	kraddsp_t *kraddsp = (kraddsp_t *)arg;

	printf("kraddsp Shutting Down\n");
	
	int pg;
	portgroup_t *portgroup;
	for (pg = 0; pg < PORTGROUP_MAX; pg++) {
		portgroup = kraddsp->portgroup[pg];
		if ((portgroup != NULL) && (portgroup->active)) {
	
			printf("I'm killing off %s\n",  portgroup->name);

			if (portgroup->type == SAMPLER) {
				destroy_sampler(portgroup->sampler);
			}

			if (portgroup->type == SYNTH) {
				destroy_synth(portgroup->synth);
			}
			
		}
	}
	
	jack_client_close (kraddsp->jack_client);
	printf("kraddsp Jack Client Closed\n");

}

int kraddsp_handler(char *data, void *pointer) {

	kraddsp_client_t *client = (kraddsp_client_t *)pointer;

	printf("Krad DSP Daemon Handler got %zu byte message: '%s' \n", strlen(data), data);
	
	//reset things from last command
	client->temp = NULL;
	strcpy(client->path, "");
	
	client->cmd = strtok_r(data, ",", &client->temp);
	
	
	//if (strncmp(client->cmd, "getpeak", 7) == 0) {
	//	client->float_value = read_peak(&client->kraddsp->portgroup[1], 0);
	//	sprintf(data, "peak=%f", client->float_value);
	//	return 0; // to not broadcast
	//}
	
	if (strncmp(client->cmd, "listcontrols", 12) == 0) {
		printf("Listing Controls\n");
		listcontrols(client, data);
		return 0; // to not broadcast
	}

	if (strncmp(client->cmd, "xruns", 5) == 0) {
		printf("Number of xruns: %d\n", client->kraddsp->xruns);
		sprintf(data, "Number of xruns: %d\n", client->kraddsp->xruns);
		return 0; // to not broadcast
	}


	if (strncmp(data, "setcontrol", 9) == 0) {
		client->float_value = atof(strrchr ( data, '/' ) + 1);
		strtok_r(data, "/", &client->temp);
		strcat(client->path, strtok_r(NULL, "/", &client->temp));
		strcat(client->path, "/");
		strcat(client->path, strtok_r(NULL, "/", &client->temp));
		strcat(client->path, "/");
		strcat(client->path, strtok_r(NULL, "/", &client->temp));
		setcontrol(client, data, client->float_value);
		sprintf(data, "kraddsp:setcontrol/%s|%f", client->path, client->float_value);
		printf("Controlled: %s\n", data);
		return 2; // to broadcast
	}

	if (strncmp(data, "kill", 4) == 0) {
		printf("Killed by a client..\n");
		kraddsp_want_shutdown();
		sprintf(data, "kraddsp shutting down now!");
		return 1; // to broadcast
	}


}
