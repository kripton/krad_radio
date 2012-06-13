#include "krad_dirac.h"

void krad_dirac_encoder_destroy (krad_dirac_t *krad_dirac) {

	schro_encoder_free (krad_dirac->encoder);

	schro_frame_unref (krad_dirac->frame);

	free (krad_dirac->format);

	free (krad_dirac->fullpacket);

	free (krad_dirac);
}

krad_dirac_t *krad_dirac_encoder_create (int width, int height) {

	krad_dirac_t *krad_dirac = calloc(1, sizeof(krad_dirac_t));
	
	krad_dirac->width = width;
	krad_dirac->height = height;
	
	schro_init ();

	krad_dirac->encoder = schro_encoder_new ();
	krad_dirac->format = schro_encoder_get_video_format (krad_dirac->encoder);

	krad_dirac->format->width = krad_dirac->width;
	krad_dirac->format->height = krad_dirac->height;
	krad_dirac->format->clean_width = krad_dirac->width;
	krad_dirac->format->clean_height = krad_dirac->height;
	krad_dirac->format->left_offset = 0;
	krad_dirac->format->top_offset = 0;
	krad_dirac->format->chroma_format = SCHRO_CHROMA_420;
	schro_encoder_set_video_format (krad_dirac->encoder, krad_dirac->format);
	
	//schro_debug_set_level (SCHRO_LEVEL_DEBUG);
	
	schro_encoder_setting_set_double (krad_dirac->encoder, "gop_structure", SCHRO_ENCODER_GOP_INTRA_ONLY);
	schro_encoder_setting_set_double (krad_dirac->encoder, "rate_control", SCHRO_ENCODER_RATE_CONTROL_CONSTANT_BITRATE);
//	schro_encoder_setting_set_double (krad_dirac->encoder, "rate_control", SCHRO_ENCODER_RATE_CONTROL_LOW_DELAY);

	schro_encoder_setting_set_double (krad_dirac->encoder, "bitrate", 15000000);

	schro_encoder_start (krad_dirac->encoder);

	krad_dirac->size = krad_dirac->width * krad_dirac->height;
	krad_dirac->size += krad_dirac->width/2 * krad_dirac->height/2;
	krad_dirac->size += krad_dirac->width/2 * krad_dirac->height/2;

	//fixme dum, but aint going to bigger than uncompressed
	krad_dirac->fullpacket = calloc (1, krad_dirac->size);

	krad_dirac->frame = schro_frame_new_and_alloc (NULL,
												   SCHRO_FRAME_FORMAT_U8_420,
												   krad_dirac->width,
												   krad_dirac->height);

	return krad_dirac;
	
}

void krad_dirac_encoder_frame_free (SchroFrame *frame, void *priv) {
	//free (priv);
}

int krad_dirac_encoder_write (krad_dirac_t *krad_dirac, unsigned char **packet) {


	krad_dirac->go = 1;
		
	int packet_len;
	int framenum;
	
	framenum = 0;
	packet_len = 0;
	
	
	//KLUDGE
	*packet = krad_dirac->fullpacket;
		
	//krad_dirac->frame = schro_frame_new_from_data_I420 (krad_dirac->picture, krad_dirac->width, krad_dirac->height);
	//krad_dirac->frame = schro_frame_new_from_data_YV12 (frame, krad_dirac->width, krad_dirac->height);
	//schro_frame_set_free_callback (krad_dirac->frame, krad_dirac_encoder_frame_free, krad_dirac->picture);
	schro_encoder_push_frame (krad_dirac->encoder, krad_dirac->frame);
	
	while (krad_dirac->go) {
		switch (schro_encoder_wait (krad_dirac->encoder)) {
			case SCHRO_STATE_NEED_FRAME:
				//printk ("need frame!");
				
				krad_dirac->frame = schro_frame_new_and_alloc (NULL,
															   SCHRO_FRAME_FORMAT_U8_420,
															   krad_dirac->width,
															   krad_dirac->height);
				
				return packet_len;
			case SCHRO_STATE_HAVE_BUFFER:
				//printk ("have buffer!");
				krad_dirac->buffer = schro_encoder_pull (krad_dirac->encoder, &framenum);
				memcpy (krad_dirac->fullpacket + packet_len, krad_dirac->buffer->data, krad_dirac->buffer->length);
				packet_len += krad_dirac->buffer->length;
				schro_buffer_unref (krad_dirac->buffer);
				break;
			case SCHRO_STATE_AGAIN:
				printk ("again!");
				break;
			case SCHRO_STATE_END_OF_STREAM:
				printk ("eos!");
				return packet_len;
			default:
				printk ("????");
				break;
		}
	}
	
	return 0;
}



