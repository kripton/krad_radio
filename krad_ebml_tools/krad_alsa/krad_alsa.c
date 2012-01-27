#include <krad_alsa.h>

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

#define float_24(s, d) \
	if ((s) <= NORMALIZED_FLOAT_MIN) {\
		(d) = SAMPLE_24BIT_MIN;\
	} else if ((s) >= NORMALIZED_FLOAT_MAX) {\
		(d) = SAMPLE_24BIT_MAX;\
	} else {\
		(d) = f_round ((s) * SAMPLE_24BIT_SCALING);\
	}
	
#define __BYTE_ORDER __LITTLE_ENDIAN

void sample_move_d24_sS (char *dst, float *src, unsigned long nsamples, unsigned long dst_skip)
{
        int32_t z;
	
	while (nsamples--) {
		float_24 (*src, z);
#if __BYTE_ORDER == __LITTLE_ENDIAN
		memcpy (dst, &z, 3);
#elif __BYTE_ORDER == __BIG_ENDIAN
		memcpy (dst, (char *)&z + 1, 3);
#endif
		dst += dst_skip;
		src++;
	}
}	


void sample_move_d32u24_sS (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip)
{
	while (nsamples--) {
		float_24u32 (*src, *((int32_t*) dst));
		dst += dst_skip;
		src++;
	}
}	

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


/*****************************************************************************************/
/*********************  Case: Async with read/write functions ****************************/
/*****************************************************************************************/



//In every call to async_rw_callback one period is processed
//Size of one period (bytes) = n_channels * sizeof(short) * period_size
//The asynchronous callback is called when period boundary elapses
void async_rw_callback(snd_async_handler_t *ahandler) {

    int error;

	int c, s;
	krad_alsa_t *kradalsa;
	
    snd_pcm_t *device = snd_async_handler_get_pcm(ahandler);
    kradalsa = snd_async_handler_get_callback_private(ahandler);

		
	if (kradalsa->kradaudio->process_callback != NULL) {
		kradalsa->kradaudio->process_callback(kradalsa->period_size, kradalsa->kradaudio->userdata);
	}

	if ((jack_ringbuffer_read_space (kradalsa->kradaudio->output_ringbuffer[1]) >= kradalsa->period_size * 4 * 2 ) && (jack_ringbuffer_read_space (kradalsa->kradaudio->output_ringbuffer[0]) >= kradalsa->period_size * 4 * 2 )) {
	
		for (c = 0; c < 2; c++) {
			jack_ringbuffer_read (kradalsa->kradaudio->output_ringbuffer[c], (char *)kradalsa->samples[c], (kradalsa->period_size * 4) );
		}

		for (s = 0; s < kradalsa->period_size; s++) {
			for (c = 0; c < 2; c++) {
				kradalsa->interleaved_samples[s * 2 + c] = kradalsa->samples[c][s];
			}
		}
		

		for (c = 0; c < 2; c++) {
			compute_peak(kradalsa->kradaudio, KOUTPUT, kradalsa->samples[c], c, kradalsa->period_size, 0);
		}
		
	
	} else {
	
		for (s = 0; s < kradalsa->period_size; s++) {
			for (c = 0; c < 2; c++) {
				kradalsa->interleaved_samples[s * 2 + c] = 0.0f;
			}
		}
	
	}


	sample_move_d32u24_sS ((char *)kradalsa->integer_samples, kradalsa->interleaved_samples, kradalsa->period_size * 2, 4);


	//trying to write one entire period
    if ((error = snd_pcm_writei (device, kradalsa->integer_samples, kradalsa->period_size)) < 0)
    {
        if (xrun_recovery(device, error)) {
            fprintf(stderr,"speaker: Write error: %s\n", snd_strerror(error));
            exit(EXIT_FAILURE);
        }       
    }

}

