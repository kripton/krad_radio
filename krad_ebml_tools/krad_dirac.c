#include <krad_dirac.h>


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

	schro_encoder_start (dirac->encoder);

	dirac->size = dirac->width * dirac->height;
	dirac->size += dirac->width/2 * dirac->height/2;
	dirac->size += dirac->width/2 * dirac->height/2;

	return dirac;
	
}

void krad_dirac_encoder_frame_free(SchroFrame *frame, void *priv) {

	free (priv);

}

void krad_dirac_encode (krad_dirac_t *dirac) {

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

void krad_dirac_decode (krad_dirac_t *dirac) {

	int go;
	int it;
	int eos = FALSE;
	int eof = FALSE;
	void *packet;
	int size;
	int ret;

printf("hi!\n");
			  if (dirac->frame) {
				printf("frame freed\n");
				schro_frame_unref (dirac->frame);
				dirac->frame = NULL;
			  }

	while(!eos) {
		ret = parse_packet (dirac->file, &packet, &size);
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


krad_dirac_t *krad_dirac_decoder_create() {

	krad_dirac_t *dirac = calloc(1, sizeof(krad_dirac_t));

	schro_init();

	dirac->decoder = schro_decoder_new();

	return dirac;

}


void krad_dirac_decoder_destroy(krad_dirac_t *dirac) {

	//test only
	if (dirac->file != NULL) {
		fclose (dirac->file);
	}


	schro_decoder_free (dirac->decoder);
	free(dirac->format);
	free(dirac);

}

