#include "krad_dirac.h"

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
	printf("Parse code: %s (0x%02x)\n", parse_code, p);
  
}

int parse_packet (FILE *file, void **p_data, int *p_size)
{
  unsigned char *packet;
  unsigned char header[13];
  int n;
  int size;

  n = fread (header, 1, 13, file);
  if (n == 0) {
    *p_data = NULL;
    *p_size = 0;
    return 1;
  }
  if (n < 13) {
    printf("truncated header\n");
    return 0;
  }

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
    printf("packet too large? (%d > 16777216)\n", size);
    return 0;
  }

  packet = malloc (size);
  memcpy (packet, header, 13);
  n = fread (packet + 13, 1, size - 13, file);
  if (n < size - 13) {
    free (packet);
    return 0;
  }

  *p_data = packet;
  *p_size = size;
  return 1;
}


void krad_dirac_encoder_destroy(krad_dirac_t *dirac) {

	schro_encoder_free (dirac->encoder);

	free(dirac);
}

krad_dirac_t *krad_dirac_encoder_create(int width, int height) {

	krad_dirac_t *dirac = calloc(1, sizeof(krad_dirac_t));
	
	dirac->width = width;
	dirac->height = height;
	
	schro_init();
	dirac->encoder = schro_encoder_new();
	dirac->format = schro_encoder_get_video_format(dirac->encoder);

	dirac->format->width = dirac->width;
	dirac->format->height = dirac->height;
	dirac->format->clean_width = dirac->width;
	dirac->format->clean_height = dirac->height;
	dirac->format->left_offset = 0;
	dirac->format->top_offset = 0;
	schro_encoder_set_video_format (dirac->encoder, dirac->format);
	free (dirac->format);
	//schro_debug_set_level (SCHRO_LEVEL_DEBUG);
	
	schro_encoder_setting_set_double (dirac->encoder, "gop_structure", SCHRO_ENCODER_GOP_INTRA_ONLY);
	schro_encoder_setting_set_double (dirac->encoder, "rate_control", SCHRO_ENCODER_RATE_CONTROL_CONSTANT_BITRATE);

	schro_encoder_start (dirac->encoder);

	dirac->size = dirac->width * dirac->height;
	dirac->size += dirac->width/2 * dirac->height/2;
	dirac->size += dirac->width/2 * dirac->height/2;

	return dirac;
	
}

void krad_dirac_encoder_frame_free(SchroFrame *frame, void *priv) {

	free (priv);

}

void krad_dirac_encode_test (krad_dirac_t *dirac) {

	dirac->n_frames = 0;
	dirac->go = 1;
	
	int count = 0;
	
	while (dirac->go) {

		int x;
		
		printf("count is %d\n", count++);

		switch (schro_encoder_wait (dirac->encoder)) {
		  case SCHRO_STATE_NEED_FRAME:
			if (dirac->n_frames < 100) {
			  //SCHRO_ERROR("frame %d", n_frames);

			  dirac->picture = malloc(dirac->size);
			  memset (dirac->picture, 128, dirac->size);

			  dirac->frame = schro_frame_new_from_data_I420 (dirac->picture, dirac->width, dirac->height);

			  schro_frame_set_free_callback (dirac->frame, krad_dirac_encoder_frame_free, dirac->picture);

			  schro_encoder_push_frame (dirac->encoder, dirac->frame);

			  dirac->n_frames++;
			} else {
			  schro_encoder_end_of_stream (dirac->encoder);
			}
			break;
		  case SCHRO_STATE_HAVE_BUFFER:
			dirac->buffer = schro_encoder_pull (dirac->encoder, &x);
			printf("yadda %d\n", x);
			schro_buffer_unref (dirac->buffer);
			break;
		  case SCHRO_STATE_AGAIN:
			break;
		  case SCHRO_STATE_END_OF_STREAM:
			dirac->go = 0;
			break;
		  default:
			break;
		}
	}
}

