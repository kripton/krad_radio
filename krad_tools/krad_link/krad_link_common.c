#include "krad_link_common.h"

char *krad_codec_to_string (krad_codec_t codec) {

	switch (codec) {
		case VORBIS:
			return "Vorbis";
		case FLAC:
			return "FLAC";
		case OPUS:
			return "Opus";
		case VP8:
			return "VP8";
		case THEORA:
			return "Theora";
		case DIRAC:
			return "Dirac";
		case MJPEG:
			return "Mjpeg";
		default:
			return "No Codec";
	}
}

char *krad_link_av_mode_to_string (krad_link_av_mode_t av_mode) {

	switch (av_mode) {
		case AUDIO_ONLY:
			return "audio only";
		case VIDEO_ONLY:
			return "video only";
		case AUDIO_AND_VIDEO:
			return "audio and video";
		default:
			return "Unknown";
	}
}

char *krad_link_operation_mode_to_string (krad_link_operation_mode_t operation_mode) {

	switch (operation_mode) {
		case CAPTURE:
			return "capture";
		case RECEIVE:
			return "receive";
		case RECORD:
			return "record";
		case TRANSMIT:
			return "transmit";
		case PLAYBACK:
			return "playback";
		case FAILURE:
			return "failure";
		default:
			return "Unknown";
	}
}

char *krad_video_source_to_string (krad_video_source_t video_source) {

	switch (video_source) {
		case TEST:
			return "test";
		case V4L2:
			return "V4L2";
		case DECKLINK:
			return "decklink";
		case X11:
			return "X11";
		case NOVIDEO:
			return "novideo";
		default:
			return "Unknown";
	}
}

char *krad_link_transport_mode_to_string (krad_link_transport_mode_t transport_mode) {

	switch (transport_mode) {
		case TCP:
			return "TCP";
		case UDP:
			return "UDP";
		case FILESYSTEM:
			return "filesystem";
		case FAIL:
			return "failure";
		default:
			return "Unknown";
	}
}
