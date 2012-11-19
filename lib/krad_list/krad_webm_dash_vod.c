#include "krad_webm_dash_vod.h"


krad_webm_dash_vod_t *krad_webm_dash_vod_create (int width, int height, int sample_rate, float duration, char *lang) {

	krad_webm_dash_vod_t *krad_webm_dash_vod = calloc(1, sizeof(krad_webm_dash_vod_t));
  char string[16];

  memset (string, 0, sizeof(string));

  krad_webm_dash_vod->duration = duration;  
  if (lang != NULL) {
    strncpy (krad_webm_dash_vod->lang, lang, (sizeof(krad_webm_dash_vod->lang) - 1));
  }
  krad_webm_dash_vod->width = width;
  krad_webm_dash_vod->height = height;
  krad_webm_dash_vod->sample_rate = sample_rate;

	krad_webm_dash_vod->doc = xmlNewDoc (BAD_CAST "1.0");

  krad_webm_dash_vod->root_node = xmlNewNode (NULL, BAD_CAST "MPD");
  xmlDocSetRootElement (krad_webm_dash_vod->doc, krad_webm_dash_vod->root_node);

  xmlNewProp (krad_webm_dash_vod->root_node, BAD_CAST "xmlns:xsi", BAD_CAST "http://www.w3.org/2001/XMLSchema-instance");
  xmlNewProp (krad_webm_dash_vod->root_node, BAD_CAST "xmlns", BAD_CAST "urn:mpeg:mpd:schema:mpd:2011");
  xmlNewProp (krad_webm_dash_vod->root_node, BAD_CAST "xsi:schemaLocation", BAD_CAST "urn:mpeg:dash:schema:mpd:2011 http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-DASH_schema_files/DASH-MPD.xsd");
  xmlNewProp (krad_webm_dash_vod->root_node, BAD_CAST "type", BAD_CAST "static");
  snprintf (string, (sizeof(string) - 1), "PT%0.2fS", krad_webm_dash_vod->duration);
  xmlNewProp (krad_webm_dash_vod->root_node, BAD_CAST "mediaPresentationDuration", BAD_CAST string);
  xmlNewProp (krad_webm_dash_vod->root_node, BAD_CAST "minBufferTime", BAD_CAST "PT1S");
  xmlNewProp (krad_webm_dash_vod->root_node, BAD_CAST "profiles", BAD_CAST "urn:webm:dash:profile:webm-on-demand:2012");        

  krad_webm_dash_vod->period_node = xmlNewTextChild (krad_webm_dash_vod->root_node, NULL, BAD_CAST "Period", NULL);
  xmlNewProp (krad_webm_dash_vod->period_node, BAD_CAST "id", BAD_CAST "0");
  xmlNewProp (krad_webm_dash_vod->period_node, BAD_CAST "start", BAD_CAST "PT0S");
  snprintf (string, (sizeof(string) - 1), "PT%0.2fS", krad_webm_dash_vod->duration);  
  xmlNewProp (krad_webm_dash_vod->period_node, BAD_CAST "duration", BAD_CAST string);

  krad_webm_dash_vod->video_adaptation_set_node = xmlNewTextChild (krad_webm_dash_vod->period_node, NULL, BAD_CAST "AdaptationSet", NULL);
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "id", BAD_CAST "0");
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "mimeType", BAD_CAST "video/webm");
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "codecs", BAD_CAST "vp8");
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "lang", BAD_CAST krad_webm_dash_vod->lang);
  snprintf (string, (sizeof(string) - 1), "%d", krad_webm_dash_vod->width);  
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "width", BAD_CAST string);
  snprintf (string, (sizeof(string) - 1), "%d", krad_webm_dash_vod->height);  
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "height", BAD_CAST string);
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "subsegmentAlignment", BAD_CAST "true");
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "subsegmentStartsWithSAP", BAD_CAST "1");
  xmlNewProp (krad_webm_dash_vod->video_adaptation_set_node, BAD_CAST "bitstreamSwitching", BAD_CAST "true");  

  krad_webm_dash_vod->audio_adaptation_set_node = xmlNewTextChild (krad_webm_dash_vod->period_node, NULL, BAD_CAST "AdaptationSet", NULL);
  xmlNewProp (krad_webm_dash_vod->audio_adaptation_set_node, BAD_CAST "id", BAD_CAST "1");
  xmlNewProp (krad_webm_dash_vod->audio_adaptation_set_node, BAD_CAST "mimeType", BAD_CAST "audio/webm");
  xmlNewProp (krad_webm_dash_vod->audio_adaptation_set_node, BAD_CAST "codecs", BAD_CAST "vorbis");
  xmlNewProp (krad_webm_dash_vod->audio_adaptation_set_node, BAD_CAST "lang", BAD_CAST krad_webm_dash_vod->lang);
  snprintf (string, (sizeof(string) - 1), "%d", krad_webm_dash_vod->sample_rate);    
  xmlNewProp (krad_webm_dash_vod->audio_adaptation_set_node, BAD_CAST "audioSamplingRate", BAD_CAST string);    
  xmlNewProp (krad_webm_dash_vod->audio_adaptation_set_node, BAD_CAST "subsegmentStartsWithSAP", BAD_CAST "1");  

	return krad_webm_dash_vod;

}

