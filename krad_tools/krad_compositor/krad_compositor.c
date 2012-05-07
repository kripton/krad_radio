#include "krad_compositor.h"






void krad_compositor_process (krad_compositor_t *krad_compositor) {

	int c;	
	int p;
	
	krad_frame_t *krad_frame;
	
	krad_frame = NULL;
	
	/*
	kradgui_remove_bug (krad_link->krad_gui);
	kradgui_set_bug (krad_link->krad_gui, krad_link->bug, krad_link->bug_x, krad_link->bug_y);
	krad_link->krad_gui->live = 0;
	krad_link->krad_gui->live = 1;

	krad_link->krad_gui->bug_alpha += 0.1f;
	if (krad_link->krad_gui->bug_alpha > 1.0f) {
		krad_link->krad_gui->bug_alpha = 1.0f;
	}

	krad_link->krad_gui->bug_alpha -= 0.1f;
	if (krad_link->krad_gui->bug_alpha < 0.1f) {
		krad_link->krad_gui->bug_alpha = 0.1f;
	}

	krad_link->krad_gui->render_ftest = 1;

	krad_link->render_meters = 0;

	krad_link->render_meters = 1;
	*/

	/*

	if (krad_link->operation_mode == RECEIVE) {
		
		krad_link->krad_gui->clear = 0;
		krad_link->new_capture_frame = 1;
		
		kradgui_update_elapsed_time(krad_link->krad_gui);

		if (krad_ringbuffer_read_space(krad_link->decoded_frames_buffer) >= krad_link->composited_frame_byte_size + 8) {
	
			if (krad_link->current_frame_timecode == 0) {
				krad_ringbuffer_read(krad_link->decoded_frames_buffer, 
									 (char *)&krad_link->current_frame_timecode, 
									 8 );
									 
				if (krad_link->current_frame_timecode == 0) {
					//first frame kludge
					krad_link->current_frame_timecode = 1;
				}
									 
			}
			
			if (krad_link->current_frame_timecode != 0) {
			
				if (krad_link->current_frame_timecode <= krad_link->krad_gui->elapsed_time_ms + 18) {
					krad_ringbuffer_read(krad_link->decoded_frames_buffer, 
										 (char *)krad_link->current_frame, 
										 krad_link->composited_frame_byte_size );
								 
					//printf("Current Frame Timecode: %zu Current Elapsed Time %u \r", krad_link->current_frame_timecode, krad_link->krad_gui->elapsed_time_ms);
					//fflush(stdout);
					krad_link->current_frame_timecode = 0;
				}
			}
		
		}

		memcpy( krad_link->krad_gui->data, krad_link->current_frame, krad_link->krad_gui->bytes );
	
	}
	
	*/
	
	/*
		
	if (krad_link->video_source == X11) {
		//kludgey
		krad_link->krad_gui->frame++;
	
		krad_x11_capture(krad_link->krad_x11, (unsigned char *)krad_link->krad_gui->data);
	} else {
		kradgui_render( krad_compositor->krad_gui );
	}
	
	*/
	
	/*
	
	if (krad_link->render_meters) {
		if ((krad_link->new_capture_frame == 1) || (krad_link->operation_mode == RECEIVE)) {
			
			if (krad_link->krad_audio != NULL) {
				for (c = 0; c < krad_link->audio_channels; c++) {
				
					if (krad_link->operation_mode == CAPTURE) {
						krad_link->temp_peak = read_peak(krad_link->krad_audio, KINPUT, c);
					} else {
						krad_link->temp_peak = read_peak(krad_link->krad_audio, KOUTPUT, c);
					}
				
					if (krad_link->temp_peak >= krad_link->krad_gui->output_peak[c]) {
						if (krad_link->temp_peak > 2.7f) {
							krad_link->krad_gui->output_peak[c] = krad_link->temp_peak;
							krad_link->kick = ((krad_link->krad_gui->output_peak[c] - krad_link->krad_gui->output_current[c]) / 300.0);
						}
					} else {
						if (krad_link->krad_gui->output_peak[c] == krad_link->krad_gui->output_current[c]) {
							krad_link->krad_gui->output_peak[c] -= (0.9 * (60/krad_link->capture_fps));
							if (krad_link->krad_gui->output_peak[c] < 0.0f) {
								krad_link->krad_gui->output_peak[c] = 0.0f;
							}
							krad_link->krad_gui->output_current[c] = krad_link->krad_gui->output_peak[c];
						}
					}
			
					if (krad_link->krad_gui->output_peak[c] > krad_link->krad_gui->output_current[c]) {
						krad_link->krad_gui->output_current[c] = (krad_link->krad_gui->output_current[c] + 1.4) * (1.3 + krad_link->kick); ;
					}
		
					if (krad_link->krad_gui->output_peak[c] < krad_link->krad_gui->output_current[c]) {
						krad_link->krad_gui->output_current[c] = krad_link->krad_gui->output_peak[c];
					}
			
			
				}
			}
			
		}

		kradgui_render_meter (krad_link->krad_gui, 110, krad_link->composite_height - 30, 96, krad_link->krad_gui->output_current[0]);
		kradgui_render_meter (krad_link->krad_gui, krad_link->composite_width - 110, krad_link->composite_height - 30, 96, krad_link->krad_gui->output_current[1]);
	}

	*/
	
	/*
	if (krad_ringbuffer_read_space (krad_compositor->incoming_frames_buffer) >= krad_compositor->frame_byte_size) {

		krad_ringbuffer_read (krad_compositor->incoming_frames_buffer, 
							 (char *)krad_compositor->krad_gui->data, 
							 krad_compositor->frame_byte_size );

	}
	
	*/
	
	krad_compositor->krad_gui->clear = 0;

	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].active == 1) && (krad_compositor->port[p].direction == INPUT)) {
			krad_frame = krad_compositor_port_pull_frame (&krad_compositor->port[p]);
			
			if (krad_frame != NULL) {
				memcpy ( krad_compositor->krad_gui->data, krad_frame->pixels, krad_compositor->frame_byte_size );
				krad_framepool_unref_frame (krad_frame);
			}
			break;
		}
	}
	
	
	kradgui_render ( krad_compositor->krad_gui );
	
	if (krad_compositor->hex_size > 0) {
		kradgui_render_hex (krad_compositor->krad_gui, krad_compositor->hex_x, krad_compositor->hex_y, krad_compositor->hex_size);	
	}
	
	krad_frame = krad_framepool_getframe (krad_compositor->krad_framepool);
	
	memcpy ( krad_frame->pixels, krad_compositor->krad_gui->data, krad_compositor->frame_byte_size );	
	
	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if ((krad_compositor->port[p].active == 1) && (krad_compositor->port[p].direction == OUTPUT)) {
			krad_compositor_port_push_frame (&krad_compositor->port[p], krad_frame);
			break;
		}
	}
	
	krad_framepool_unref_frame (krad_frame);	
	
	/*
	if (krad_ringbuffer_write_space (krad_compositor->composited_frames_buffer) >= krad_compositor->frame_byte_size) {

				krad_ringbuffer_write(krad_compositor->composited_frames_buffer, 
									  (char *)krad_compositor->krad_gui->data, 
									  krad_compositor->frame_byte_size );

	} else {
		dbg("Krad Compositor output full! encoding to slow! overflow! :(\n");
	}
	*/	
	
}

