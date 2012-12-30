#include "kr_client.h"

void krad_radio_command_help () {

  printf ("krad_radio STATION_SYSNAME COMMAND OPTIONS...");
  printf ("\n\n");
  printf ("Commands:\n");

  printf ("launch ls destroy uptime info tag tags stag remoteon remoteoff webon weboff oscon oscoff setrate getrate mix");
  printf ("\n");
  printf ("setdir lm ll lc tone input output rmport plug unplug map mixmap xmms2 noxmms2 listen_on listen_off link");
  printf ("\n");
  printf ("transmitter_on transmitter_off closedisplay display lstext rmtext addtest lssprites addsprite rmsprite");
  printf ("\n");
  printf ("setsprite comp setres setfps snap jsnap setport update play receive record capture");
  printf ("\n");
  printf ("addfx rmfx setfx");
  printf ("\n");
}

int main (int argc, char *argv[]) {

  kr_client_t *client;
  char *sysname;
  int ret;
  int val;

  sysname = NULL;
  client = NULL;
  ret = 0;
  val = 0;

  if ((argc == 1) || (argc == 2)) {
    if (argc == 2) {
      if (((strlen(argv[1]) == 2) && (strncmp(argv[1], "vn", 2) == 0)) ||
          ((strlen(argv[1]) == 3) && (strncmp(argv[1], "-vn", 3) == 0)) ||
          ((strlen(argv[1]) == 4) && (strncmp(argv[1], "--vn", 4) == 0))) {
          printf("%d\n", VERSION_NUMBER);
          return 0;
      }

      if (((strlen(argv[1]) == 2) && (strncmp(argv[1], "gv", 2) == 0)) ||
          ((strlen(argv[1]) == 2) && (strncmp(argv[1], "vg", 2) == 0)) ||
          ((strlen(argv[1]) == 3) && (strncmp(argv[1], "-vg", 3) == 0)) ||
          ((strlen(argv[1]) == 3) && (strncmp(argv[1], "git", 3) == 0)) ||
          ((strlen(argv[1]) == 4) && (strncmp(argv[1], "--vg", 4) == 0)) ||
          ((strlen(argv[1]) == 6) && (strncmp(argv[1], "gitver", 6) == 0)) ||
          ((strlen(argv[1]) == 10) && (strncmp(argv[1], "gitversion", 10) == 0))) {
          printf("%s\n", KRAD_GIT_VERSION);
          return 0;
      }

      if (((strlen(argv[1]) == 1) && (strncmp(argv[1], "v", 1) == 0)) ||
          ((strlen(argv[1]) == 2) && (strncmp(argv[1], "-v", 2) == 0)) ||
          ((strlen(argv[1]) >= 3) && (strncmp(argv[1], "--v", 3) == 0))) {

          printf (KRAD_VERSION_STRING "\n");
          return 0;
      }

      if ((strlen(argv[1]) == 2) && (strncmp(argv[1], "ls", 2) == 0)) {
        printf ("Running Stations: \n\n%s\n", krad_radio_running_stations ());
        return 0;
      }
    }

    krad_radio_command_help ();
    return 0;
  }

  if (!krad_valid_host_and_port (argv[1])) {
    if (!krad_valid_sysname(argv[1])) {
      fprintf (stderr, "Invalid station sysname!\n");
      return 1;
    } else {
      sysname = argv[1];
    }
  }

  if ((strncmp(argv[2], "launch", 6) == 0) || (strncmp(argv[2], "load", 4) == 0)) {
    krad_radio_launch (sysname);
    return 0;
  }

  if ((strncmp(argv[2], "destroy", 7) == 0) || (strncmp(argv[2], "kill", 4) == 0)) {
    ret = krad_radio_destroy (sysname);
    if (ret == 0) {
      printf ("Daemon shutdown\n");
    }
    if (ret == 1) {
      printf ("Daemon was killed\n");
    }
    if (ret == -1) {
      printf ("Daemon was not running\n");
    }
    return 0;
  }

  client = kr_connect (sysname);

  if (client == NULL) {
    fprintf (stderr, "Could not connect to %s krad radio daemon\n", sysname);
    return 1;
  }

  /* Krad Radio Commands */

  if ((strncmp(argv[2], "ls", 2) == 0) && (strlen(argv[2]) == 2)) {
    if (argc == 3) {
      kr_transponder_list (client);
      kr_client_print_response (client);

      kr_compositor_port_list (client);
      kr_client_print_response (client);

      kr_mixer_portgroups_list (client);
      kr_client_print_response (client);          
    }
  }      

  if (((strncmp(argv[2], "jacked", 6) == 0) || (strncmp(argv[2], "jackup", 6) == 0)) && (strlen(argv[2]) == 6)) {
    if (argc == 3) {
      kr_mixer_jack_running (client);
      kr_client_print_response (client);
    }
  }

  if ((strncmp(argv[2], "lsd", 3) == 0) && (strlen(argv[2]) == 3)) {
    if (argc == 3) {
      kr_transponder_decklink_list (client);
      kr_client_print_response (client);
    }
  }

  if ((strncmp(argv[2], "lsv", 3) == 0) && (strlen(argv[2]) == 3)) {
    if (argc == 3) {
      kr_transponder_v4l2_list (client);
      kr_client_print_response (client);
    }
  }


  if (strncmp(argv[2], "uptime", 6) == 0) {
    kr_uptime (client);
    kr_client_print_response (client);
  }

  if (strncmp(argv[2], "info", 4) == 0) {
    kr_system_info (client);
    kr_client_print_response (client);
  }
  
  if (strncmp(argv[2], "optionals", 9) == 0) {
    kr_optionals_info (client);
    kr_client_print_response (client);
  }

  if (strncmp(argv[2], "cpu", 4) == 0) {
    kr_system_cpu_usage (client);
    kr_client_print_response (client);
  }      

  if (strncmp(argv[2], "tags", 4) == 0) {

    if (argc == 3) {
      kr_tags (client, NULL);    
      kr_client_print_response (client);
    }
    if (argc == 4) {
      kr_tags (client, argv[3]);    
      kr_client_print_response (client);
    }          

  } else {
    if (strncmp(argv[2], "tag", 3) == 0) {
      if (argc == 4) {
        kr_tag (client, NULL, argv[3]);
        kr_client_print_response (client);
      }
      if (argc == 5) {
        kr_tag (client, argv[3], argv[4]);
        kr_client_print_response (client);            
      }    
    }
  }

  if (strncmp(argv[2], "stag", 4) == 0) {
    if (argc == 5) {
      kr_set_tag (client, NULL, argv[3], argv[4]);
    }
    if (argc == 6) {
      kr_set_tag (client, argv[3], argv[4], argv[5]);
    }
  }

  if (strncmp(argv[2], "remoteon", 8) == 0) {
    if (argc == 4) {
      kr_remote_enable (client, atoi(argv[3]));
      kr_client_print_response (client);
    }
  }      

  if (strncmp(argv[2], "remoteoff", 9) == 0) {
    if (argc == 3) {
      kr_remote_disable (client);
    }
  }

  if (strncmp(argv[2], "webon", 5) == 0) {
    if (argc == 5) {
      kr_web_enable (client, atoi(argv[3]), atoi(argv[4]), "", "", "");
    }
    if (argc == 6) {
      kr_web_enable (client, atoi(argv[3]), atoi(argv[4]), argv[5], "", "");
    }
    if (argc == 7) {
      kr_web_enable (client, atoi(argv[3]), atoi(argv[4]), argv[5], argv[6], "");
    }
    if (argc == 8) {
      kr_web_enable (client, atoi(argv[3]), atoi(argv[4]), argv[5], argv[6], argv[7]);
    }
  }      

  if (strncmp(argv[2], "weboff", 6) == 0) {
    if (argc == 3) {
      kr_web_disable (client);
    }
  }

  if (strncmp(argv[2], "oscon", 5) == 0) {
    if (argc == 4) {
      kr_osc_enable (client, atoi(argv[3]));
    }
  }      

  if (strncmp(argv[2], "oscoff", 6) == 0) {
    if (argc == 3) {
      kr_osc_disable (client);
    }
  }

  if (strncmp(argv[2], "setdir", 6) == 0) {
    if (argc == 4) {
      kr_set_dir (client, argv[3]);
    }
  }

  if (strncmp(argv[2], "logname", 7) == 0) {
    kr_logname (client);
    kr_client_print_response (client);
  }

  /* Krad Mixer Commands */

  if (strncmp(argv[2], "lm", 2) == 0) {
    if (argc == 3) {
      kr_mixer_portgroups_list (client);
      kr_client_print_response (client);
    }
  }

  if (strncmp(argv[2], "rate", 4) == 0) {
    if (argc == 3) {
      kr_mixer_sample_rate (client);
      kr_client_print_response (client);
    }
  }      

  if (strncmp(argv[2], "setrate", 7) == 0) {
    if (argc == 4) {
      kr_mixer_set_sample_rate (client, atoi(argv[3]));
      kr_client_print_response (client);
    }
  }        

  if (strncmp(argv[2], "tone", 4) == 0) {
    if (argc == 4) {
      kr_mixer_push_tone (client, argv[3]);
    }
  }      

  if (strncmp(argv[2], "input", 5) == 0) {
    if (argc == 4) {
      kr_mixer_create_portgroup (client, argv[3], "input", 2);
    }
    if (argc == 5) {
      kr_mixer_create_portgroup (client, argv[3], "input", atoi (argv[4]));
    }
  }      

  if (strncmp(argv[2], "output", 6) == 0) {
    if (argc == 4) {
      kr_mixer_create_portgroup (client, argv[3], "output", 2);
    }
    if (argc == 5) {
      kr_mixer_create_portgroup (client, argv[3], "output", atoi (argv[4]));
    }
  }
  
  if (strncmp(argv[2], "auxout", 6) == 0) {
    if (argc == 4) {
      kr_mixer_create_portgroup (client, argv[3], "auxout", 2);
    }
    if (argc == 5) {
      kr_mixer_create_portgroup (client, argv[3], "auxout", atoi (argv[4]));
    }
  }  

  if (strncmp(argv[2], "rmport", 6) == 0) {
    if (argc == 4) {
      kr_mixer_remove_portgroup (client, argv[3]);
    }
  }

  if (strncmp(argv[2], "plug", 4) == 0) {
    if (argc == 5) {
      kr_mixer_plug_portgroup (client, argv[3], argv[4]);
    }
  }

  if (strncmp(argv[2], "unplug", 6) == 0) {
    if (argc == 4) {
      kr_mixer_unplug_portgroup (client, argv[3], "");
    }
    if (argc == 5) {
      kr_mixer_unplug_portgroup (client, argv[3], argv[4]);
    }        
  }            

  if (strncmp(argv[2], "map", 3) == 0) {
    if (argc == 6) {
      kr_mixer_update_portgroup_map_channel (client, argv[3], atoi(argv[4]), atoi(argv[5]));
    }
  }

  if (strncmp(argv[2], "mixmap", 3) == 0) {
    if (argc == 6) {
      kr_mixer_update_portgroup_mixmap_channel (client, argv[3], atoi(argv[4]), atoi(argv[5]));
    }
  }      

  if (strncmp(argv[2], "crossfade", 9) == 0) {
    if (argc == 4) {
      kr_mixer_update_portgroup (client, argv[3], EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, "");
    }
    if (argc == 5) {
      kr_mixer_update_portgroup (client, argv[3], EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME, argv[4]);
    }
  }      

  if (strncmp(argv[2], "xmms2", 5) == 0) {
    if (argc == 5) {
      if ((strncmp(argv[4], "play", 4) == 0) || (strncmp(argv[4], "pause", 5) == 0) ||
        (strncmp(argv[4], "stop", 4) == 0) || (strncmp(argv[4], "next", 4) == 0) ||
        (strncmp(argv[4], "prev", 4) == 0)) {
        kr_mixer_portgroup_xmms2_cmd (client, argv[3], argv[4]);
        return 0;
      } else {
        kr_mixer_bind_portgroup_xmms2 (client, argv[3], argv[4]);
      }
    }
  }  

  if (strncmp(argv[2], "noxmms2", 7) == 0) {
    if (argc == 4) {
      kr_mixer_unbind_portgroup_xmms2 (client, argv[3]);
    }
  }

  if (strncmp(argv[2], "set", 3) == 0) {
    if (argc == 6) {
      kr_mixer_set_control (client, argv[3], argv[4], atof(argv[5]));
    }
  }

  if (strncmp(argv[2], "addfx", 5) == 0) {
    if (argc == 5) {
      kr_mixer_add_effect (client, argv[3], argv[4]);
    }
  }

  if (strncmp(argv[2], "rmfx", 4) == 0) {
    if (argc == 5) {
      kr_mixer_remove_effect (client, argv[3], atoi(argv[4]));
    }
  }

  if (strncmp(argv[2], "setfx", 5) == 0) {
    if (argc == 8) {
      kr_mixer_set_effect_control (client, argv[3], atoi(argv[4]), argv[5], atoi(argv[6]), atof(argv[7]));
    }
  }

  /* Krad Transponder Commands */      

  if ((strncmp(argv[2], "ll", 2) == 0) && (strlen(argv[2]) == 2)) {
    if (argc == 3) {
      kr_transponder_list (client);
      kr_client_print_response (client);
    }
  }

  if (strncmp(argv[2], "listen_on", 9) == 0) {
    if (argc == 4) {
      kr_transponder_listen_enable (client, atoi(argv[3]));
    }
  }

  if (strncmp(argv[2], "listen_off", 10) == 0) {
    if (argc == 3) {
      kr_transponder_listen_disable (client);
    }
  }

  if (strncmp(argv[2], "transmitter_on", 14) == 0) {
    if (argc == 4) {
      kr_transponder_transmitter_enable (client, atoi(argv[3]));
    }
  }

  if (strncmp(argv[2], "transmitter_off", 15) == 0) {
    if (argc == 3) {
      kr_transponder_transmitter_disable (client);
    }
  }    

  if ((strncmp(argv[2], "link", 4) == 0) || (strncmp(argv[2], "transmit", 8) == 0)) {
    if (argc == 7) {
      if (strncmp(argv[2], "transmitav", 10) == 0) {
        kr_transponder_transmit (client, AUDIO_AND_VIDEO, argv[3], atoi(argv[4]), argv[5], argv[6], NULL, 0, 0, 0, "");
      } else {
        kr_transponder_transmit (client, AUDIO_ONLY, argv[3], atoi(argv[4]), argv[5], argv[6], NULL, 0, 0, 0, "");
      }
    }
    if (argc == 8) {
      kr_transponder_transmit (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], NULL,
                       0, 0, 0, "");
    }

    if (argc == 9) {
      kr_transponder_transmit (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
                       0, 0, 0, "");
    }

    if (argc == 10) {
      kr_transponder_transmit (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
                       atoi(argv[9]), 0, 0, "");
    }

    if (argc == 11) {
      kr_transponder_transmit (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
                       atoi(argv[9]), atoi(argv[10]), 0, "");
    }

    if (argc == 12) {
      kr_transponder_transmit (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
                       atoi(argv[9]), atoi(argv[10]), atoi(argv[11]), "");
    }

    if (argc == 13) {
      kr_transponder_transmit (client, krad_link_string_to_av_mode (argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7], argv[8],
                       atoi(argv[9]), atoi(argv[10]), atoi(argv[11]), argv[12]);
    }                                

  }    

  if (strncmp(argv[2], "capture", 7) == 0) {

    if (krad_link_string_to_video_source (argv[3]) == DECKLINK) {
      val = AUDIO_AND_VIDEO;
    } else {
      val = VIDEO_ONLY;
    }

    if (argc == 4) {
      kr_transponder_capture (client, krad_link_string_to_video_source (argv[3]), "",
                      0, 0, 0, 0, val, "", "");
    }
    if (argc == 5) {
      kr_transponder_capture (client, krad_link_string_to_video_source (argv[3]), argv[4],
                      0, 0, 0, 0, val, "", "");
    }
    if (argc == 7) {
      kr_transponder_capture (client, krad_link_string_to_video_source (argv[3]), argv[4],
                      atoi(argv[5]), atoi(argv[6]), 0, 0, val, "", "");
    }
    if (argc == 9) {
      kr_transponder_capture (client, krad_link_string_to_video_source (argv[3]), argv[4],
                      atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), val, "", "");
    }
    if (argc == 10) {
      kr_transponder_capture (client, krad_link_string_to_video_source (argv[3]), argv[4],
                      atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]),
                      krad_link_string_to_av_mode (argv[9]), "", "");
    }
    if (argc == 11) {
      kr_transponder_capture (client, krad_link_string_to_video_source (argv[3]), argv[4],
                      atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]),
                      krad_link_string_to_av_mode (argv[9]), argv[10], "");
    }
    if (argc == 12) {
      kr_transponder_capture (client, krad_link_string_to_video_source (argv[3]), argv[4],
                      atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]),
                      krad_link_string_to_av_mode (argv[9]), argv[10], argv[11]);
    }
    usleep (100000);
  }

  if (strncmp(argv[2], "record", 6) == 0) {
    if (argc == 4) {
      if (strncmp(argv[2], "recordav", 8) == 0) {
        kr_transponder_record (client, AUDIO_AND_VIDEO, argv[3], NULL, 0, 0, 0, "");
      } else {
        kr_transponder_record (client, AUDIO_ONLY, argv[3], NULL, 0, 0, 0, "");
      }
    }
    if (argc == 5) {
      kr_transponder_record (client, krad_link_string_to_av_mode (argv[3]), argv[4], NULL,
                     0, 0, 0, "");
    }

    if (argc == 6) {
      kr_transponder_record (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
                     0, 0, 0, "");
    }

    if (argc == 7) {
      kr_transponder_record (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
                     atoi(argv[6]), 0, 0, "");
    }

    if (argc == 8) {
      kr_transponder_record (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
                     atoi(argv[6]), atoi(argv[7]), 0, "");
    }

    if (argc == 9) {
      kr_transponder_record (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
                     atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), "");
    }

    if (argc == 10) {
      kr_transponder_record (client, krad_link_string_to_av_mode (argv[3]), argv[4], argv[5],
                     atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), argv[9]);          
    }                                

  }

  if (strncmp(argv[2], "receive", 7) == 0) {
    if (argc == 4) {
      kr_transponder_receive (client, atoi(argv[3]));
    }
  }        

  if (strncmp(argv[2], "play", 4) == 0) {
    if (argc == 4) {
      kr_transponder_play (client, argv[3]);
    }
    if (argc == 6) {
      kr_transponder_play_remote (client, argv[3], atoi(argv[4]), argv[5] );
    }
  }

  if ((strncmp(argv[2], "rm", 2) == 0) && (strlen(argv[2]) == 2)) {
    if (argc == 4) {
      kr_transponder_destroy (client, atoi(argv[3]));
      kr_client_print_response (client);      
    }
  }

  if (strncmp(argv[2], "update", 2) == 0) {

    if (argc == 5) {
      if (strcmp(argv[4], "vp8_keyframe") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_VP8_FORCE_KEYFRAME, 1);
      }
    }

    if (argc == 6) {
      if (strcmp(argv[4], "vp8_bitrate") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, atoi(argv[5]));
      }
      if (strcmp(argv[4], "vp8_min_quantizer") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER, atoi(argv[5]));
      }
      if (strcmp(argv[4], "vp8_max_quantizer") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER, atoi(argv[5]));
      }
      if (strcmp(argv[4], "vp8_deadline") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE, atoi(argv[5]));
      }
      if (strcmp(argv[4], "theora_quality") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY, atoi(argv[5]));
      }                                      
      if (strcmp(argv[4], "opus_bitrate") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, atoi(argv[5]));
      }        
      if (strcmp(argv[4], "opus_bandwidth") == 0) {
        kr_transponder_update_str (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH, argv[5]);
      }
      if (strcmp(argv[4], "opus_signal") == 0) {
        kr_transponder_update_str (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL, argv[5]);
      }
      if (strcmp(argv[4], "opus_complexity") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY, atoi(argv[5]));
      }
      if (strcmp(argv[4], "opus_framesize") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE, atoi(argv[5]));
      }                    
      if (strcmp(argv[4], "ogg_maxpackets") == 0) {
        kr_transponder_update (client, atoi(argv[3]), EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE, atoi(argv[5]));
      }
    }        
  }
  
  /* Krad Compositor Commands */

  if ((strncmp(argv[2], "lc", 2) == 0) && (strlen(argv[2]) == 2)) {
    if (argc == 3) {
      kr_compositor_port_list (client);
      kr_client_print_response (client);
    }
  }

  if (strncmp(argv[2], "setport", 7) == 0) {
    if (argc == 14) {
      kr_compositor_set_port_mode (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]),
                         atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]),
                         atoi(argv[10]), atoi(argv[11]), atof(argv[12]), atof(argv[13]));
    }
  }

  if (strncmp(argv[2], "snap", 4) == 0) {
    if (argc == 3) {
      kr_compositor_snapshot (client);
    }
  }

  if (strncmp(argv[2], "jsnap", 5) == 0) {
    if (argc == 3) {
      kr_compositor_snapshot_jpeg (client);
    }
  }
  
  if (strncmp(argv[2], "lastsnap", 8) == 0) {
    kr_compositor_last_snap_name (client);
    kr_client_print_response (client);        
  }

  if (strncmp(argv[2], "comp", 4) == 0) {
    if (argc == 3) {
      kr_compositor_info (client);
      kr_client_print_response (client);
    }
  }

  if (strncmp(argv[2], "res", 3) == 0) {
    if (argc == 5) {
      kr_compositor_set_resolution (client, atoi(argv[3]), atoi(argv[4]));
    }
  }

  if (strncmp(argv[2], "fps", 3) == 0) {
    if (argc == 4) {
      kr_compositor_set_frame_rate (client, atoi(argv[3]) * 1000, 1000);
    }      
    if (argc == 5) {
      kr_compositor_set_frame_rate (client, atoi(argv[3]), atoi(argv[4]));
    }
  }

  if (strncmp(argv[2], "addsprite", 9) == 0) {
    if (argc == 4) {
      kr_compositor_add_sprite (client, argv[3], 0, 0, 0, 4,
                      1.0f, 1.0f, 0.0f);
    }
    if (argc == 5) {
      kr_compositor_add_sprite (client, argv[3], atoi(argv[4]), 0, 0, 4,
                      1.0f, 1.0f, 0.0f);
    }        
    if (argc == 6) {
      kr_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), 0, 4,
                      1.0f, 1.0f, 0.0f);
    }
    if (argc == 7) {
      kr_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), 4,
                      1.0f, 1.0f, 0.0f);
    }
    if (argc == 8) {
      kr_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), 
                                      1.0f, 1.0f, 0.0f);
    }
    if (argc == 9) {
      kr_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), 
                                      atof(argv[8]), 1.0f, 0.0f);
    }
    if (argc == 10) {
      kr_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), 
                                      atof(argv[8]), atof(argv[9]), 0.0f);
    }
    if (argc == 11) {
      kr_compositor_add_sprite (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), 
                                      atof(argv[8]), atof(argv[9]), atof(argv[10]));
    }
  }

  if (strncmp(argv[2], "setsprite", 9) == 0) {
    if (argc == 6) {
      kr_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]),  0, 4,
                      1.0f, 1.0f, 0.0f);
    }
    if (argc == 7) {
      kr_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), 4,
                      1.0f, 1.0f, 0.0f);
    }
    if (argc == 8) {
      kr_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      1.0f, 1.0f, 0.0f);
    }
    if (argc == 9) {
      kr_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), 1.0f, 0.0f);
    }
    if (argc == 10) {
      kr_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), 0.0f);
    }
    if (argc == 11) {
      kr_compositor_set_sprite (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), atof(argv[10]));
    }
  }

  if (strncmp(argv[2], "rmsprite", 8) == 0) {
    if (argc == 4) {
      kr_compositor_remove_sprite (client, atoi(argv[3]));
    }
  }

  if (strncmp(argv[2], "lssprite", 8) == 0) {
    if (argc == 3) {
      kr_compositor_list_sprites (client);
      kr_client_print_response (client);      
    }
  }

  if (strncmp(argv[2], "addtext", 7) == 0) {
    if (argc == 4) {
      kr_compositor_add_text (client, argv[3], 32, 32, 0, 4,
                      20.0f, 1.0f, 0.0f, 244.0f, 16.0f, 16.0f, "sans");
    }
    if (argc == 5) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), 32, 0, 4,
                      20.0f, 1.0f, 0.0f, 244, 16, 16, "sans");
    }        
    if (argc == 6) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), 0, 4,
                      20.0f, 1.0f, 0.0f, 244, 16, 16, "sans");
    }
    if (argc == 7) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), 4,
                      20.0f, 1.0f, 0.0f, 244, 16, 16, "sans");
    }
    if (argc == 8) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      20.0f, 1.0f, 0.0f, 244, 16, 16, "sans");
    }
    if (argc == 9) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), 1.0f, 0.0f, 244, 16, 16, "sans");
    }
    if (argc == 10) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), 0.0f, 244, 16, 16, "sans");
    }
    if (argc == 11) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), atof(argv[10]), 244, 16, 16, "sans");
    }
    if (argc == 12) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), atof(argv[10]), atoi(argv[11]),16,  16, "sans");
    }
    if (argc == 13) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), atof(argv[10]), atoi(argv[11]), atoi(argv[12]), 16,  "sans");
    }
    if (argc == 14) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                                    atof(argv[8]), atof(argv[9]), atof(argv[10]), atoi(argv[11]), atoi(argv[12]), atoi(argv[13]), "sans");
    }
    if (argc == 15) {
      kr_compositor_add_text (client, argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                                    atof(argv[8]), atof(argv[9]), atof(argv[10]), atoi(argv[11]), atoi(argv[12]), atoi(argv[13]), argv[14]);
    }
  }

  if (strncmp(argv[2], "settext", 7) == 0) {
    if (argc == 6) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), 0, 4,
                      20.0f, 1.0f, 0.0f, 244.0f, 16.0f, 16.0f);
    }
    if (argc == 7) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), 4,
                      20.0f, 1.0f, 0.0f, 244.0f, 16.0f, 16.0f);
    }
    if (argc == 8) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      20.0f, 1.0f, 0.0f, 244.0f, 16.0f, 16.0f);
    }
    if (argc == 9) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), 1.0f, 0.0f, 244.0f, 16.0f, 16.0f);
    }
    if (argc == 10) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[8]), 0.0f, 244.0f, 16.0f, 16.0f);
    }
    if (argc == 11) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), atof(argv[10]), 244.0f, 16.0f, 16.0f);
    }
    if (argc == 12) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), atof(argv[10]), atof(argv[11]), 16.0f, 16.0f);
    }
    if (argc == 13) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                      atof(argv[8]), atof(argv[9]), atof(argv[10]), atof(argv[11]), atof(argv[12]), 16.0f);
    }
    if (argc == 14) {
      kr_compositor_set_text (client, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]),
                                    atof(argv[8]), atof(argv[9]), atof(argv[10]), atof(argv[11]), atof(argv[12]), atof(argv[13]));
    }                        

  }

  if (strncmp(argv[2], "rmtext", 6) == 0) {
    if (argc == 4) {
      kr_compositor_remove_text (client, atoi(argv[3]));
    }
  }

  if (strncmp(argv[2], "lstext", 6) == 0) {
    if (argc == 3) {
      kr_compositor_list_texts (client);
      kr_client_print_response (client);      
    }
  }                          

  if (strncmp(argv[2], "background", 10) == 0) {
    if (argc == 4) {
      kr_compositor_background (client, argv[3]);
    }
  }      

  if (strncmp(argv[2], "display", 7) == 0) {
    if (argc == 3) {
      kr_compositor_open_display (client, 0, 0);
    }
    if (argc == 4) {
      kr_compositor_open_display (client, 1, 1);
    }          
    if (argc == 5) {
      kr_compositor_open_display (client, atoi(argv[3]), atoi(argv[4]));
    }        
  }

  if (strncmp(argv[2], "closedisplay", 12) == 0) {
    if (argc == 3) {
      kr_compositor_close_display (client);
    }
  }
  
  kr_disconnect (&client);

  return 0;

}
