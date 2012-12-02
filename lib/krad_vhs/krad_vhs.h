#ifndef KRAD_VHS_H

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
extern "C" {
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}

#include <image.h>
#include <libiz.h>

class KrImage : public IZ::Image<>
{
public:
  KrImage();
  ~KrImage();
};

extern "C" {
#endif

typedef struct krad_vhs_St krad_vhs_t;

struct krad_vhs_St {

  int encoder;
  int width;
  int height;
  
	struct SwsContext *converter;	

  unsigned char *buffer;
  unsigned char *enc_buffer;  

};

void krad_vhs_destroy (krad_vhs_t *krad_vhs);
krad_vhs_t *krad_vhs_create_encoder (int width, int height);
krad_vhs_t *krad_vhs_create_decoder ();
int krad_vhs_encode (krad_vhs_t *krad_vhs, unsigned char *pixels);
int krad_vhs_decode (krad_vhs_t *krad_vhs, unsigned char *buffer, unsigned char *pixels);

#ifdef __cplusplus
}
#endif

#endif
