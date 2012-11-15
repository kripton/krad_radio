#include "krad_theora.h"


krad_theora_encoder_t *krad_theora_encoder_create (int width, int height,
												   int fps_numerator, int fps_denominator, int color_depth, int quality) {

	krad_theora_encoder_t *krad_theora;
	
	krad_theora = calloc (1, sizeof(krad_theora_encoder_t));
	
	krad_theora->width = width;
	krad_theora->height = height;
	krad_theora->quality = quality;
	krad_theora->color_depth = color_depth;

	th_info_init (&krad_theora->info);
	th_comment_init (&krad_theora->comment);

	krad_theora->info.frame_width = krad_theora->width;
	if (krad_theora->width % 16) {
	  krad_theora->info.frame_width += 16 - (krad_theora->width % 16);
	}
	krad_theora->info.frame_height = krad_theora->height;
	if (krad_theora->height % 16) {
	  krad_theora->info.frame_height += 16 - (krad_theora->height % 16);
	}
	krad_theora->info.pic_width = krad_theora->width;
	krad_theora->info.pic_height = krad_theora->height;
	krad_theora->info.pic_x = 0;
	krad_theora->info.pic_y = 0;
	krad_theora->info.aspect_denominator = 1;
	krad_theora->info.aspect_numerator = 1;
	krad_theora->info.target_bitrate = 0;
	krad_theora->info.quality = krad_theora->quality;
  if (krad_theora->color_depth == PIX_FMT_YUV420P) {
	  krad_theora->info.pixel_fmt = TH_PF_420;
  }
  if (krad_theora->color_depth == PIX_FMT_YUV422P) {
	  krad_theora->info.pixel_fmt = TH_PF_422;
  }
  if (krad_theora->color_depth == PIX_FMT_YUV444P) {
	  krad_theora->info.pixel_fmt = TH_PF_444;
  }
	krad_theora->keyframe_shift = krad_theora->info.keyframe_granule_shift;
	
	printk ("Loading Theora encoder version %s", th_version_string());
	printk ("Theora keyframe shift %d", krad_theora->keyframe_shift);

	krad_theora->info.fps_numerator = fps_numerator;
	krad_theora->info.fps_denominator = fps_denominator;
	//krad_theora->keyframe_distance = 30;

	krad_theora->encoder = th_encode_alloc (&krad_theora->info);

	//th_encode_ctl (krad_theora->encoder, TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE, &krad_theora->keyframe_distance, sizeof(int));
	//printk ("Theora keyframe max distance %u\n", krad_theora->keyframe_distance);
		
	if (strstr(krad_system.unix_info.machine, "x86") == NULL) {
		//FOR ARM Realtime
		th_encode_ctl (krad_theora->encoder, TH_ENCCTL_GET_SPLEVEL_MAX, &krad_theora->speed, sizeof(int));
		printk ("Theora encoder speed: %d quality: %d", krad_theora->speed, krad_theora->quality);
		th_encode_ctl (krad_theora->encoder, TH_ENCCTL_SET_SPLEVEL, &krad_theora->speed, sizeof(int));
	} else {
		//FOR x86 Realtime
		th_encode_ctl (krad_theora->encoder, TH_ENCCTL_GET_SPLEVEL_MAX, &krad_theora->speed, sizeof(int));
		if ((krad_theora->width <= 1280) && (krad_theora->height <= 720)) {
		  krad_theora->speed -= 1;
		}
		printk ("Theora encoder speed: %d quality: %d", krad_theora->speed, krad_theora->quality);
		th_encode_ctl (krad_theora->encoder, TH_ENCCTL_SET_SPLEVEL, &krad_theora->speed, sizeof(int));
	}

	while (th_encode_flushheader ( krad_theora->encoder, &krad_theora->comment, &krad_theora->packet) > 0) {
	
		krad_theora->header_combined_size += krad_theora->packet.bytes;
	
		krad_theora->header[krad_theora->header_count] = malloc(krad_theora->packet.bytes);
		memcpy (krad_theora->header[krad_theora->header_count], krad_theora->packet.packet, krad_theora->packet.bytes);
		krad_theora->header_len[krad_theora->header_count] = krad_theora->packet.bytes;
		krad_theora->header_count++;
		
		//printk ("krad_theora_encoder_create th_encode_flushheader got header packet %"PRIi64" which is %ld bytes\n", 
		//		krad_theora->packet.packetno, krad_theora->packet.bytes);
	}
	
	//printk ("krad_theora_encoder_create Got %d header packets\n", krad_theora->header_count);

	krad_theora->header_combined = calloc (1, krad_theora->header_combined_size + 10);

	krad_theora->header_combined[0] = 0x02;
	krad_theora->header_combined_pos++;

	//printk ("main is %ld\n", vorbis->header_main.bytes);
	if (krad_theora->header_len[0] > 255) {
		failfast ("theora mainheader to long for code");
	}
	
	krad_theora->demented = krad_theora->header_len[0];
	krad_theora->header_combined[1] = (char)krad_theora->demented;
	krad_theora->header_combined_pos++;
	
	//printk ("comments is %ld\n", vorbis->header_comments.bytes);
	if (krad_theora->header_len[1] > 255) {
		failfast ("theora comments header to long for code");
	}
	
	krad_theora->demented = krad_theora->header_len[1];
	krad_theora->header_combined[2] = (char)krad_theora->demented;
	krad_theora->header_combined_pos++;
	
	krad_theora->header_combined_size += 3;
	
	memcpy (krad_theora->header_combined + krad_theora->header_combined_pos, krad_theora->header[0], krad_theora->header_len[0]);
	krad_theora->header_combined_pos += krad_theora->header_len[0];
		
	//printf("main is %ld bytes headerpos is  %d \n", vorbis->header_main.bytes, vorbis->headerpos);
		
	memcpy (krad_theora->header_combined + krad_theora->header_combined_pos, krad_theora->header[1], krad_theora->header_len[1]);
	krad_theora->header_combined_pos += krad_theora->header_len[1];

	//printf("comments is %ld bytes headerpos is  %d \n", vorbis->header_comments.bytes, vorbis->headerpos);
		
	memcpy (krad_theora->header_combined + krad_theora->header_combined_pos, krad_theora->header[2], krad_theora->header_len[2]);
	krad_theora->header_combined_pos += krad_theora->header_len[2];

	krad_theora->krad_codec_header.header[0] = krad_theora->header[0];
	krad_theora->krad_codec_header.header_size[0] = krad_theora->header_len[0];
	krad_theora->krad_codec_header.header[1] = krad_theora->header[1];
	krad_theora->krad_codec_header.header_size[1] = krad_theora->header_len[1];
	krad_theora->krad_codec_header.header[2] = krad_theora->header[2];
	krad_theora->krad_codec_header.header_size[2] = krad_theora->header_len[2];

	krad_theora->krad_codec_header.header_combined = krad_theora->header_combined;
	krad_theora->krad_codec_header.header_combined_size = krad_theora->header_combined_size;
	krad_theora->krad_codec_header.header_count = 3;

	krad_theora->ycbcr[0].stride =  krad_theora->info.frame_width;
	krad_theora->ycbcr[0].width =  krad_theora->info.frame_width;	
	krad_theora->ycbcr[0].height =  krad_theora->info.frame_height;	
	
	krad_theora->ycbcr[0].data = calloc(1, krad_theora->info.frame_width * krad_theora->info.frame_height);

  if (krad_theora->color_depth == PIX_FMT_YUV420P) {
	  krad_theora->ycbcr[1].stride = krad_theora->info.frame_width / 2;
	  krad_theora->ycbcr[1].width = krad_theora->info.frame_width / 2;
	  krad_theora->ycbcr[1].height = krad_theora->info.frame_height / 2;
	  krad_theora->ycbcr[2].stride = krad_theora->info.frame_width / 2;
	  krad_theora->ycbcr[2].width = krad_theora->info.frame_width / 2;
	  krad_theora->ycbcr[2].height = krad_theora->info.frame_height / 2;

	  krad_theora->ycbcr[1].data = calloc(1, ((krad_theora->info.frame_width / 2) * (krad_theora->info.frame_height / 2)));
	  krad_theora->ycbcr[2].data = calloc(1, ((krad_theora->info.frame_width / 2) * (krad_theora->info.frame_height / 2)));
  }
  if (krad_theora->color_depth == PIX_FMT_YUV422P) {
	  krad_theora->ycbcr[1].stride = krad_theora->info.frame_width / 2;
	  krad_theora->ycbcr[1].width =  krad_theora->info.frame_width / 2;
	  krad_theora->ycbcr[1].height =  krad_theora->info.frame_height;
	  krad_theora->ycbcr[2].stride = krad_theora->info.frame_width / 2;
	  krad_theora->ycbcr[2].width = krad_theora->info.frame_width / 2;
	  krad_theora->ycbcr[2].height =  krad_theora->info.frame_height;
	  
	  krad_theora->ycbcr[1].data = calloc(1, ((krad_theora->info.frame_width / 2) * krad_theora->info.frame_height));
	  krad_theora->ycbcr[2].data = calloc(1, ((krad_theora->info.frame_width / 2) * krad_theora->info.frame_height));
	  
  }
  if (krad_theora->color_depth == PIX_FMT_YUV444P) {
	  krad_theora->ycbcr[1].stride = krad_theora->info.frame_width;
	  krad_theora->ycbcr[1].width = krad_theora->info.frame_width;	
	  krad_theora->ycbcr[1].height = krad_theora->info.frame_height;
	  krad_theora->ycbcr[2].stride = krad_theora->info.frame_width;
	  krad_theora->ycbcr[2].width = krad_theora->info.frame_width;	
	  krad_theora->ycbcr[2].height = krad_theora->info.frame_height;
	  
	  krad_theora->ycbcr[1].data = calloc(1, krad_theora->info.frame_width * krad_theora->info.frame_height);
	  krad_theora->ycbcr[2].data = calloc(1, krad_theora->info.frame_width * krad_theora->info.frame_height);	  
  }

	return krad_theora;

}

