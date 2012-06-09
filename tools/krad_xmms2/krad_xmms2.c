#include "krad_xmms2.h"



void krad_xmms2_destroy (krad_xmms2_t *krad_xmms2) {

	free (krad_xmms2);

}

krad_xmms2_t *krad_xmms2_create () {

	krad_xmms2_t *krad_xmms2 = calloc (1, sizeof(krad_xmms2_t));


	
	return krad_xmms2;

}
