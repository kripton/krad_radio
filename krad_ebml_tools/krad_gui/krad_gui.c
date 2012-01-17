#include <cairo.h>
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>  
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "krad_gui.h"

kradgui_t *kradgui_create(int width, int height) {

	kradgui_t *kradgui;

	if ((kradgui = calloc (1, sizeof (kradgui_t))) == NULL) {
		fprintf(stderr, "kradgui mem alloc fail\n");
		exit (1);
	}
	
	kradgui->width = width;
	kradgui->height = height;
	kradgui_set_playback_state(kradgui, KRADGUI_PLAYING);
	kradgui_reset_elapsed_time(kradgui);
	kradgui_set_total_track_time_ms(kradgui, 45 * 60 * 1000);
	
	clock_gettime(CLOCK_REALTIME, &kradgui->start_time);
	
	return kradgui;

}

void kradgui_set_size(kradgui_t *kradgui, int width, int height) {

	kradgui->width = width;
	kradgui->height = height;

}


void kradgui_set_background_color(kradgui_t *kradgui, float r, float g, float b, float a) {


	//BGCOLOR_TRANS


}

void kradgui_destroy(kradgui_t *kradgui) {

	if (kradgui->reel_to_reel != NULL) {
		kradgui_reel_to_reel_destroy(kradgui->reel_to_reel);
	}
	
	if (kradgui->playback_state_status != NULL) {
		kradgui_playback_state_status_destroy(kradgui->playback_state_status);
	}

	free(kradgui);

}

void kradgui_add_item(kradgui_t *kradgui, kradgui_item_t item) {

	switch (item) {
	
	case REEL_TO_REEL:
		kradgui->reel_to_reel = kradgui_reel_to_reel_create(kradgui);
		break;

	case PLAYBACK_STATE_STATUS:
		kradgui->playback_state_status = kradgui_playback_state_status_create(kradgui);
		break;
	}

}

void kradgui_remove_item(kradgui_t *kradgui, kradgui_item_t item) {

	switch (item) {
	
	case REEL_TO_REEL:
		kradgui_reel_to_reel_destroy(kradgui->reel_to_reel);
		kradgui->reel_to_reel = NULL;
		break;
	
	case PLAYBACK_STATE_STATUS:
		kradgui_playback_state_status_destroy(kradgui->playback_state_status);
		kradgui->playback_state_status = NULL;
		break;
	}

}


kradgui_reel_to_reel_t *kradgui_reel_to_reel_create(kradgui_t *kradgui) {

	kradgui_reel_to_reel_t *kradgui_reel_to_reel;

	if ((kradgui_reel_to_reel = calloc (1, sizeof (kradgui_reel_to_reel_t))) == NULL) {
		fprintf(stderr, "kradgui kradgui reel to reel mem alloc fail\n");
		exit (1);
	}
	
	kradgui_reel_to_reel->kradgui = kradgui;
	
	kradgui_reel_to_reel->reel_size = NORMAL_REEL_SIZE;
	kradgui_reel_to_reel->reel_speed = SLOW_NORMAL_REEL_SPEED;
	//kradgui->reel_size = PRO_REEL_SIZE;
	//kradgui->reel_speed = PRO_REEL_SPEED;
	
	kradgui_update_reel_to_reel_information(kradgui_reel_to_reel);
	
	return kradgui_reel_to_reel;

}


void kradgui_reel_to_reel_destroy(kradgui_reel_to_reel_t *kradgui_reel_to_reel) {

	free(kradgui_reel_to_reel);

}

kradgui_playback_state_status_t *kradgui_playback_state_status_create(kradgui_t *kradgui) {

	kradgui_playback_state_status_t *kradgui_playback_state_status;

	if ((kradgui_playback_state_status = calloc (1, sizeof (kradgui_playback_state_status_t))) == NULL) {
		fprintf(stderr, "kradgui kradgui reel to reel mem alloc fail\n");
		exit (1);
	}
	
	kradgui_playback_state_status->kradgui = kradgui;

	return kradgui_playback_state_status;

}

void kradgui_playback_state_status_destroy(kradgui_playback_state_status_t *kradgui_playback_state_status) {

	free(kradgui_playback_state_status);	

}