void krad_theora_encoder_destroy(krad_theora_encoder_t *krad_theora) {

	while (krad_theora->header_count--) {
		//printf("krad_theora_encoder_destroy freeing header %d\n", krad_theora->header_count);
		free (krad_theora->header[krad_theora->header_count]);
	}

	th_info_clear (&krad_theora->info);
	th_comment_clear (&krad_theora->comment);
	th_encode_free (krad_theora->encoder);
	free (krad_theora->header_combined);
	
	free (krad_theora->ycbcr[0].data);
	free (krad_theora->ycbcr[1].data);
	free (krad_theora->ycbcr[2].data);
	
	free (krad_theora);

}

int krad_theora_encoder_quality_get (krad_theora_encoder_t *krad_theora) {
	return krad_theora->quality;
}

void krad_theora_encoder_quality_set (krad_theora_encoder_t *krad_theora, int quality) {
	krad_theora->quality = quality;
	krad_theora->update_config = 1;
}

int krad_theora_encoder_write (krad_theora_encoder_t *krad_theora, unsigned char **packet, int *keyframe) {
	
	int ret;
	int key;

	if (krad_theora->update_config) {
		th_encode_ctl (krad_theora->encoder, TH_ENCCTL_SET_QUALITY, &krad_theora->quality, sizeof(int));
		krad_theora->update_config = 0;	
	}

	
	ret = th_encode_ycbcr_in (krad_theora->encoder, krad_theora->ycbcr);
	if (ret != 0) {
		failfast ("krad_theora_encoder_write th_encode_ycbcr_in failed! %d", ret);
	}
	
	// Note: Currently the encoder operates in a one-frame-in, one-packet-out manner. However, this may be changed in the future.
	
	ret = th_encode_packetout (krad_theora->encoder, krad_theora->finish, &krad_theora->packet);
	if (ret < 1) {
		failfast ("krad_theora_encoder_write th_encode_packetout failed! %d", ret);
	}
	
	*packet = krad_theora->packet.packet;
	
	key = th_packet_iskeyframe (&krad_theora->packet);
	*keyframe = key;
	if (*keyframe == -1) {
		failfast ("krad_theora_encoder_write th_packet_iskeyframe failed! %d", *keyframe);
	}
	
	if (key) {
		//printk ("its a keyframe\n");
	}
	
	// Double check
	//ogg_packet test_packet;
	//ret = th_encode_packetout (krad_theora->encoder, krad_theora->finish, &test_packet);
	//if (ret != 0) {
	//	printf("krad_theora_encoder_write th_encode_packetout offerd up an extra packet! %d\n", ret);
	//	exit(1);
	//}
	
	return krad_theora->packet.bytes;
}



