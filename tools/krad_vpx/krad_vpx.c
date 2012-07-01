#include "krad_vpx.h"

static void krad_vpx_fail (vpx_codec_ctx_t *ctx, const char *s);

krad_vpx_encoder_t *krad_vpx_encoder_create (int width, int height, int fps_numerator,
											 int fps_denominator, int bitrate) {

	krad_vpx_encoder_t *kradvpx;
	
	kradvpx = calloc(1, sizeof(krad_vpx_encoder_t));
	
	kradvpx->width = width;
	kradvpx->height = height;
	kradvpx->fps_numerator = fps_numerator;
	kradvpx->fps_denominator = fps_denominator;
	kradvpx->bitrate = bitrate;
	
	printk ("Krad Radio using libvpx version: %s\n", vpx_codec_version_str ());

	if ((kradvpx->image = vpx_img_alloc (NULL, VPX_IMG_FMT_YV12, kradvpx->width, kradvpx->height, 1)) == NULL) {
		failfast ("Failed to allocate vpx image\n");
	}

	kradvpx->res = vpx_codec_enc_config_default (interface, &kradvpx->cfg, 0);

	if (kradvpx->res) {
		failfast ("Failed to get config: %s\n", vpx_codec_err_to_string(kradvpx->res));
    }

	krad_vpx_encoder_print_config (kradvpx);

	kradvpx->cfg.g_w = kradvpx->width;
	kradvpx->cfg.g_h = kradvpx->height;
	/* Next two lines are really right */
	kradvpx->cfg.g_timebase.num = kradvpx->fps_denominator;
	kradvpx->cfg.g_timebase.den = kradvpx->fps_numerator;
	kradvpx->cfg.rc_target_bitrate = bitrate;	
	kradvpx->cfg.g_threads = 3;
	kradvpx->cfg.kf_mode = VPX_KF_AUTO;
	kradvpx->cfg.rc_end_usage = VPX_VBR;
	
	//kradvpx->cfg.g_lag_in_frames = 1;
	
	//kradvpx->cfg.rc_buf_sz = 8000;
	//kradvpx->cfg.rc_buf_initial_sz = 4300;
	//kradvpx->cfg.rc_buf_optimal_sz = 6500;
	
	//kradvpx->cfg.rc_max_quantizer = 55;
	
	kradvpx->quality = 15 * 1000;

	krad_vpx_encoder_print_config (kradvpx);

	if (vpx_codec_enc_init(&kradvpx->encoder, interface, &kradvpx->cfg, 0)) {
		 krad_vpx_fail (&kradvpx->encoder, "Failed to initialize encoder");
	}

	krad_vpx_encoder_print_config (kradvpx);

	return kradvpx;

}

void krad_vpx_encoder_print_config (krad_vpx_encoder_t *kradvpx) {

	printk ("Krad VP8 Encoder config");
	printk ("WxH %dx%d", kradvpx->cfg.g_w, kradvpx->cfg.g_h);
	printk ("Threads: %d", kradvpx->cfg.g_threads);
	printk ("rc_target_bitrate: %d", kradvpx->cfg.rc_target_bitrate);
	printk ("kf_max_dist: %d", kradvpx->cfg.kf_max_dist);
	printk ("kf_min_dist: %d", kradvpx->cfg.kf_min_dist);	
	printk ("deadline: %d", kradvpx->quality);	
	printk ("rc_buf_sz: %d", kradvpx->cfg.rc_buf_sz);	
	printk ("rc_buf_initial_sz: %d", kradvpx->cfg.rc_buf_initial_sz);	
	printk ("rc_buf_optimal_sz: %d", kradvpx->cfg.rc_buf_optimal_sz);
	printk ("rc_dropframe_thresh: %d", kradvpx->cfg.rc_dropframe_thresh);
	printk ("g_lag_in_frames: %d", kradvpx->cfg.g_lag_in_frames);
	printk ("rc_min_quantizer: %d", kradvpx->cfg.rc_min_quantizer);
	printk ("rc_max_quantizer: %d", kradvpx->cfg.rc_max_quantizer);

}

void krad_vpx_encoder_bitrate_set (krad_vpx_encoder_t *kradvpx, int bitrate) {
	kradvpx->bitrate = bitrate;
	kradvpx->cfg.rc_target_bitrate = kradvpx->bitrate;
	kradvpx->update_config = 1;
}

void krad_vpx_encoder_quality_set (krad_vpx_encoder_t *kradvpx, int quality) {
	kradvpx->quality = quality;
}

int krad_vpx_encoder_quality_get (krad_vpx_encoder_t *kradvpx) {
	return kradvpx->quality;
}

void krad_vpx_encoder_config_set (krad_vpx_encoder_t *kradvpx, vpx_codec_enc_cfg_t *cfg) {

	int ret;

	ret = vpx_codec_enc_config_set (&kradvpx->encoder, cfg);

	if (ret != VPX_CODEC_OK) {
		printke ("VPX Config problem: %s\n", vpx_codec_err_to_string (ret));
	}

}

void krad_vpx_encoder_destroy (krad_vpx_encoder_t *kradvpx) {

	if (kradvpx->image != NULL) {
		vpx_img_free (kradvpx->image);
		kradvpx->image = NULL;
	}
	vpx_codec_destroy (&kradvpx->encoder);

	free (kradvpx);

}