/*		DECODER STUFF		*/


void krad_dirac_packet_type(unsigned char p) {

	char *parse_code;

	switch (p) {
		case SCHRO_PARSE_CODE_SEQUENCE_HEADER:
		  parse_code = "access unit header";
		  break;
		case SCHRO_PARSE_CODE_AUXILIARY_DATA:
		  parse_code = "auxiliary data";
		  break;
		case SCHRO_PARSE_CODE_INTRA_REF:
		  parse_code = "intra ref";
		  break;
		case SCHRO_PARSE_CODE_INTRA_NON_REF:
		  parse_code = "intra non-ref";
		  break;
		case SCHRO_PARSE_CODE_INTER_REF_1:
		  parse_code = "inter ref 1";
		  break;
		case SCHRO_PARSE_CODE_INTER_REF_2:
		  parse_code = "inter ref 2";
		  break;
		case SCHRO_PARSE_CODE_INTER_NON_REF_1:
		  parse_code = "inter non-ref 1";
		  break;
		case SCHRO_PARSE_CODE_INTER_NON_REF_2:
		  parse_code = "inter non-ref 2";
		  break;
		case SCHRO_PARSE_CODE_END_OF_SEQUENCE:
		  parse_code = "end of sequence";
		  break;
		case SCHRO_PARSE_CODE_LD_INTRA_REF:
		  parse_code = "low-delay intra ref";
		  break;
		case SCHRO_PARSE_CODE_LD_INTRA_NON_REF:
		  parse_code = "low-delay intra non-ref";
		  break;
		case SCHRO_PARSE_CODE_INTRA_REF_NOARITH:
		  parse_code = "intra ref noarith";
		  break;
		case SCHRO_PARSE_CODE_INTRA_NON_REF_NOARITH:
		  parse_code = "intra non-ref noarith";
		  break;
		case SCHRO_PARSE_CODE_INTER_REF_1_NOARITH:
		  parse_code = "inter ref 1 noarith";
		  break;
		case SCHRO_PARSE_CODE_INTER_REF_2_NOARITH:
		  parse_code = "inter ref 2 noarith";
		  break;
		case SCHRO_PARSE_CODE_INTER_NON_REF_1_NOARITH:
		  parse_code = "inter non-ref 1 noarith";
		  break;
		case SCHRO_PARSE_CODE_INTER_NON_REF_2_NOARITH:
		  parse_code = "inter non-ref 2 noarith";
		  break;
		case SCHRO_PARSE_CODE_PADDING:
		  parse_code = "padding";
		  break;
		default:
		  parse_code = "unknown";
		  break;
	}
	printk ("Parse code: %s (0x%02x)\n", parse_code, p);
  
}

int pull_packet (unsigned char *buffer, unsigned char **p_data, int *p_size) {

	unsigned char *packet;
	unsigned char header[13];
	int n;
	int size;

	memcpy(header, buffer, 13);
	
	if (header[0] != 'B' || header[1] != 'B' || header[2] != 'C' ||
	  header[3] != 'D') {
	return 0;
	}

	size = (header[5]<<24) | (header[6]<<16) | (header[7]<<8) | (header[8]);
	if (size == 0) {
	size = 13;
	}
	if (size < 13) {
	return 0;
	}
	if (size > 16*1024*1024) {
	printk ("packet too large? (%d > 16777216)\n", size);
	return 0;
	}

	packet = malloc (size);
	memcpy (packet, header, 13);
	
	memcpy (packet + 13, buffer + 13, size - 13);
	
	n = size - 13;
	
	if (n < size - 13) {
	free (packet);
	return 0;
	}

	*p_data = packet;
	*p_size = size;
	return 1;
}

void krad_dirac_decoder_buffer_free (SchroBuffer *buf, void *priv) {
	free (priv);
}