void async_rw(krad_alsa_t *kradalsa)
{

    snd_async_handler_t *ahandler;//async handler
    int error;

    
	//adding async handler for PCM with private data and callback async_rw_callback
    if ((error = snd_async_add_pcm_handler(&ahandler, kradalsa->device, async_rw_callback, kradalsa)) < 0)
    { 
        fprintf(stderr,"speaker: Unable to register async handler\n");
        exit(EXIT_FAILURE);
    }
    
    
	memset (kradalsa->integer_samples, 0, sizeof(kradalsa->integer_samples));
    
    int count = 0;
    
    while (count < kradalsa->buffer_size / kradalsa->period_size) {
    
		error = snd_pcm_writei(kradalsa->device, kradalsa->integer_samples, kradalsa->period_size);

		if (error < 0) {
			printf("Initial write error: %s\n", snd_strerror(error));
			exit(1);
		}
	
		if (error != kradalsa->period_size) {
			printf("Initial write error: written %i expected %li\n", error, kradalsa->period_size);
			exit(1);
		}
		
		if (snd_pcm_state(kradalsa->device) == SND_PCM_STATE_PREPARED) {
			error = snd_pcm_start(kradalsa->device);
			if (error < 0) {
				printf("Start error: %s\n", snd_strerror(error));
				exit(1);
			}
		}
	
		printf("#########krad audio %s\n", kradalsa->device_name);	
	
		count++;
	
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

void kradalsa_destroy(krad_alsa_t *kradalsa) {

	printf ("kradalsa: BYE BYE\n");

    snd_pcm_close (kradalsa->device);

	free(kradalsa->samples[0]);
	free(kradalsa->samples[1]);

	free(kradalsa->interleaved_samples);
	free(kradalsa->integer_samples);
	
	free(kradalsa);

}


krad_alsa_t *kradalsa_create(krad_audio_t *kradaudio) {

	krad_alsa_t *kradalsa;

	if ((kradalsa = calloc (1, sizeof (krad_alsa_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
		exit (1);
	}
	
	kradalsa->kradaudio = kradaudio;


	kradalsa->samples[0] = malloc(24 * 8192);
	kradalsa->samples[1] = malloc(24 * 8192);
	kradalsa->interleaved_samples = malloc(48 * 8192);
	kradalsa->integer_samples = malloc(48 * 8192);

	kradalsa->sample_rate = 44100;
	kradalsa->n_channels = 2;
	kradalsa->device_name = "hw:0,0";  
	kradalsa->access = 2;
	kradalsa->buffer_size = 8192;
	kradalsa->period_size = 512;


	/************************************** opens the device *****************************************/

    if ((kradalsa->error = snd_pcm_open (&kradalsa->device, kradalsa->device_name, SND_PCM_STREAM_PLAYBACK, methods[kradalsa->access].open_mode)) < 0) {
        fprintf (stderr, "speaker: Device cannot be opened %s (%s)\n", 
             kradalsa->device_name,
             snd_strerror (kradalsa->error));
        	exit (1);
    }
    fprintf (stderr, "speaker: Device: %s open_mode = %d\n", kradalsa->device_name, methods[kradalsa->access].open_mode);
    
 	//allocating the hardware configuration structure
    if ((kradalsa->error = snd_pcm_hw_params_malloc (&kradalsa->hw_params)) < 0) {
        fprintf (stderr, "speaker: Hardware configuration structure cannot be allocated (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }

    //assigning the hw configuration structure to the device        
    if ((kradalsa->error = snd_pcm_hw_params_any (kradalsa->device, kradalsa->hw_params)) < 0) {
        fprintf (stderr, "speaker: Hardware configuration structure cannot be assigned to device (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }

	/*********************************** shows the audio capture method ****************************/
    
    switch(methods[kradalsa->access].method)
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
    
/*************************** configures access method *********************************************/  
    //sets the configuration method
    fprintf (stderr, "speaker: Access method: %d\n", methods[kradalsa->access].access);
    if ((kradalsa->error = snd_pcm_hw_params_set_access (kradalsa->device, kradalsa->hw_params, methods[kradalsa->access].access)) < 0) {
        fprintf (stderr, "speaker: Access method cannot be configured (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }

    //checks the access method
    if ((kradalsa->error = snd_pcm_hw_params_get_access (kradalsa->hw_params, &kradalsa->real_access)) < 0) {
        fprintf (stderr, "speaker: Access method cannot be obtained (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    //shows the access method
    switch(kradalsa->real_access)
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
/****************************  configures the playback format ******************************/   
    //SND_PCM_FORMAT_S16_LE => 16 bit signed little endian
    if ((kradalsa->error = snd_pcm_hw_params_set_format (kradalsa->device, kradalsa->hw_params, SND_PCM_FORMAT_S32_LE)) < 0)
    {
        fprintf (stderr, "speaker: Playback sample format cannot be configured (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    //checks playback format
    if ((kradalsa->error = snd_pcm_hw_params_get_format (kradalsa->hw_params, &kradalsa->real_sample_format)) < 0)
    {
        fprintf (stderr, "speaker: Playback sample format cannot be obtained (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    //just shows the capture format in a human readable way
    switch(kradalsa->real_sample_format)
    {
    case SND_PCM_FORMAT_S32_LE:
        fprintf (stderr, "speaker: PCM sample format: SND_PCM_FORMAT_S32_LE \n");
        break;
    default:
        fprintf (stderr, "speaker: Real_sample_format = %d\n", kradalsa->real_sample_format);
    }    
/*************************** configures the sample rate ***************************/    
	//sets the sample rate
    if ((kradalsa->error = snd_pcm_hw_params_set_rate (kradalsa->device, kradalsa->hw_params, kradalsa->sample_rate, 0)) < 0) {
        fprintf (stderr, "speaker: Sample rate cannot be configured (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    //cheks the sample rate
    if ((kradalsa->error = snd_pcm_hw_params_get_rate (kradalsa->hw_params, &kradalsa->real_sample_rate, 0)) < 0) {
        fprintf (stderr, "speaker: Sample rate cannot be obtained (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    fprintf (stderr, "speaker: real_sample_rate = %d\n", kradalsa->real_sample_rate);
    
/**************************** configures the number of channels ********************************/    
    //sets the number of channels
    if ((kradalsa->error = snd_pcm_hw_params_set_channels (kradalsa->device, kradalsa->hw_params, kradalsa->n_channels)) < 0) {
        fprintf (stderr, "speaker: Number of channels cannot be configured (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    //checks the number of channels
    if ((kradalsa->error = snd_pcm_hw_params_get_channels (kradalsa->hw_params, &kradalsa->real_n_channels)) < 0) {
        fprintf (stderr, "speaker: Number of channels cannot be obtained (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    fprintf (stderr, "speaker: real_n_channels = %d\n", kradalsa->real_n_channels);
    
/***************************** configures the buffer size *************************************/
    //sets the buffer size
    if (snd_pcm_hw_params_set_buffer_size(kradalsa->device, kradalsa->hw_params, kradalsa->buffer_size) < 0) {
        fprintf (stderr, "speaker: Buffer size cannot be configured (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    //checks the value of the buffer size
    if (snd_pcm_hw_params_get_buffer_size(kradalsa->hw_params, &kradalsa->real_buffer_size) < 0) {
		fprintf (stderr, "speaker: Buffer size cannot be obtained (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    fprintf (stderr, "speaker: Buffer size = %d [frames]\n", (int) kradalsa->real_buffer_size);
/***************************** configures period size *************************************/
    kradalsa->dir=0;//dir=0  =>  period size must be equal to period_size
    //sets the period size
    if ((kradalsa->error = snd_pcm_hw_params_set_period_size(kradalsa->device, kradalsa->hw_params, kradalsa->period_size, kradalsa->dir)) < 0) {
        fprintf (stderr, "speaker: Period size cannot be configured (%s)\n",
             snd_strerror (kradalsa->error));
        //exit (1);
    }
    //checks the value of period size
    if (snd_pcm_hw_params_get_period_size(kradalsa->hw_params, &kradalsa->real_period_size, &kradalsa->dir) < 0) {
    fprintf (stderr, "speaker: Period size cannot be obtained (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    fprintf (stderr, "speaker:  Period size = %d [frames]\n", (int) kradalsa->real_period_size);
/************************* applies the hardware configuration ******************************/
    
    if ((kradalsa->error = snd_pcm_hw_params (kradalsa->device, kradalsa->hw_params)) < 0) {
        fprintf (stderr, "speaker: Hardware parameters cannot be configured (%s)\n",
             snd_strerror (kradalsa->error));
        exit (1);
    }
    //frees the structure of hardware configuration
    snd_pcm_hw_params_free (kradalsa->hw_params);

/*********************** filling capture_device_param *************************************/
    
    kradalsa->capture_device_params.access_type = kradalsa->real_access;          //real access method
    kradalsa->capture_device_params.buffer_size = kradalsa->real_buffer_size;     //real buffer size
    kradalsa->capture_device_params.period_size = kradalsa->real_period_size;     //real period size
    kradalsa->capture_device_params.buffer_time = kradalsa->buffer_time;          //real buffer time
    kradalsa->capture_device_params.period_time = kradalsa->period_time;          //real period time
    kradalsa->capture_device_params.sample_format = kradalsa->real_sample_format; //real samples format
    kradalsa->capture_device_params.sample_rate = kradalsa->real_sample_rate;     //real sample rate 
    kradalsa->capture_device_params.n_channels = kradalsa->n_channels;            //real number of channels

/************************ selects the appropriate access method for audio capture *********************/
    /*
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
	*/
    
	async_rw(kradalsa);

	return kradalsa;
}


