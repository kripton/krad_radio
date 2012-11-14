#include "krad_y4m.h"


void krad_y4m_destroy (krad_y4m_t *krad_y4m) {


	free (krad_y4m->planes[0]);
	free (krad_y4m->planes[1]);
	free (krad_y4m->planes[2]);

	free (krad_y4m);
}


krad_y4m_t *krad_y4m_create (int width, int height, int color_depth) {

	krad_y4m_t *krad_y4m = calloc(1, sizeof(krad_y4m_t));

	if (krad_y4m == NULL) {
		failfast ("Krad y4m: Out of memory");
	}

  krad_y4m->width = width;
  krad_y4m->height = height;
  krad_y4m->color_depth = color_depth;

	krad_y4m->size[0] = krad_y4m->width * krad_y4m->height;
	krad_y4m->size[1] = ((krad_y4m->width / 2) * (krad_y4m->height / 2));
	krad_y4m->size[2] = ((krad_y4m->width / 2) * (krad_y4m->height / 2));
	

	krad_y4m->planes[0] = malloc(krad_y4m->size[0]);
	krad_y4m->planes[1] = malloc(krad_y4m->size[1]);
	krad_y4m->planes[2] = malloc(krad_y4m->size[2]);
	
	krad_y4m->strides[0] = krad_y4m->width;
	krad_y4m->strides[1] = krad_y4m->width / 2;
	krad_y4m->strides[2] = krad_y4m->width / 2;

  krad_y4m->frame_size = krad_y4m->size[0] + krad_y4m->size[1] + krad_y4m->size[2];

	return krad_y4m;

}

int krad_y4m_generate_header (unsigned char *header, int width, int height, int frame_rate_numerator, int frame_rate_denominator, int color_depth) {

  int n;
  char *colorspace;
  
  colorspace = NULL;
  n = 0;

  switch (color_depth) {
    case 420:
        colorspace = "";
        break;
    case 422:
        colorspace = "C422";
        break;
    case 444:
        colorspace = "C444";
        break;
  }

  n = sprintf((char *)header, "%s W%d H%d F%d:%d Ip A1:1 %s\n",
              Y4M_MAGIC, width, height,
              frame_rate_numerator, frame_rate_denominator, colorspace);

  return n;
}


