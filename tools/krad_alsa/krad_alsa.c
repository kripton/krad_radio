#include "krad_alsa.h"

#define SAMPLE_24BIT_SCALING  8388607.0f
#define SAMPLE_16BIT_SCALING  32767.0f

#define SAMPLE_24BIT_MAX  8388607  
#define SAMPLE_24BIT_MIN  -8388607 
#define SAMPLE_24BIT_MAX_F  8388607.0f  
#define SAMPLE_24BIT_MIN_F  -8388607.0f 

#define SAMPLE_16BIT_MAX  32767
#define SAMPLE_16BIT_MIN  -32767
#define SAMPLE_16BIT_MAX_F  32767.0f
#define SAMPLE_16BIT_MIN_F  -32767.0f

#define NORMALIZED_FLOAT_MIN -1.0f
#define NORMALIZED_FLOAT_MAX  1.0f

#define float_24u32(s, d) \
	if ((s) <= NORMALIZED_FLOAT_MIN) {\
		(d) = SAMPLE_24BIT_MIN << 8;\
	} else if ((s) >= NORMALIZED_FLOAT_MAX) {\
		(d) = SAMPLE_24BIT_MAX << 8;\
	} else {\
		(d) = f_round ((s) * SAMPLE_24BIT_SCALING) << 8;\
	}

#define float_16(s, d)\
	if ((s) <= NORMALIZED_FLOAT_MIN) {\
		(d) = SAMPLE_16BIT_MIN;\
	} else if ((s) >= NORMALIZED_FLOAT_MAX) {\
		(d) = SAMPLE_16BIT_MAX;\
	} else {\
		(d) = f_round ((s) * SAMPLE_16BIT_SCALING);\
	}

void sample_move_d32u24_sS (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip)
{
	while (nsamples--) {
		float_24u32 (*src, *((int32_t*) dst));
		dst += dst_skip;
		src++;
	}
}

void sample_move_dS_s32u24 (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip)
{
	/* ALERT: signed sign-extension portability !!! */

	while (nsamples--) {
		*dst = (*((int *) src) >> 8) / SAMPLE_24BIT_SCALING;
		dst++;
		src += src_skip;
	}
}

void sample_move_dS_s16 (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip) 
	
{
	/* ALERT: signed sign-extension portability !!! */
	while (nsamples--) {
		*dst = (*((short *) src)) / SAMPLE_16BIT_SCALING;
		dst++;
		src += src_skip;
	}
}

void sample_move_d16_sS (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip)	
{
	while (nsamples--) {
		float_16 (*src, *((int16_t*) dst));
		dst += dst_skip;
		src++;
	}
}
/*
//array of the available I/O methods defined for capture
static struct io_method methods[] = {
    { METHOD_DIRECT_RW, SND_PCM_ACCESS_RW_INTERLEAVED, 0 },
    { METHOD_DIRECT_MMAP, SND_PCM_ACCESS_MMAP_INTERLEAVED, 0 },
    { METHOD_ASYNC_RW,SND_PCM_ACCESS_RW_INTERLEAVED, 0 },
    { METHOD_ASYNC_MMAP,SND_PCM_ACCESS_MMAP_INTERLEAVED, 0 },
    { METHOD_RW_AND_POLL,SND_PCM_ACCESS_RW_INTERLEAVED, 0 },
    { METHOD_DIRECT_RW_NI,SND_PCM_ACCESS_RW_NONINTERLEAVED, 0 }//not implemented
    //SND_PCM_ACCESS_RW_NONINTERLEAVED not supported by the most kind of cards
};

//recovery callback in case of error

static int xrun_recovery(snd_pcm_t *handle, int error)
{
    switch(error)
    {
        case -EPIPE:    // Buffer  Over-run   
            fprintf(stderr,"speaker: \"Buffer Underrun\" \n");
            if ((error = snd_pcm_prepare(handle)) < 0)
                fprintf(stderr,"speaker: Buffer underrun cannot be recovered, snd_pcm_prepare fail: %s\n", snd_strerror(error));
            return 0;
            break;
                
        case -ESTRPIPE: //suspend event occurred
            fprintf(stderr,"speaker: Error ESTRPIPE\n");
            //EAGAIN means that the request cannot be processed immediately
            while ((error = snd_pcm_resume(handle)) == -EAGAIN) 
                sleep(1);// waits until the suspend flag is clear

            if (error < 0) // error case
            {
                if ((error = snd_pcm_prepare(handle)) < 0)
                    fprintf(stderr,"speaker: Suspend cannot be recovered, snd_pcm_prepare fail: %s\n", snd_strerror(error));
            }
            return 0;
            break;
            
        case -EBADFD://Error PCM descriptor is wrong
            fprintf(stderr,"speaker: Error EBADFD\n");
            break;
            
        default:
            fprintf(stderr,"speaker: Error unknown, error: %d\n",error);
            break;
    }
    return error;
}

*/
/*****************************************************************************************/
/*********************  Case: Async with read/write functions ****************************/
/*****************************************************************************************/



