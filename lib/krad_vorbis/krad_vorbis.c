#include <krad_vorbis.h>

/* Encoding */

void krad_vorbis_encoder_destroy (krad_vorbis_t *krad_vorbis) {

	vorbis_block_clear (&krad_vorbis->vblock);
	vorbis_dsp_clear (&krad_vorbis->vdsp);
	vorbis_info_clear (&krad_vorbis->vinfo);

	free (krad_vorbis);
}

krad_vorbis_t *krad_vorbis_encoder_create (int channels, int sample_rate, float quality) {

	krad_vorbis_t *krad_vorbis = calloc (1, sizeof(krad_vorbis_t));
	
	krad_vorbis->channels = channels;
	krad_vorbis->sample_rate = sample_rate;
	krad_vorbis->quality = quality;	
	
	vorbis_info_init (&krad_vorbis->vinfo);

	krad_vorbis->ret = vorbis_encode_init_vbr (&krad_vorbis->vinfo,
											   krad_vorbis->channels,
											   krad_vorbis->sample_rate,
											   krad_vorbis->quality);

	if (krad_vorbis->ret == OV_EIMPL) {
		failfast ("Krad Vorbis Encoder: Sorry, but this vorbis mode is not supported currently...");
	}

	if (krad_vorbis->ret == OV_EINVAL) {
		failfast ("Krad Vorbis Encoder: Sorry, but this is an illegal vorbis mode...");
	}

  	krad_vorbis->ret = vorbis_analysis_init (&krad_vorbis->vdsp, &krad_vorbis->vinfo);
	krad_vorbis->ret = vorbis_block_init (&krad_vorbis->vdsp, &krad_vorbis->vblock);
	
	vorbis_comment_init (&krad_vorbis->vc);

	vorbis_comment_add_tag (&krad_vorbis->vc, "ENCODER", APPVERSION);

	vorbis_analysis_headerout (&krad_vorbis->vdsp,
							   &krad_vorbis->vc, 
							   &krad_vorbis->header_main,
							   &krad_vorbis->header_comments,
							   &krad_vorbis->header_codebooks);

	krad_vorbis->header[0] = 0x02;
	krad_vorbis->headerpos++;

	if (krad_vorbis->header_main.bytes > 255) {
		failfast ("Krad Vorbis Encoder: mainheader to long for code");
	}
	
	krad_vorbis->head_count = krad_vorbis->header_main.bytes;
	krad_vorbis->header[1] = (char)krad_vorbis->head_count;
	krad_vorbis->headerpos++;
	
	if (krad_vorbis->header_comments.bytes > 255) {
		failfast ("Krad Vorbis Encoder: comments header to long for code..");
	}
	
	krad_vorbis->head_count = krad_vorbis->header_comments.bytes;
	krad_vorbis->header[2] = (char)krad_vorbis->head_count;
	krad_vorbis->headerpos++;
	
	memcpy (krad_vorbis->header + krad_vorbis->headerpos,
			krad_vorbis->header_main.packet,
			krad_vorbis->header_main.bytes);
	krad_vorbis->krad_codec_header.header[0] = krad_vorbis->header + krad_vorbis->headerpos;
	krad_vorbis->headerpos += krad_vorbis->header_main.bytes;
	krad_vorbis->krad_codec_header.header_size[0] = krad_vorbis->header_main.bytes;
		
	memcpy (krad_vorbis->header + krad_vorbis->headerpos,
			krad_vorbis->header_comments.packet,
			krad_vorbis->header_comments.bytes);
	krad_vorbis->krad_codec_header.header[1] = krad_vorbis->header + krad_vorbis->headerpos;
	krad_vorbis->headerpos += krad_vorbis->header_comments.bytes;
	krad_vorbis->krad_codec_header.header_size[1] = krad_vorbis->header_comments.bytes;
		
	memcpy (krad_vorbis->header + krad_vorbis->headerpos,
			krad_vorbis->header_codebooks.packet,
			krad_vorbis->header_codebooks.bytes);
	krad_vorbis->krad_codec_header.header[2] = krad_vorbis->header + krad_vorbis->headerpos;
	krad_vorbis->headerpos += krad_vorbis->header_codebooks.bytes;
	krad_vorbis->krad_codec_header.header_size[2] = krad_vorbis->header_codebooks.bytes;

	krad_vorbis->krad_codec_header.header_combined = krad_vorbis->header;
	krad_vorbis->krad_codec_header.header_combined_size = krad_vorbis->headerpos;
	krad_vorbis->krad_codec_header.header_count = 3;

	return krad_vorbis;
}


