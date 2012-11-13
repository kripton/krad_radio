#include "krad_x264.h"


static void krad_x264_encoder_process_headers (krad_x264_encoder_t *krad_x264) {
	
    int sps_size = krad_x264->headers[0].i_payload - 4;
    int pps_size = krad_x264->headers[1].i_payload - 4;
//    int sei_size = krad_x264->headers[2].i_payload;

    uint8_t *sps = krad_x264->headers[0].p_payload + 4;
    uint8_t *pps = krad_x264->headers[1].p_payload + 4;
//    uint8_t *sei = krad_x264->headers[2].p_payload;

    krad_x264->header_len[0] = 5 + 1 + 2 + sps_size + 1 + 2 + pps_size;
    krad_x264->header[0] = malloc ( krad_x264->header_len[0] );

    krad_x264->header[0][0] = 1;
    krad_x264->header[0][1] = sps[1];
    krad_x264->header[0][2] = sps[2];
    krad_x264->header[0][3] = sps[3];
    krad_x264->header[0][4] = 0xff; // nalu size length is four bytes
    krad_x264->header[0][5] = 0xe1; // one sps

    krad_x264->header[0][6] = sps_size >> 8;
    krad_x264->header[0][7] = sps_size;

    memcpy( krad_x264->header[0]+8, sps, sps_size );

    krad_x264->header[0][8+sps_size] = 1; // one pps
    krad_x264->header[0][9+sps_size] = pps_size >> 8;
    krad_x264->header[0][10+sps_size] = pps_size;

    memcpy( krad_x264->header[0]+11+sps_size, pps, pps_size );	

	krad_x264->krad_codec_header.header[0] = krad_x264->header[0];
	krad_x264->krad_codec_header.header_size[0] = krad_x264->header_len[0];

//	krad_x264->krad_codec_header.header_combined = krad_x264->header[0];
//	krad_x264->krad_codec_header.header_combined_size = krad_x264->header_len[0];
	
	krad_x264->krad_codec_header.header_count = 3;	

	printk("x264 header length is %d\n", krad_x264->header_len[0]);
//	printf("x264 sei length is %d\n", sei_size);
}

int krad_x264_is_keyframe (unsigned char *buffer) {

	unsigned int nal_type = buffer[4] & 0x1F;

	if ((nal_type == 5) || (nal_type == 7) || (nal_type == 8)) {
		return 1;
	}

	return 0;
}

krad_x264_encoder_t *krad_x264_encoder_create (int width, int height,
											   int fps_numerator, int fps_denominator, int bitrate) {

	krad_x264_encoder_t *krad_x264;
	
	krad_x264 = calloc (1, sizeof(krad_x264_encoder_t));
	
	krad_x264->params = calloc (1, sizeof(x264_param_t));
	krad_x264->picture = calloc (1, sizeof(x264_picture_t));
		
	krad_x264->width = width;
	krad_x264->height = height;
	krad_x264->bitrate = bitrate;
	krad_x264->fps_numerator = fps_numerator;
	krad_x264->fps_denominator = fps_denominator;

  x264_param_default_preset ( krad_x264->params, "ultrafast", "film,zerolatency" );
	x264_param_apply_profile ( krad_x264->params, "high" );
	
	krad_x264->params->i_width = krad_x264->width;
	krad_x264->params->i_height = krad_x264->height;
	krad_x264->params->i_csp = X264_CSP_I420;
	krad_x264->params->i_fps_num = krad_x264->fps_numerator;
	krad_x264->params->i_fps_den = krad_x264->fps_denominator;
	krad_x264->params->rc.i_bitrate = krad_x264->bitrate;
	krad_x264->params->rc.i_vbv_max_bitrate = krad_x264->bitrate;
	krad_x264->encoder = x264_encoder_open ( krad_x264->params );
	x264_encoder_parameters ( krad_x264->encoder, krad_x264->params );
	x264_encoder_headers ( krad_x264->encoder, &krad_x264->headers, &krad_x264->header_count );

	krad_x264_encoder_process_headers (krad_x264);
	
	x264_picture_alloc ( krad_x264->picture, X264_CSP_I420, krad_x264->width, krad_x264->height );

	return krad_x264;

}

void krad_x264_encoder_destroy(krad_x264_encoder_t *krad_x264) {

	x264_encoder_close ( krad_x264->encoder );
	x264_picture_clean ( krad_x264->picture );

	if (krad_x264->header[0] != NULL) {
		free (krad_x264->header[0]);
	}

	free (krad_x264->params);
	free (krad_x264->picture);
	
	free (krad_x264);

}

int krad_x264_encoder_write (krad_x264_encoder_t *krad_x264, unsigned char **packet, int *keyframe) {
	
    int frame_size;
    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal;
    
    frame_size = 0;

    frame_size = x264_encoder_encode ( krad_x264->encoder, &nal, &i_nal, krad_x264->picture, &pic_out );

	if (frame_size) {
		*packet = nal[0].p_payload;
		*keyframe = pic_out.b_keyframe;		
	}
	
	if (*keyframe == 1) {
		if (*keyframe != krad_x264_is_keyframe (nal[0].p_payload)) {
			printke ("krad x264: uh oh keyframe not keyframe!");
		}
	}
	
	return frame_size;

}
	
	