/* decoder */

void krad_theora_decoder_decode(krad_theora_decoder_t *krad_theora, void *buffer, int len) {

	int ret;
	
	krad_theora->packet.packet = buffer;
	krad_theora->packet.bytes = len;
	krad_theora->packet.packetno++;
	
	ret = th_decode_packetin (krad_theora->decoder, &krad_theora->packet, &krad_theora->granulepos);
	
	if (ret == 0) {
		th_decode_ycbcr_out (krad_theora->decoder, krad_theora->ycbcr);
	} else {
	
		printke ("theora decoder err! %d", ret);
	
		if (ret == TH_DUPFRAME) {
			printke ("theora decoder DUPFRAME!");
		}

		if (ret == TH_EFAULT) {
			printke ("theora decoder EFAULT!");
		}
		
		if (ret == TH_EBADPACKET) {
			printke ("theora decoder EBADPACKET!");
		}
		
		if (ret == TH_EIMPL) {
			printke ("theora decoder EIMPL!");
		}
	
	}

}

void krad_theora_decoder_timecode(krad_theora_decoder_t *krad_theora, uint64_t *timecode) {

	float frame_rate;
	ogg_int64_t iframe;
	ogg_int64_t pframe;
	
	frame_rate = krad_theora->info.fps_numerator / krad_theora->info.fps_denominator;
	
	iframe = krad_theora->granulepos >> krad_theora->info.keyframe_granule_shift;
	pframe = krad_theora->granulepos - (iframe << krad_theora->info.keyframe_granule_shift);
	/* kludged, we use the default shift of 6 and assume a 3.2.1+ bitstream */
	*timecode = ((iframe + pframe - 1) / frame_rate) * 1000.0;

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

	krad_theora->packet.packet = header2;
	krad_theora->packet.bytes = header2len;
	krad_theora->packet.b_o_s = 0;
	krad_theora->packet.packetno = 2;
	th_decode_headerin(&krad_theora->info, &krad_theora->comment, &krad_theora->setup_info, &krad_theora->packet);

	krad_theora->packet.packet = header3;
	krad_theora->packet.bytes = header3len;
	krad_theora->packet.packetno = 3;
	th_decode_headerin(&krad_theora->info, &krad_theora->comment, &krad_theora->setup_info, &krad_theora->packet);

	krad_theora->color_depth = PIX_FMT_YUV420P;
  if (krad_theora->info.pixel_fmt == TH_PF_422) {
	  krad_theora->color_depth = PIX_FMT_YUV422P;
	  printk ("Theora color depth 422");
  }
  if (krad_theora->info.pixel_fmt == TH_PF_444) {
	  krad_theora->color_depth = PIX_FMT_YUV444P;
	  printk ("Theora color depth 444");	  
  }

	printk ("Theora %dx%d %.02f fps video\n Encoded frame content is %dx%d with %dx%d offset",
		   krad_theora->info.frame_width, krad_theora->info.frame_height, 
		   (double)krad_theora->info.fps_numerator/krad_theora->info.fps_denominator,
		   krad_theora->info.pic_width, krad_theora->info.pic_height, krad_theora->info.pic_x, krad_theora->info.pic_y);


	krad_theora->offset_y = krad_theora->info.pic_y;
	krad_theora->offset_x = krad_theora->info.pic_x;

	krad_theora->width = krad_theora->info.pic_width;
	krad_theora->height = krad_theora->info.pic_height;

	krad_theora->decoder = th_decode_alloc(&krad_theora->info, krad_theora->setup_info);

	th_setup_free(krad_theora->setup_info);

	return krad_theora;

}
