#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>

#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>

#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

#define interface (vpx_codec_vp8_cx())

typedef struct krad_vpx_encoder_St krad_vpx_encoder_t;
typedef struct krad_vpx_decoder_St krad_vpx_decoder_t;

struct krad_vpx_encoder_St {

	int width;
	int height;

    vpx_codec_ctx_t encoder;
    vpx_codec_enc_cfg_t cfg;
	vpx_image_t *image;
    vpx_codec_err_t	res;
	vpx_codec_iter_t iter;
	const vpx_codec_cx_pkt_t *pkt;

    int flags;
	int frames;
	unsigned long quality;
	
	int frame_byte_size;
	unsigned char *rgb_frame_data;
	pthread_rwlock_t frame_lock;
	
	float preview_angle;
	
};

struct krad_vpx_decoder_St {

	int width;
	int height;

    int flags;
	int frames;
	int quality;
    int dec_flags;

    vpx_codec_ctx_t codec;
    vpx_codec_err_t	res;
    vpx_dec_ctx_t decoder;
    vp8_postproc_cfg_t ppcfg;
    vpx_codec_dec_cfg_t cfg;
	vpx_codec_stream_info_t stream_info;
	vpx_dec_iter_t iter;
	vpx_image_t *img;

    uint8_t *buf;
	unsigned char compressed_video_buffer[800000];

	unsigned char *frame_data;

};

/* public */

void krad_vpx_encoder_finish(krad_vpx_encoder_t *kradvpx);
void krad_vpx_encoder_set(krad_vpx_encoder_t *kradvpx, vpx_codec_enc_cfg_t *cfg);
krad_vpx_encoder_t *krad_vpx_encoder_create(int width, int height, int bitrate);
void krad_vpx_encoder_destroy(krad_vpx_encoder_t *kradvpx);
int krad_vpx_encoder_write(krad_vpx_encoder_t *kradvpx, unsigned char **packet, int *keyframe);


krad_vpx_decoder_t *krad_vpx_decoder_create();
void krad_vpx_decoder_destroy(krad_vpx_decoder_t *kradvpx);
void krad_vpx_decoder_decode(krad_vpx_decoder_t *kradvpx, void *buffer, int len);

/* private */
void krad_vpx_fail(vpx_codec_ctx_t *ctx, const char *s);