void krad_compositor_port_push_frame (krad_compositor_port_t *krad_compositor_port, krad_frame_t *krad_frame) {

	krad_framepool_ref_frame (krad_frame);
	
	krad_ringbuffer_write (krad_compositor_port->frame_ring, (char *)krad_frame, 4);
	
	//krad_compositor_port->krad_frame = krad_frame;

}


krad_frame_t *krad_compositor_port_pull_frame (krad_compositor_port_t *krad_compositor_port) {

	krad_frame_t *krad_frame;
	
	if (krad_ringbuffer_read_space (krad_compositor_port->frame_ring) >= 4) {
		krad_ringbuffer_read (krad_compositor_port->frame_ring, (char *)&krad_frame, 4);
		return krad_frame;
	}
	//krad_compositor_port->krad_frame = krad_frame;

	return NULL;

}

krad_compositor_port_t *krad_compositor_port_create (krad_compositor_t *krad_compositor, char *sysname, int direction) {

	krad_compositor_port_t *krad_compositor_port;

	int p;

	for (p = 0; p < KRAD_COMPOSITOR_MAX_PORTS; p++) {
		if (krad_compositor->port[p].active == 0) {
			krad_compositor_port = &krad_compositor->port[p];
			break;
		}
	}
	
	if (krad_compositor_port == NULL) {
		return NULL;
	}
	
	krad_compositor_port->krad_compositor = krad_compositor;
	
	strcpy (krad_compositor_port->sysname, sysname);	
	
	krad_compositor_port->direction = direction;	
	
	krad_compositor_port->active = 1;	
	
	krad_compositor_port->frame_ring = krad_ringbuffer_create ( 128 * 4 );	
	
	return krad_compositor_port;

}


