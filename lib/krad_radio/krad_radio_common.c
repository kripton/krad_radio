#include "krad_radio_common.h"


kr_radio_t *kr_radio_rep_create () {
  kr_radio_t *radio_rep;
  radio_rep = calloc (1, sizeof (kr_radio_t));
  return radio_rep;
}

void kr_radio_rep_destroy (kr_radio_t *radio_rep) {
  free (radio_rep);
}
