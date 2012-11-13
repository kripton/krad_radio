#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "DeckLinkAPI.h"

#include "krad_decklink_capture.h"



DeckLinkCaptureDelegate::DeckLinkCaptureDelegate() : m_refCount(0)
{
	pthread_mutex_init(&m_mutex, NULL);
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
	pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
	pthread_mutex_lock(&m_mutex);
		m_refCount++;
	pthread_mutex_unlock(&m_mutex);

	return (ULONG)m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
	pthread_mutex_lock(&m_mutex);
		m_refCount--;
	pthread_mutex_unlock(&m_mutex);

	if (m_refCount == 0)
	{
		delete this;
		return 0;
	}

	return (ULONG)m_refCount;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* video_frame, IDeckLinkAudioInputPacket* audio_frame) {

	void *frame_data;
	long int frame_data_size;
	void *audio_data;
	IDeckLinkTimecode *timecode;
	BMDTimecodeFormat timecodeFormat;
	int audio_frames;
	
#ifdef IS_LINUX
	const char *timecodeString;
#endif
#ifdef IS_MACOSX
  CFStringRef timecodeString;
#endif

	timecodeString = NULL;
	timecodeFormat = 0;
	frame_data_size = 0;
	
	if (!((audio_frame) && ((video_frame) && (!(video_frame->GetFlags() & bmdFrameHasNoInputSource))))) {
    printk ("Decklink A/V Capture skipped a frame!");
    return S_OK;
  }

	if (audio_frame) {
		audio_frame->GetBytes(&audio_data);
		audio_frames = audio_frame->GetSampleFrameCount();
		if (krad_decklink_capture->verbose) {
			//printkd ("Audio Frame received %d frames\r", audio_frames);
		}
		if (krad_decklink_capture->audio_frames_callback != NULL) {
			krad_decklink_capture->audio_frames_callback (krad_decklink_capture->callback_pointer, audio_data, audio_frames);
		}
		krad_decklink_capture->audio_frames += audio_frames;
	}
	
	
	if (krad_decklink_capture->skip_frame == 1) {
		krad_decklink_capture->skip_frame = 0;
	    return S_OK;
	}

	if (krad_decklink_capture->skip_frame == 0) {
	
		if ((krad_decklink_capture->display_mode == bmdModeHD720p60) ||
			(krad_decklink_capture->display_mode == bmdModeHD720p5994) ||
			(krad_decklink_capture->display_mode == bmdModeHD1080p5994) ||
			(krad_decklink_capture->display_mode == bmdModeHD1080p6000)) {
	
			krad_decklink_capture->skip_frame = 1;
		}
	}

	if (video_frame) {

		if (video_frame->GetFlags() & bmdFrameHasNoInputSource) {
			printke ("Frame received %lu - No input signal detected\n", krad_decklink_capture->video_frames);
		} else {

			if (timecodeFormat != 0) {
				if (video_frame->GetTimecode(timecodeFormat, &timecode) == S_OK) {
					timecode->GetString(&timecodeString);
				}
			}

			frame_data_size = video_frame->GetRowBytes() * video_frame->GetHeight();

			if (krad_decklink_capture->verbose) {
				//printkd ("Frame received %lu [%s] - Size: %li bytes ", 
				//		krad_decklink_capture->video_frames, 
				//		timecodeString != NULL ? timecodeString : "No timecode",
				//		frame_data_size);
			}

			if (timecodeString) {
#ifdef IS_LINUX
				free ((void*)timecodeString);
#endif
#ifdef IS_MACOSX
				CFRelease(timecodeString);
#endif
			}

			video_frame->GetBytes(&frame_data);

			if (krad_decklink_capture->video_frame_callback != NULL) {
				krad_decklink_capture->video_frame_callback (krad_decklink_capture->callback_pointer, frame_data, frame_data_size);
			}

		}

		krad_decklink_capture->video_frames++;

	}

    return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags) {

	printke ("ruh oh! video format changed?!?\n");

    return S_OK;
}