//In every call to async_rw_callback one period is processed
//Size of one period (bytes) = n_channels * sizeof(short) * period_size
//The asynchronous callback is called when period boundary elapses
void async_rw_callback(snd_async_handler_t *ahandler) {
/*
    int error;

	int c, s;
	krad_alsa_t *krad_alsa;
	
    snd_pcm_t *device = snd_async_handler_get_pcm(ahandler);
    krad_alsa = snd_async_handler_get_callback_private(ahandler);

	if (krad_alsa->capture) {

		if ((krad_ringbuffer_write_space (krad_alsa->krad_audio->input_ringbuffer[1]) >= krad_alsa->period_size * 4 ) && (krad_ringbuffer_write_space (krad_alsa->krad_audio->input_ringbuffer[0]) >= krad_alsa->period_size * 4 )) {
	
	
			//trying to read one entire period
			if ((error = snd_pcm_readi (device, krad_alsa->integer_samples, krad_alsa->period_size)) < 0)
			{
				if (xrun_recovery(device, error)) {
				    fprintf(stderr,"alsa input: read error: %s\n", snd_strerror(error));
				    exit(EXIT_FAILURE);
				}
			}
			
			if (krad_alsa->sample_format == SND_PCM_FORMAT_S32_LE) {
				sample_move_dS_s32u24 (krad_alsa->interleaved_samples, (char *)krad_alsa->integer_samples, krad_alsa->period_size * 2, krad_alsa->sample_size);
			}

			if (krad_alsa->sample_format == SND_PCM_FORMAT_S16_LE) {
				sample_move_dS_s16 (krad_alsa->interleaved_samples, (char *)krad_alsa->integer_samples, krad_alsa->period_size * 2, krad_alsa->sample_size);
			}			
			
			for (s = 0; s < krad_alsa->period_size; s++) {
				for (c = 0; c < 2; c++) {
					krad_alsa->samples[c][s] = krad_alsa->interleaved_samples[s * 2 + c];	
				}
			}
			
			for (c = 0; c < 2; c++) {
				krad_ringbuffer_write (krad_alsa->krad_audio->input_ringbuffer[c], (char *)krad_alsa->samples[c], (krad_alsa->period_size * 4) );
			}
		

			for (c = 0; c < 2; c++) {
				compute_peak(krad_alsa->krad_audio, KINPUT, krad_alsa->samples[c], c, krad_alsa->period_size, 0);
			}
		
	
		}
*/
/*
	}
		
	if (krad_alsa->krad_audio->process_callback != NULL) {
		krad_alsa->krad_audio->process_callback(krad_alsa->period_size, krad_alsa->krad_audio->userdata);
	}

	if (krad_alsa->playback) {

		if ((krad_ringbuffer_read_space (krad_alsa->krad_audio->output_ringbuffer[1]) >= krad_alsa->period_size * 4 ) && (krad_ringbuffer_read_space (krad_alsa->krad_audio->output_ringbuffer[0]) >= krad_alsa->period_size * 4 )) {
	
			for (c = 0; c < 2; c++) {
				krad_ringbuffer_read (krad_alsa->krad_audio->output_ringbuffer[c], (char *)krad_alsa->samples[c], (krad_alsa->period_size * 4) );
			}

			for (s = 0; s < krad_alsa->period_size; s++) {
				for (c = 0; c < 2; c++) {
					krad_alsa->interleaved_samples[s * 2 + c] = krad_alsa->samples[c][s];
				}
			}
		

			for (c = 0; c < 2; c++) {
				compute_peak(krad_alsa->krad_audio, KOUTPUT, krad_alsa->samples[c], c, krad_alsa->period_size, 0);
			}
		
	
		} else {
			printf("underrrrun!\n");
			for (s = 0; s < krad_alsa->period_size; s++) {
				for (c = 0; c < 2; c++) {
					krad_alsa->interleaved_samples[s * 2 + c] = 0.0f;
				}
			}
	
		}

		if (krad_alsa->sample_format == SND_PCM_FORMAT_S32_LE) {
			sample_move_d32u24_sS ((char *)krad_alsa->integer_samples, krad_alsa->interleaved_samples, krad_alsa->period_size * 2, krad_alsa->sample_size);
		}

		if (krad_alsa->sample_format == SND_PCM_FORMAT_S16_LE) {
			sample_move_d16_sS ((char *)krad_alsa->integer_samples, krad_alsa->interleaved_samples, krad_alsa->period_size * 2, krad_alsa->sample_size);
		}	


		//trying to write one entire period
		if ((error = snd_pcm_writei (device, krad_alsa->integer_samples, krad_alsa->period_size)) < 0)
		{
		    if (xrun_recovery(device, error)) {
		        fprintf(stderr,"speaker: Write error: %s\n", snd_strerror(error));
		        exit(EXIT_FAILURE);
		    }       
		}
	}
*/
}

