#include "krad_radio.h"

void convert_kvhs_to ( char *infilename, char *outfilename ) {

  krad_ebml_t *videoinfile;
  krad_ebml_t *videooutfile;
  int bytes;
  int packet_size;
  void *video_packet;  
  int keyframe;
  int frames;
  int track;
  uint64_t timecode;
  uint64_t last_timecode;  
  uint64_t total_audio_frames;
  int audio_frames;
  krad_vhs_t *krad_vhs;
  krad_vpx_encoder_t *krad_vpx;
	unsigned char *planes[3];
	int strides[3];  
	struct SwsContext *converter;
  unsigned char *buffer;
  unsigned char *pixels;
	unsigned char *srcpixels[4];
	int input_rgb_stride_arr[4];
  int color_depth;	
	
  //krad_y4m_t *krad_y4m;
	krad_theora_encoder_t *krad_theora_encoder;  
  	
	
	//krad_codec_t audio_codec;
	krad_codec_t video_codec;
	
  buffer = malloc (8000000);
  pixels = malloc (8000000);
  
  bytes = 1;
  frames = 0;

  timecode = 0;
  last_timecode = 0;
  audio_frames = 0;
  keyframe = 0;
  total_audio_frames = 0;
  track = 0;
  
  video_codec = VP8;
  video_codec = THEORA;

  krad_vhs = krad_vhs_create_decoder ();
  videoinfile = krad_ebml_open_file (infilename, KRAD_EBML_IO_READONLY); 
  videooutfile = krad_ebml_open_file (outfilename, KRAD_EBML_IO_WRITEONLY);


  if (videoinfile->tracks[2].sample_rate) {
    krad_ebml_add_audio_track (videooutfile, FLAC, videoinfile->tracks[2].sample_rate, videoinfile->tracks[2].channels, 
                               videoinfile->tracks[2].codec_data, videoinfile->tracks[2].codec_data_size);
  }

		if (video_codec == VP8) {
		
		color_depth = PIX_FMT_YUV420P;		
		
  krad_ebml_header (videooutfile, "webm", APPVERSION);		
		
  krad_ebml_add_video_track (videooutfile, VP8, 24 * 1000, 1000, 1920, 800);  
  krad_vpx = krad_vpx_encoder_create (1920,
                                 800,
                                 24000,
                                 1000,                                 
                                 6000);		
		
			planes[0] = krad_vpx->image->planes[0];
			planes[1] = krad_vpx->image->planes[1];
			planes[2] = krad_vpx->image->planes[2];
			strides[0] = krad_vpx->image->stride[0];
			strides[1] = krad_vpx->image->stride[1];
			strides[2] = krad_vpx->image->stride[2];
		}
		
		if (video_codec == THEORA) {

		color_depth = PIX_FMT_YUV444P;
		
  krad_ebml_header (videooutfile, "matroska", APPVERSION);
  krad_theora_encoder = krad_theora_encoder_create (1920,
                                 800,
                                 24000,
                                 1000,
																	 color_depth,
																	 34);		

  
			krad_ebml_add_video_track_with_private_data (videooutfile, 
														      video_codec,
														   	  24000,
															  1000,
															  1920,
															  800,
															  krad_theora_encoder->krad_codec_header.header_combined,
															  krad_theora_encoder->krad_codec_header.header_combined_size);
  
			planes[0] = krad_theora_encoder->ycbcr[0].data;
			planes[1] = krad_theora_encoder->ycbcr[1].data;
			planes[2] = krad_theora_encoder->ycbcr[2].data;
			strides[0] = krad_theora_encoder->ycbcr[0].stride;
			strides[1] = krad_theora_encoder->ycbcr[1].stride;
			strides[2] = krad_theora_encoder->ycbcr[2].stride;	
		}
		/*
		if (video_codec == Y4M) {
			planes[0] = krad_y4m->planes[0];
			planes[1] = krad_y4m->planes[1];
			planes[2] = krad_y4m->planes[2];
			strides[0] = krad_y4m->strides[0];
			strides[1] = krad_y4m->strides[1];
			strides[2] = krad_y4m->strides[2];
		}
    */

  srcpixels[0] = pixels;


converter =
				sws_getContext ( 1920,
								 800,
								 PIX_FMT_RGB32,
								 1920,
								 800,
								 color_depth, 
								 SWS_BICUBIC,
								 NULL, NULL, NULL);


  input_rgb_stride_arr[1] = 0;
  input_rgb_stride_arr[2] = 0;
  input_rgb_stride_arr[3] = 0;
  input_rgb_stride_arr[0] = 4*1920;

  while (bytes > 0) {

    bytes = krad_ebml_read_packet (videoinfile, &track, &timecode, buffer);
  
    if ((bytes > 0) && (track == 1)) {
    
      printf("Processing frame %d VHS: %dKB ", frames, bytes / 1000);

      krad_vhs_decode (krad_vhs, buffer, pixels);

  sws_scale (converter, (const uint8_t * const*)srcpixels,
             input_rgb_stride_arr, 0, 800, planes, strides);

		if (video_codec == VP8) {
      packet_size = krad_vpx_encoder_write (krad_vpx,
                                            (unsigned char **)&video_packet,
                                            &keyframe);
      if (packet_size) {
      
        printf("VP8: %dKB \n", packet_size / 1000);
      
        krad_ebml_add_video (videooutfile, track, video_packet, packet_size, keyframe);
      }
		
		}

		if (video_codec == THEORA) {

				packet_size = krad_theora_encoder_write (krad_theora_encoder,
									   (unsigned char **)&video_packet,
									   					 &keyframe);
      //if (packet_size) {
      
        printf("Theora: %dKB \n", packet_size / 1000);
      
        krad_ebml_add_video (videooutfile, track, video_packet, packet_size, keyframe);
      //}		
		
		}

      
      frames++;


    }
    
    if ((bytes > 0) && (track == 2)) {
      total_audio_frames = round ((timecode * videoinfile->tracks[2].sample_rate) / 1000);            
      audio_frames = total_audio_frames - round ((last_timecode * videoinfile->tracks[2].sample_rate) / 1000);
      krad_ebml_add_audio (videooutfile, track, buffer, bytes, audio_frames);
      last_timecode = timecode;
    }    

  }

  sws_freeContext ( converter );
  krad_vhs_destroy (krad_vhs);
  krad_vpx_encoder_destroy (krad_vpx);  
  krad_ebml_destroy (videooutfile);
  free (buffer);
  free (pixels);  

}