int krad_dirac_encode (krad_dirac_t *dirac, void *frame, void *output) {

	//dirac->n_frames = 0;
	dirac->go = 1;
	
	int headerlen;
	
	int framenum;
	
	framenum = 0;
	
	
	headerlen = 0;
			  //dirac->frame = schro_frame_new_from_data_I420 (dirac->picture, dirac->width, dirac->height);
			  dirac->frame = schro_frame_new_from_data_YV12 (frame, dirac->width, dirac->height);
			  schro_frame_set_free_callback (dirac->frame, krad_dirac_encoder_frame_free, dirac->picture);
			  schro_encoder_push_frame (dirac->encoder, dirac->frame);
			  //	*took = 1;

	while (dirac->go) {

		switch (schro_encoder_wait (dirac->encoder)) {
		  case SCHRO_STATE_NEED_FRAME:
		  
		  	//if (*took == 1) {
		  		return 0;
		  	//}
		  
		  
		  	printf("need frame!\n");
		  	
		  	//*took = 1;
		  	
			//if (dirac->n_frames < 100) {
			  //SCHRO_ERROR("frame %d", n_frames);


			  //dirac->n_frames++;
			  
			  //usleep(1000000);
			  
			//} else {
			//  schro_encoder_end_of_stream (dirac->encoder);
			//}
			break;
		  case SCHRO_STATE_HAVE_BUFFER:
		  	printf("have buffer!\n");
			dirac->buffer = schro_encoder_pull (dirac->encoder, &framenum);
			memcpy(output + headerlen, dirac->buffer->data, dirac->buffer->length);
			
			if (headerlen == 0) {
				headerlen = dirac->buffer->length;
				schro_buffer_unref (dirac->buffer);
				break;
			}
			
			schro_buffer_unref (dirac->buffer);
			return headerlen + dirac->buffer->length;
		  case SCHRO_STATE_AGAIN:
		  	printf("again!\n");
			break;
		  case SCHRO_STATE_END_OF_STREAM:
		  	printf("eos!\n");
			dirac->go = 0;
			break;
		  default:
		  	printf("????\n");
			break;
		}
	}
	
	return 0;
}

void krad_dirac_decoder_buffer_free (SchroBuffer *buf, void *priv) {

	free (priv);

}


void krad_dirac_test_video_open (krad_dirac_t *dirac, char *filename) {

	//int go;
	int it;
	//int eos = FALSE;
	//int eof = FALSE;
	void *packet;
	int size;
	int ret;

	dirac->verbose = 1;


	dirac->file = fopen (filename, "r");



		ret = parse_packet (dirac->file, &packet, &size);
		if (!ret) {
		  exit(1);
		}

		//printf("packet size %d\n", size);
		if (size == 0) {
		  //eof = TRUE;
		  schro_decoder_push_end_of_stream (dirac->decoder);
		} else {
		  dirac->buffer = schro_buffer_new_with_data (packet, size);
		  dirac->buffer->free = krad_dirac_decoder_buffer_free;
		  dirac->buffer->priv = packet;

		  it = schro_decoder_push (dirac->decoder, dirac->buffer);
		  if (it == SCHRO_DECODER_FIRST_ACCESS_UNIT) {
			dirac->format = schro_decoder_get_video_format (dirac->decoder);
			
			dirac->width = dirac->format->width;
			dirac->height = dirac->format->height;
			
			printf("dirac video info: width: %d height %d\n", dirac->format->width, dirac->format->height);
			printf("frame rate: %d / %d\n", dirac->format->frame_rate_numerator, dirac->format->frame_rate_denominator);
		  }
		}
}