void krad_compositor_port_destroy (krad_compositor_t *krad_compositor, krad_compositor_port_t *krad_compositor_port) {

	krad_ringbuffer_free ( krad_compositor_port->frame_ring );

	krad_compositor_port->active = 0;

}

void krad_compositor_destroy (krad_compositor_t *krad_compositor) {

	krad_ringbuffer_free ( krad_compositor->composited_frames_buffer );

	kradgui_destroy (krad_compositor->krad_gui);

	free (krad_compositor->port);

	free (krad_compositor);

}

krad_compositor_t *krad_compositor_create (int width, int height) {

	krad_compositor_t *krad_compositor = calloc(1, sizeof(krad_compositor_t));

	krad_compositor->width = width;
	krad_compositor->height = height;

	krad_compositor->frame_byte_size = krad_compositor->width * krad_compositor->height * 4;

	krad_compositor->port = calloc(KRAD_COMPOSITOR_MAX_PORTS, sizeof(krad_compositor_port_t));

	krad_compositor->composited_frames_buffer = krad_ringbuffer_create (krad_compositor->frame_byte_size * DEFAULT_COMPOSITOR_BUFFER_FRAMES);
	
	krad_compositor->krad_framepool = krad_framepool_create ( krad_compositor->width, krad_compositor->height, DEFAULT_COMPOSITOR_BUFFER_FRAMES);
	
	krad_compositor->krad_gui = kradgui_create_with_internal_surface (krad_compositor->width, krad_compositor->height);	
	
	krad_compositor->hex_x = 150;
	krad_compositor->hex_y = 100;
	krad_compositor->hex_size = 33;
	
	return krad_compositor;

}


int krad_compositor_handler ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc ) {


	uint32_t ebml_id;
	
	uint32_t command;
	uint64_t ebml_data_size;

	uint64_t element;
	uint64_t response;
	
	char linkname[64];	
	float floatval;
	
	uint64_t bigint;
	uint8_t tinyint;
	int k;
	
	bigint = 0;
	k = 0;
	floatval = 0;

	krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

	switch ( command ) {

		case EBML_ID_KRAD_COMPOSITOR_CMD_HEX_DEMO:

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_HEX_X) {
				printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->hex_x = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_HEX_Y) {
				printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->hex_y = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
			
			krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_COMPOSITOR_HEX_SIZE) {
				printf("hrm wtf3\n");
			} else {
				//printf("tag value size %zu\n", ebml_data_size);
			}

			krad_compositor->hex_size = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);			

			/*
			krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_LINK_MSG, &response);
			krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_LINK_LINK_LIST, &element);	
			
			for (k = 0; k < KRAD_LINKER_MAX_LINKS; k++) {
				if (krad_linker->krad_link[k] != NULL) {
					printf("Link %d Active: %s\n", k, krad_linker->krad_link[k]->mount);
					krad_linker_link_to_ebml ( krad_ipc, krad_linker->krad_link[k]);
				}
			}
			
			krad_ipc_server_response_list_finish ( krad_ipc, element );
			krad_ipc_server_response_finish ( krad_ipc, response );	
			*/		
			break;
	}

	return 0;
}