void async_rw(krad_alsa_t *krad_alsa)
{

    snd_async_handler_t *ahandler;//async handler
    int error;

    
	//adding async handler for PCM with private data and callback async_rw_callback
    if ((error = snd_async_add_pcm_handler(&ahandler, krad_alsa->device, async_rw_callback, krad_alsa)) < 0)
    { 
        fprintf(stderr,"speaker: Unable to register async handler\n");
        exit(EXIT_FAILURE);
    }
    
    
	memset (krad_alsa->integer_samples, 0, sizeof(krad_alsa->integer_samples));
    
    int count = 0;
    
    if (krad_alsa->playback) {
    
		while (count < krad_alsa->buffer_size / krad_alsa->period_size) {
		
			error = snd_pcm_writei(krad_alsa->device, krad_alsa->integer_samples, krad_alsa->period_size);

			if (error < 0) {
				printf("Initial write error: %s\n", snd_strerror(error));
				exit(1);
			}
	
			if (error != krad_alsa->period_size) {
				printf("Initial write error: written %i expected %li\n", error, krad_alsa->period_size);
				exit(1);
			}
		
			if (snd_pcm_state(krad_alsa->device) == SND_PCM_STATE_PREPARED) {
				error = snd_pcm_start(krad_alsa->device);
				if (error < 0) {
					printf("Start error: %s\n", snd_strerror(error));
					exit(1);
				}
			}
	
			printf("#########krad audio %s\n", krad_alsa->device_name);	
	
			count++;
	
		}
		
	}
	
	
    if (krad_alsa->capture) {

		if (snd_pcm_state(krad_alsa->device) == SND_PCM_STATE_PREPARED) {
			error = snd_pcm_start(krad_alsa->device);
			if (error < 0) {
				printf("Start error: %s\n", snd_strerror(error));
				exit(1);
			}
		}

		printf("#########krad audio %s\n", krad_alsa->device_name);	

	}
	
}

