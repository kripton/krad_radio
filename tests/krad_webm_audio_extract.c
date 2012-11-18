#include "krad_webm_dash_vod.h"
#include "krad_radio_version.h"
#include "krad_transmitter.h"
#include "krad_system.h"
#include "krad_codec_header.h"
#include "krad_ebml.h"

void vod_test () {

  krad_webm_dash_vod_t *krad_webm_dash_vod;
  char *filename;
  int width;
  int height;
  int sample_rate;
  float duration;
  char *lang;
  
  char *url;
  int bandwidth;
  uint64_t range_start;
  uint64_t range_end;
  uint64_t index_range_start;
  uint64_t index_range_end;
	
  filename = "/home/oneman/kode/webm_dash_vod_test.mpd";
  width = 720;
  height = 306;
  sample_rate = 44100;
  duration = 888.05f;
  lang = "eng";

  url = "mevq_logo_720x306_0500k_int-150-150.webm";
  bandwidth = 6666;
  index_range_start = 166960740;
  index_range_end = 166963291;
  range_start = 0;
	range_end = 229;

  
  krad_webm_dash_vod = krad_webm_dash_vod_create (width, height, sample_rate, duration, lang);

  krad_webm_dash_vod_add_audio (krad_webm_dash_vod, url, bandwidth, range_start,
                                range_end, index_range_start, index_range_end);

  krad_webm_dash_vod_add_video (krad_webm_dash_vod, url, bandwidth, range_start,
                                range_end, index_range_start, index_range_end);

  krad_webm_dash_vod_add_video (krad_webm_dash_vod, url, bandwidth, range_start,
                                range_end, index_range_start, index_range_end);

  krad_webm_dash_vod_add_video (krad_webm_dash_vod, url, bandwidth, range_start,
                                range_end, index_range_start, index_range_end);

  krad_webm_dash_vod_add_video (krad_webm_dash_vod, url, bandwidth, range_start,
                                range_end, index_range_start, index_range_end);


  krad_webm_dash_vod_save_file (krad_webm_dash_vod, filename);

  krad_webm_dash_vod_destroy (krad_webm_dash_vod);

}

void audio_extract ( char *infilename, char *outfilename ) {

  krad_ebml_t *infile;
  krad_ebml_t *outfile;
  int bytes;
  int outtrack;
  int track;
  uint64_t timecode;
  uint64_t last_timecode;  
  uint64_t total_frames;
  int frames;
  unsigned char *buffer;
  
  buffer = malloc (1000000);
  
  timecode = 0;
  last_timecode = 0;
  bytes = 1;

  infile = krad_ebml_open_file (infilename, KRAD_EBML_IO_READONLY);  
  outfile = krad_ebml_open_file (outfilename, KRAD_EBML_IO_WRITEONLY);

  krad_ebml_header (outfile, "webm", APPVERSION);

  krad_ebml_bump_tracknumber (outfile);

  //printf("vorbis header size is %d\n", infile->tracks[2].codec_data_size);
  
  outtrack = krad_ebml_add_audio_track (outfile, VORBIS, infile->tracks[2].sample_rate, infile->tracks[2].channels, 
										  infile->tracks[2].codec_data, infile->tracks[2].codec_data_size);

  while (bytes > 0) {
    bytes = krad_ebml_read_packet (infile, &track, &timecode, buffer);
  
    if ((bytes > 0) && (track == 2)) {

      total_frames = round ((timecode * infile->tracks[2].sample_rate) / 1000);            
      frames = total_frames - round ((last_timecode * infile->tracks[2].sample_rate) / 1000);

      //printf("frames is %d\n", frames);
      //printf("total_frames is %d\n", total_frames);      
      krad_ebml_add_audio (outfile, outtrack, buffer, bytes, frames);
      last_timecode = timecode;
    }
  
  }


  krad_ebml_destroy (infile);
  krad_ebml_destroy (outfile);
  
  free (buffer);
}

int main (int argc, char *argv[]) {

  vod_test ();
  
  char *infilename = "/home/oneman/Videos/kradtoday_test_vpx.webm";
  char *outfilename = "/home/oneman/Videos/kradtoday_test_vpx_audio.webm";

  audio_extract (infilename, outfilename);

	return 0;

}