void convert_to_kvhs ( char *indirname, int fps, char *inflacname, char *outfilename ) {

  krad_ebml_t *videooutfile;
  int bytes;
  int frames;
  krad_sprite_t *frame;
  krad_vhs_t *krad_vhs;
  
  char filename[1024];
  
  bytes = 1;
  frames = 0;

  krad_vhs = krad_vhs_create_encoder (1920, 800);
  videooutfile = krad_ebml_open_file (outfilename, KRAD_EBML_IO_WRITEONLY);
  krad_ebml_header (videooutfile, "mkv", APPVERSION);
  krad_ebml_add_video_track (videooutfile, KVHS, fps * 1000, 1000, 1920, 800);

  while ((bytes > 0) && (frames < 17620)) {

    sprintf(filename, "%s/graded_edit_final_%05d.png", indirname, frames + 1);
    printf("Processing %d\n", frames);

    frame = krad_sprite_create_from_file (filename);
    bytes = krad_vhs_encode (krad_vhs, (unsigned char *)cairo_image_surface_get_data (frame->sprite));
    krad_ebml_add_video (videooutfile, 1, krad_vhs->enc_buffer, bytes, (!(frames % fps)));
    frames++;
    krad_sprite_destroy (frame);

  }

  krad_vhs_destroy (krad_vhs);
  krad_ebml_destroy (videooutfile);
  

}

int main (int argc, char *argv[]) {

  /*
  char *indirname = "/media/oneman/26cb0171-8636-45ec-a51f-aaed14951c7b/tears/media.xiph.org/tearsofsteel/tearsofsteel-1080-png";
  char *inflacname = "/media/oneman/26cb0171-8636-45ec-a51f-aaed14951c7b/tearstearsofsteel-stereo.flac";
  char *outfilename = "/home/oneman/Videos/tears_of_steel_kvhs.mkv";

  convert_to_kvhs (indirname, 24, inflacname, outfilename);
  */


  char *infilename = "/media/oneman/23dc83e5-1652-4d64-b1f9-d92814e1c4b6/tears_of_steel_kvhs.mkv";
  char *webmfilename = "/home/oneman/Videos/tears_of_steel_theora_from_kvhs.mkv";

//  char *infilename = "/home/oneman/kode/demos/recordings/hello_krad_world_kvhs.mkv";
//  char *webmfilename = "/home/oneman/kode/demos/recordings/hello_krad_world_from_kvhs.webm";

  convert_kvhs_to ( infilename, webmfilename );

  return 0;

}