/*******************************************************************************/
/*********************** case async with memory mapping ************************/
/*******************************************************************************/
/*
//This case uses an async callback and memory mapping
void async_mmap_callback(snd_async_handler_t *ahandler)
{
    int error,state;
    snd_pcm_t *device = snd_async_handler_get_pcm(ahandler);
    struct async_private_data *data = snd_async_handler_get_callback_private(ahandler);
    int n_channels = data->n_channels;
    snd_pcm_sframes_t period_size = data->period_size;
    const snd_pcm_channel_area_t *my_areas;//memory area info
    snd_pcm_uframes_t offset, frames, size;
    snd_pcm_sframes_t avail, commitres;
    int first = 0;
    
    while(1)
    {   
        state = snd_pcm_state(device);//checks the PCM descriptor state
        switch(state)
        {
            case SND_PCM_STATE_XRUN://checks if the buffer is in a wrong state
                //fprintf(stderr,"speaker: SND_PCM_STATE_XRUN\n");
                if (error = xrun_recovery(device, -EPIPE) < 0) 
                {
                    fprintf(stderr,"speaker: XRUN recovery failed: %s\n", snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
                first = 1;
                break;
                
            case SND_PCM_STATE_SUSPENDED://checks if PCM is in a suspend state
                //fprintf(stderr,"speaker: SND_PCM_STATE_SUSPENDED\n");
                if (error = xrun_recovery(device, -ESTRPIPE) < 0) 
                {
                    fprintf(stderr,"speaker: SUSPEND recovery failed: %s\n", snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
                break;
        }
        avail = snd_pcm_avail_update(device);
        if (avail < 0)//error case
        {
            if (error = xrun_recovery(device, avail) < 0) {
                fprintf(stderr,"speaker: Recovery fail: %s\n", snd_strerror(error));
                exit(error);
            }
            first = 1;
            continue;   
        }   
        if (avail < period_size)
        {         
            switch(first) 
            {
                case 1://initializing PCM
                    fprintf(stderr,"speaker: Restarting PCM \n");
                    first = 0;
                    if (error = snd_pcm_start(device) < 0) {
                        fprintf(stderr,"speaker: Start error: %s\n", snd_strerror(error));
                        exit(EXIT_FAILURE);
                    }
                    break;
                    
                case 0:                   
                    return;
            } 
            continue;//we don't have enough data for one period
        }
        size = period_size;
        while (size > 0)//wait until we have period_size frames
        {
            frames = size;//expected frames number to be processed
			//frames is a bidirectional variable, that means the real number of frames processed is written 
			//to this variable by the function.
            
            //sending request for the start of the data writing by the application
            if ((error = snd_pcm_mmap_begin (device, &my_areas, &offset, &frames)) < 0) {
                if ((error = xrun_recovery(device, error)) < 0) {
                    fprintf(stderr,"speaker: MMAP begin avail error: %s\n", snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
                first = 1;
            } 
            //reading data from standard input
            read(STDIN_FILENO, (my_areas[0].addr)+(offset*sizeof(short)*n_channels), sizeof(short) * frames * n_channels);

			//sending signal for the end of the data reading by the application 
            commitres = snd_pcm_mmap_commit(device, offset, frames);
            if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                if ((error = xrun_recovery(device, commitres >= 0 ? commitres : -EPIPE)) < 0) {
                    fprintf(stderr,"speaker: MMAP commit error: %s\n", snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
                first = 1;
            }
            size -= frames;//needed for the condition of the while loop (size == 0 means end of writing)
        }
    }   
}

void async_mmap(snd_pcm_t *device, struct device_parameters cap_dev_params)
{
    snd_async_handler_t *ahandler;// async handler
    struct async_private_data data;// private data passed to the async callback
    snd_pcm_sframes_t period_size = cap_dev_params.period_size;
    int n_channels = cap_dev_params.n_channels;
    data.n_channels = n_channels;
    data.period_size = cap_dev_params.period_size;
    int error;
    snd_pcm_uframes_t offset, frames;
    snd_pcm_sframes_t avail, commitres;
    const snd_pcm_channel_area_t *my_areas;//memory area info
    
	//adding async handler for PCM
    if (error = snd_async_add_pcm_handler(&ahandler, device, async_mmap_callback, &data) < 0)
    { //async_rw_callback is called every time that the period is full
        fprintf(stderr,"speaker: Unable to register async handler\n");
        exit(EXIT_FAILURE);
    }
    //sending data writing request
    if ((error = snd_pcm_mmap_begin (device, &my_areas, &offset, &frames))<0) {
        fprintf (stderr, "speaker: Memory mapping cannot be started (%s)\n",snd_strerror (error));
        exit (1);
    }
    //reading data from standard input
    read(STDIN_FILENO, (my_areas[0].addr)+(offset*sizeof(short)*n_channels), sizeof(short) * frames * n_channels);    

	//sending signal for the end of the data writing by the application
    commitres = snd_pcm_mmap_commit(device, offset, frames);
            if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                if ((error = xrun_recovery(device, commitres >= 0 ? commitres : -EPIPE)) < 0) {
                    fprintf(stderr,"speaker: MMAP commit error: %s\n", snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
            }

    //starting PCM
    if ((error = snd_pcm_start(device)) < 0) {
        fprintf (stderr, "speaker: Unable to start PCM (%s)\n",snd_strerror (error));
        exit (1);
    }

	//the remainder work is made by the handler and the callback
    while (1) {
        sleep(1);
    }
}
*/
static char *krad_alsa_id_str (snd_ctl_elem_id_t *id) {

	static char str[128];

	sprintf(str, "%i,%i,%i,%s,%i", 
		snd_ctl_elem_id_get_interface(id),
		snd_ctl_elem_id_get_device(id),
		snd_ctl_elem_id_get_subdevice(id),
		snd_ctl_elem_id_get_name(id),
		snd_ctl_elem_id_get_index(id));
	return str;

}

