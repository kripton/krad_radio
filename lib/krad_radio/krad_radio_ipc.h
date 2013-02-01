#define EBML_ID_KRAD_RADIO_CMD 0x7384
#define EBML_ID_KRAD_MIXER_CMD 0x73A4
#define EBML_ID_KRAD_COMPOSITOR_CMD 0x73C4
#define EBML_ID_KRAD_TRANSPONDER_CMD 0x73C5

#define EBML_ID_KRAD_RADIO_MSG 0x437C
#define EBML_ID_KRAD_MIXER_MSG 0x450D
#define EBML_ID_KRAD_TRANSPONDER_MSG 0x6911
#define EBML_ID_KRAD_COMPOSITOR_MSG 0x53B8

#define EBML_ID_KRAD_RADIO_GLOBAL_BROADCAST 0xC3
#define EBML_ID_KRAD_SYSTEM_BROADCAST 0xF5
#define EBML_ID_KRAD_RADIO_CMD_BROADCAST_SUBSCRIBE 0x54AA

#define EBML_ID_CONTROL_SET 0x6944

#define EBML_ID_KRAD_RADIO_CMD_OSC_ENABLE 0x467E
#define EBML_ID_KRAD_RADIO_CMD_OSC_DISABLE 0x61A7

#define EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE 0xCE
#define EBML_ID_KRAD_RADIO_CMD_WEB_DISABLE 0xCD

#define EBML_ID_KRAD_RADIO_CMD_GET_REMOTE_STATUS 0x6624
#define EBML_ID_KRAD_RADIO_REMOTE_STATUS_LIST 0x4461
#define EBML_ID_KRAD_RADIO_REMOTE_STATUS 0xE9
#define EBML_ID_KRAD_RADIO_REMOTE_INTERFACE 0xF7
#define EBML_ID_KRAD_RADIO_REMOTE_PORT 0xF1

#define EBML_ID_KRAD_RADIO_CMD_REMOTE_ENABLE 0x80
#define EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE 0x89

#define EBML_ID_KRAD_RADIO_CMD_SET_TAG 0xAE
#define EBML_ID_KRAD_RADIO_CMD_GET_TAG 0xD7
#define EBML_ID_KRAD_RADIO_CMD_LIST_TAGS 0xB9

#define EBML_ID_KRAD_RADIO_CMD_UPTIME 0xA6
#define EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_INFO 0xA5
#define EBML_ID_KRAD_RADIO_CMD_SET_DIR 0x6264

#define EBML_ID_KRAD_RADIO_DIR 0x7D7B
#define EBML_ID_KRAD_RADIO_UPTIME 0xFB
#define EBML_ID_KRAD_RADIO_SYSTEM_INFO 0xFD

#define EBML_ID_KRAD_RADIO_HTTP_PORT 0xCA
#define EBML_ID_KRAD_RADIO_WEBSOCKET_PORT 0xCB
#define EBML_ID_KRAD_RADIO_TCP_PORT 0x8F
#define EBML_ID_KRAD_RADIO_UDP_PORT 0x535F

#define EBML_ID_KRAD_RADIO_WEB_HEADCODE 0x6E67
#define EBML_ID_KRAD_RADIO_WEB_HEADER 0x78B5
#define EBML_ID_KRAD_RADIO_WEB_FOOTER 0x63C6

#define EBML_ID_KRAD_RADIO_CMD_GET_LOGNAME 0xED
#define EBML_ID_KRAD_RADIO_LOGNAME 0xDB

#define EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_CPU_USAGE 0x45A3
#define EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE 0x4487

#define EBML_ID_KRAD_RADIO_TAG_LIST 0x9C
#define EBML_ID_KRAD_RADIO_TAG 0x88
#define EBML_ID_KRAD_RADIO_TAG_ITEM 0x63C0
#define EBML_ID_KRAD_RADIO_TAG_NAME 0x45A3
#define EBML_ID_KRAD_RADIO_TAG_VALUE 0x4487
#define EBML_ID_KRAD_RADIO_TAG_SOURCE 0x63C9 // triplets? S4 future? oh my!

