#include <krad_sdl_opengl_display.h>
#include <krad_ebml.h>
#include <krad_vpx.h>
#include <krad_dirac.h>
#include <krad_flac.h>
#include <krad_vorbis.h>
#include <krad_opus.h>
#include <krad_gui.h>
#include "SDL.h"

#define APPVERSION "Krad EBML File Broadcast 0.42"

typedef struct krad_ebml_video_player_hud_test_St krad_ebml_video_player_hud_test_t;

struct krad_ebml_video_player_hud_test_St {

	krad_audio_t *audio;
	kradgui_t *kradgui;
	krad_vorbis_t *krad_vorbis;
	
	float *samples[2];
	
	
};


void krad_ebml_video_player_hud_test_audio_callback(int frames, void *userdata) {

	krad_ebml_video_player_hud_test_t *hudtest = (krad_ebml_video_player_hud_test_t *)userdata;

	int len;
	
	len = krad_vorbis_decoder_read_audio(hudtest->krad_vorbis, 0, (char *)hudtest->samples[0], frames * 4);

	if (len) {
		kradaudio_write (hudtest->audio, 0, (char *)hudtest->samples[0], len );
	}

	len = krad_vorbis_decoder_read_audio(hudtest->krad_vorbis, 1, (char *)hudtest->samples[1], frames * 4);

	if (len) {
		kradaudio_write (hudtest->audio, 1, (char *)hudtest->samples[1], len );
	}
	
	//kradgui_add_current_track_time_ms(hudtest->kradgui, (frames / 44.1));
}


