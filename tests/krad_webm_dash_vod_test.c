#include "krad_webm_dash_vod.h"


int main (int argc, char *argv[]) {

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

	return 0;

}
