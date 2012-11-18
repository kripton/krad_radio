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

	krad_y4m->size[0] = krad_y4m->width * krad_y4m->height;
	krad_y4m->strides[0] = krad_y4m->width;

  switch (color_depth) {
    case PIX_FMT_YUV420P:
    case 420:
        krad_y4m->color_depth = PIX_FMT_YUV420P;
        krad_y4m->size[1] = ((krad_y4m->width / 2) * (krad_y4m->height / 2));
        krad_y4m->size[2] = ((krad_y4m->width / 2) * (krad_y4m->height / 2));
        krad_y4m->strides[1] = krad_y4m->width / 2;
        krad_y4m->strides[2] = krad_y4m->width / 2;
        break;
    case PIX_FMT_YUV422P:
    case 422:    
        krad_y4m->color_depth = PIX_FMT_YUV422P;
        krad_y4m->size[1] = ((krad_y4m->width / 2) * krad_y4m->height);
        krad_y4m->size[2] = ((krad_y4m->width / 2) * krad_y4m->height);
        krad_y4m->strides[1] = krad_y4m->width / 2;
        krad_y4m->strides[2] = krad_y4m->width / 2;        
        break;
    case PIX_FMT_YUV444P:
    case 444:
        krad_y4m->color_depth = PIX_FMT_YUV444P;
        krad_y4m->size[1] = krad_y4m->width * krad_y4m->height;
        krad_y4m->size[2] = krad_y4m->width * krad_y4m->height;
        krad_y4m->strides[1] = krad_y4m->width;
        krad_y4m->strides[2] = krad_y4m->width;        
        break;
  }
	
	krad_y4m->planes[0] = malloc(krad_y4m->size[0]);
	krad_y4m->planes[1] = malloc(krad_y4m->size[1]);
	krad_y4m->planes[2] = malloc(krad_y4m->size[2]);

  krad_y4m->frame_size = krad_y4m->size[0] + krad_y4m->size[1] + krad_y4m->size[2];

	return krad_y4m;

}

int krad_y4m_generate_header (unsigned char *header, int width, int height, int frame_rate_numerator, int frame_rate_denominator, int color_depth) {

  int n;
  char *colorspace;
  
  colorspace = NULL;
  n = 0;

  switch (color_depth) {
    case PIX_FMT_YUV420P:
    case 420:
        colorspace = "";
        break;
    case PIX_FMT_YUV422P:
    case 422:    
        colorspace = "C422";
        break;
    case PIX_FMT_YUV444P:
    case 444:    
        colorspace = "C444";
        break;
  }

  n = sprintf((char *)header, "%s W%d H%d F%d:%d Ip A1:1 %s\n",
              Y4M_MAGIC, width, height,
              frame_rate_numerator, frame_rate_denominator, colorspace);

  return n;
}

int krad_y4m_parse_header (unsigned char *buffer, int *width, int *height, int *frame_rate_numerator,
                           int *frame_rate_denominator, int *color_depth) {

  int i;
  char *tokstart, *tokend, *header_end;
  char header[MAX_YUV4_HEADER_SIZE];

  i = 0;

  for (i = 0; i < MAX_YUV4_HEADER_SIZE; i++) {
    header[i] = buffer[i];
    if (header[i] == '\n') {
      header[i + 1] = 0x20;
      header[i + 2] = 0;
      break;
    }
  }

  if (strncmp(header, Y4M_MAGIC, strlen(Y4M_MAGIC))) {
    return -1;
  }

  header_end = &header[i + 1]; // Include space
  for (tokstart = &header[strlen(Y4M_MAGIC) + 1];
       tokstart < header_end; tokstart++) {
      if (*tokstart == 0x20)
          continue;
      switch (*tokstart++) {
      case 'W': // Width. Required.
          *width    = strtol(tokstart, &tokend, 10);
          tokstart = tokend;
          break;
      case 'H': // Height. Required.
          *height   = strtol(tokstart, &tokend, 10);
          tokstart = tokend;
          break;
      case 'C': // Color space
          if (strncmp("420jpeg", tokstart, 7) == 0) {
              *color_depth = PIX_FMT_YUV420P;
          } else if (strncmp("420mpeg2", tokstart, 8) == 0) {
              *color_depth = PIX_FMT_YUV420P;
          } else if (strncmp("420paldv", tokstart, 8) == 0) {
              *color_depth = PIX_FMT_YUV420P;
          } else if (strncmp("420", tokstart, 3) == 0) {
              *color_depth = PIX_FMT_YUV420P;
          } else if (strncmp("411", tokstart, 3) == 0)
              *color_depth = PIX_FMT_YUV411P;
          else if (strncmp("422", tokstart, 3) == 0)
              *color_depth = PIX_FMT_YUV422P;
          else if (strncmp("444alpha", tokstart, 8) == 0 ) {
              return -1;
          } else if (strncmp("444", tokstart, 3) == 0)
              *color_depth = PIX_FMT_YUV444P;
          else if (strncmp("mono", tokstart, 4) == 0) {
              *color_depth = PIX_FMT_GRAY8;
          } else {
              return -1;
          }
          while (tokstart < header_end && *tokstart != 0x20)
              tokstart++;
          break;
      case 'I': // Interlace type
        return -1;
        break;
      case 'F': // Frame rate
          sscanf(tokstart, "%d:%d", frame_rate_numerator, frame_rate_denominator); // 0:0 if unknown
          while (tokstart < header_end && *tokstart != 0x20)
              tokstart++;
          break;
      case 'A': // Pixel aspect
          //sscanf(tokstart, "%d:%d", &aspectn, &aspectd); // 0:0 if unknown
          while (tokstart < header_end && *tokstart != 0x20)
              tokstart++;
          break;
      case 'X': // Vendor extensions
          while (tokstart < header_end && *tokstart != 0x20)
              tokstart++;
          break;
      }
  }

  if (*width == -1 || *height == -1) {
      return -1;
  }

  if (*color_depth == PIX_FMT_NONE) {
    *color_depth = PIX_FMT_YUV420P;
  }

  if (*frame_rate_numerator <= 0 || *frame_rate_denominator <= 0) {
      // Frame rate unknown
      *frame_rate_numerator = 30;
      *frame_rate_denominator = 1;
  }

  return i;

}