void krad_alsa_list_cards () {

  int card_num;
  char *card_name;

  card_num = -1;
  card_name = NULL;

  while ((snd_card_next (&card_num) == 0) && (card_num != -1)) {
    snd_card_get_name (card_num, &card_name);
    printf ("ALSA Card %d: %s\n", card_num, card_name);
  }
}

static int krad_alsa_control (krad_alsa_t *krad_alsa, snd_ctl_elem_id_t *id, int value) {

	snd_ctl_elem_value_t *ctl;
	int err;

  printf("hix %d\n", value);
	snd_ctl_elem_value_alloca (&ctl);
	snd_ctl_elem_value_set_id (ctl, id);

  snd_ctl_elem_value_set_integer (ctl, 0, value);
	err = snd_ctl_elem_write (krad_alsa->control, ctl);
	if (err < 0) {
		printf ("Cannot write control '%s': %s", krad_alsa_id_str(id), snd_strerror(err));
		return err;
	}

  return 0;

}


static int krad_alsa_control_funtime (krad_alsa_t *krad_alsa) {

  int value;
  int c;
  int err;

	snd_ctl_elem_id_t *elem_id;

  printf("hi\n");

	snd_ctl_elem_list_alloca (&krad_alsa->control_list);
  snd_ctl_elem_id_alloca (&elem_id);

	err = snd_ctl_elem_list (krad_alsa->control, krad_alsa->control_list);
	if (err < 0) {
		printf ("Cannot determine controls: %s\n", snd_strerror(err));
    return err;
	}
	krad_alsa->controls_count = snd_ctl_elem_list_get_count (krad_alsa->control_list);
	if (krad_alsa->controls_count == 0) {
		err = 0;
    return err;
	} else {
		printf ("ALSA Control Count: %d\n", krad_alsa->controls_count);
  }


	snd_ctl_elem_list_set_offset (krad_alsa->control_list, 0);
	if (snd_ctl_elem_list_alloc_space (krad_alsa->control_list, krad_alsa->controls_count) < 0) {
		printf ("No enough memory...");
    return err;
	}
	if ((err = snd_ctl_elem_list (krad_alsa->control, krad_alsa->control_list)) < 0) {
		printf ("Cannot determine controls (2): %s\n", snd_strerror(err));
    return err;
	}
  printf("hid\n");

  value = rand() % 31;
  value = 0;

	for (c = 0; c < krad_alsa->controls_count; c++) {
		snd_ctl_elem_list_get_id (krad_alsa->control_list, c, elem_id);
    err = krad_alsa_control (krad_alsa, elem_id, value);
  	if (err < 0) {
        return err;
    }
	}

  return 0;
}


int krad_alsa_enable_control (krad_alsa_t *krad_alsa) {

  int err;

  krad_alsa->control_enabled = 1;

  sprintf (krad_alsa->control_name, "hw:%d", krad_alsa->card_num);

	err = snd_ctl_open (&krad_alsa->control, krad_alsa->control_name, 0);
	if (err < 0) {
    printf ("snd_ctl_open error: %s", snd_strerror(err));
    return err;
	}


  return 0;

}

