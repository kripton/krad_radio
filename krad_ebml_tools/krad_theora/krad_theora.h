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

#include <theora/theoradec.h>
#include <theora/theoraenc.h>

#include <libswscale/swscale.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

//typedef struct krad_theora_encoder_St krad_theora_encoder_t;
typedef struct krad_theora_decoder_St krad_theora_decoder_t;
/*
struct krad_theora_encoder_St {

	struct SwsContext *rgb_sws_context;
	struct SwsContext *sws_context;
	int width;
	int height;

    int flags;
	int frames;
	int quality;
	
	int frame_byte_size;
	unsigned char *rgb_frame_data;
	pthread_rwlock_t frame_lock;
	
	float preview_angle;
	
};
*/
struct krad_theora_decoder_St {

	int width;
	int height;

    int flags;
	int frames;
	int quality;
    int dec_flags;

	th_info	info;
	th_comment	comment;
	th_setup_info	*setup_info;
	th_dec_ctx	*decoder;
	th_ycbcr_buffer ycbcr;
	ogg_packet packet;
	ogg_int64_t  granulepos;
	
    uint8_t *buf;
	unsigned char compressed_video_buffer[800000];

	unsigned char *frame_data;

};

/* public */
/*
krad_theora_encoder_t *krad_theora_encoder_create(int width, int height);
void krad_theora_encoder_destroy(krad_theora_encoder_t *krad_theora);
int krad_theora_encoder_write(krad_theora_encoder_t *krad_theora, unsigned char **packet, int *keyframe);
void krad_theora_convert_frame_for_local_gl_display(krad_theora_encoder_t *krad_theora);
*/
krad_theora_decoder_t *krad_theora_decoder_create();
void krad_theora_decoder_destroy(krad_theora_decoder_t *krad_theora);
void krad_theora_decoder_write(krad_theora_decoder_t *krad_theora);
void krad_theora_decoder_decode(krad_theora_decoder_t *krad_theora, void *buffer, int len);
//int krad_theora_convert_uyvy2yv12(theora_image_t *theora_img, char *uyvy, int w, int h);

/* private */
//void krad_theora_fail(theora_codec_ctx_t *ctx, const char *s);

