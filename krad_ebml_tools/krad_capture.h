/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <getopt.h>             /* getopt_long() */
#include <signal.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

#include "math.h"
#include "../libvpx/vpx/vpx_encoder.h"
#include "../libvpx/vpx/vp8cx.h"

#include "../kradradio/include/util/kradsource.h"

#include "libswscale/swscale.h"

#define interface (vpx_codec_vp8_cx())

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef struct kradcapture_St kradcapture_t;

struct kradcapture_St {

	kradsource_t *kradsource;
	struct SwsContext *sws_context;
	vpx_image_t *image;
	int width;
	int height;

    vpx_codec_ctx_t codec;
    vpx_codec_enc_cfg_t cfg;

    vpx_codec_err_t	res;
    int flags;
	vpx_codec_iter_t iter;
	const vpx_codec_cx_pkt_t *pkt;
	int frames;
	int quality;
	
	int frame_byte_size;
	unsigned char *rgb_frame_data;
	char *dev_name;
	pthread_rwlock_t frame_lock;
	
	float preview_angle;
	
	
};


typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct buffer {
        void *                  start;
        size_t                  length;
};



kradcapture_t *kradcapture_create ();
void kradcapture_run (kradcapture_t *kradcapture);
void kradcapture_destroy (kradcapture_t *kradcapture);