void krad_alsa_disable_control (krad_alsa_t *krad_alsa) {

  if (krad_alsa->control_enabled == 1) {
  	snd_ctl_elem_list_free_space (krad_alsa->control_list);
    snd_ctl_close (krad_alsa->control);
    krad_alsa->control_enabled = 0;
  }

}

void krad_alsa_control_test (krad_alsa_t *krad_alsa) {

  int t;

  t = 20;
  krad_alsa_list_cards ();
  krad_alsa_enable_control (krad_alsa);
  while (t) {
    krad_alsa_control_funtime (krad_alsa);
    t--;  
  }
  krad_alsa_disable_control (krad_alsa);

}

void krad_alsa_destroy (krad_alsa_t *krad_alsa) {

	printk ("krad alsa: destroyed");

  /*
  snd_pcm_close (krad_alsa->device);

	free(krad_alsa->samples[0]);
	free(krad_alsa->samples[1]);

	free(krad_alsa->interleaved_samples);
	free(krad_alsa->integer_samples);
	*/
	free (krad_alsa);

}

krad_alsa_t *krad_alsa_create (krad_audio_t *krad_audio, int card_num) {

	krad_alsa_t *krad_alsa;

	if ((krad_alsa = calloc (1, sizeof (krad_alsa_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}
	
	krad_alsa->krad_audio = krad_audio;


  krad_alsa->card_num = card_num;


  krad_alsa_control_test (krad_alsa);

  /*
	krad_alsa->samples[0] = malloc(24 * 8192);
	krad_alsa->samples[1] = malloc(24 * 8192);
	krad_alsa->interleaved_samples = malloc(48 * 8192);
	krad_alsa->integer_samples = malloc(48 * 8192);

	krad_alsa->sample_format = SND_PCM_FORMAT_S32_LE;
	krad_alsa->sample_size = 4;
	krad_alsa->sample_rate = 44100;
	krad_alsa->n_channels = 2;
	
	krad_alsa->access = 2;
	krad_alsa->buffer_size = 1024;
	krad_alsa->period_size = 512;

	
	if ((krad_audio->direction == KINPUT) || (krad_audio->direction == KDUPLEX)) {
		krad_alsa->capture = 1;
	}

	if ((krad_audio->direction == KOUTPUT) || (krad_audio->direction == KDUPLEX)) {
		krad_alsa->playback = 1;
	}
	
	if (krad_alsa->playback) {
	
		krad_alsa->stream = SND_PCM_STREAM_PLAYBACK;
	}

	if (krad_alsa->capture) {
		
		if (strncmp(krad_alsa->device_name, "hw:1,0", 6) == 0) {
			// likely a webcam
			krad_alsa->stream = SND_PCM_STREAM_CAPTURE;
			krad_alsa->sample_format = SND_PCM_FORMAT_S16_LE;
			krad_alsa->sample_rate = 32000;
		}
	}
	


    if ((krad_alsa->error = snd_pcm_open (&krad_alsa->device, krad_alsa->device_name, krad_alsa->stream, methods[krad_alsa->access].open_mode)) < 0) {
        fprintf (stderr, "speaker: Device cannot be opened %s (%s)\n", 
             krad_alsa->device_name,
             snd_strerror (krad_alsa->error));
        	exit (1);
    }
    fprintf (stderr, "speaker: Device: %s open_mode = %d\n", krad_alsa->device_name, methods[krad_alsa->access].open_mode);
    
 	//allocating the hardware configuration structure
    if ((krad_alsa->error = snd_pcm_hw_params_malloc (&krad_alsa->hw_params)) < 0) {
        fprintf (stderr, "speaker: Hardware configuration structure cannot be allocated (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }

    //assigning the hw configuration structure to the device        
    if ((krad_alsa->error = snd_pcm_hw_params_any (krad_alsa->device, krad_alsa->hw_params)) < 0) {
        fprintf (stderr, "speaker: Hardware configuration structure cannot be assigned to device (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }


    
    switch(methods[krad_alsa->access].method)
    {
        case METHOD_DIRECT_RW:
            fprintf (stderr, "microphone: Capture method: METHOD_DIRECT_RW (m = 0) \n");
            break;
        case METHOD_DIRECT_MMAP:
            fprintf (stderr, "microphone: Capture method: METHOD_DIRECT_MMAP (m = 1)\n");
            break;
        case METHOD_ASYNC_MMAP:
            fprintf (stderr, "microphone: Capture method: METHOD_ASYNC_MMAP (m = 2)\n");
            break;
        case METHOD_ASYNC_RW:
            fprintf (stderr, "microphone: Capture method: METHOD_ASYNC_RW (m = 3)\n");
            break;
        case METHOD_RW_AND_POLL:
            fprintf (stderr, "microphone: Capture method: METHOD_RW_AND_POLL (m = 4)\n");
            break;
        case METHOD_DIRECT_RW_NI://not implemented
            fprintf (stderr, "microphone: Capture method: METHOD_DIRECT_RW_NI (m = 5)\n");
            break;
    }
    

    //sets the configuration method
    fprintf (stderr, "speaker: Access method: %d\n", methods[krad_alsa->access].access);
    if ((krad_alsa->error = snd_pcm_hw_params_set_access (krad_alsa->device, krad_alsa->hw_params, methods[krad_alsa->access].access)) < 0) {
        fprintf (stderr, "speaker: Access method cannot be configured (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }

    //checks the access method
    if ((krad_alsa->error = snd_pcm_hw_params_get_access (krad_alsa->hw_params, &krad_alsa->real_access)) < 0) {
        fprintf (stderr, "speaker: Access method cannot be obtained (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    //shows the access method
    switch(krad_alsa->real_access)
    {
    case SND_PCM_ACCESS_MMAP_INTERLEAVED:
        fprintf (stderr, "speaker: PCM access method: SND_PCM_ACCESS_MMAP_INTERLEAVED \n");
        break;
    case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
        fprintf (stderr, "speaker: PCM access method: SND_PCM_ACCESS_MMAP_NONINTERLEAVED \n");
        break;
    case SND_PCM_ACCESS_MMAP_COMPLEX:
        fprintf (stderr, "speaker: PCM access method: SND_PCM_ACCESS_MMAP_COMPLEX \n");
        break;
    case SND_PCM_ACCESS_RW_INTERLEAVED:
        fprintf (stderr, "speaker: PCM access method: SND_PCM_ACCESS_RW_INTERLEAVED \n");
        break;
    case SND_PCM_ACCESS_RW_NONINTERLEAVED:
        fprintf (stderr, "speaker: PCM access method: SND_PCM_ACCESS_RW_NONINTERLEAVED \n");
        break;
    }   

    //SND_PCM_FORMAT_S16_LE => 16 bit signed little endian
    if ((krad_alsa->error = snd_pcm_hw_params_set_format (krad_alsa->device, krad_alsa->hw_params, krad_alsa->sample_format)) < 0)
    {
        fprintf (stderr, "speaker: Playback sample format cannot be configured (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    //checks playback format
    if ((krad_alsa->error = snd_pcm_hw_params_get_format (krad_alsa->hw_params, &krad_alsa->real_sample_format)) < 0)
    {
        fprintf (stderr, "speaker: Playback sample format cannot be obtained (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    //just shows the capture format in a human readable way
    switch(krad_alsa->real_sample_format)
    {
    case SND_PCM_FORMAT_S32_LE:
        fprintf (stderr, "speaker: PCM sample format: SND_PCM_FORMAT_S32_LE \n");
        
		krad_alsa->sample_size = 4;
        
        break;
    case SND_PCM_FORMAT_S16_LE:
        fprintf (stderr, "speaker: PCM sample format: SND_PCM_FORMAT_S16_LE \n");
        
        krad_alsa->sample_size = 2;
        
        break;
    default:
        fprintf (stderr, "speaker: Real_sample_format = %d\n", krad_alsa->real_sample_format);
    }    

	//sets the sample rate
    if ((krad_alsa->error = snd_pcm_hw_params_set_rate (krad_alsa->device, krad_alsa->hw_params, krad_alsa->sample_rate, 0)) < 0) {
        fprintf (stderr, "speaker: Sample rate cannot be configured (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    //cheks the sample rate
    if ((krad_alsa->error = snd_pcm_hw_params_get_rate (krad_alsa->hw_params, &krad_alsa->real_sample_rate, 0)) < 0) {
        fprintf (stderr, "speaker: Sample rate cannot be obtained (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    
//	krad_audio->sample_rate = krad_alsa->real_sample_rate;
    
    fprintf (stderr, "speaker: real_sample_rate = %d\n", krad_alsa->real_sample_rate);
    

    //sets the number of channels
    if ((krad_alsa->error = snd_pcm_hw_params_set_channels (krad_alsa->device, krad_alsa->hw_params, krad_alsa->n_channels)) < 0) {
        fprintf (stderr, "speaker: Number of channels cannot be configured (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    //checks the number of channels
    if ((krad_alsa->error = snd_pcm_hw_params_get_channels (krad_alsa->hw_params, &krad_alsa->real_n_channels)) < 0) {
        fprintf (stderr, "speaker: Number of channels cannot be obtained (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    fprintf (stderr, "speaker: real_n_channels = %d\n", krad_alsa->real_n_channels);
    

    //sets the buffer size
    if (snd_pcm_hw_params_set_buffer_size(krad_alsa->device, krad_alsa->hw_params, krad_alsa->buffer_size) < 0) {
        fprintf (stderr, "speaker: Buffer size cannot be configured (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    //checks the value of the buffer size
    if (snd_pcm_hw_params_get_buffer_size(krad_alsa->hw_params, &krad_alsa->real_buffer_size) < 0) {
		fprintf (stderr, "speaker: Buffer size cannot be obtained (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    fprintf (stderr, "speaker: Buffer size = %d [frames]\n", (int) krad_alsa->real_buffer_size);

    krad_alsa->dir=0;//dir=0  =>  period size must be equal to period_size
    //sets the period size
    if ((krad_alsa->error = snd_pcm_hw_params_set_period_size(krad_alsa->device, krad_alsa->hw_params, krad_alsa->period_size, krad_alsa->dir)) < 0) {
        fprintf (stderr, "speaker: Period size cannot be configured (%s)\n",
             snd_strerror (krad_alsa->error));
        //exit (1);
    }
    //checks the value of period size
    if (snd_pcm_hw_params_get_period_size(krad_alsa->hw_params, &krad_alsa->real_period_size, &krad_alsa->dir) < 0) {
    fprintf (stderr, "speaker: Period size cannot be obtained (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    fprintf (stderr, "speaker:  Period size = %d [frames]\n", (int) krad_alsa->real_period_size);

    
    if ((krad_alsa->error = snd_pcm_hw_params (krad_alsa->device, krad_alsa->hw_params)) < 0) {
        fprintf (stderr, "speaker: Hardware parameters cannot be configured (%s)\n",
             snd_strerror (krad_alsa->error));
        exit (1);
    }
    //frees the structure of hardware configuration
    snd_pcm_hw_params_free (krad_alsa->hw_params);


    
    krad_alsa->capture_device_params.access_type = krad_alsa->real_access;          //real access method
    krad_alsa->capture_device_params.buffer_size = krad_alsa->real_buffer_size;     //real buffer size
    krad_alsa->capture_device_params.period_size = krad_alsa->real_period_size;     //real period size
    krad_alsa->capture_device_params.buffer_time = krad_alsa->buffer_time;          //real buffer time
    krad_alsa->capture_device_params.period_time = krad_alsa->period_time;          //real period time
    krad_alsa->capture_device_params.sample_format = krad_alsa->real_sample_format; //real samples format
    krad_alsa->capture_device_params.sample_rate = krad_alsa->real_sample_rate;     //real sample rate 
    krad_alsa->capture_device_params.n_channels = krad_alsa->n_channels;            //real number of channels


    
	switch(methods[access].method)
	{
		case METHOD_DIRECT_RW:
			direct_rw(device, capture_device_params);
		break;
		
		case METHOD_DIRECT_MMAP:
			direct_mmap(device, capture_device_params);
		break;
		
		case METHOD_ASYNC_RW:
			async_rw(device, capture_device_params);            
		break;
		
		case METHOD_ASYNC_MMAP:
			async_mmap(device, capture_device_params);
		break;
		
		case  METHOD_RW_AND_POLL:
			rw_and_poll_loop(device, capture_device_params);
		break;
	}

	async_rw (krad_alsa);

	*/

	return krad_alsa;
}


