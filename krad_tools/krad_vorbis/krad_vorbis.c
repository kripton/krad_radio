#include <krad_vorbis.h>


void krad_vorbis_encoder_destroy(krad_vorbis_t *vorbis) {

	vorbis_block_clear (&vorbis->vblock);
	vorbis_dsp_clear (&vorbis->vdsp);
	vorbis_info_clear (&vorbis->vinfo);

	free(vorbis);
}

krad_vorbis_t *krad_vorbis_encoder_create (int channels, int sample_rate, float quality) {

	krad_vorbis_t *vorbis = calloc (1, sizeof(krad_vorbis_t));
	
	vorbis_info_init (&vorbis->vinfo);

	vorbis->ret = vorbis_encode_init_vbr (&vorbis->vinfo, channels, sample_rate, quality);

	if (vorbis->ret == OV_EIMPL) {
		printf ("Sorry, but this vorbis mode is not supported currently...\n");
		exit (1);
	}

	if (vorbis->ret == OV_EINVAL) {
		printf ("Sorry, but this is an illegal vorbis mode...\n");
		exit (1);
	}

  	vorbis->ret = vorbis_analysis_init (&vorbis->vdsp, &vorbis->vinfo);
	
	vorbis->ret = vorbis_block_init (&vorbis->vdsp, &vorbis->vblock);
	
	vorbis_comment_init (&vorbis->vc);

	vorbis_comment_add (&vorbis->vc, "ENCODEDBY=KradRadio");

	vorbis_analysis_headerout (&vorbis->vdsp, &vorbis->vc, 
							   &vorbis->header_main,
							   &vorbis->header_comments,
							   &vorbis->header_codebooks);

	vorbis->header[0] = 0x02;
	vorbis->headerpos++;

	//printf("main is %ld\n", vorbis->header_main.bytes);
	if (vorbis->header_main.bytes > 255) {
		printf("vorbis mainheader to long for code\n");
		exit(1);
	}
	
	vorbis->demented = vorbis->header_main.bytes;
	vorbis->header[1] = (char)vorbis->demented;
	vorbis->headerpos++;
	
	//printf("comments is %ld\n", vorbis->header_comments.bytes);
	if (vorbis->header_comments.bytes > 255) {
		printf("vorbis comments header to long for code\n");
		exit(1);
	}
	
	vorbis->demented = vorbis->header_comments.bytes;
	vorbis->header[2] = (char)vorbis->demented;
	vorbis->headerpos++;
	
	memcpy (vorbis->header + vorbis->headerpos, vorbis->header_main.packet, vorbis->header_main.bytes);
	vorbis->krad_codec_header.header[0] = vorbis->header + vorbis->headerpos;
	vorbis->headerpos += vorbis->header_main.bytes;
	vorbis->krad_codec_header.header_size[0] = vorbis->header_main.bytes;
		
	//printf("main is %ld bytes headerpos is  %d \n", vorbis->header_main.bytes, vorbis->headerpos);
		
	memcpy (vorbis->header + vorbis->headerpos, vorbis->header_comments.packet, vorbis->header_comments.bytes);
	vorbis->krad_codec_header.header[1] = vorbis->header + vorbis->headerpos;
	vorbis->headerpos += vorbis->header_comments.bytes;
	vorbis->krad_codec_header.header_size[1] = vorbis->header_comments.bytes;

	//printf("comments is %ld bytes headerpos is  %d \n", vorbis->header_comments.bytes, vorbis->headerpos);
		
	memcpy (vorbis->header + vorbis->headerpos, vorbis->header_codebooks.packet, vorbis->header_codebooks.bytes);
	vorbis->krad_codec_header.header[2] = vorbis->header + vorbis->headerpos;
	vorbis->headerpos += vorbis->header_codebooks.bytes;
	vorbis->krad_codec_header.header_size[2] = vorbis->header_codebooks.bytes;

	//printf("codebooks is %ld bytes headerpos is  %d \n", vorbis->header_codebooks.bytes, vorbis->headerpos);
	//printf("Vorbis header is %d bytes\n", vorbis->headerpos);

	vorbis->krad_codec_header.header_combined = vorbis->header;
	vorbis->krad_codec_header.header_combined_size = vorbis->headerpos;
	vorbis->krad_codec_header.header_count = 3;


	return vorbis;
}


ogg_packet *krad_vorbis_encode (krad_vorbis_t *vorbis, int frames, krad_ringbuffer_t *ring0, krad_ringbuffer_t *ring1) {


	if (vorbis->in_blockout == 0) {

		if ((krad_ringbuffer_read_space(ring0) >= frames * 4) && (krad_ringbuffer_read_space(ring1) >= frames * 4)) {

			vorbis->buffer = vorbis_analysis_buffer(&vorbis->vdsp, frames);
			
			krad_ringbuffer_read(ring0, (char *)&vorbis->buffer[0][0], frames * 4);
			krad_ringbuffer_read(ring1, (char *)&vorbis->buffer[1][0], frames * 4);
	
			vorbis->ret = vorbis_analysis_wrote(&vorbis->vdsp, frames);

		} else {
		
			return NULL;
		
		}
	
	}

	while (vorbis_analysis_blockout(&vorbis->vdsp, &vorbis->vblock)) {

		vorbis->in_blockout = 1;

		vorbis_analysis(&vorbis->vblock, NULL);
		vorbis_bitrate_addblock(&vorbis->vblock);

		if (vorbis_bitrate_flushpacket(&vorbis->vdsp, &vorbis->op)) {

			return &vorbis->op;

		}


	}
	
	vorbis->in_blockout = 0;
	
	return krad_vorbis_encode(vorbis, frames, ring0, ring1);
		
}



