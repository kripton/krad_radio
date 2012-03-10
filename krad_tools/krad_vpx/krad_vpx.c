#include "krad_vpx.h"


krad_vpx_encoder_t *krad_vpx_encoder_create(int width, int height, int bitrate) {

	krad_vpx_encoder_t *kradvpx;
	
	kradvpx = calloc(1, sizeof(krad_vpx_encoder_t));
	
	kradvpx->width = width;
	kradvpx->height = height;

	if ((kradvpx->image = vpx_img_alloc(NULL, VPX_IMG_FMT_YV12, kradvpx->width, kradvpx->height, 1)) == NULL) {
		printf("Failed to allocate vpx image\n");
		exit(1);
	}
	
	kradvpx->quality = 24 * 1000;
	
	
	//printf("\n\n encoding quality set to %ld\n\n", kradvpx->quality);
	

	kradvpx->frame_byte_size = kradvpx->height * kradvpx->width * 4;

	kradvpx->rgb_frame_data = calloc (1, kradvpx->frame_byte_size);
	

	kradvpx->res = vpx_codec_enc_config_default(interface, &kradvpx->cfg, 0);

	if (kradvpx->res) {
		printf("Failed to get config: %s\n", vpx_codec_err_to_string(kradvpx->res));
		exit(1);
    }

	kradvpx->cfg.rc_target_bitrate = bitrate;
	kradvpx->cfg.g_w = kradvpx->width;
	kradvpx->cfg.g_h = kradvpx->height;
	kradvpx->cfg.g_threads = 5;
	kradvpx->cfg.kf_mode = VPX_KF_AUTO;
	kradvpx->cfg.kf_max_dist = 45;
	kradvpx->cfg.rc_end_usage = VPX_VBR;

	if (vpx_codec_enc_init(&kradvpx->encoder, interface, &kradvpx->cfg, 0)) {
		 krad_vpx_fail(&kradvpx->encoder, "Failed to initialize encoder");
	}



	kradvpx->rgb_sws_context = sws_getContext ( kradvpx->width, kradvpx->height, PIX_FMT_YUV420P, kradvpx->width, kradvpx->height, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	kradvpx->sws_context = sws_getContext ( kradvpx->width, kradvpx->height, PIX_FMT_YUV420P, kradvpx->width, kradvpx->height, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
     

	// now that cfg exists
	//write_webm_header(&kradcomposite->ebml, &kradcomposite->cfg, &kradcomposite->arg_framerate, kradcomposite->stereo_fmt, kradcomposite->vorbis->header, kradcomposite->vorbis->headerpos);

	return kradvpx;

}

void krad_vpx_encoder_set(krad_vpx_encoder_t *kradvpx, vpx_codec_enc_cfg_t *cfg) {

	int ret;

	ret = vpx_codec_enc_config_set (&kradvpx->encoder, cfg);

	if (ret != VPX_CODEC_OK) {
		printf("VPX Config problem: %s\n", vpx_codec_err_to_string(ret));
	}

}

void krad_vpx_encoder_destroy(krad_vpx_encoder_t *kradvpx) {

	if (kradvpx->image != NULL) {
		vpx_img_free(kradvpx->image);
		kradvpx->image = NULL;
	}
	vpx_codec_destroy(&kradvpx->encoder);
	sws_freeContext (kradvpx->sws_context);
	sws_freeContext (kradvpx->rgb_sws_context);
	free(kradvpx->rgb_frame_data);
	free(kradvpx);

}

void krad_vpx_encoder_finish(krad_vpx_encoder_t *kradvpx) {

	if (kradvpx->image != NULL) {
		vpx_img_free(kradvpx->image);
		kradvpx->image = NULL;
	}



}

void krad_vpx_fail(vpx_codec_ctx_t *ctx, const char *s) {
    
    const char *detail = vpx_codec_error_detail(ctx);
    
    printf("%s: %s\n", s, vpx_codec_error(ctx));

	if (detail) {
    	printf("%s\n", detail);
	}

    exit(1);
}

int krad_vpx_encoder_write(krad_vpx_encoder_t *kradvpx, unsigned char **packet, int *keyframe) {

	//kradvpx->quality = VPX_DL_GOOD_QUALITY;
	//kradvpx->quality = VPX_DL_BEST_QUALITY;
	//kradvpx->quality = VPX_DL_REALTIME;

	if (vpx_codec_encode(&kradvpx->encoder, kradvpx->image, kradvpx->frames, 1, kradvpx->flags, kradvpx->quality)) {
		krad_vpx_fail(&kradvpx->encoder, "Failed to encode frame");
	}

	kradvpx->frames++;
	
	kradvpx->iter = NULL;
	while ((kradvpx->pkt = vpx_codec_get_cx_data(&kradvpx->encoder, &kradvpx->iter))) {
		//printf("Got packet\n");
		if (kradvpx->pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
			*packet = kradvpx->pkt->data.frame.buf;
			*keyframe = kradvpx->pkt->data.frame.flags & VPX_FRAME_IS_KEY;
			
			//printf("keyframe is %d pts is -%ld-\n", *keyframe, kradvpx->pkt->data.frame.pts);
			return kradvpx->pkt->data.frame.sz;
		}
	}
	
	return 0;
}

void krad_vpx_convert_frame_for_local_gl_display(krad_vpx_encoder_t *kradvpx) {

	int rgb_stride_arr[3] = {4*kradvpx->width, 0, 0};

	const uint8_t *yv12_arr[4];
	
	yv12_arr[0] = kradvpx->image->planes[0];
	yv12_arr[1] = kradvpx->image->planes[2];
	yv12_arr[2] = kradvpx->image->planes[1];

	unsigned char *dst[4];
	dst[0] = kradvpx->rgb_frame_data;


	sws_scale(kradvpx->sws_context, yv12_arr, kradvpx->image->stride, 0, kradvpx->height, dst, rgb_stride_arr);

}

int krad_vpx_convert_uyvy2yv12(vpx_image_t *vpx_img, char *uyvy, int w, int h) {

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
            * u++ = p[3];

    p = uyvy;

    for (i = 0; i<(h >> 1); i++, p += (w << 1))
        for (j = 0; j<(w >> 1); j++, p += 4)
            * v++ = p[1];

    return 0;
}

/*
void krad_vpx_convert_yv12_to_rgb(krad_vpx_receiver_client_t *krad_vpx_receiver_client) {


	int rgb_stride_arr[3] = {4*krad_vpx_receiver_client->img->d_w, 0, 0};

	const uint8_t *yv12_arr[4];
	
	yv12_arr[0] = krad_vpx_receiver_client->img->planes[0];
	yv12_arr[1] = krad_vpx_receiver_client->img->planes[2];
	yv12_arr[2] = krad_vpx_receiver_client->img->planes[1];

	unsigned char *dst[4];
	
	dst[0] = krad_vpx_receiver_client->frame_data;


	sws_scale(krad_vpx_receiver_client->sws_context, yv12_arr, krad_vpx_receiver_client->img->stride, 0, krad_vpx_receiver_client->img->d_h, dst, rgb_stride_arr);


}
*/


////// Decoder

//	sws_scale(krad_vpx_receiver_client->sws_context, yv12_arr, krad_vpx_receiver_client->img->stride, 0, krad_vpx_receiver_client->img->d_h, dst, rgb_stride_arr);
//	sws_context = sws_getContext ( krad_vpx_receiver_client->img->d_w, krad_vpx_receiver_client->img->d_h, PIX_FMT_YUV420P, krad_vpx_receiver_client->img->d_w, krad_vpx_receiver_client->img->d_h, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
// 	sws_freeContext (krad_vpx_receiver_client->sws_context);

/*

		krad_vpx_receiver_client->slice_size = kradsource_receiver_client_slice(krad_vpx_receiver_client->kradsource_receiver_client, krad_vpx_receiver_client->compressed_video_buffer);

		if (!krad_vpx_receiver_client->slice_size) { 
			printf("krad_vpx_receiver client hung up\n");
			break;
		}

    	krad_vpx_receiver_client->iter = NULL;

		if (vpx_codec_decode(&krad_vpx_receiver_client->decoder, krad_vpx_receiver_client->compressed_video_buffer, krad_vpx_receiver_client->slice_size, 0, 0))
		{
		    printf("Failed to decode %d byte frame: %s\n", krad_vpx_receiver_client->slice_size, vpx_codec_error(&krad_vpx_receiver_client->decoder));
		    //exit(1);
		} else {
			
		    while((krad_vpx_receiver_client->img = vpx_codec_get_frame(&krad_vpx_receiver_client->decoder, &krad_vpx_receiver_client->iter))) {

				if (krad_vpx_receiver_client->sws_context == NULL) {
					krad_vpx_receiver_client->sws_context = sws_getContext ( krad_vpx_receiver_client->img->d_w, krad_vpx_receiver_client->img->d_h, PIX_FMT_YUV420P, krad_vpx_receiver_client->img->d_w, krad_vpx_receiver_client->img->d_h, PIX_FMT_RGB32, SWS_FAST_BILINEAR, NULL, NULL, NULL);
				}
				
				if (krad_vpx_receiver_client->frame_data == NULL) {
					krad_vpx_receiver_client->frame_data = calloc(1, krad_vpx_receiver_client->img->d_w * krad_vpx_receiver_client->img->d_h * 8);
					printf("got w of %d and h of %d  dh %d dw %d\n", krad_vpx_receiver_client->img->w, krad_vpx_receiver_client->img->h, krad_vpx_receiver_client->img->d_w, krad_vpx_receiver_client->img->d_h);
				}
				
				yv12_to_rgb(krad_vpx_receiver_client);

				if (krad_vpx_receiver_client->krad_vpx_receiver->frame_callback != NULL) {
					krad_vpx_receiver_client->krad_vpx_receiver->frame_callback(krad_vpx_receiver_client->client_callback_pointer, krad_vpx_receiver_client->frame_data);
				}
		      
		    }
		    
			//vpx_codec_get_stream_info(&krad_vpx_receiver->decoder, &krad_vpx_receiver->stream_info);
			//printf("Stream info sez: W:%d H:%d KF:%d\n", krad_vpx_receiver->stream_info.w, krad_vpx_receiver->stream_info.h, krad_vpx_receiver->stream_info.is_kf);
		}
		
*/


void krad_vpx_decoder_decode(krad_vpx_decoder_t *kradvpx, void *buffer, int len) {


	if (vpx_codec_decode(&kradvpx->decoder, buffer, len, 0, 0))
	{
		printf("Failed to decode %d byte frame: %s\n", len, vpx_codec_error(&kradvpx->decoder));
		//exit(1);
	}

	vpx_codec_get_stream_info(&kradvpx->decoder, &kradvpx->stream_info);
	//printf("VPX Stream Info: W:%d H:%d KF:%d\n", kradvpx->stream_info.w, kradvpx->stream_info.h, kradvpx->stream_info.is_kf);


	if (kradvpx->img == NULL) {
	
		kradvpx->width = kradvpx->stream_info.w;
		kradvpx->height = kradvpx->stream_info.h;
		
		if ((kradvpx->img = vpx_img_alloc(NULL, VPX_IMG_FMT_YV12, kradvpx->stream_info.w, kradvpx->stream_info.h, 1)) == NULL) {
			printf("Failed to allocate vpx image\n");
			exit(1);
		}
	
	}



	kradvpx->iter = NULL;
	kradvpx->img = vpx_codec_get_frame(&kradvpx->decoder, &kradvpx->iter);


}



void krad_vpx_decoder_destroy(krad_vpx_decoder_t *kradvpx) {

	vpx_codec_destroy(&kradvpx->decoder);
	vpx_img_free(kradvpx->img);
	free(kradvpx);

}

krad_vpx_decoder_t *krad_vpx_decoder_create() {

	krad_vpx_decoder_t *kradvpx;
	
	kradvpx = calloc(1, sizeof(krad_vpx_decoder_t));

	kradvpx->stream_info.sz = sizeof(kradvpx->stream_info);
	kradvpx->dec_flags = 0;
	kradvpx->cfg.threads = 4;
	
    vpx_codec_dec_init(&kradvpx->decoder, vpx_codec_vp8_dx(), &kradvpx->cfg, kradvpx->dec_flags);

	//kradvpx->ppcfg.post_proc_flag = VP8_DEBLOCK;
	//kradvpx->ppcfg.deblocking_level = 1;
	//kradvpx->ppcfg.noise_level = 0;

	kradvpx->ppcfg.post_proc_flag = VP8_DEMACROBLOCK | VP8_DEBLOCK | VP8_ADDNOISE;
	kradvpx->ppcfg.deblocking_level = 5;
	kradvpx->ppcfg.noise_level = 1;

	vpx_codec_control(&kradvpx->decoder, VP8_SET_POSTPROC, &kradvpx->ppcfg);

	kradvpx->img = NULL;

	return kradvpx;

}
