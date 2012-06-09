#include "krad_decklink.h"

void krad_decklink_info (krad_decklink_t *krad_decklink) {

	krad_decklink_capture_info();

}

void krad_decklink_destroy (krad_decklink_t *krad_decklink) {

	int c;
	
	krad_decklink_stop(krad_decklink);

	for (c = 0; c < 2; c++) {
		free(krad_decklink->samples[c]);	
	}

	//free (krad_decklink->captured_frame_rgb);

	free (krad_decklink);

}

krad_decklink_t *krad_decklink_create() {

	int c;

	krad_decklink_t *krad_decklink = (krad_decklink_t *)calloc(1, sizeof(krad_decklink_t));
	
	//krad_decklink->captured_frame_rgb = malloc(1920 * 1080 * 4);
	
	for (c = 0; c < 2; c++) {
		krad_decklink->samples[c] = malloc(4 * 8192);
	}
	
	return krad_decklink;

}

void krad_decklink_set_verbose (krad_decklink_t *krad_decklink, int verbose) {

	krad_decklink->verbose = verbose;
	if (krad_decklink->krad_decklink_capture != NULL) {
		krad_decklink_capture_set_verbose(krad_decklink->krad_decklink_capture, krad_decklink->verbose);
	}
}

void krad_decklink_start(krad_decklink_t *krad_decklink) {

	krad_decklink->krad_decklink_capture = krad_decklink_capture_create();

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





