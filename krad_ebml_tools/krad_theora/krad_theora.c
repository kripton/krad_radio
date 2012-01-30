#include "krad_theora.h"

/*
krad_theora_encoder_t *krad_theora_encoder_create(int width, int height) {

	krad_theora_encoder_t *krad_theora;
	
	krad_theora = calloc(1, sizeof(krad_theora_encoder_t));
	
	krad_theora->width = width;
	krad_theora->height = height;

    th_encode_flushheader(td,&tc,&op);
    timebase=th_granule_time(td,op.granulepos);

	krad_theora->frame_byte_size = krad_theora->height * krad_theora->width * 4;

	krad_theora->rgb_frame_data = calloc (1, krad_theora->frame_byte_size);
	
    th_info_init(&ti);



	krad_theora->rgb_sws_context = sws_getContext ( krad_theora->width, krad_theora->height, PIX_FMT_YUV420P, krad_theora->width, krad_theora->height, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	krad_theora->sws_context = sws_getContext ( krad_theora->width, krad_theora->height, PIX_FMT_YUV420P, krad_theora->width, krad_theora->height, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
     


	return krad_theora;

}

void krad_theora_encoder_destroy(krad_theora_encoder_t *krad_theora) {


    th_encode_free(krad_theora->encoder);
    th_comment_clear(&krad_theora->comment);
	sws_freeContext (krad_theora->sws_context);
	sws_freeContext (krad_theora->rgb_sws_context);
	free(krad_theora->rgb_frame_data);
	free(krad_theora);

}

void krad_theora_fail(theora_codec_ctx_t *ctx, const char *s) {
    
    const char *detail = theora_codec_error_detail(ctx);
    
    printf("%s: %s\n", s, theora_codec_error(ctx));

	if (detail) {
    	printf("%s\n", detail);
	}

    exit(1);
}

int krad_theora_encoder_write(krad_theora_encoder_t *krad_theora, unsigned char **packet, int *keyframe) {

	//krad_theora->quality = theora_DL_GOOD_QUALITY;
	//krad_theora->quality = theora_DL_BEST_QUALITY;
	//krad_theora->quality = theora_DL_REALTIME;

	if (theora_codec_encode(&krad_theora->encoder, krad_theora->image, krad_theora->frames, 1, krad_theora->flags, krad_theora->quality)) {
		krad_theora_fail(&krad_theora->encoder, "Failed to encode frame");
	}

	krad_theora->frames++;
	
	krad_theora->iter = NULL;
	while ((krad_theora->pkt = theora_codec_get_cx_data(&krad_theora->encoder, &krad_theora->iter))) {
		//printf("Got packet\n");
		if (krad_theora->pkt->kind == theora_CODEC_CX_FRAME_PKT) {
			*packet = krad_theora->pkt->data.frame.buf;
			*keyframe = krad_theora->pkt->data.frame.flags & theora_FRAME_IS_KEY;
			
			//printf("keyframe is %d pts is -%ld-\n", *keyframe, krad_theora->pkt->data.frame.pts);
			return krad_theora->pkt->data.frame.sz;
		}
	}
	
	return 0;
}


void krad_theora_convert_frame_for_local_gl_display(krad_theora_encoder_t *krad_theora) {

	int rgb_stride_arr[3] = {4*krad_theora->width, 0, 0};

	const uint8_t *yv12_arr[4];
	
	yv12_arr[0] = krad_theora->image->planes[0];
	yv12_arr[1] = krad_theora->image->planes[2];
	yv12_arr[2] = krad_theora->image->planes[1];

	unsigned char *dst[4];
	dst[0] = krad_theora->rgb_frame_data;


	sws_scale(krad_theora->sws_context, yv12_arr, krad_theora->image->stride, 0, krad_theora->height, dst, rgb_stride_arr);

}

int krad_theora_convert_uyvy2yv12(theora_image_t *theora_img, char *uyvy, int w, int h) {

    unsigned char *y = theora_img->img_data;
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
            * u++ = p[3];

    p = uyvy;

    for (i = 0; i<(h >> 1); i++, p += (w << 1))
        for (j = 0; j<(w >> 1); j++, p += 4)
            * v++ = p[1];

    return 0;
}

*/


void krad_theora_decoder_decode(krad_theora_decoder_t *krad_theora, void *buffer, int len) {


	//printf("theora decode with %d\n", len);
	
	krad_theora->packet.packet = buffer;
	krad_theora->packet.bytes = len;
	krad_theora->packet.packetno++;
	
	th_decode_packetin(krad_theora->decoder, &krad_theora->packet, &krad_theora->granulepos);
	
	th_decode_ycbcr_out(krad_theora->decoder, krad_theora->ycbcr);

}



void krad_theora_decoder_destroy(krad_theora_decoder_t *krad_theora) {

    th_decode_free(krad_theora->decoder);
    th_comment_clear(&krad_theora->comment);
    th_info_clear(&krad_theora->info);
	free(krad_theora);

}

krad_theora_decoder_t *krad_theora_decoder_create(unsigned char *header1, int header1len, unsigned char *header2, int header2len, unsigned char *header3, int header3len) {

	krad_theora_decoder_t *krad_theora;
	
	krad_theora = calloc(1, sizeof(krad_theora_decoder_t));

	krad_theora->granulepos = -1;

	th_comment_init(&krad_theora->comment);
	th_info_init(&krad_theora->info);

	krad_theora->packet.packet = header1;
	krad_theora->packet.bytes = header1len;
	krad_theora->packet.b_o_s = 1;
	krad_theora->packet.packetno = 1;
	th_decode_headerin(&krad_theora->info, &krad_theora->comment, &krad_theora->setup_info, &krad_theora->packet);
	//printf("x is %d len is %d\n", x, header1len);

	krad_theora->packet.packet = header2;
	krad_theora->packet.bytes = header2len;
	krad_theora->packet.b_o_s = 0;
	krad_theora->packet.packetno = 2;
	th_decode_headerin(&krad_theora->info, &krad_theora->comment, &krad_theora->setup_info, &krad_theora->packet);
	//printf("x is %d len is %d\n", x, header2len);

	krad_theora->packet.packet = header3;
	krad_theora->packet.bytes = header3len;
	krad_theora->packet.packetno = 3;
	th_decode_headerin(&krad_theora->info, &krad_theora->comment, &krad_theora->setup_info, &krad_theora->packet);

	printf("Theora %dx%d %.02f fps video\n Encoded frame content is %dx%d with %dx%d offset\n",
		   krad_theora->info.frame_width, krad_theora->info.frame_height, 
		   (double)krad_theora->info.fps_numerator/krad_theora->info.fps_denominator,
		   krad_theora->info.pic_width, krad_theora->info.pic_height, krad_theora->info.pic_x, krad_theora->info.pic_y);


	krad_theora->decoder = th_decode_alloc(&krad_theora->info, krad_theora->setup_info);

	th_setup_free(krad_theora->setup_info);

	return krad_theora;

}