void krad_dirac_decode_test (krad_dirac_t *dirac) {

	int go;
	int it;
	int eos = FALSE;
	int eof = FALSE;
	unsigned char *packet;
	int size;
	int ret;
	
	static int packetcount = 0;

printf("hi!\n");
			  if (dirac->frame) {
				printf("frame freed\n");
				schro_frame_unref (dirac->frame);
				dirac->frame = NULL;
			  }

	while(!eos) {
		ret = parse_packet (dirac->file, (void **)&packet, &size);
		krad_dirac_packet_type(packet[4]);
		printf("\n\nPAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAKKKKKKKKKKKKKKKAAAAAAAAAATTT %d\n\n\n\n\n\n", packetcount++);
		if (!ret) {
		  exit(1);
		}

		//printf("packet size %d\n", size);
		if (size == 0) {
		  eof = TRUE;
		  schro_decoder_push_end_of_stream (dirac->decoder);
		} else {
		  dirac->buffer = schro_buffer_new_with_data (packet, size);
		  dirac->buffer->free = krad_dirac_decoder_buffer_free;
		  dirac->buffer->priv = packet;

		  it = schro_decoder_push (dirac->decoder, dirac->buffer);
		  //if (it == SCHRO_DECODER_FIRST_ACCESS_UNIT) {
		//	dirac->format = schro_decoder_get_video_format (dirac->decoder);
			
		//	printf("dirac video info: width: %d height %d\n", dirac->format->width, dirac->format->height);
			
		  //}
		}

		go = 1;
		while (go) {
		  it = schro_decoder_wait (dirac->decoder);

		  switch (it) {
			case SCHRO_DECODER_NEED_BITS:
			  go = 0;
			  break;
			case SCHRO_DECODER_NEED_FRAME:
			  schro_video_format_get_picture_luma_size (dirac->format, &dirac->width, &dirac->height);
			  dirac->frame = schro_frame_new_and_alloc (NULL,
				  schro_params_get_frame_format(8, dirac->format->chroma_format),
				  dirac->width, dirac->height);
			  schro_decoder_add_output_picture (dirac->decoder, dirac->frame);
			  break;
			case SCHRO_DECODER_OK:
			  if (dirac->verbose) printf("picture number %d\n",
				  schro_decoder_get_picture_number (dirac->decoder));
			  		dirac->frame = schro_decoder_pull (dirac->decoder);

				printf("Frame Info: format: %d  width: %d height %d \n", dirac->frame->format, dirac->frame->width, dirac->frame->height);
				printf("c0 %d  c1: %d c2 %d \n", dirac->frame->components[0].length, dirac->frame->components[1].length, dirac->frame->components[2].length);
				
					return;
				
			  if (dirac->frame) {
				schro_frame_unref (dirac->frame);
			  }
			  break;
			case SCHRO_DECODER_EOS:
				printf("got eos\n");
			  if (eof) {
				eos = TRUE;
			  }
			  go = 0;
			  break;
			case SCHRO_DECODER_ERROR:
			  exit(0);
			  break;
		  }
		}
	}

	
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
	printf("packet too large? (%d > 16777216)\n", size);
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
			printf("frame freed\n");
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
			printf("buffer size is %d, packet size is %d\n", len, packetsize);
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
				printf("dirac video info: width: %d height %d\n", dirac->width, dirac->height);			
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
						printf("dirac video info: width: %d height %d\n", dirac->width, dirac->height);			
					}
					pushed = 1;
					break;
				} else {
					printf("noframe need bits!\n");
					return;
				}
			case SCHRO_DECODER_NEED_FRAME:
				//schro_video_format_get_picture_luma_size (dirac->format, &dirac->width, &dirac->height);
				dirac->frame2 = schro_frame_new_and_alloc (NULL, schro_params_get_frame_format(8, dirac->format->chroma_format), dirac->width, dirac->height);
				//testbuffer = malloc(2000000);
				//dirac->frame2 = schro_frame_new_from_data_I420 (testbuffer, dirac->width, dirac->height);
				
				schro_decoder_add_output_picture (dirac->decoder, dirac->frame2);
				dirac->frame2 = NULL;
				break;
			case SCHRO_DECODER_OK:
				if (dirac->verbose) {
					printf("****picture number %d\n", schro_decoder_get_picture_number (dirac->decoder));
				}
				
				dirac->frame = schro_decoder_pull (dirac->decoder);
				if (dirac->frame->width == 0) {
					//schro_frame_unref (dirac->frame);
					dirac->frame = NULL;
					printf("fuuuuuuuuuuuuuuuuuuuuuuuu\n");
					break;
				}
				if (dirac->verbose) {
					printf("\n\n\n\n****Frame Info: format: %d  width: %d height %d \n", dirac->frame->format, dirac->frame->width, dirac->frame->height);
					printf("c0 %d  c1: %d c2 %d \n", dirac->frame->components[0].length, dirac->frame->components[1].length, dirac->frame->components[2].length);
				}
				
				return;
				
			case SCHRO_DECODER_EOS:
				printf("got eos\n");
				exit(1);
				return;
			case SCHRO_DECODER_ERROR:
				printf("dirac decode fail\n");
				exit(1);
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

	//test only
	if (dirac->file != NULL) {
		fclose (dirac->file);
	}
	
	// ???
	schro_decoder_push_end_of_stream (dirac->decoder);


	schro_decoder_free (dirac->decoder);
	free(dirac->format);
	free(dirac);

}

