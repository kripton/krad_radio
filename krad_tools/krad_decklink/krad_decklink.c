#include <krad_decklink.h>



void krad_decklink_destroy(krad_decklink_t *krad_decklink) {

	krad_decklink_capture_stop (krad_decklink->krad_decklink_capture);

	free(krad_decklink);

}

krad_decklink_t *krad_decklink_create() {

	krad_decklink_t *krad_decklink = (krad_decklink_t *)calloc(1, sizeof(krad_decklink_t));

	krad_decklink_info ();
	
	krad_decklink->krad_decklink_capture = krad_decklink_capture_start();
	
	return krad_decklink;

}