#define EBML_ID_KRAD_COMPOSITOR_FILENAME 0x466E

#define EBML_ID_KRAD_COMPOSITOR_CMD_CLOSE_DISPLAY 0x4461
#define EBML_ID_KRAD_COMPOSITOR_CMD_OPEN_DISPLAY 0x7BA9

#define EBML_ID_KRAD_COMPOSITOR_X 0x66FC
#define EBML_ID_KRAD_COMPOSITOR_Y 0x66BF
#define EBML_ID_KRAD_COMPOSITOR_SIZE 0x66A5

#define EBML_ID_KRAD_RADIO_SUBUNIT_DESTROYED 0x437E
#define EBML_ID_KRAD_RADIO_UNIT 0xAA
#define EBML_ID_KRAD_RADIO_SUBUNIT 0xAA
#define EBML_ID_KRAD_RADIO_SUBUNIT_ADDRESS_NAME 0x437E
#define EBML_ID_KRAD_RADIO_SUBUNIT_ADDRESS_NUMBER 0x437E

#define EBML_ID_KRAD_COMPOSITOR_CMD_LOCAL_VIDEOPORT_CREATE 0x54B3
#define EBML_ID_KRAD_COMPOSITOR_CMD_LOCAL_VIDEOPORT_DESTROY 0x54CC

#define EBML_ID_KRAD_COMPOSITOR_CMD_UPDATE_PORT 0x6FAB
#define EBML_ID_KRAD_COMPOSITOR_CMD_INFO 0x3A9697
#define EBML_ID_KRAD_COMPOSITOR_CMD_LIST_PORTS 0x3E83BB
#define EBML_ID_KRAD_COMPOSITOR_CMD_SET_BACKGROUND 0x6E67
#define EBML_ID_KRAD_COMPOSITOR_CMD_SET_FRAME_RATE 0x78B5
#define EBML_ID_KRAD_COMPOSITOR_CMD_SET_RESOLUTION 0x63C6
#define EBML_ID_KRAD_COMPOSITOR_CMD_SNAPSHOT 0x4254
#define EBML_ID_KRAD_COMPOSITOR_CMD_SNAPSHOT_JPEG 0xE6

#define EBML_ID_KRAD_COMPOSITOR_CMD_GET_LAST_SNAPSHOT_NAME 0xAF
#define EBML_ID_KRAD_COMPOSITOR_LAST_SNAPSHOT_NAME 0x447A

#define EBML_ID_KRAD_COMPOSITOR_CMD_ADD_SPRITE 0x447A
#define EBML_ID_KRAD_COMPOSITOR_CMD_SET_SPRITE 0x6EBC
#define EBML_ID_KRAD_COMPOSITOR_CMD_REMOVE_SPRITE 0x4484
#define EBML_ID_KRAD_COMPOSITOR_CMD_LIST_SPRITES 0x4485

#define EBML_ID_KRAD_COMPOSITOR_CMD_ADD_TEXT 0xB0
#define EBML_ID_KRAD_COMPOSITOR_CMD_SET_TEXT 0xEA
#define EBML_ID_KRAD_COMPOSITOR_CMD_REMOVE_TEXT 0x9B
#define EBML_ID_KRAD_COMPOSITOR_CMD_LIST_TEXTS 0xDB

#define EBML_ID_KRAD_COMPOSITOR_SPRITE_LIST 0x5035
#define EBML_ID_KRAD_COMPOSITOR_SPRITE_NUMBER 0x67C8
#define EBML_ID_KRAD_COMPOSITOR_SPRITE_TICKRATE 0x4DBB
#define EBML_ID_KRAD_COMPOSITOR_SPRITE_SCALE 0x66A5
#define EBML_ID_KRAD_COMPOSITOR_SPRITE_OPACITY 0x6955
#define EBML_ID_KRAD_COMPOSITOR_SPRITE_ROTATION 0x537F