void kradgui_render_playback_state_status(kradgui_playback_state_status_t *kradgui_playback_state_status) {

	cairo_t *cr;
	cr = kradgui_playback_state_status->kradgui->cr;
	
	cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 20.0);
	
	switch (kradgui_playback_state_status->kradgui->playback_state) {
		case KRADGUI_PLAYING:
			cairo_set_source_rgb (cr, GREEN);
			break;
			
		case KRADGUI_PAUSED:
			cairo_set_source_rgb (cr, BLUE);
			break;
			
		case KRADGUI_STOPPED:
			cairo_set_source_rgb (cr, GREY);
			break;
	}
	
	cairo_move_to (cr, kradgui_playback_state_status->kradgui->width/8.0 * 6, kradgui_playback_state_status->kradgui->height - 232.0);
	
	cairo_show_text (cr, kradgui_playback_state_status->kradgui->playback_state_status_string);

}

void kradgui_set_playback_state(kradgui_t *kradgui, kradgui_playback_state_t playback_state) {

	kradgui->playback_state = playback_state;
	kradgui_update_playback_state_status_string(kradgui);

}


void kradgui_control_speed_up(kradgui_t *kradgui) {

	if (kradgui->control_speed_up_callback != NULL) {
		kradgui->control_speed_up_callback(kradgui->callback_pointer);
	}


}

void kradgui_control_speed_down(kradgui_t *kradgui) {

	if (kradgui->control_speed_down_callback != NULL) {
		kradgui->control_speed_down_callback(kradgui->callback_pointer);
	}

}

void kradgui_control_speed(kradgui_t *kradgui, float value) {

	if (kradgui->control_speed_callback != NULL) {
		kradgui->control_speed_callback(kradgui->callback_pointer, value);
	}

}


void kradgui_set_control_speed_callback(kradgui_t *kradgui, void control_speed_callback(void *, float)) {

	kradgui->control_speed_callback = control_speed_callback;


}

void kradgui_set_control_speed_down_callback(kradgui_t *kradgui, void control_speed_down_callback(void *)) {

	kradgui->control_speed_down_callback = control_speed_down_callback;


}

void kradgui_set_control_speed_up_callback(kradgui_t *kradgui, void control_speed_up_callback(void *)) {

	kradgui->control_speed_up_callback = control_speed_up_callback;


}

void kradgui_set_callback_pointer(kradgui_t *kradgui, void *callback_pointer) {

	kradgui->callback_pointer = callback_pointer;


}


void kradgui_update_reel_to_reel_information(kradgui_reel_to_reel_t *kradgui_reel_to_reel) {

	// calc rotation example
	// determine rotation in degrees per millisecond
	// pro is 0.0381cm per millisecond 38.1 / 1000
	// 1 degree in pro is 0.074083333cm 26.67 / 360 
	// time ms * 0.381 = distance
	// distance / 0.74083333 = rotation
	
	kradgui_reel_to_reel->distance = kradgui_reel_to_reel->kradgui->current_track_time_ms * (kradgui_reel_to_reel->reel_speed / 1000);
	kradgui_reel_to_reel->total_distance = kradgui_reel_to_reel->kradgui->total_track_time_ms * (kradgui_reel_to_reel->reel_speed / 1000);
	if (kradgui_reel_to_reel->distance >= kradgui_reel_to_reel->total_distance) {
		kradgui_reel_to_reel->distance = kradgui_reel_to_reel->total_distance;
	}
	
	kradgui_reel_to_reel->angle = (unsigned int)(kradgui_reel_to_reel->distance / (kradgui_reel_to_reel->reel_size / 360)) % 360;
	
	//printf("MS: %zu Reel Size: %fcm Reel Speed: %fcm/s Distance: %fcm Rotation: %fdegrees\n", kradgui_reel_to_reel->kradgui->current_track_time_ms, kradgui_reel_to_reel->reel_size, kradgui_reel_to_reel->reel_speed, kradgui_reel_to_reel->distance, kradgui_reel_to_reel->angle);
	
}