void krad_vpx_encoder_finish (krad_vpx_encoder_t *kradvpx) {

	if (kradvpx->image != NULL) {
		vpx_img_free (kradvpx->image);
		kradvpx->image = NULL;
	}

}

void krad_vpx_encoder_want_keyframe (krad_vpx_encoder_t *kradvpx) {
	kradvpx->flags = VPX_EFLAG_FORCE_KF;
}

int krad_vpx_encoder_write (krad_vpx_encoder_t *kradvpx, unsigned char **packet, int *keyframe) {

	if (kradvpx->update_config == 1) {
		krad_vpx_encoder_config_set (kradvpx, &kradvpx->cfg);
		kradvpx->update_config = 0;
		//printk ("Krad VP8: bitrate should now be: %dk", kradvpx->cfg.rc_target_bitrate);
		krad_vpx_encoder_print_config (kradvpx);
	}

	if (vpx_codec_encode(&kradvpx->encoder, kradvpx->image, kradvpx->frames, 1, kradvpx->flags, kradvpx->quality)) {
		krad_vpx_fail (&kradvpx->encoder, "Failed to encode frame");
	}

	kradvpx->frames++;

	kradvpx->flags = 0;
	
	kradvpx->iter = NULL;
	while ((kradvpx->pkt = vpx_codec_get_cx_data (&kradvpx->encoder, &kradvpx->iter))) {
		//printkd ("Got packet\n");
		if (kradvpx->pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
			*packet = kradvpx->pkt->data.frame.buf;
			*keyframe = kradvpx->pkt->data.frame.flags & VPX_FRAME_IS_KEY;
			if (*keyframe == 0) {
				kradvpx->frames_since_keyframe++;
			} else {
				kradvpx->frames_since_keyframe = 0;
				//printkd ("keyframe is %d pts is -%ld-\n", *keyframe, kradvpx->pkt->data.frame.pts);
			}
			return kradvpx->pkt->data.frame.sz;
		}
	}
	
	return 0;
}

/* decoder */

void krad_vpx_decoder_decode (krad_vpx_decoder_t *kradvpx, void *buffer, int len) {


	if (vpx_codec_decode(&kradvpx->decoder, buffer, len, 0, 0))
	{
		failfast ("Failed to decode %d byte frame: %s\n", len, vpx_codec_error(&kradvpx->decoder));
		//exit(1);
	}

	vpx_codec_get_stream_info (&kradvpx->decoder, &kradvpx->stream_info);
	//printf("VPX Stream Info: W:%d H:%d KF:%d\n", kradvpx->stream_info.w, kradvpx->stream_info.h, 
	//		  kradvpx->stream_info.is_kf);
	
	if (kradvpx->width == 0) {
		kradvpx->width = kradvpx->stream_info.w;
		kradvpx->height = kradvpx->stream_info.h;
	}
	/*
	if (kradvpx->img == NULL) {
	
		kradvpx->width = kradvpx->stream_info.w;
		kradvpx->height = kradvpx->stream_info.h;
		
		if ((kradvpx->img = vpx_img_alloc(NULL, VPX_IMG_FMT_YV12, kradvpx->stream_info.w, 
										  kradvpx->stream_info.h, 1)) == NULL) {
			failfast ("Failed to allocate vpx image\n");
		}
	
	}
	*/
	kradvpx->iter = NULL;
	kradvpx->img = vpx_codec_get_frame (&kradvpx->decoder, &kradvpx->iter);

}



void krad_vpx_decoder_destroy (krad_vpx_decoder_t *kradvpx) {

	vpx_codec_destroy (&kradvpx->decoder);
	vpx_img_free (kradvpx->img);
	free (kradvpx);
}

krad_vpx_decoder_t *krad_vpx_decoder_create () {

	krad_vpx_decoder_t *kradvpx;
	
	kradvpx = calloc(1, sizeof(krad_vpx_decoder_t));

	kradvpx->stream_info.sz = sizeof(kradvpx->stream_info);
	kradvpx->dec_flags = 0;
	kradvpx->cfg.threads = 3;
	
    vpx_codec_dec_init(&kradvpx->decoder, vpx_codec_vp8_dx(), &kradvpx->cfg, kradvpx->dec_flags);

	//kradvpx->ppcfg.post_proc_flag = VP8_DEBLOCK;
	//kradvpx->ppcfg.deblocking_level = 1;
	//kradvpx->ppcfg.noise_level = 0;

	kradvpx->ppcfg.post_proc_flag = VP8_DEMACROBLOCK | VP8_DEBLOCK | VP8_ADDNOISE;
	kradvpx->ppcfg.deblocking_level = 5;
	kradvpx->ppcfg.noise_level = 1;

	vpx_codec_control (&kradvpx->decoder, VP8_SET_POSTPROC, &kradvpx->ppcfg);

	kradvpx->img = NULL;

	return kradvpx;

}

static void krad_vpx_fail (vpx_codec_ctx_t *ctx, const char *s) {
    
    const char *detail = vpx_codec_error_detail(ctx);
    
    printke ("%s: %s\n", s, vpx_codec_error(ctx));

	if (detail) {
    	printke ("%s\n", detail);
	}

    failfast ("");
}