#define EBML_ID_KRAD_COMPOSITOR_TEXT_LIST 0x4255
#define EBML_ID_KRAD_COMPOSITOR_TEXT 0xF1
#define EBML_ID_KRAD_COMPOSITOR_TEXT_NUMBER 0x67C8
#define EBML_ID_KRAD_COMPOSITOR_TEXT_TICKRATE 0x4DBB
#define EBML_ID_KRAD_COMPOSITOR_TEXT_SCALE 0x66A5
#define EBML_ID_KRAD_COMPOSITOR_TEXT_OPACITY 0x6955
#define EBML_ID_KRAD_COMPOSITOR_TEXT_ROTATION 0x537F

#define EBML_ID_KRAD_COMPOSITOR_FRAME_RATE 0xCE
#define EBML_ID_KRAD_COMPOSITOR_FRAME_SIZE 0x89

#define EBML_ID_KRAD_COMPOSITOR_RED 0xE2
#define EBML_ID_KRAD_COMPOSITOR_GREEN 0xE3
#define EBML_ID_KRAD_COMPOSITOR_BLUE 0xE4
#define EBML_ID_KRAD_COMPOSITOR_FONT 0xE4

#define EBML_ID_KRAD_COMPOSITOR_WIDTH 0x6D80
#define EBML_ID_KRAD_COMPOSITOR_HEIGHT 0x6240
#define EBML_ID_KRAD_COMPOSITOR_FPS_NUMERATOR 0x5031
#define EBML_ID_KRAD_COMPOSITOR_FPS_DENOMINATOR 0x5032

#define EBML_ID_KRAD_COMPOSITOR_INFO 0x2383E3

#define EBML_ID_KRAD_COMPOSITOR_PORT_LIST 0x3C83AB
#define EBML_ID_KRAD_COMPOSITOR_PORT 0x3EB923
#define EBML_ID_KRAD_COMPOSITOR_PORT_NUMBER 0x2AD7B1
#define EBML_ID_KRAD_COMPOSITOR_PORT_DIRECTION 0x4DBB
#define EBML_ID_KRAD_COMPOSITOR_PORT_SOURCE_WIDTH 0x53AB
#define EBML_ID_KRAD_COMPOSITOR_PORT_SOURCE_HEIGHT 0x53AC
#define EBML_ID_KRAD_COMPOSITOR_PORT_WIDTH 0x4489
#define EBML_ID_KRAD_COMPOSITOR_PORT_HEIGHT 0x4D80
#define EBML_ID_KRAD_COMPOSITOR_PORT_X 0x5741
#define EBML_ID_KRAD_COMPOSITOR_PORT_Y 0x5854
#define EBML_ID_KRAD_COMPOSITOR_PORT_CROP_WIDTH 0x58D7
#define EBML_ID_KRAD_COMPOSITOR_PORT_CROP_HEIGHT 0x55AA
#define EBML_ID_KRAD_COMPOSITOR_PORT_CROP_X 0x6DE7
#define EBML_ID_KRAD_COMPOSITOR_PORT_CROP_Y 0x6DF8
#define EBML_ID_KRAD_COMPOSITOR_PORT_OPACITY 0x6955
#define EBML_ID_KRAD_COMPOSITOR_PORT_ROTATION 0x537F
#define EBML_ID_KRAD_COMPOSITOR_PORT_FPS_NUMERATOR 0x63A2
#define EBML_ID_KRAD_COMPOSITOR_PORT_FPS_DENOMINATOR 0x7446


#define EBML_ID_KRAD_MIXER_PORTGROUP_CREATED 0xE0 // Broadcast
#define EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED 0xFA // Broadcast

#define EBML_ID_KRAD_MIXER_CMD_PORTGROUP_INFO 0x6DF8
#define EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS 0xB0
#define EBML_ID_KRAD_MIXER_CMD_CREATE_PORTGROUP 0xEA
#define EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP 0x9B
#define EBML_ID_KRAD_MIXER_CMD_DESTROY_PORTGROUP 0xDB
#define EBML_ID_KRAD_MIXER_CMD_PUSH_TONE 0x54AA
#define EBML_ID_KRAD_MIXER_CMD_SET_SAMPLE_RATE 0x4444
#define EBML_ID_KRAD_MIXER_CMD_GET_INFO 0x6924