extern "C" {

krad_decklink_capture_t *krad_decklink_capture_create(int device) {

	int d;
	krad_decklink_capture_t *krad_decklink_capture = (krad_decklink_capture_t *)calloc(1, sizeof(krad_decklink_capture_t));
	
	krad_decklink_capture->device = device;
	krad_decklink_capture->inputFlags = 0;
	krad_decklink_capture->audio_input = bmdAudioConnectionEmbedded;	
	krad_decklink_capture->audio_sample_rate = bmdAudioSampleRate48kHz;
	krad_decklink_capture->audio_channels = 2;
	krad_decklink_capture->audio_bit_depth = 16;
	
	krad_decklink_capture->deckLinkIterator = CreateDeckLinkIteratorInstance();
	
	if (!krad_decklink_capture->deckLinkIterator) {
		printke("Krad Decklink: No DeckLink drivers installed.");
		free (krad_decklink_capture);
		return NULL;
	}
	
	for (d = 0; d < 64; d++) {
	
		krad_decklink_capture->result = krad_decklink_capture->deckLinkIterator->Next(&krad_decklink_capture->deckLink);
		if (krad_decklink_capture->result != S_OK) {
			printke ("Krad Decklink: No DeckLink devices found.");
			free (krad_decklink_capture);
			return NULL;
		}
		
		if (d == krad_decklink_capture->device) {
			break;
		}
		
	}

	return krad_decklink_capture;
	
}

void krad_decklink_capture_set_audio_input(krad_decklink_capture_t *krad_decklink_capture, char *audio_input) {

	krad_decklink_capture->audio_input = bmdAudioConnectionEmbedded;

	if (strlen(audio_input)) {

		if ((strstr(audio_input, "Analog") == 0) || (strstr(audio_input, "analog") == 0)) {
			printk ("Krad Decklink: Selected Analog Audio Input");
			krad_decklink_capture->audio_input = bmdAudioConnectionAnalog;
			return;
		}

		if ((strstr(audio_input, "AESEBU") == 0) || (strstr(audio_input, "aesebu") == 0) || 
			(strstr(audio_input, "SPDIF") == 0) || (strstr(audio_input, "spdif") == 0)) {
			printk ("Krad Decklink: Selected AESEBU Audio Input");		
			krad_decklink_capture->audio_input = bmdAudioConnectionAESEBU;
			return;
		}
	
	}

	printk ("Krad Decklink: Selected Embedded Audio Input");
	
}


void krad_decklink_capture_set_video_mode(krad_decklink_capture_t *krad_decklink_capture, int width, int height,
										  int fps_numerator, int fps_denominator) {


	krad_decklink_capture->width = width;
	krad_decklink_capture->height = height;
	krad_decklink_capture->fps_numerator = fps_numerator;
	krad_decklink_capture->fps_denominator = fps_denominator;

	krad_decklink_capture->pixel_format = bmdFormat8BitYUV;
	//krad_decklink_capture->pixel_format = bmdFormat8BitARGB;	

	if ((krad_decklink_capture->width == 720) && (krad_decklink_capture->height == 486)) {
		if ((krad_decklink_capture->fps_numerator == 30000) && (krad_decklink_capture->fps_denominator == 1001)) {
			krad_decklink_capture->display_mode = bmdModeNTSC;
		}
		if ((krad_decklink_capture->fps_numerator == 60000) && (krad_decklink_capture->fps_denominator == 1001)) {
			krad_decklink_capture->display_mode = bmdModeNTSCp;
		}
	}
	

	if ((krad_decklink_capture->width == 720) && (krad_decklink_capture->height == 576)) {
		if (((krad_decklink_capture->fps_numerator == 25) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 25000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModePAL;
		}
		if (((krad_decklink_capture->fps_numerator == 50) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 50000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModePALp;
		}
	}	

	if ((krad_decklink_capture->width == 1280) && (krad_decklink_capture->height == 720)) {
		if (((krad_decklink_capture->fps_numerator == 50) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 50000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModeHD720p50;
		}
		if ((krad_decklink_capture->fps_numerator == 60000) && (krad_decklink_capture->fps_denominator == 1001)) {
			krad_decklink_capture->display_mode = bmdModeHD720p5994;
		}
		if (((krad_decklink_capture->fps_numerator == 60) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 60000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModeHD720p60;
		}
	}
	
	if ((krad_decklink_capture->width == 1920) && (krad_decklink_capture->height == 1080)) {
		if ((krad_decklink_capture->fps_numerator == 24000) && (krad_decklink_capture->fps_denominator == 1001)) {
			krad_decklink_capture->display_mode = bmdModeHD1080p2398;
		}
		if (((krad_decklink_capture->fps_numerator == 24) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 24000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModeHD1080p24;
		}
		if (((krad_decklink_capture->fps_numerator == 25) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 25000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModeHD1080p25;
		}
		if ((krad_decklink_capture->fps_numerator == 30000) && (krad_decklink_capture->fps_denominator == 1001)) {
			krad_decklink_capture->display_mode = bmdModeHD1080p2997;
		}
		if (((krad_decklink_capture->fps_numerator == 30) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 30000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModeHD1080p30;
		}
		if (((krad_decklink_capture->fps_numerator == 50) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 50000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModeHD1080p50;
		}
		if ((krad_decklink_capture->fps_numerator == 60000) && (krad_decklink_capture->fps_denominator == 1001)) {
			krad_decklink_capture->display_mode = bmdModeHD1080p5994;
		}
		if (((krad_decklink_capture->fps_numerator == 60) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 60000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdModeHD1080p6000;
		}
	}	

	if ((krad_decklink_capture->width == 2048) && (krad_decklink_capture->height == 1556)) {
		if ((krad_decklink_capture->fps_numerator == 24000) && (krad_decklink_capture->fps_denominator == 1001)) {
			krad_decklink_capture->display_mode = bmdMode2k2398;
		}
		if (((krad_decklink_capture->fps_numerator == 24) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 24000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdMode2k24;
		}
		if (((krad_decklink_capture->fps_numerator == 25) && (krad_decklink_capture->fps_denominator == 1)) || 
			 ((krad_decklink_capture->fps_numerator == 25000) && (krad_decklink_capture->fps_denominator == 1000))) {
			krad_decklink_capture->display_mode = bmdMode2k25;
		}
	}
}
	

void krad_decklink_capture_set_video_callback(krad_decklink_capture_t *krad_decklink_capture, int video_frame_callback(void *, void *, int)) {
	krad_decklink_capture->video_frame_callback = video_frame_callback;
}

void krad_decklink_capture_set_audio_callback(krad_decklink_capture_t *krad_decklink_capture, int audio_frames_callback(void *, void *, int)) {
	krad_decklink_capture->audio_frames_callback = audio_frames_callback;
}

void krad_decklink_capture_set_callback_pointer(krad_decklink_capture_t *krad_decklink_capture, void *callback_pointer) {
	krad_decklink_capture->callback_pointer = callback_pointer;
}

void krad_decklink_capture_set_verbose(krad_decklink_capture_t *krad_decklink_capture, int verbose) {
	krad_decklink_capture->verbose = verbose;
}

void krad_decklink_capture_start(krad_decklink_capture_t *krad_decklink_capture) {
    
	krad_decklink_capture->result = krad_decklink_capture->deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&krad_decklink_capture->deckLinkInput);
	if (krad_decklink_capture->result != S_OK) {
		printke ("Krad Decklink: Fail QueryInterface\n");
	}

	krad_decklink_capture->result = krad_decklink_capture->deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&krad_decklink_capture->deckLinkConfiguration);
	if (krad_decklink_capture->result != S_OK) {
		printke ("Krad Decklink: Fail QueryInterface to get configuration\n");
	} else {

		krad_decklink_capture->result = krad_decklink_capture->deckLinkConfiguration->SetInt(bmdDeckLinkConfigAudioInputConnection, krad_decklink_capture->audio_input);
		if (krad_decklink_capture->result != S_OK) {
			printke ("Krad Decklink: Fail confguration to set bmdAudioConnectionEmbedded\n");
		}
	}

	krad_decklink_capture->delegate = new DeckLinkCaptureDelegate();
	krad_decklink_capture->delegate->krad_decklink_capture = krad_decklink_capture;
	krad_decklink_capture->deckLinkInput->SetCallback(krad_decklink_capture->delegate);

	krad_decklink_capture->result = krad_decklink_capture->deckLinkInput->EnableVideoInput(krad_decklink_capture->display_mode, krad_decklink_capture->pixel_format, krad_decklink_capture->inputFlags);
	if (krad_decklink_capture->result != S_OK) {
		printke ("Krad Decklink: Fail EnableVideoInput\n");
	}

	krad_decklink_capture->result = krad_decklink_capture->deckLinkInput->EnableAudioInput(krad_decklink_capture->audio_sample_rate, krad_decklink_capture->audio_bit_depth, krad_decklink_capture->audio_channels);
	if (krad_decklink_capture->result != S_OK) {
		printke ("Krad Decklink: Fail EnableAudioInput\n");
	}

	krad_decklink_capture->result = krad_decklink_capture->deckLinkInput->StartStreams();
	if (krad_decklink_capture->result != S_OK) {
		printke ("Krad Decklink: Fail StartStreams\n");
	}
	
}

void krad_decklink_capture_stop(krad_decklink_capture_t *krad_decklink_capture) {

    if (krad_decklink_capture->deckLinkInput != NULL) {
    
		krad_decklink_capture->result = krad_decklink_capture->deckLinkInput->StopStreams ();
		if (krad_decklink_capture->result != S_OK) {
			printke ("Krad Decklink: Fail StopStreams\n");
		}		
		krad_decklink_capture->result = krad_decklink_capture->deckLinkInput->DisableVideoInput ();
		if (krad_decklink_capture->result != S_OK) {
			printke ("Krad Decklink: Fail DisableVideoInput\n");
		}
		krad_decklink_capture->result = krad_decklink_capture->deckLinkInput->DisableAudioInput ();
		if (krad_decklink_capture->result != S_OK) {
			printke ("Krad Decklink: Fail DisableAudioInput\n");
		}
		
		krad_decklink_capture->result = krad_decklink_capture->deckLinkConfiguration->Release();
		if (krad_decklink_capture->result != S_OK) {
			printke ("Krad Decklink: Fail to Release deckLinkConfiguration\n");
		}
		krad_decklink_capture->deckLinkConfiguration = NULL;
		
		krad_decklink_capture->result = krad_decklink_capture->deckLinkInput->Release();
		if (krad_decklink_capture->result != S_OK) {
			printke ("Krad Decklink: Fail Release\n");
		}
        krad_decklink_capture->deckLinkInput = NULL;
    }

    if (krad_decklink_capture->deckLink != NULL) {
        krad_decklink_capture->deckLink->Release();
        krad_decklink_capture->deckLink = NULL;
    }

	if (krad_decklink_capture->deckLinkIterator != NULL) {
		krad_decklink_capture->deckLinkIterator->Release();
	}
	
	//printf("\n");

	free(krad_decklink_capture);

}
#ifdef IS_LINUX
void krad_decklink_capture_info () {

	IDeckLink *deckLink;
	IDeckLinkInput *deckLinkInput;
	IDeckLinkIterator *deckLinkIterator;
	IDeckLinkDisplayModeIterator *displayModeIterator;
	IDeckLinkDisplayMode *displayMode;
	
	HRESULT result;
	int displayModeCount;
	char *displayModeString;

	displayModeString = NULL;
	displayModeCount = 0;
	
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	
	if (!deckLinkIterator) {
		printke ("Krad Decklink: This application requires the DeckLink drivers installed.\n");
	}
	
	/* Connect to the first DeckLink instance */
	result = deckLinkIterator->Next(&deckLink);
	if (result != S_OK) {
		printke ("Krad Decklink: No DeckLink PCI cards found.\n");
	}
    
	result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
	if (result != S_OK) {
		printke ("Krad Decklink: Fail QueryInterface\n");
	}
	
	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK) {
		printke ("Krad Decklink: Could not obtain the video output display mode iterator - result = %08x\n", result);
	}


    while (displayModeIterator->Next(&displayMode) == S_OK) {

        result = displayMode->GetName((const char **) &displayModeString);
        
        if (result == S_OK) {
			
			BMDTimeValue frameRateDuration, frameRateScale;
			displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);

			printkd ("%2d:  %-20s \t %li x %li \t %g FPS\n", 
				displayModeCount, displayModeString, displayMode->GetWidth(), displayMode->GetHeight(), 
				(double)frameRateScale / (double)frameRateDuration);
			
            free (displayModeString);
			displayModeCount++;
		}
		
		displayMode->Release();
	}
	
	if (displayModeIterator != NULL) {
		displayModeIterator->Release();
		displayModeIterator = NULL;
	}

    if (deckLinkInput != NULL) {
        deckLinkInput->Release();
        deckLinkInput = NULL;
    }

    if (deckLink != NULL) {
        deckLink->Release();
        deckLink = NULL;
    }

	if (deckLinkIterator != NULL) {
		deckLinkIterator->Release();
	}
}
#endif

int krad_decklink_cpp_detect_devices () {

	IDeckLinkIterator *deckLinkIterator;
	IDeckLink *deckLink;
	int device_count;
	//HRESULT result;
	
	device_count = 0;
	
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	
	if (deckLinkIterator == NULL) {
		printke ("krad_decklink_detect_devices: The DeckLink drivers may not be installed.");
		return 0;
	}
	
	while (deckLinkIterator->Next(&deckLink) == S_OK) {

		device_count++;
		
		deckLink->Release();
	}
	
	deckLinkIterator->Release();
	
	return device_count;

}


void krad_decklink_cpp_get_device_name (int device_num, char *device_name) {

	IDeckLinkIterator *deckLinkIterator;
	IDeckLink *deckLink;
	int device_count;
	HRESULT result;
#ifdef IS_LINUX
	char *device_name_temp;
#endif
#ifdef IS_MACOSX
  CFStringRef device_name_temp;
#endif
	device_name_temp = NULL;
	device_count = 0;
	
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	
	if (deckLinkIterator == NULL) {
		printke ("krad_decklink_detect_devices: The DeckLink drivers may not be installed.");
		return;
	}
	
	while (deckLinkIterator->Next(&deckLink) == S_OK) {

		if (device_count == device_num) {
#ifdef IS_LINUX
			result = deckLink->GetModelName((const char **) &device_name_temp);
			if (result == S_OK) {
				strcpy(device_name, device_name_temp);
				free(device_name_temp);
#endif
#ifdef IS_MACOSX
			result = deckLink->GetModelName(&device_name_temp);
			if (result == S_OK) {
        CFStringGetCString(device_name_temp, device_name, 64, kCFStringEncodingMacRoman);
				CFRelease(device_name_temp);
#endif

			} else {
				strcpy(device_name, "Unknown Error in GetModelName");
			}
			deckLink->Release();
			deckLinkIterator->Release();
			return;
		}

		device_count++;
		deckLink->Release();
	}
	
	deckLinkIterator->Release();
	
	sprintf(device_name, "Could not get a device name for device %d", device_num);
	
	return;

}




}

	

