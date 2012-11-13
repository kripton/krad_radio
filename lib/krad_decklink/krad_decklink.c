#include "krad_decklink.h"

void krad_decklink_info (krad_decklink_t *krad_decklink) {
#ifdef IS_LINUX
	krad_decklink_capture_info();
#endif
}

void krad_decklink_destroy (krad_decklink_t *krad_decklink) {

	int c;
	
	krad_decklink_stop(krad_decklink);

	for (c = 0; c < 2; c++) {
		free(krad_decklink->samples[c]);	
	}

	free (krad_decklink);

}

krad_decklink_t *krad_decklink_create(char *device) {

	int c;

	krad_decklink_t *krad_decklink = (krad_decklink_t *)calloc(1, sizeof(krad_decklink_t));
	
	krad_decklink->devicenum = atoi(device);
	if (krad_decklink->devicenum > 0) {
		sprintf(krad_decklink->simplename, "Decklink%d", krad_decklink->devicenum);
	} else {
		sprintf(krad_decklink->simplename, "Decklink");
	}

	for (c = 0; c < 2; c++) {
		krad_decklink->samples[c] = malloc(4 * 8192);
	}
	
	krad_decklink->krad_decklink_capture = krad_decklink_capture_create(krad_decklink->devicenum);
	
	return krad_decklink;

}

void krad_decklink_set_video_mode(krad_decklink_t *krad_decklink, int width, int height,
								  int fps_numerator, int fps_denominator) {			  
					
	printk ("Krad Decklink set video mode: %dx%d - %d / %d",
	        width, height, fps_numerator, fps_denominator);
						  
	krad_decklink_capture_set_video_mode(krad_decklink->krad_decklink_capture, width, height,
										 fps_numerator, fps_denominator);
						  
}

void krad_decklink_set_audio_input (krad_decklink_t *krad_decklink, char *audio_input) {

	krad_decklink_capture_set_audio_input (krad_decklink->krad_decklink_capture, audio_input);

}

void krad_decklink_set_verbose (krad_decklink_t *krad_decklink, int verbose) {

	krad_decklink->verbose = verbose;
	if (krad_decklink->krad_decklink_capture != NULL) {
		krad_decklink_capture_set_verbose(krad_decklink->krad_decklink_capture, krad_decklink->verbose);
	}
}

void krad_decklink_start(krad_decklink_t *krad_decklink) {

	krad_decklink_capture_set_video_callback(krad_decklink->krad_decklink_capture, krad_decklink->video_frame_callback);
	krad_decklink_capture_set_audio_callback(krad_decklink->krad_decklink_capture, krad_decklink->audio_frames_callback);
	krad_decklink_capture_set_callback_pointer(krad_decklink->krad_decklink_capture, krad_decklink->callback_pointer);
	krad_decklink_capture_set_verbose(krad_decklink->krad_decklink_capture, krad_decklink->verbose);
	
	krad_decklink_capture_start(krad_decklink->krad_decklink_capture);

}

void krad_decklink_stop(krad_decklink_t *krad_decklink) {

	if (krad_decklink->krad_decklink_capture != NULL) {
		krad_decklink_capture_stop (krad_decklink->krad_decklink_capture);
		krad_decklink->krad_decklink_capture = NULL;
	}

}


int krad_decklink_detect_devices () {

	return krad_decklink_cpp_detect_devices();

}

void krad_decklink_get_device_name (int device_num, char *device_name) {

	krad_decklink_cpp_get_device_name (device_num, device_name);

}

