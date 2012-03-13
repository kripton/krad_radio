#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#define KRAD_DECKLINK_H
#include <krad_decklink_capture.h>

typedef struct krad_decklink_St krad_decklink_t;

struct krad_decklink_St {

	krad_decklink_capture_t * krad_decklink_capture;


};



void krad_decklink_destroy(krad_decklink_t *decklink);
krad_decklink_t *krad_decklink_create();