#define EBML_ID_KRAD_MIXER_CMD_LOCAL_AUDIOPORT_CREATE 0x54B3
#define EBML_ID_KRAD_MIXER_CMD_LOCAL_AUDIOPORT_DESTROY 0x54CC

#define EBML_ID_KRAD_MIXER_CMD_ADD_EFFECT 0x3E83BB
#define EBML_ID_KRAD_MIXER_CMD_REMOVE_EFFECT 0x6955
#define EBML_ID_KRAD_EFFECT_NAME 0xC9
#define EBML_ID_KRAD_MIXER_PORTGROUP_EFFECT_NUM 0xB3
#define EBML_ID_KRAD_MIXER_CMD_SET_EFFECT_CONTROL 0xBA

#define EBML_ID_KRAD_SUBUNIT 0x46AE

#define EBML_ID_KRAD_MIXER_CMD_SET_PORTGROUP_DELAY 0x2AD7B1
#define EBML_ID_KRAD_MIXER_PORTGROUP_DELAY 0x4DBB

#define EBML_ID_KRAD_MIXER_CMD_PLUG_PORTGROUP 0x53AB
#define EBML_ID_KRAD_MIXER_CMD_UNPLUG_PORTGROUP 0x53AC

#define EBML_ID_KRAD_MIXER_MAP_CHANNEL 0x4255
#define EBML_ID_KRAD_MIXER_MIXMAP_CHANNEL 0x5035
#define EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL 0x5034

#define EBML_ID_KRAD_MIXER_SAMPLE_RATE 0x69FC
#define EBML_ID_KRAD_MIXER_TONE_NAME 0x54BB
#define EBML_ID_KRAD_MIXER_PORTGROUP_LIST 0xBA
#define EBML_ID_KRAD_MIXER_PORTGROUP 0xE1

#define EBML_ID_KRAD_MIXER_PORTGROUP_NAME 0xE2
#define EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS 0xE3
#define EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION 0xAF
#define EBML_ID_KRAD_MIXER_PORTGROUP_TYPE 0xE4
#define EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME 0xE5
#define EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS 0xE6

#define EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME 0xA3
#define EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE 0xE7 // the value itself

#define EBML_ID_KRAD_MIXER_CMD_SET_CONTROL 0xA7
#define EBML_ID_KRAD_MIXER_CMD_GET_CONTROL 0xAB
#define EBML_ID_KRAD_MIXER_CONTROL 0xA1


#define EBML_ID_KRAD_MIXER_CONTROL_NAME 0xF7
#define EBML_ID_KRAD_MIXER_CONTROL_VALUE 0xF1
#define EBML_ID_KRAD_MIXER_CONTROL_DURATION 0xF5
#define EBML_ID_KRAD_MIXER_PORTGROUP_XMMS2 0x66A5

#define EBML_ID_KRAD_MIXER_CMD_BIND_PORTGROUP_XMMS2 0x7E8A
#define EBML_ID_KRAD_MIXER_CMD_UNBIND_PORTGROUP_XMMS2 0x7E9A
#define EBML_ID_KRAD_MIXER_XMMS2_IPC_PATH 0x7EA5

#define EBML_ID_KRAD_MIXER_CMD_PORTGROUP_XMMS2_CMD 0x5741
#define EBML_ID_KRAD_MIXER_XMMS2_CMD 0x5854

#define EBML_ID_KRAD_TRANSPONDER_CMD_LIST_LINKS 0xB7
#define EBML_ID_KRAD_TRANSPONDER_CMD_CREATE_LINK 0xED
#define EBML_ID_KRAD_TRANSPONDER_CMD_DESTROY_LINK 0xE9
#define EBML_ID_KRAD_TRANSPONDER_CMD_UPDATE_LINK 0xB3

#define EBML_ID_KRAD_TRANSPONDER_LINK_NUMBER 0xB5

