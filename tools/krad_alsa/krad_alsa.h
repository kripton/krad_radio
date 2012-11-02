typedef struct krad_alsa_St krad_alsa_t;


#include "krad_audio.h"

#ifndef KRAD_ALSA_H
#define KRAD_ALSA_H

#define DEFAULT_ALSA_CAPTURE_DEVICE "hw:1,0"
#define DEFAULT_ALSA_PLAYBACK_DEVICE "hw:0,0"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#include <string.h>
#include <math.h>
#include <memory.h>

#include <stdint.h>
#include <limits.h>
#include <endian.h>


#define f_round(f) lrintf(f)

//general configuration parameters of the device
struct device_parameters {
    snd_pcm_sframes_t buffer_size;      //buffer size in frames
    snd_pcm_sframes_t period_size;      //period size in frames
    unsigned int buffer_time;           //length of the circular buffer in usec
    unsigned int period_time;           //length of one period in usec 
    int n_channels;                     //number of channels
    unsigned int sample_rate;           //frame rate
    snd_pcm_format_t sample_format;     //format of the samples
    snd_pcm_access_t access_type;       //PCM access type
};

//Enum needed to choose the type of I/O loop
typedef enum {
    METHOD_DIRECT_RW,   //method with direct use of read/write functions
    METHOD_DIRECT_MMAP, //method with direct use of memory mapping
    METHOD_ASYNC_MMAP,  //method with async use of memory mapping
    METHOD_ASYNC_RW,    //method with async use of read/write functions
    METHOD_RW_AND_POLL, //method with use of read/write functions and pool
    METHOD_DIRECT_RW_NI //method with direct use of read/write and non-interleaved format (not implemented)
} enum_io_method;

//struct that defines one I/O method
struct io_method {
    enum_io_method method;   //I/O loop type
    snd_pcm_access_t access; //PCM access type
    int open_mode;           //open function flags
};


struct krad_alsa_St {

	krad_audio_t *krad_audio;
    
	int dir;
	int error;
	unsigned int sample_rate;
	unsigned int real_sample_rate;//real frame rate
	int n_channels;
	unsigned int real_n_channels;
	char * device_name;
	snd_pcm_t *device;
	snd_pcm_hw_params_t *hw_params;
	int access;
	snd_pcm_sframes_t buffer_size;
	snd_pcm_sframes_t period_size;
	snd_pcm_uframes_t real_buffer_size;
	snd_pcm_uframes_t real_period_size;
	unsigned int buffer_time;
	unsigned int period_time;
	int sample_size;
	snd_pcm_format_t sample_format;
	snd_pcm_format_t real_sample_format;
	snd_pcm_access_t real_access;
	struct device_parameters capture_device_params;
    
	float *samples[8];
	float *interleaved_samples;
	int *integer_samples;
    
    int capture;
    int playback;
    int stream;
    

  int card_num;
  int control_enabled;
  char control_name[16];

  snd_ctl_t *control;
  int controls_count;
	snd_ctl_elem_list_t *control_list;
	snd_ctl_elem_info_t *info;

};


void krad_alsa_list_cards ();

void krad_alsa_destroy (krad_alsa_t *krad_alsa);
krad_alsa_t *krad_alsa_create (krad_audio_t *krad_audio, int card_num);


#endif