int main (int argc, char *argv[]) {

	struct stat st;
	krad_ebml_video_player_hud_test_t *hudtest;
	krad_codec_type_t audio_codec;
	krad_codec_type_t video_codec;

	krad_sdl_opengl_display_t *krad_opengl_display;
	krad_vpx_decoder_t *krad_vpx_decoder;
	krad_dirac_t *krad_dirac;
	kradebml_t *krad_ebml;
	kradebml_t *krad_ebml_output;
	kradgui_t *kradgui;
	
	krad_audio_t *audio;
	krad_audio_api_t audio_api;
	
	krad_vorbis_t *krad_vorbis;
	krad_flac_t *krad_flac;
	SDL_Event event;
	
	cairo_surface_t *cst;
	cairo_t *cr;
	
	int hud_width, hud_height;
	int hud_stride;
	int hud_byte_size;
	unsigned char *hud_data;

	unsigned char *buffer;
	
	int bytes;
	float *audio_data;
	int audio_frames;
	
	int i;
	int audio_packets;
	int video_packets;
	int shutdown;
	int packet_time_ms;
	int last_packet_time_ms;
	int nosleep;
	struct timespec packet_time;
	struct timespec sleep_time;
	
	int fd;
	int fd2;
	unsigned char *header;
	int header_pos;
	unsigned char *tracks;	
	int tracks_pos;
	int byte_position;
	int ret;
	int to_read;
	
	byte_position = 0;
	
	int dirac_output_unset;
	
	dirac_output_unset = true;
		
	if (argc < 2) {
		printf("specify a video file\n");
		exit(1);
	}

	if (stat(argv[1], &st) != 0) {
		printf("cant find file %s\n", argv[1]);
		exit(1);
	}
	
	shutdown = 0;
	
	hud_width = 320;
	hud_height = 340;

	audio_packets = 0;
	video_packets = 0;

	//audio_data = calloc(1, 8192 * 4 * 4);
	hudtest = calloc(1, sizeof(krad_ebml_video_player_hud_test_t));
	
	hud_stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, hud_width);
	hud_byte_size = hud_stride * hud_height;
	hud_data = calloc (1, hud_byte_size);
	cst = cairo_image_surface_create_for_data (hud_data, CAIRO_FORMAT_ARGB32, hud_width, hud_height, hud_stride);
	
	
	//hudtest->samples[0] = malloc(4 * 8192);
	//hudtest->samples[1] = malloc(4 * 8192);
	
	//audio_api = PULSE;
	//audio = kradaudio_create("Krad EBML Video HUD Test Player", audio_api);
	
	kradgui = kradgui_create(hud_width, hud_height);

	kradgui_add_item(kradgui, REEL_TO_REEL);
	//kradgui_add_item(kradgui, PLAYBACK_STATE_STATUS);

	buffer = malloc(6000000);
	header = malloc(1000000);
	tracks = malloc(1000000);
	//krad_dirac = krad_dirac_decoder_create();
	//krad_vpx_decoder = krad_vpx_decoder_create();
	krad_ebml = kradebml_create();
	//krad_flac = krad_flac_decoder_create();
	kradebml_open_input_file(krad_ebml, argv[1]);
	//kradebml_open_input_stream(krad_ebml_player->ebml, "192.168.1.2", 9080, "/teststream.krado");
	
	kradebml_debug(krad_ebml);

	kradgui_set_total_track_time_ms(kradgui, (krad_ebml->duration / 1e9) * 1000);
	kradgui->render_timecode = 1;
	
	video_codec = krad_ebml_track_codec(krad_ebml, 0);
	audio_codec = krad_ebml_track_codec(krad_ebml, 1);
	
	printf("video codec is %d and audio codec is %d\n", video_codec, audio_codec);
	
	//if (audio_codec == KRAD_FLAC) {
	//	bytes = kradebml_read_audio_header(krad_ebml, 1, buffer);
	//	printf("got flac header bytes of %d\n", bytes);
	//	//exit(1);
	//	krad_flac_decode(krad_flac, buffer, bytes, audio_data);
	//}
	
	//if (audio_codec == KRAD_VORBIS) {
	//	krad_vorbis = krad_vorbis_decoder_create(krad_ebml->vorbis_header1, krad_ebml->vorbis_header1_len, krad_ebml->vorbis_header2, krad_ebml->vorbis_header2_len, krad_ebml->vorbis_header3, krad_ebml->vorbis_header3_len);
	//}

	//krad_opengl_display = krad_sdl_opengl_display_create(1920, 1080, krad_ebml->vparams.width, krad_ebml->vparams.height);
	krad_opengl_display = krad_sdl_opengl_display_create(krad_ebml->vparams.width, krad_ebml->vparams.height, krad_ebml->vparams.width, krad_ebml->vparams.height);
	
	krad_opengl_display->hud_width = hud_width;
	krad_opengl_display->hud_height = hud_height;
	krad_opengl_display->hud_data = hud_data;
	
	hudtest->audio = audio;
	hudtest->krad_vorbis = krad_vorbis;
	hudtest->kradgui = kradgui;
	
	//if (audio_codec == KRAD_VORBIS) {
	//	kradaudio_set_process_callback(audio, krad_ebml_video_player_hud_test_audio_callback, hudtest);
	//}	
		
	//if (audio_api == JACK) {
	//	krad_jack_t *jack = (krad_jack_t *)audio->api;
	//	kradjack_connect_port(jack->jack_client, "Krad EBML Video HUD Test Player:Left", "desktop_recorder:input_1");
	//	kradjack_connect_port(jack->jack_client, "Krad EBML Video HUD Test Player:Right", "desktop_recorder:input_2");
	//}
	
	kradgui_reset_elapsed_time(kradgui);
	
	printf("\n\n");
	
	fd = open(argv[1], O_RDONLY);
	
	fd2 = open("/home/oneman/Videos/dumptest.mkv", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	krad_ebml_output = kradebml_create();
	kradebml_open_output_stream(krad_ebml_output, "192.168.1.2", 9080, "/teststream.webm", "secretkode");
	kradebml_header(krad_ebml_output, "matroska", APPVERSION);
	header_pos = krad_ebml_output->ebml->buffer_pos;
	memcpy(header, krad_ebml_output->ebml->buffer, header_pos);
	
	
	write(fd2, header, header_pos);
	
	kradebml_clone_tracks(krad_ebml, krad_ebml_output);
	tracks_pos = krad_ebml_output->ebml->buffer_pos - header_pos;
	memcpy(tracks, krad_ebml_output->ebml->buffer + header_pos, tracks_pos);
	write(fd2, tracks, tracks_pos);
	
	close(fd2);
	
	server_send(krad_ebml_output->server, header, header_pos);
	server_send(krad_ebml_output->server, tracks, tracks_pos);
	
	//byte_position = 3739;
	
	read(fd, buffer, 20000);
	
	byte_position = krad_ebml_find_first_cluster(buffer, 20000);
	
	printf("got cluster pos! %zu\n", byte_position);
	
	lseek(fd, byte_position, SEEK_SET);
	
	while (nestegg_read_packet(krad_ebml->ctx, &krad_ebml->pkt) > 0) {
		
		nestegg_packet_track(krad_ebml->pkt, &krad_ebml->pkt_track);
		nestegg_packet_count(krad_ebml->pkt, &krad_ebml->pkt_cnt);
		nestegg_packet_tstamp(krad_ebml->pkt, &krad_ebml->pkt_tstamp);

		packet_time_ms = (krad_ebml->pkt_tstamp / 1e9) * 1000;
		
		packet_time.tv_sec = (packet_time_ms - (packet_time_ms % 1000)) / 1000;
		packet_time.tv_nsec = (packet_time_ms % 1000) * 1000000;
		
		//if (krad_ebml->pkt_track == krad_ebml->video_track) {

			//if (packet_time_ms < last_packet_time_ms) {
			//	nosleep = true;
			//} else {
				nosleep = false;
			//	last_packet_time_ms = packet_time_ms;
			//}

			printf("packet time ms %d :: %ld : %ld\r", packet_time_ms % 1000, packet_time.tv_sec, packet_time.tv_nsec);
			fflush(stdout);
			kradgui_set_current_track_time_ms(kradgui, packet_time_ms);
		
			video_packets++;
			/*
			for (i = 0; i < krad_ebml->pkt_cnt; ++i) {

				nestegg_packet_data(krad_ebml->pkt, i, &buffer, &krad_ebml->size);

				if (video_codec == KRAD_VP8) {

					krad_vpx_decoder_decode(krad_vpx_decoder, buffer, krad_ebml->size);
				
					if (krad_vpx_decoder->img != NULL) {

						//printf("vpx img: %d %d %d\n", krad_vpx_decoder->img->stride[0],  krad_vpx_decoder->img->stride[1],  krad_vpx_decoder->img->stride[2]); 
						//cr = cairo_create(cst);
						//kradgui->cr = cr;
						//kradgui_render(kradgui);
						//cairo_destroy(cr);

						//krad_sdl_opengl_display_hud_item(krad_opengl_display, hud_data);
						krad_sdl_opengl_display_render(krad_opengl_display, krad_vpx_decoder->img->planes[0], krad_vpx_decoder->img->stride[0], krad_vpx_decoder->img->planes[1], krad_vpx_decoder->img->stride[1], krad_vpx_decoder->img->planes[2], krad_vpx_decoder->img->stride[2]);

						//usleep(13000);
					}
				} else {
				
					//printf("dirac packet size %zu\n", krad_ebml->size);		

					krad_dirac_decode(krad_dirac, buffer, krad_ebml->size);

					if ((krad_dirac->format != NULL) && (dirac_output_unset == true)) {
						switch (krad_dirac->format->chroma_format) {
	
							case SCHRO_CHROMA_444:
								krad_sdl_opengl_display_set_input_format(krad_opengl_display, PIX_FMT_YUV444P);
							case SCHRO_CHROMA_422:
								krad_sdl_opengl_display_set_input_format(krad_opengl_display, PIX_FMT_YUV422P);
							case SCHRO_CHROMA_420:
								break;
								// default
								//krad_sdl_opengl_display_set_input_format(krad_sdl_opengl_display, PIX_FMT_YUV420P);

						}
				
						dirac_output_unset = false;				
					}

					if (krad_dirac->frame != NULL) {
						krad_sdl_opengl_display_render(krad_opengl_display, krad_dirac->frame->components[0].data, krad_dirac->frame->components[0].stride, krad_dirac->frame->components[1].data, krad_dirac->frame->components[1].stride, krad_dirac->frame->components[2].data, krad_dirac->frame->components[2].stride);
					}

				}

				*/
				
				
				to_read = krad_ebml->total_bytes_read - byte_position;
				if (to_read > 0) {
					ret = read(fd, buffer, to_read);
					printf("\ngoing to send out %d bytes yaa %d\n", to_read, ret);
					server_send(krad_ebml_output->server, buffer, ret);
					byte_position += ret;

				}
			
				cr = cairo_create(cst);
				kradgui->cr = cr;
				kradgui_render(kradgui);
				cairo_destroy(cr);

				if (nosleep == false) {
					//printf("hi\n");
					kradgui_update_elapsed_time(kradgui);
					sleep_time = timespec_diff(kradgui->elapsed_time, packet_time);

//					if (sleep_time.tv_nsec > 16600000 * 1) {
						//sleep_time.tv_nsec -= 16600000 * 1;
						//printf("\nsleep time nsec is %ld\n", sleep_time.tv_nsec);
						nanosleep(&sleep_time, NULL);
	//				}
				}

				krad_sdl_opengl_draw_screen( krad_opengl_display );
//				nanosleep(&sleep_time, NULL);
				
				
			//}
		//}
		
		/*
		if (krad_ebml->pkt_track == krad_ebml->audio_track) {
			audio_packets++;

			nestegg_packet_data(krad_ebml->pkt, 0, &buffer, &krad_ebml->size);
			//printf("\nAudio packet! %zu bytes\n", krad_ebml->size);

			if (audio_codec == KRAD_VORBIS) {
				krad_vorbis_decoder_decode(krad_vorbis, buffer, krad_ebml->size);
			}
			
			if (audio_codec == KRAD_FLAC) {
				audio_frames = krad_flac_decode(krad_flac, buffer, krad_ebml->size, audio_data);
				kradaudio_write (audio, 0, (char *)audio_data, audio_frames * 4 );
				kradaudio_write (audio, 1, (char *)audio_data, audio_frames * 4 );
			}
		}
		
		if (krad_ebml->pkt_track == krad_ebml->video_track) {
			printf("Timecode: %f :: Video Packets %d Audio Packets: %d\r", krad_ebml->pkt_tstamp / 1e9, video_packets, audio_packets);
			fflush(stdout);
		}
		*/
		nestegg_free_packet(krad_ebml->pkt);



		if ( SDL_PollEvent( &event ) ){
			switch( event.type ){
				/* Look for a keypress */
				case SDL_KEYDOWN:
				    /* Check the SDLKey values and move change the coords */
				    switch( event.key.keysym.sym ){
				        case SDLK_LEFT:
				            break;
				        case SDLK_RIGHT:
				            break;
				        case SDLK_UP:
				            break;
				        case SDLK_q:
				        	shutdown = 1;
				            break;
				        case SDLK_f:
				        	
				            break;
				        default:
				            break;
				    }
				    break;

				case SDL_KEYUP:
				    switch( event.key.keysym.sym ) {
				        case SDLK_LEFT:
				            break;
				        case SDLK_RIGHT:
				            break;
				        case SDLK_UP:
				            break;
				        case SDLK_DOWN:
				            break;
				        default:
				            break;
				    }
				    break;
				
				default:
				    break;
			}
		}

		if (shutdown == 1) {
			break;
		}

    
	}
	
	printf("dunzo\n");
	close(fd);
	printf("\n");
	kradebml_destroy(krad_ebml_output);
	//krad_vpx_decoder_destroy(krad_vpx_decoder);
	//krad_dirac_decoder_destroy(krad_dirac);
	kradebml_destroy(krad_ebml);
	krad_sdl_opengl_display_destroy(krad_opengl_display);
	kradgui_destroy(kradgui);

	// must be before vorbis
	//kradaudio_destroy(audio);

	//if (audio_codec == KRAD_VORBIS) {
	//	krad_vorbis_decoder_destroy(krad_vorbis);
	//}
	
	//krad_flac_decoder_info(krad_flac);
	//krad_flac_decoder_destroy(krad_flac);
	
	free(buffer);
	free(header);
	free(tracks);	
	cairo_surface_destroy(cst);
	
	//free(hudtest->samples[0]);
	//free(hudtest->samples[1]);
	
	free(hudtest);
	//free(audio_data);
	return 0;

}
