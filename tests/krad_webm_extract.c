#include "krad_webm_dash_vod.h"
#include "krad_radio_version.h"
#include "krad_transmitter.h"
#include "krad_system.h"
#include "krad_codec_header.h"
#include "krad_ebml.h"

void dash_vod_create (char *filename, char *videofilename, char *audiofilename) {

  krad_webm_dash_vod_t *krad_webm_dash_vod;
  krad_ebml_t *videofile;
  krad_ebml_t *audiofile;

  char *lang;
  int bandwidth;

  lang = "eng";
  bandwidth = 12420109;


  audiofile = krad_ebml_open_file (audiofilename, KRAD_EBML_IO_READONLY);
  videofile = krad_ebml_open_file (videofilename, KRAD_EBML_IO_READONLY);
  
  
  int track;
  uint64_t timecode;
  unsigned char *buffer;
  
  buffer = malloc (1000000);  
  krad_ebml_read_packet (audiofile, &track, &timecode, buffer);
  krad_ebml_read_packet (videofile, &track, &timecode, buffer);
  free (buffer);


  krad_webm_dash_vod = krad_webm_dash_vod_create (videofile->tracks[1].width, videofile->tracks[1].height,
                                                  audiofile->tracks[1].sample_rate, audiofile->segment_duration / 1000.0, lang);

  krad_webm_dash_vod_add_audio (krad_webm_dash_vod, strrchr(audiofilename, '/') + 1, bandwidth, 0,
                                audiofile->first_cluster_pos, audiofile->cues_file_pos, audiofile->cues_file_end_range);

  krad_webm_dash_vod_add_video (krad_webm_dash_vod, strrchr(videofilename, '/') + 1, bandwidth, 0,
                                videofile->first_cluster_pos, videofile->cues_file_pos, videofile->cues_file_end_range);

  krad_webm_dash_vod_save_file (krad_webm_dash_vod, filename);

  krad_webm_dash_vod_destroy (krad_webm_dash_vod);

  krad_ebml_destroy (videofile);
  krad_ebml_destroy (audiofile);

}

void webm_extract ( char *infilename, char *videooutfilename, char *audiooutfilename ) {

  krad_ebml_t *infile;
  krad_ebml_t *videooutfile;
  krad_ebml_t *audiooutfile;  
  int bytes;
  int track;
  uint64_t timecode;
  uint64_t last_timecode;  
  uint64_t total_frames;
  int frames;
  int video_frames;
  int keyframe;
  unsigned char *buffer;
  
  buffer = malloc (1000000);
  
  timecode = 0;
  last_timecode = 0;
  bytes = 1;
  video_frames = 0;
  keyframe = 0;

  infile = krad_ebml_open_file (infilename, KRAD_EBML_IO_READONLY);  
  videooutfile = krad_ebml_open_file (videooutfilename, KRAD_EBML_IO_WRITEONLY);
  audiooutfile = krad_ebml_open_file (audiooutfilename, KRAD_EBML_IO_WRITEONLY);

  krad_ebml_header (videooutfile, "webm", APPVERSION);
  krad_ebml_header (audiooutfile, "webm", APPVERSION);
  
  //krad_ebml_bump_tracknumber (audiooutfile);

  krad_ebml_add_video_track (videooutfile, VP8, 30000, 1001, 1280, 720);
  
  krad_ebml_add_audio_track (audiooutfile, VORBIS, infile->tracks[2].sample_rate, infile->tracks[2].channels, 
                             infile->tracks[2].codec_data, infile->tracks[2].codec_data_size);

  while (bytes > 0) {

    bytes = krad_ebml_read_packet (infile, &track, &timecode, buffer);
  
    if ((bytes > 0) && (track == 1)) {
      if ((video_frames % 150) == 0) {
        keyframe = 1;
      } else {
        keyframe = 0;
      } 
      video_frames++;
             
      krad_ebml_add_video (videooutfile, track, buffer, bytes, keyframe);

    }
  
    if ((bytes > 0) && (track == 2)) {
      total_frames = round ((timecode * infile->tracks[2].sample_rate) / 1000);            
      frames = total_frames - round ((last_timecode * infile->tracks[2].sample_rate) / 1000);
      krad_ebml_add_audio (audiooutfile, 1, buffer, bytes, frames);
      last_timecode = timecode;
    }
  
  }


  krad_ebml_destroy (infile);
  krad_ebml_destroy (videooutfile);
  krad_ebml_destroy (audiooutfile);
  
  free (buffer);
}

int main (int argc, char *argv[]) {

  char *infilename = "/home/oneman/Videos/kradtoday_faded2.webm";
  char *videooutfilename = "/home/oneman/Videos/webmdash/kradtoday_faded2_video.webm";
  char *audiooutfilename = "/home/oneman/Videos/webmdash/kradtoday_faded2_audio.webm";  
  char *vodfile = "/home/oneman/Videos/webmdash/webm_dash_vod_test.mpd";  


  webm_extract (infilename, videooutfilename, audiooutfilename);

  dash_vod_create (vodfile, videooutfilename, audiooutfilename);


	return 0;

}