void kradgui_render_reel(kradgui_reel_to_reel_t *kradgui_reel_to_reel, int x, int y) {


	cairo_t *cr;
	int rotation_offset;
	float reel_inner_diameter;
	float reel_diameter;
	float tape_on_dest_reel;
	float tape_on_source_reel;
	float max_tape_on_reel;
	//float min_tape_on_reel;
	float middle_of_tape_on_dest_reel;
	float middle_of_tape_on_source_reel;
	float tape_exit_point;
	float tape_entrance_point;
	
	cr = kradgui_reel_to_reel->kradgui->cr;
	
	reel_inner_diameter = 13;
	reel_diameter = 45;
	
	max_tape_on_reel = reel_diameter - 5 - (reel_inner_diameter + 2);
	//min_tape_on_reel = 2 + reel_inner_diameter;
	
	tape_on_dest_reel = (kradgui_reel_to_reel->kradgui->current_track_progress * 0.01) * max_tape_on_reel;
	tape_on_source_reel = ((100.0 - kradgui_reel_to_reel->kradgui->current_track_progress) * 0.01) * max_tape_on_reel;
	middle_of_tape_on_source_reel = reel_inner_diameter + (tape_on_source_reel / 2) + 2;
	middle_of_tape_on_dest_reel = reel_inner_diameter + (tape_on_dest_reel / 2) + 2;
	tape_exit_point = reel_inner_diameter + tape_on_source_reel;
	tape_entrance_point = reel_inner_diameter + tape_on_dest_reel;

	// this is so the reels don't appear identical
	rotation_offset = y;

	// reel
	cairo_set_line_width(cr, 1.5);
	cairo_set_source_rgb(cr, WHITE);
	cairo_save(cr);
	cairo_translate (cr, x, y);
	cairo_save(cr);
	cairo_arc (cr, 0.0, 0.0, 3.0, 0, 2*M_PI);
	cairo_stroke(cr);
	cairo_arc (cr, 0.0, 0.0, 10.0, 0, 2*M_PI);
	cairo_stroke(cr);
	cairo_arc (cr, 0.0, 0.0, reel_inner_diameter, 0, 2*M_PI);
	cairo_stroke(cr);
	cairo_arc(cr, 0.0, 0.0, reel_diameter, 0, 2 * M_PI);
	cairo_stroke(cr);



	//tape on source reel
	if (x > 200) {
		
		cairo_set_source_rgb(cr, GREY3);
		cairo_set_line_width(cr, tape_on_source_reel);
		cairo_arc (cr, 0.0, 0.0, middle_of_tape_on_source_reel, 0, 2*M_PI);
		cairo_stroke(cr);
		cairo_set_line_width(cr, 2);
	}

	//tape on dest reel
	if (x < 200) {
		cairo_set_source_rgb(cr, GREY3);
		cairo_set_line_width(cr, tape_on_dest_reel);
		cairo_arc (cr, 0.0, 0.0, middle_of_tape_on_dest_reel, 0, 2*M_PI);
		cairo_stroke(cr);
		cairo_set_line_width(cr, 2);
	}
	
	// spokes
	cairo_set_source_rgba(cr, WHITE_TRANS);
	cairo_set_line_width(cr, 2);
	cairo_rotate (cr, (kradgui_reel_to_reel->angle + rotation_offset % 360) * (M_PI/180.0));
	cairo_rotate (cr, 120 * (M_PI/180.0));
	cairo_move_to(cr, 19.0,  0.0);
	cairo_line_to (cr, 37.0, 0.0);
	cairo_move_to(cr, 0.0, 0.0);
	cairo_stroke(cr);
	
	cairo_rotate (cr, 120 * (M_PI/180.0));
	cairo_move_to(cr, 19.0,  0.0);
	cairo_line_to (cr, 37.0, 0.0);
	cairo_move_to(cr, 0.0, 0.0);
	cairo_stroke(cr);

	cairo_rotate (cr, 120 * (M_PI/180.0));
	cairo_move_to(cr, 19.0,  0.0);
	cairo_line_to (cr, 37.0, 0.0);
	cairo_move_to(cr, 0.0, 0.0);
	cairo_stroke(cr);
	
	// tape transfer from reel to reel
	cairo_restore(cr);
	if ((x > 200) && (kradgui_reel_to_reel->kradgui->current_track_progress < 100.0)) {
		cairo_set_source_rgb(cr, GREY3);
		cairo_move_to (cr, tape_exit_point, 7);
		cairo_line_to (cr, 4, 63);
		cairo_stroke(cr);
		
		cairo_move_to (cr, 2, 63);
		cairo_line_to (cr, -24, 57);
		cairo_stroke(cr);
		

		cairo_move_to (cr, -25, 56);
		cairo_line_to (cr, -66, 67);
		cairo_stroke(cr);
	
		cairo_move_to (cr, -81, 67);
		cairo_line_to (cr, -121, 56);
		cairo_stroke(cr);

		cairo_move_to (cr, -122, 56);
		cairo_line_to (cr, -154, 63);
		cairo_stroke(cr);
			
		cairo_move_to (cr, -155, 62);
		cairo_line_to (cr, -153 - tape_entrance_point, 7);
		cairo_stroke(cr);

	}

	cairo_restore(cr);
	
}