void krad_vorbis_encoder_prepare (krad_vorbis_t *krad_vorbis, int frames, float ***buffer) {

	*buffer = vorbis_analysis_buffer (&krad_vorbis->vdsp, frames);

}

int krad_vorbis_encoder_wrote (krad_vorbis_t *krad_vorbis, int frames) {

	return vorbis_analysis_wrote (&krad_vorbis->vdsp, frames);
}

int krad_vorbis_encoder_finish (krad_vorbis_t *krad_vorbis) {

	return vorbis_analysis_wrote (&krad_vorbis->vdsp, 0);
}

int krad_vorbis_encoder_write (krad_vorbis_t *krad_vorbis, float **samples, int frames) {

	int c;

	krad_vorbis_encoder_prepare (krad_vorbis, frames, &krad_vorbis->buffer);
	
	for (c = 0; c < krad_vorbis->channels; c++) {
		memcpy (krad_vorbis->buffer[c], (unsigned char *)samples[c], frames * 4);
	}

	return krad_vorbis_encoder_wrote (krad_vorbis, frames);
}
	
int krad_vorbis_encoder_read (krad_vorbis_t *krad_vorbis, int *out_frames, unsigned char **buffer) {

	if (vorbis_analysis_blockout(&krad_vorbis->vdsp, &krad_vorbis->vblock)) {
		vorbis_analysis (&krad_vorbis->vblock, &krad_vorbis->op);
		*out_frames = krad_vorbis->op.granulepos - krad_vorbis->frames_encoded;
		krad_vorbis->frames_encoded = krad_vorbis->op.granulepos;		
		*buffer = krad_vorbis->op.packet;
		return krad_vorbis->op.bytes;
	}

	return 0;
}


/* Decoding */