void krad_dirac_decode (krad_dirac_t *dirac, unsigned char *buffer, int len) {


	int it;
	//unsigned char *buffer_cpy;
	int pushed;
	//void *testbuffer;
	unsigned char *packet;
	int packetsize;

	int buffer_pos;

	pushed = 0;
	dirac->verbose = 0;
	
	if (dirac->frame != NULL) {
		if (dirac->verbose) {
			printk ("frame freed\n");
			schro_frame_unref (dirac->frame);
			dirac->frame = NULL;
		}
	}
	
	//buffer_cpy = malloc(1000000);
	//memcpy(buffer_cpy, buffer, len);

	buffer_pos = 0;

	while (buffer_pos != len) {
		pull_packet(buffer + buffer_pos, &packet, &packetsize);
		if (dirac->verbose) {
			krad_dirac_packet_type(buffer[4]);
			printk ("buffer size is %d, packet size is %d\n", len, packetsize);
		}
		dirac->buffer = schro_buffer_new_with_data (packet, packetsize);
		dirac->buffer->free = krad_dirac_decoder_buffer_free;
		dirac->buffer->priv = packet;

		buffer_pos += packetsize;
		
		it = schro_decoder_push (dirac->decoder, dirac->buffer);
		if (it == SCHRO_DECODER_FIRST_ACCESS_UNIT) {
			dirac->format = schro_decoder_get_video_format (dirac->decoder);
			dirac->width = dirac->format->width;
			dirac->height = dirac->format->height;
			if (dirac->verbose) {
				printk ("dirac video info: width: %d height %d\n", dirac->width, dirac->height);			
			}
		}
		pushed = 1;

	}

	while (1) {
	
		it = schro_decoder_wait (dirac->decoder);

		switch (it) {
			case SCHRO_DECODER_NEED_BITS:
				if (pushed == 0) {
					it = schro_decoder_push (dirac->decoder, dirac->buffer);
					if (it == SCHRO_DECODER_FIRST_ACCESS_UNIT) {
						dirac->format = schro_decoder_get_video_format (dirac->decoder);
						dirac->width = dirac->format->width;
						dirac->height = dirac->format->height;
						printk ("dirac video info: width: %d height %d\n", dirac->width, dirac->height);			
					}
					pushed = 1;
					break;
				} else {
					printk ("noframe need bits!\n");
					return;
				}
			case SCHRO_DECODER_NEED_FRAME:
				//schro_video_format_get_picture_luma_size (dirac->format, &dirac->width, &dirac->height);
				dirac->frame2 = schro_frame_new_and_alloc (NULL,
														   schro_params_get_frame_format(8, dirac->format->chroma_format),
														   dirac->width,
														   dirac->height);
				//testbuffer = malloc(2000000);
				//dirac->frame2 = schro_frame_new_from_data_I420 (testbuffer, dirac->width, dirac->height);
				
				schro_decoder_add_output_picture (dirac->decoder, dirac->frame2);
				dirac->frame2 = NULL;
				break;
			case SCHRO_DECODER_OK:
				if (dirac->verbose) {
					printk ("****picture number %d\n", schro_decoder_get_picture_number (dirac->decoder));
				}
				
				dirac->frame = schro_decoder_pull (dirac->decoder);
				if (dirac->frame->width == 0) {
					//schro_frame_unref (dirac->frame);
					dirac->frame = NULL;
					printk ("fuuuuuuuuuuuuuuuuuuuuuuuu\n");
					break;
				}
				if (dirac->verbose) {
					printk ("\n\n\n\n****Frame Info: format: %d  width: %d height %d \n", dirac->frame->format, dirac->frame->width, dirac->frame->height);
					printk ("c0 %d  c1: %d c2 %d \n", dirac->frame->components[0].length, dirac->frame->components[1].length, dirac->frame->components[2].length);
				}
				
				return;
				
			case SCHRO_DECODER_EOS:
				failfast ("got eos\n");
			case SCHRO_DECODER_ERROR:
				failfast ("dirac decode fail\n");
		}
	}
}

krad_dirac_t *krad_dirac_decoder_create() {

	krad_dirac_t *dirac = calloc(1, sizeof(krad_dirac_t));

	schro_init();

	dirac->decoder = schro_decoder_new();
	//schro_decoder_set_picture_order (dirac->decoder, SCHRO_DECODER_PICTURE_ORDER_CODED);
	//schro_decoder_set_skip_ratio(dirac->decoder, 1.0);
	//schro_debug_set_level (SCHRO_LEVEL_DEBUG);
	return dirac;

}

void krad_dirac_decoder_destroy(krad_dirac_t *dirac) {

	// ???
	schro_decoder_push_end_of_stream (dirac->decoder);

	schro_decoder_free (dirac->decoder);
	free(dirac->format);
	free(dirac);

}