void kradgui_render_reel_to_reel(kradgui_reel_to_reel_t *kradgui_reel_to_reel) {

	cairo_t *cr;
	
	cr = kradgui_reel_to_reel->kradgui->cr;

	// reels
	kradgui_render_reel(kradgui_reel_to_reel, 82, 57);
	kradgui_render_reel(kradgui_reel_to_reel, 235, 57);

	// roller wheels
	cairo_set_source_rgb (cr, WHITE);
	cairo_arc (cr, 236.0, 116.0, 4.0, 0, 2*M_PI);
	cairo_stroke(cr);

	cairo_arc (cr, 210.0, 121.0, 7.0, 0, 2*M_PI);
	cairo_stroke(cr);
	
	cairo_arc (cr, 111.0, 120.0, 7.0, 0, 2*M_PI);
	cairo_stroke(cr);
	cairo_arc (cr, 111.0, 120.0, 2.0, 0, 2*M_PI);
	cairo_fill(cr);
	
	
	cairo_arc (cr, 82.0, 116.0, 4.0, 0, 2*M_PI);
	cairo_stroke(cr);		

	// tape head	
	cairo_rectangle (cr, 154, 111, 14, 13);
	cairo_stroke(cr);
	
}

void kradgui_render_elapsed_timecode(kradgui_t *kradgui) {

	kradgui_update_elapsed_time(kradgui);

	cairo_select_font_face (kradgui->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (kradgui->cr, 22.0);
	cairo_set_source_rgb (kradgui->cr, ORANGE);
	
	cairo_move_to (kradgui->cr, kradgui->width/4.0, kradgui->height - 174.0);	
	cairo_show_text (kradgui->cr, "Elapsed Time");
	
	cairo_move_to (kradgui->cr, kradgui->width/4.0, kradgui->height - 131.0);
	cairo_show_text (kradgui->cr, kradgui->elapsed_time_timecode_string);
}

void kradgui_render_timecode(kradgui_t *kradgui) {

	cairo_select_font_face (kradgui->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (kradgui->cr, 22.0);
	cairo_set_source_rgb (kradgui->cr, ORANGE);

	cairo_move_to (kradgui->cr, kradgui->width/4.0, kradgui->height - 84.0);
	cairo_show_text (kradgui->cr, "Track Time");

	
	cairo_move_to (kradgui->cr, kradgui->width/4.0, kradgui->height - 40.0);
	cairo_show_text (kradgui->cr, kradgui->current_track_time_timecode_string);
}

void kradgui_render_rgb(kradgui_t *kradgui) {

	int size;
	int xoffset;
	int yoffset;
	
	size = kradgui->width / 7;
	xoffset = size;
	yoffset = kradgui->height / 7;
	
	cairo_set_source_rgb (kradgui->cr, 1.0, 0.0, 0.0);
	cairo_rectangle (kradgui->cr, xoffset, yoffset, size, size);
	cairo_fill (kradgui->cr);
	
	cairo_set_source_rgb (kradgui->cr, 0.0, 1.0, 0.0);
	cairo_rectangle (kradgui->cr, xoffset + size, yoffset, size, size);
	cairo_fill (kradgui->cr);
	
	cairo_set_source_rgb (kradgui->cr, 0.0, 0.0, 1.0);
	cairo_rectangle (kradgui->cr, xoffset + (size * 2), yoffset, size, size);
	cairo_fill (kradgui->cr);
		
}


void kradgui_render_tearbar(kradgui_t *kradgui) {

	int tearbar_width;
	int movement_range;
	int tearbar_position;
	
	if (kradgui->tearbar_speed == 0.0) {
		
		kradgui->tearbar_speed = 0.01;
		kradgui->tearbar_speed_adj = 0.01;
		
	}
	
	
	tearbar_width = kradgui->width / 15;
	movement_range = kradgui->width - tearbar_width;

	tearbar_position = 0 + (movement_range / 2) + round(movement_range * sin(kradgui->tearbar_positioner) / 2);

	//printf("position is: %d positioner is %f\n", kradbar->bars[i].position, kradbar->bars[i].positioner);

	kradgui->tearbar_positioner += kradgui->tearbar_speed;

	if (kradgui->tearbar_positioner > 2 * M_PI) {
		kradgui->tearbar_positioner = 0.0f;
		
		if (kradgui->tearbar_speed < 0.2) {
			kradgui->tearbar_speed += kradgui->tearbar_speed_adj;
		} else {
			kradgui->tearbar_speed_adj = -0.01;
			kradgui->tearbar_speed += kradgui->tearbar_speed_adj;
		}
	
		if (kradgui->tearbar_speed <= 0.0) {
			kradgui->tearbar_speed_adj = 0.01;
			kradgui->tearbar_speed += kradgui->tearbar_speed_adj;
		}
		
	}
	
	cairo_set_source_rgb (kradgui->cr, 1.0, 1.0, 1.0);
	cairo_rectangle (kradgui->cr, tearbar_position, 0, tearbar_width, kradgui->height);
	cairo_fill (kradgui->cr);

}


void kradgui_test_screen(kradgui_t *kradgui, char *info) {

	time_t t;

	t = time(0);
	sprintf(kradgui->test_start_time, "Test Started: %s", ctime(&t));
	kradgui->test_start_time[strlen(kradgui->test_start_time) - 1] = '\0';

	if (info != NULL) {
		strncpy(kradgui->test_info_text, info, 510);
	} else {
		strcpy(kradgui->test_info_text, "No Test Information");
	}
	
	
	kradgui_go_live(kradgui);
	kradgui->render_rgb = 1;
	kradgui->render_test_text = 1;
	kradgui->render_rotator = 1;

}

void kradgui_render_test_text(kradgui_t *kradgui) {

	time_t t;

	cairo_select_font_face (kradgui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (kradgui->cr, 62.0);
	cairo_set_source_rgb (kradgui->cr, 1.0, 0.1, 0.1);
	cairo_move_to (kradgui->cr, kradgui->width/10, kradgui->height - 360);
	cairo_show_text (kradgui->cr, "KRAD TEST SIGNAL");

	cairo_set_font_size (kradgui->cr, 20.0);
	cairo_set_source_rgb (kradgui->cr, 0.9, 0.9, 0.9);
	cairo_move_to (kradgui->cr, 50, kradgui->height - 280);
	cairo_show_text (kradgui->cr, kradgui->test_info_text);


	cairo_select_font_face (kradgui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_source_rgb (kradgui->cr, 0.8, 0.8, 0.8);
	cairo_set_font_size (kradgui->cr, 32.0);

	t = time(0);
	sprintf(kradgui->test_text_time, "%s", ctime(&t));
	kradgui->test_text_time[strlen(kradgui->test_text_time) - 1] = '\0';
	cairo_move_to (kradgui->cr, kradgui->width/10.0, kradgui->height - 84.0);
	cairo_show_text (kradgui->cr, kradgui->test_text_time);

	cairo_set_source_rgb (kradgui->cr, 0.4, 0.4, 0.4);
	cairo_set_font_size (kradgui->cr, 26.0);
	cairo_move_to (kradgui->cr, 44, 50);
	cairo_show_text (kradgui->cr, kradgui->test_start_time);

	cairo_set_source_rgb (kradgui->cr, 0.8, 0.8, 0.8);
	cairo_set_font_size (kradgui->cr, 32.0);
	sprintf(kradgui->test_text, "W: %d H: %d F: %llu", kradgui->width, kradgui->height, kradgui->frame);
	cairo_move_to (kradgui->cr, kradgui->width/2, kradgui->height - 84.0);
	cairo_show_text (kradgui->cr, kradgui->test_text);

}

void kradgui_render_rotator(kradgui_t *kradgui) {

	int width;
	
	
	width = 16;
	kradgui->rotator_speed = 2;
	
	
	cairo_set_source_rgb (kradgui->cr, 0.5, 0.5, 0.5);
	
	cairo_save (kradgui->cr);

	cairo_translate (kradgui->cr, kradgui->width/4 * 3, kradgui->height/5 * 3);
	cairo_rotate (kradgui->cr, (kradgui->rotator_angle % 360) * (M_PI/180.0));
	cairo_translate (kradgui->cr, width/2 * -1, (width * 16)/2 * -1);
	cairo_rectangle (kradgui->cr, 0, 0, width, width * 16);
	cairo_fill (kradgui->cr);
	cairo_restore (kradgui->cr);

	kradgui->rotator_angle += kradgui->rotator_speed;

}

void kradgui_render_recording(kradgui_t *kradgui) {

	kradgui->recording_elapsed_time = timespec_diff(kradgui->recording_start_time, kradgui->current_time);
	sprintf(kradgui->recording_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld", kradgui->recording_elapsed_time.tv_sec / 86400 % 365, kradgui->recording_elapsed_time.tv_sec / 3600 % 24, kradgui->recording_elapsed_time.tv_sec / 60 % 60, kradgui->recording_elapsed_time.tv_sec % 60, kradgui->recording_elapsed_time.tv_nsec / 1000000);

	cairo_set_source_rgba (kradgui->cr, 0.7, 0.0, 0.0, 0.3);
	cairo_rectangle (kradgui->cr, kradgui->recording_box_margin, kradgui->recording_box_margin, kradgui->recording_box_width, kradgui->recording_box_height);
	cairo_fill (kradgui->cr);

	cairo_select_font_face (kradgui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (kradgui->cr, kradgui->recording_box_font_size);
	cairo_set_source_rgb (kradgui->cr, 1.0, 0.0, 0.0);

	cairo_move_to (kradgui->cr, kradgui->recording_box_margin + kradgui->recording_box_padding,  kradgui->recording_box_margin + (kradgui->recording_box_height - kradgui->recording_box_padding));
	cairo_show_text (kradgui->cr, "RECORDING");


//	cairo_move_to (kradgui->cr, 50, 22);
//	cairo_show_text (kradgui->cr, kradgui->recording_time_timecode_string);

}

void kradgui_render_live(kradgui_t *kradgui) {

	kradgui->live_elapsed_time = timespec_diff(kradgui->live_start_time, kradgui->current_time);
	sprintf(kradgui->live_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld", kradgui->live_elapsed_time.tv_sec / 86400 % 365, kradgui->live_elapsed_time.tv_sec / 3600 % 24, kradgui->live_elapsed_time.tv_sec / 60 % 60, kradgui->live_elapsed_time.tv_sec % 60, kradgui->live_elapsed_time.tv_nsec / 1000000);

	cairo_set_source_rgba (kradgui->cr, 0.0, 0.5, 0.0, 0.1);
	cairo_rectangle (kradgui->cr, kradgui->width - (kradgui->live_box_margin + kradgui->live_box_width), kradgui->live_box_margin, kradgui->live_box_width, kradgui->live_box_height);
	cairo_fill (kradgui->cr);

	cairo_select_font_face (kradgui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (kradgui->cr, kradgui->live_box_font_size);
	cairo_set_source_rgb (kradgui->cr, GREEN);

	cairo_move_to (kradgui->cr, kradgui->width - (kradgui->live_box_width + kradgui->live_box_margin) + kradgui->live_box_padding,  kradgui->live_box_margin + (kradgui->live_box_height - kradgui->live_box_padding));
	cairo_show_text (kradgui->cr, "LIVE");

	cairo_set_font_size (kradgui->cr, kradgui->live_box_font_size / 3);
	cairo_move_to (kradgui->cr, kradgui->width - ((kradgui->live_box_width + kradgui->live_box_margin) + kradgui->live_box_margin),  kradgui->live_box_height + (kradgui->live_box_padding + kradgui->live_box_margin * 2));
	cairo_show_text (kradgui->cr, kradgui->live_time_timecode_string);

}

void kradgui_go_live(kradgui_t *kradgui) {

	clock_gettime(CLOCK_REALTIME, &kradgui->live_start_time);
	
	kradgui->live_box_width = 270;
	kradgui->live_box_height = 114;
	kradgui->live_box_margin = 44;
	kradgui->live_box_padding = (kradgui->live_box_height / 5);
	kradgui->live_box_font_size = (kradgui->live_box_height / 8) * 6.5;
	kradgui->live = 1;

}

void kradgui_go_off(kradgui_t *kradgui) {

	kradgui->live = 0;

}

void kradgui_start_recording(kradgui_t *kradgui) {

	clock_gettime(CLOCK_REALTIME, &kradgui->recording_start_time);
	
	kradgui->recording_box_width = 654;
	kradgui->recording_box_height = 114;
	kradgui->recording_box_margin = 44;
	kradgui->recording_box_padding = (kradgui->recording_box_height / 5);
	kradgui->recording_box_font_size = (kradgui->recording_box_height / 8) * 6.5;
	
	
	kradgui->recording = 1;

}

void kradgui_stop_recording(kradgui_t *kradgui) {

	kradgui->recording = 0;
	
}



void kradgui_update_current_track_progress(kradgui_t *kradgui) {
	
	//printf("%zums / %zums\n", kradgui->current_track_time_ms, kradgui->total_track_time_ms);
	
	if ((kradgui->total_track_time_ms != 0) && (kradgui->current_track_time_ms != 0)) {
		kradgui->current_track_progress = (kradgui->current_track_time_ms * 1000 / kradgui->total_track_time_ms) / 10.0;
	} else {
		kradgui->current_track_progress = 0;
	}
	
	if (kradgui->current_track_progress > 100.0) {
		kradgui->current_track_progress = 100.0;
	}
	
	if (kradgui->current_track_progress < 0.0) {
		kradgui->current_track_progress = 0.0;
	}
	
	//printf("Progress is: %f%%\n", kradgui->current_track_progress);
}

void kradgui_update_current_track_time_ms(kradgui_t *kradgui) {
	
	kradgui->current_track_time_ms = ((kradgui->current_track_time.tv_sec * 1000) + (kradgui->current_track_time.tv_nsec / 1000000));

}

void kradgui_update_total_track_time_ms(kradgui_t *kradgui) {
	
	kradgui->total_track_time_ms = ((kradgui->total_track_time.tv_sec * 1000) + (kradgui->total_track_time.tv_nsec / 1000000));

}

void kradgui_update_elapsed_time_ms(kradgui_t *kradgui) {
	
	kradgui->elapsed_time_ms = ((kradgui->elapsed_time.tv_sec * 1000) + (kradgui->elapsed_time.tv_nsec / 1000000));

}


void kradgui_update_current_track_time_timecode_string(kradgui_t *kradgui) {

	sprintf(kradgui->current_track_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld", kradgui->current_track_time.tv_sec / 86400 % 365, kradgui->current_track_time.tv_sec / 3600 % 24, kradgui->current_track_time.tv_sec / 60 % 60, kradgui->current_track_time.tv_sec % 60, kradgui->current_track_time.tv_nsec / 1000000);

}

void kradgui_update_total_track_time_timecode_string(kradgui_t *kradgui) {

	sprintf(kradgui->total_track_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld", kradgui->total_track_time.tv_sec / 86400 % 365, kradgui->total_track_time.tv_sec / 3600 % 24, kradgui->total_track_time.tv_sec / 60 % 60, kradgui->total_track_time.tv_sec % 60, kradgui->total_track_time.tv_nsec / 1000000);

}

void kradgui_update_playback_state_status_string(kradgui_t *kradgui) {

	switch (kradgui->playback_state) {
		case KRADGUI_PLAYING:
			strcpy(kradgui->playback_state_status_string, "Playing");
			break;
			
		case KRADGUI_PAUSED:
			strcpy(kradgui->playback_state_status_string, "Paused");
			break;
			
		case KRADGUI_STOPPED:
			strcpy(kradgui->playback_state_status_string, "Stopped");
			break;
	}

}

void kradgui_update_elapsed_time_timecode_string(kradgui_t *kradgui) {

	sprintf(kradgui->elapsed_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld", kradgui->elapsed_time.tv_sec / 86400 % 365, kradgui->elapsed_time.tv_sec / 3600 % 24, kradgui->elapsed_time.tv_sec / 60 % 60, kradgui->elapsed_time.tv_sec % 60, kradgui->elapsed_time.tv_nsec / 1000000);

}


void kradgui_reset_elapsed_time(kradgui_t *kradgui) {

	clock_gettime(CLOCK_REALTIME, &kradgui->start_time);
	memcpy(&kradgui->current_time, &kradgui->start_time, sizeof(struct timespec));
	kradgui->elapsed_time = timespec_diff(kradgui->start_time, kradgui->current_time);
	kradgui_update_elapsed_time_ms(kradgui);
	kradgui_update_elapsed_time_timecode_string(kradgui);

}

void kradgui_update_elapsed_time(kradgui_t *kradgui) {

	clock_gettime(CLOCK_REALTIME, &kradgui->current_time);
	kradgui->elapsed_time = timespec_diff(kradgui->start_time, kradgui->current_time);
	kradgui_update_elapsed_time_ms(kradgui);
	kradgui_update_elapsed_time_timecode_string(kradgui);

}

void kradgui_set_current_track_time(kradgui_t *kradgui, struct timespec current_track_time) {

	kradgui->current_track_time = current_track_time;
	kradgui_update_current_track_time_ms(kradgui);
	kradgui_update_current_track_progress(kradgui);
	kradgui_update_current_track_time_timecode_string(kradgui);

}

void kradgui_set_total_track_time(kradgui_t *kradgui, struct timespec total_track_time) {

	kradgui->total_track_time = total_track_time;
	kradgui_update_current_track_progress(kradgui);
	kradgui_update_total_track_time_timecode_string(kradgui);
}

void kradgui_set_current_track_time_ms(kradgui_t *kradgui, unsigned int current_track_time_ms) {

	//printf("Set current track time to: %zums\n", current_track_time_ms);

	kradgui->current_track_time.tv_sec = (current_track_time_ms - (current_track_time_ms % 1000)) / 1000;
	kradgui->current_track_time.tv_nsec = (current_track_time_ms % 1000) * 1000000;
	kradgui_update_current_track_time_ms(kradgui);
	kradgui_update_current_track_progress(kradgui);
	kradgui_update_current_track_time_timecode_string(kradgui);

}

void kradgui_add_current_track_time_ms(kradgui_t *kradgui, unsigned int additional_ms) {

	unsigned int current_track_time_ms;
	
	current_track_time_ms = ((kradgui->current_track_time.tv_sec * 1000) + (kradgui->current_track_time.tv_nsec / 1000000));

	current_track_time_ms += additional_ms;

	//printf("Set current track time to: %zums\n", current_track_time_ms);

	kradgui->current_track_time.tv_sec = (current_track_time_ms - (current_track_time_ms % 1000)) / 1000;
	kradgui->current_track_time.tv_nsec = (current_track_time_ms % 1000) * 1000000;
	kradgui_update_current_track_time_ms(kradgui);
	kradgui_update_current_track_progress(kradgui);
	kradgui_update_current_track_time_timecode_string(kradgui);

}

void kradgui_set_total_track_time_ms(kradgui_t *kradgui, unsigned int total_track_time_ms) {

	kradgui->total_track_time.tv_sec = (total_track_time_ms - (total_track_time_ms % 1000)) / 1000;
	kradgui->total_track_time.tv_nsec = (total_track_time_ms % 1000) * 1000000;
	kradgui_update_total_track_time_ms(kradgui);
	kradgui_update_current_track_progress(kradgui);
	kradgui_update_total_track_time_timecode_string(kradgui);

}


void kradgui_render(kradgui_t *kradgui) {

	if (kradgui->overlay) {
		cairo_save (kradgui->cr);
		cairo_set_source_rgba (kradgui->cr, BGCOLOR_TRANS);
		cairo_set_operator (kradgui->cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint (kradgui->cr);
		cairo_restore (kradgui->cr);
	} else {
		cairo_set_source_rgb (kradgui->cr, BGCOLOR);
		cairo_paint (kradgui->cr);
	}

	if (kradgui->reel_to_reel != NULL) {
		kradgui_update_reel_to_reel_information(kradgui->reel_to_reel);
		kradgui_render_reel_to_reel(kradgui->reel_to_reel);
	}
	
	if (kradgui->playback_state_status != NULL) {
		kradgui_render_playback_state_status(kradgui->playback_state_status);
	}
	
	if (kradgui->render_timecode) {
		kradgui_render_timecode(kradgui);
		kradgui_render_elapsed_timecode(kradgui);
	}
	
	if (kradgui->render_test_text) {
		kradgui_update_elapsed_time(kradgui);
	}
	
	if (kradgui->live) {
		kradgui_render_live(kradgui);
	}
	
	if (kradgui->recording) {
		kradgui_render_recording(kradgui);
	}
	
	if (kradgui->render_rgb) {
		kradgui_render_rgb(kradgui);
	}
	
	if (kradgui->render_rotator) {
		kradgui_render_rotator(kradgui);
	}
	
	if (kradgui->render_test_text) {
		kradgui_render_test_text(kradgui);
	}
	
	if (kradgui->render_tearbar) {
		kradgui_render_tearbar(kradgui);
	}
	
	kradgui->frame++;

	// frame debug
	/*
	sprintf(kradgui->debugline, "%dx%d %d", kradgui->width, kradgui->height, kradgui->frame);
	cairo_select_font_face (kradgui->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (kradgui->cr, 14.0);
	cairo_set_source_rgb (kradgui->cr, GREY);
	cairo_move_to (kradgui->cr, kradgui->width/2 + 50, kradgui->height - 10.0);
	//cairo_rotate (kradgui->cr, M_PI / (kradgui->frame % 60));
	cairo_show_text (kradgui->cr, kradgui->debugline);
	*/

}


struct timespec timespec_diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

