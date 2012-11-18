#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "krad_system.h"

typedef struct {

  xmlDocPtr doc;
  xmlNodePtr root_node;
	xmlNodePtr period_node;

	xmlNodePtr video_adaptation_set_node;
	xmlNodePtr audio_adaptation_set_node;
	xmlNodePtr audio_node;	
		
  int video_id;
	xmlAttrPtr audio_id_prop;  
		
  int width;
  int height;
  int sample_rate;
  float duration;
  char lang[16];

} krad_webm_dash_vod_t;


krad_webm_dash_vod_t *krad_webm_dash_vod_create (int width, int height, int sample_rate, float duration, char *lang);

void krad_webm_dash_vod_add_video (krad_webm_dash_vod_t *krad_webm_dash_vod, char *url, int bandwidth,
                                   uint64_t init_range_start, uint64_t init_range_end,
                                   uint64_t index_range_start, uint64_t index_range_end);

void krad_webm_dash_vod_add_audio (krad_webm_dash_vod_t *krad_webm_dash_vod, char *url, int bandwidth,
                                   uint64_t init_range_start, uint64_t init_range_end,
                                   uint64_t index_range_start, uint64_t index_range_end);

void krad_webm_dash_vod_save_file (krad_webm_dash_vod_t *krad_webm_dash_vod, char *filename);
void krad_webm_dash_vod_destroy (krad_webm_dash_vod_t *krad_webm_dash_vod);



