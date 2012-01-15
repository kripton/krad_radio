#include "kradcapture.h"


static io_method	io		= IO_METHOD_MMAP;
static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;



static void
errno_exit                      (const char *           s)
{
        fprintf (stderr, "%s error %d, %s\n",
                 s, errno, strerror (errno));

        exit (EXIT_FAILURE);
}

static void die_codec(vpx_codec_ctx_t *ctx, const char *s) {                  //
    const char *detail = vpx_codec_error_detail(ctx);                         //
                                                                              //
    printf("%s: %s\n", s, vpx_codec_error(ctx));                              //
    if(detail)                                                                //
        printf("    %s\n",detail);                                            //
    exit(EXIT_FAILURE);                                                       //
}

static void die(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    if(fmt[strlen(fmt)-1] != '\n')
        printf("\n");
    exit(EXIT_FAILURE);
}

static int
xioctl                          (int                    fd,
                                 int                    request,
                                 void *                 arg)
{
        int r;

        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);

        return r;
}

void network_connect(kradcapture_t *kradcapture, char *host, int port) {

	//kradcapture->kradsource = kradsource_create("192.168.1.2", 9006);
	kradcapture->kradsource = kradsource_create(host, port);
	char *header = "I am a little vpX";

	kradsource_header(kradcapture->kradsource, header, strlen(header));
	
	printf("Connected!\n");

}



void kradcapture_encode_video_frame(kradcapture_t *kradcapture) {

	kradcapture->quality = VPX_DL_GOOD_QUALITY;
	//kradcapture->quality = VPX_DL_BEST_QUALITY;
	kradcapture->quality = VPX_DL_REALTIME;
																	
	if (vpx_codec_encode(&kradcapture->codec, kradcapture->image, kradcapture->frames, 1, kradcapture->flags, kradcapture->quality)) {
		die_codec(&kradcapture->codec, "Failed to encode frame");
	}

	kradcapture->frames++;
	
	kradcapture->iter = NULL;
	while ((kradcapture->pkt = vpx_codec_get_cx_data(&kradcapture->codec, &kradcapture->iter))) {
		//printf("Got packet\n");
		if (kradcapture->pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
			kradsource_slice(kradcapture->kradsource, kradcapture->pkt->data.frame.buf, kradcapture->pkt->data.frame.sz);
		}
	}
}


int uyvy2yv12(vpx_image_t *vpx_img, char *uyvy, int w, int h)
{
    unsigned char *y = vpx_img->img_data;
    unsigned char *u = w * h + y;
    unsigned char *v = w / 2 * h / 2 + u;
    int i, j;

    char *p = uyvy;

    // pretty clearly a very slow way to do this even in c
    // super easy simd conversion
    for (; y < u; p += 4)
    {
        *y++ = p[0];
        *y++ = p[2];
    }

    p = uyvy;

    for (i = 0; i<(h >> 1); i++, p += (w << 1))
        for (j = 0; j<(w >> 1); j++, p += 4)
            * u++ = p[1];

    p = uyvy;

    for (i = 0; i<(h >> 1); i++, p += (w << 1))
        for (j = 0; j<(w >> 1); j++, p += 4)
            * v++ = p[3];

    return 0;
}


void kradcapture_convert_frame_for_local_gl_display(kradcapture_t *kradcapture) {

	int rgb_stride_arr[3] = {4*kradcapture->width, 0, 0};

	const uint8_t *yv12_arr[4];
	
	yv12_arr[0] = kradcapture->image->planes[0];
	yv12_arr[1] = kradcapture->image->planes[2];
	yv12_arr[2] = kradcapture->image->planes[1];

	unsigned char *dst[4];
	dst[0] = kradcapture->rgb_frame_data;

	pthread_rwlock_wrlock (&kradcapture->frame_lock);
	sws_scale(kradcapture->sws_context, yv12_arr, kradcapture->image->stride, 0, kradcapture->height, dst, rgb_stride_arr);
	//usleep(10000);
	pthread_rwlock_unlock (&kradcapture->frame_lock);
}

static void process_image (kradcapture_t *kradcapture, const void *p)
{
	fputc ('.', stdout);
	fflush (stdout);

	uyvy2yv12(kradcapture->image, (char *)p, kradcapture->width, kradcapture->height);
	
	kradcapture_convert_frame_for_local_gl_display(kradcapture);
	
	kradcapture_encode_video_frame(kradcapture);

}



kradcapture_t *kradcapture_create () {

	kradcapture_t *kradcapture = calloc(1, sizeof(kradcapture_t));

	pthread_rwlock_init(&kradcapture->frame_lock, NULL);
	pthread_rwlock_wrlock(&kradcapture->frame_lock);

	kradcapture->width = 320;
	kradcapture->height = 240;

	kradcapture->frame_byte_size = kradcapture->height * kradcapture->width * 10;

	kradcapture->rgb_frame_data = calloc (1, kradcapture->frame_byte_size);

	kradcapture->dev_name = "/dev/video0";

	open_device (kradcapture);

	init_device (kradcapture);
	
	vpx_setup(kradcapture);
	
	
	kradcapture->sws_context = sws_getContext ( kradcapture->width, kradcapture->height, PIX_FMT_YUV420P, kradcapture->width, kradcapture->height, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	
	pthread_rwlock_unlock(&kradcapture->frame_lock);
        
	return kradcapture;
        
}


void kradcapture_run (kradcapture_t *kradcapture) {

	network_connect(kradcapture, "kradradio.com", 9007);

	start_capturing (kradcapture);

	capture_frames(kradcapture);
	
}


void kradcapture_destroy (kradcapture_t *kradcapture) {
	
  	pthread_rwlock_wrlock(&kradcapture->frame_lock);
	
	stop_capturing (kradcapture);

	uninit_device (kradcapture);

	close_device (kradcapture);
	
	sws_freeContext (kradcapture->sws_context);	
     
	pthread_rwlock_unlock(&kradcapture->frame_lock);
	pthread_rwlock_destroy(&kradcapture->frame_lock); 
        
	free(kradcapture);
 
        
}