void krad_vorbis_decoder_destroy (krad_vorbis_t *vorbis) {

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


krad_vorbis_t *krad_vorbis_decoder_create (unsigned char *header1, int header1len,
										   unsigned char *header2, int header2len,
										   unsigned char *header3, int header3len) {

	krad_vorbis_t *vorbis = calloc(1, sizeof(krad_vorbis_t));

  int ret;
	int c;

	vorbis_info_init(&vorbis->vinfo);
	vorbis_comment_init(&vorbis->vc);
	
	vorbis->op.packet = header1;
	vorbis->op.bytes = header1len;
	vorbis->op.b_o_s = 1;
	vorbis->op.packetno = 0;
	ret = vorbis_synthesis_headerin(&vorbis->vinfo, &vorbis->vc, &vorbis->op);
	if (ret != 0) {
	  if (ret == OV_ENOTVORBIS) {
	    printke("vorbis decoder says its not a vorbis on packet %llu", vorbis->op.packetno);
	  }
	  if (ret == OV_EBADHEADER) {
	    printke("vorbis decoder says bad header on packet %llu", vorbis->op.packetno);
	  }
	  if (ret == OV_EFAULT) {
	    printke("vorbis decoder fault on packet %llu", vorbis->op.packetno);
	  }
	}

	vorbis->op.packet = header2;
	vorbis->op.bytes = header2len;
	vorbis->op.b_o_s = 0;
	vorbis->op.packetno = 1;
	ret = vorbis_synthesis_headerin(&vorbis->vinfo, &vorbis->vc, &vorbis->op);
	if (ret != 0) {
	  if (ret == OV_ENOTVORBIS) {
	    printke("vorbis decoder says its not a vorbis on packet %llu", vorbis->op.packetno);
	  }
	  if (ret == OV_EBADHEADER) {
	    printke("vorbis decoder says bad header on packet %llu", vorbis->op.packetno);
	  }
	  if (ret == OV_EFAULT) {
	    printke("vorbis decoder fault on packet %llu", vorbis->op.packetno);
	  }
	}

	vorbis->op.packet = header3;
	vorbis->op.bytes = header3len;
	vorbis->op.packetno = 2;
	ret = vorbis_synthesis_headerin(&vorbis->vinfo, &vorbis->vc, &vorbis->op);
	if (ret != 0) {
	  if (ret == OV_ENOTVORBIS) {
	    printke("vorbis decoder says its not a vorbis on packet %llu", vorbis->op.packetno);
	  }
	  if (ret == OV_EBADHEADER) {
	    printke("vorbis decoder says bad header on packet %llu", vorbis->op.packetno);
	  }
	  if (ret == OV_EFAULT) {
	    printke("vorbis decoder fault on packet %llu", vorbis->op.packetno);
	  }
	}

	ret = vorbis_synthesis_init(&vorbis->vdsp, &vorbis->vinfo);
	if (ret != 0) {
	  printke("vorbis synthesis init fails!");
  }
	
	vorbis_block_init(&vorbis->vdsp, &vorbis->vblock);

	printk ("Vorbis Info - Version: %d Channels: %d Sample Rate: %ld Bitrate: %ld %ld %ld\n",
			vorbis->vinfo.version, vorbis->vinfo.channels, vorbis->vinfo.rate, vorbis->vinfo.bitrate_upper,
			vorbis->vinfo.bitrate_nominal, vorbis->vinfo.bitrate_lower);

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

int krad_vorbis_decoder_read_audio (krad_vorbis_t *vorbis, int channel, char *buffer, int buffer_length) {

	return krad_ringbuffer_read (vorbis->ringbuf[channel], (char *)buffer, buffer_length );
	
}

void krad_vorbis_decoder_decode (krad_vorbis_t *vorbis, unsigned char *buffer, int bufferlen) {

	int sample_count;
	float **pcm;
	int ret;

	vorbis->op.packet = buffer;
	vorbis->op.bytes = bufferlen;
	vorbis->op.packetno++;

  //if (vorbis->op.packetno == 4) {
  //  vorbis->op.granulepos = 576;
  //}
  
  //if (vorbis->op.packetno == 5) {
  //  vorbis->op.granulepos = 1600;
  //}
  
  //if (vorbis->op.packetno > 5) {
  //  vorbis->op.granulepos = 1600 + ((vorbis->op.packetno - 5) * 1024);
  //} 

	ret = vorbis_synthesis (&vorbis->vblock, &vorbis->op);
	
	if (ret != 0) {
	  if (ret == OV_ENOTAUDIO) {
	    printke("vorbis decoder not audio packet %llu - %d", vorbis->op.packetno, bufferlen);
	  }
	  if (ret == OV_EBADPACKET) {
	    printke("vorbis decoder bad packet %llu - %d", vorbis->op.packetno, bufferlen);
	  }	  
	}
	
	ret = vorbis_synthesis_blockin(&vorbis->vdsp, &vorbis->vblock);
	
	if (ret != 0) {
	  if (ret == OV_EINVAL) {
	    printke("vorbis decoder not ready for blockin!");
	  }
  }	
	
	sample_count = vorbis_synthesis_pcmout(&vorbis->vdsp, &pcm);
	
	if (sample_count) {
		//printf("Vorbis decoded %d samples\n", sample_count);
		while((krad_ringbuffer_write_space(vorbis->ringbuf[0]) < sample_count * 4) || (krad_ringbuffer_write_space(vorbis->ringbuf[1]) < sample_count * 4)) {
			usleep (15000);
		}
	
		krad_ringbuffer_write (vorbis->ringbuf[0], (char *)pcm[0], sample_count * 4 );
		if (vorbis->channels == 1) {
			krad_ringbuffer_write (vorbis->ringbuf[1], (char *)pcm[0], sample_count * 4 );
		}
		if (vorbis->channels == 2) {
			krad_ringbuffer_write (vorbis->ringbuf[1], (char *)pcm[1], sample_count * 4 );
		}
		ret = vorbis_synthesis_read(&vorbis->vdsp, sample_count);
	  if (ret != 0) {
	    if (ret == OV_EINVAL) {
	      printke("vorbis decoder synth read more than in buffer!");
	    }
    }		
		
	}

}