#define EBML_ID_KRAD_TRANSPONDER_LINK_CREATED 0x53AB
#define EBML_ID_KRAD_TRANSPONDER_LINK_UPDATED 0x53AC
#define EBML_ID_KRAD_TRANSPONDER_LINK_DESTROYED 0x4489



#define EBML_ID_KRAD_LIST_COUNT 0x5741

#define EBML_ID_KRAD_TRANSPONDER_CMD_LIST_DECKLINK 0x55EE
#define EBML_ID_KRAD_TRANSPONDER_DECKLINK_LIST 0xE7
#define EBML_ID_KRAD_TRANSPONDER_DECKLINK_DEVICE_NAME 0x22B59C


#define EBML_ID_KRAD_TRANSPONDER_CMD_LIST_V4L2 0x54B0
#define EBML_ID_KRAD_TRANSPONDER_V4L2_LIST 0x54BA
#define EBML_ID_KRAD_TRANSPONDER_V4L2_DEVICE_FILENAME 0x5031


#define EBML_ID_KRAD_MIXER_CMD_JACK_RUNNING 0x4598
#define EBML_ID_KRAD_MIXER_JACK_RUNNING 0x55EE

#define EBML_ID_KRAD_LINK_LINK_CAPTURE_CODEC 0x92
#define EBML_ID_KRAD_LINK_LINK_USE_PASSTHRU_CODEC 0x92
#define EBML_ID_KRAD_TRANSPONDER_LINK_LIST 0xC0

#define EBML_ID_KRAD_TRANSPONDER_LINK 0xC1
#define EBML_ID_KRAD_LINK_LINK_OPERATION_MODE 0xC2
#define EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE 0xC3
#define EBML_ID_KRAD_LINK_LINK_AV_MODE 0xC4

#define EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE 0x3CB923

#define EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC 0x91
#define EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC 0x92

#define EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE 0x26B240

#define EBML_ID_KRAD_LINK_LINK_OPUS_APPLICATION 0x23E383
#define EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE 0x23314F
#define EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL 0x22B59C
#define EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH 0x4598
#define EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY 0x55EE
#define EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE 0x536E

#define EBML_ID_KRAD_LINK_LINK_FILENAME 0xC5

#define EBML_ID_KRAD_LINK_LINK_HOST 0xC6
#define EBML_ID_KRAD_LINK_LINK_PORT 0xC7
#define EBML_ID_KRAD_LINK_LINK_MOUNT 0xC8
#define EBML_ID_KRAD_LINK_LINK_PASSWORD 0xC9

#define EBML_ID_KRAD_LINK_LINK_CAPTURE_DEVICE 0x5854
#define EBML_ID_KRAD_LINK_LINK_CAPTURE_DECKLINK_AUDIO_INPUT 0x4598

#define EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH 0x54B0
#define EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT 0x54BA
#define EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR 0x5031
#define EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR 0x5032

#define EBML_ID_KRAD_LINK_LINK_VIDEO_COLOR_DEPTH 0x58D7

#define EBML_ID_KRAD_LINK_LINK_AUDIO_CHANNELS 0x6FAB
#define EBML_ID_KRAD_LINK_LINK_AUDIO_SAMPLE_RATE 0x3A9697

#define EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY 0x3E83BB
#define EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH 0x6E67
#define EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY 0x78B5

#define EBML_ID_KRAD_LINK_LINK_VP8_BITRATE 0x6922
#define EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER 0x63C6
#define EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER 0x54BB
#define EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE 0x4254

#define EBML_ID_KRAD_LINK_LINK_VP8_FORCE_KEYFRAME 0x6933

#define EBML_ID_KRAD_TRANSPONDER_CMD_LISTEN_ENABLE 0x54B3
#define EBML_ID_KRAD_TRANSPONDER_CMD_LISTEN_DISABLE 0x54CC

#define EBML_ID_KRAD_TRANSPONDER_CMD_TRANSMITTER_ENABLE 0x54DD
#define EBML_ID_KRAD_TRANSPONDER_CMD_TRANSMITTER_DISABLE 0x47E6
