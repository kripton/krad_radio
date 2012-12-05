#include "krad_radio.h"

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

  char *indirname = "/media/oneman/26cb0171-8636-45ec-a51f-aaed14951c7b/tears/media.xiph.org/tearsofsteel/tearsofsteel-1080-png";
  char *inflacname = "/media/oneman/26cb0171-8636-45ec-a51f-aaed14951c7b/tearstearsofsteel-stereo.flac";
  char *outfilename = "/home/oneman/Videos/tears_of_steel_kvhs.mkv";


  convert_to_kvhs (indirname, 24, inflacname, outfilename);


	return 0;

}

