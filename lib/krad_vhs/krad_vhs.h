#ifndef KRAD_VHS_H

#ifdef __cplusplus
extern "C" {
#include <libswscale/swscale.h>
}
#endif

#ifdef __cplusplus

#include <image.h>
#include <libiz.h>


class KrImage : public IZ::Image<>
{
public:
    KrImage();
    ~KrImage();


private:
    int m_components;
    int m_maxVal;
};

#endif

#ifdef __cplusplus
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
krad_vhs_t *krad_vhs_create ();

int krad_vhs_encode (krad_vhs_t *krad_vhs, unsigned char *pixels);
int krad_vhs_decode (krad_vhs_t *krad_vhs, unsigned char *buffer, unsigned char *pixels);

#ifdef __cplusplus
}
#endif

#endif