void krad_vorbis_decoder_destroy(krad_vorbis_t *vorbis) {

	int c;

	for (c = 0; c < vorbis->vinfo.channels; c++) {
		krad_ringbuffer_free (vorbis->ringbuf[c]);
	}
	
	//nasty kludge, discusting
	if (vorbis->channels == 1) {
		krad_ringbuffer_free (vorbis->ringbuf[1]);
	}

	vorbis_info_clear(&vorbis->vinfo);
	vorbis_comment_clear(&vorbis->vc);
	vorbis_block_clear(&vorbis->vblock);
	
	//ogg_sync_destroy(&vorbis->oy);
	//ogg_stream_destroy(&vorbis->oggstate);
	
	free(vorbis);

}


krad_vorbis_t *krad_vorbis_decoder_create(unsigned char *header1, int header1len, unsigned char *header2, int header2len, unsigned char *header3, int header3len) {

	krad_vorbis_t *vorbis = calloc(1, sizeof(krad_vorbis_t));

	//int x;
	int c;

	//char *ogg_in_buffer;

	srand(time(NULL));

	vorbis_info_init(&vorbis->vinfo);
	vorbis_comment_init(&vorbis->vc);
	
	vorbis->op.packet = header1;
	vorbis->op.bytes = header1len;
	vorbis->op.b_o_s = 1;
	vorbis->op.packetno = 1;
	vorbis_synthesis_headerin(&vorbis->vinfo, &vorbis->vc, &vorbis->op);
	//printf("x is %d len is %d\n", x, header1len);

	vorbis->op.packet = header2;
	vorbis->op.bytes = header2len;
	vorbis->op.b_o_s = 0;
	vorbis->op.packetno = 2;
	vorbis_synthesis_headerin(&vorbis->vinfo, &vorbis->vc, &vorbis->op);
	//printf("x is %d len is %d\n", x, header2len);

	vorbis->op.packet = header3;
	vorbis->op.bytes = header3len;
	vorbis->op.packetno = 3;
	vorbis_synthesis_headerin(&vorbis->vinfo, &vorbis->vc, &vorbis->op);
	//printf("x is %d len is %d\n", x, header3len);

	vorbis_synthesis_init(&vorbis->vdsp, &vorbis->vinfo);
	vorbis_block_init(&vorbis->vdsp, &vorbis->vblock);

	printf("Vorbis Info - Version: %d Channels: %d Sample Rate: %ld Bitrate: %ld %ld %ld\n", vorbis->vinfo.version, vorbis->vinfo.channels, vorbis->vinfo.rate, vorbis->vinfo.bitrate_upper, vorbis->vinfo.bitrate_nominal, vorbis->vinfo.bitrate_lower);

	vorbis->channels = vorbis->vinfo.channels;
	vorbis->sample_rate = vorbis->vinfo.rate;

	for (c = 0; c < vorbis->channels; c++) {
		vorbis->ringbuf[c] = krad_ringbuffer_create (RINGBUFFER_SIZE);
	}
	
	//nasty kludge, discusting
	if (vorbis->channels == 1) {
		vorbis->ringbuf[1] = krad_ringbuffer_create (RINGBUFFER_SIZE);
	}

	return vorbis;

}

int krad_vorbis_decoder_read_audio(krad_vorbis_t *vorbis, int channel, char *buffer, int buffer_length) {

	return krad_ringbuffer_read (vorbis->ringbuf[channel], (char *)buffer, buffer_length );
	
}

void krad_vorbis_decoder_decode(krad_vorbis_t *vorbis, unsigned char *buffer, int bufferlen) {

	int sample_count;
	float **pcm;

	vorbis->op.packet = buffer;
	vorbis->op.bytes = bufferlen;
	vorbis->op.packetno++;

	vorbis_synthesis(&vorbis->vblock, &vorbis->op);
	vorbis_synthesis_blockin(&vorbis->vdsp, &vorbis->vblock);
	sample_count = vorbis_synthesis_pcmout(&vorbis->vdsp, &pcm);
	
	if (sample_count) {
		//printf("Vorbis decoded %d samples\n", sample_count);
		while((krad_ringbuffer_write_space(vorbis->ringbuf[0]) < sample_count * 4) || (krad_ringbuffer_write_space(vorbis->ringbuf[1]) < sample_count * 4)) {
			usleep(15000);
		}
	
		krad_ringbuffer_write (vorbis->ringbuf[0], (char *)pcm[0], sample_count * 4 );
		if (vorbis->channels == 1) {
			krad_ringbuffer_write (vorbis->ringbuf[1], (char *)pcm[0], sample_count * 4 );
		}
		if (vorbis->channels == 2) {
			krad_ringbuffer_write (vorbis->ringbuf[1], (char *)pcm[1], sample_count * 4 );
		}
		vorbis_synthesis_read(&vorbis->vdsp, sample_count);
	}

}