void krad_webm_dash_vod_handle_audio_id (krad_webm_dash_vod_t *krad_webm_dash_vod) {

  char audio_id_str[8];

  if (krad_webm_dash_vod->audio_node != NULL) {
    sprintf (audio_id_str, "%d", krad_webm_dash_vod->video_id + 1);
    if (krad_webm_dash_vod->audio_id_prop == NULL) {
      krad_webm_dash_vod->audio_id_prop = xmlNewProp (krad_webm_dash_vod->audio_node, BAD_CAST "id", BAD_CAST audio_id_str);
    } else {
      krad_webm_dash_vod->audio_id_prop = xmlSetProp (krad_webm_dash_vod->audio_node, BAD_CAST "id", BAD_CAST audio_id_str);    
    }
  }
}

void krad_webm_dash_vod_add_video (krad_webm_dash_vod_t *krad_webm_dash_vod, char *url, int bandwidth,
                                   uint64_t init_range_start, uint64_t init_range_end,
                                   uint64_t index_range_start, uint64_t index_range_end) {

	xmlNodePtr video_node;
	xmlNodePtr video_segment_node;
	xmlNodePtr video_segment_init_range_node;
  char string[128];

	krad_webm_dash_vod->video_id++;
  krad_webm_dash_vod_handle_audio_id (krad_webm_dash_vod);
	
  video_node = xmlNewTextChild (krad_webm_dash_vod->video_adaptation_set_node, NULL, BAD_CAST "Representation", NULL);
  
  sprintf (string, "%d", krad_webm_dash_vod->video_id);
  xmlNewProp (video_node, BAD_CAST "id", BAD_CAST string);

  sprintf (string, "%d", bandwidth);
  xmlNewProp (video_node, BAD_CAST "bandwidth", BAD_CAST string);
  
  xmlNewTextChild (video_node, NULL, BAD_CAST "BaseURL", BAD_CAST url);
  
  video_segment_node = xmlNewTextChild (video_node, NULL, BAD_CAST "SegmentBase", NULL);
  sprintf (string, "%"PRIu64"-%"PRIu64"", index_range_start, index_range_end);
  xmlNewProp (video_segment_node, BAD_CAST "indexRange", BAD_CAST string);

  if (init_range_end > 0) {
    video_segment_init_range_node = xmlNewTextChild (video_segment_node, NULL, BAD_CAST "Initialization", NULL);
    sprintf (string, "%"PRIu64"-%"PRIu64"", init_range_start, init_range_end);
    xmlNewProp (video_segment_init_range_node, BAD_CAST "range", BAD_CAST string);
  }
}

void krad_webm_dash_vod_add_audio (krad_webm_dash_vod_t *krad_webm_dash_vod, char *url, int bandwidth,
                                   uint64_t init_range_start, uint64_t init_range_end,
                                   uint64_t index_range_start, uint64_t index_range_end) {

	xmlNodePtr audio_segment_node;
	xmlNodePtr audio_segment_init_range_node;
  char string[128];

  krad_webm_dash_vod->audio_node = xmlNewTextChild (krad_webm_dash_vod->audio_adaptation_set_node, NULL, BAD_CAST "Representation", NULL);
  
  krad_webm_dash_vod_handle_audio_id (krad_webm_dash_vod);  
  
  sprintf (string, "%d", bandwidth);
  xmlNewProp (krad_webm_dash_vod->audio_node, BAD_CAST "bandwidth", BAD_CAST string);
  
  xmlNewTextChild (krad_webm_dash_vod->audio_node, NULL, BAD_CAST "BaseURL", BAD_CAST url);
  
  audio_segment_node = xmlNewTextChild (krad_webm_dash_vod->audio_node, NULL, BAD_CAST "SegmentBase", NULL);
  sprintf (string, "%"PRIu64"-%"PRIu64"", index_range_start, index_range_end);  
  xmlNewProp (audio_segment_node, BAD_CAST "indexRange", BAD_CAST string);

  if (init_range_end > 0) {
    audio_segment_init_range_node = xmlNewTextChild (audio_segment_node, NULL, BAD_CAST "Initialization", NULL);
    sprintf (string, "%"PRIu64"-%"PRIu64"", init_range_start, init_range_end);  
    xmlNewProp (audio_segment_init_range_node, BAD_CAST "range", BAD_CAST string);
  }
}


void krad_webm_dash_vod_save_file (krad_webm_dash_vod_t *krad_webm_dash_vod, char *filename) {
  xmlSaveFormatFileEnc (filename, krad_webm_dash_vod->doc, "UTF-8", 1);
}


void krad_webm_dash_vod_destroy (krad_webm_dash_vod_t *krad_webm_dash_vod) {
	xmlFreeDoc (krad_webm_dash_vod->doc);
	free (krad_webm_dash_vod);
}

