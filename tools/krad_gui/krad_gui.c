#include "krad_gui.h"

krad_gui_t *krad_gui_create (int width, int height) {

	krad_gui_t *krad_gui;

	if ((krad_gui = calloc (1, sizeof (krad_gui_t))) == NULL) {
		failfast ("Krad GUI mem alloc fail");
	}
	
	krad_gui->clear = 1;
	krad_gui->width = width;
	krad_gui->height = height;
	krad_gui_set_playback_state (krad_gui, krad_gui_PLAYING);
	krad_gui_reset_elapsed_time (krad_gui);
	krad_gui_set_total_track_time_ms (krad_gui, 45 * 60 * 1000);
	
	krad_gui->cursor_x = -1;
	krad_gui->cursor_y = -1;		
	
	clock_gettime (CLOCK_MONOTONIC, &krad_gui->start_time);
	
	return krad_gui;

}

void krad_gui_set_surface (krad_gui_t *krad_gui, cairo_surface_t *cst) {

	if (krad_gui->cr != NULL) {
		cairo_destroy (krad_gui->cr);
		krad_gui->cr = NULL;
	}
		
	krad_gui->cst = cst;
	krad_gui->cr = cairo_create (krad_gui->cst);
}

krad_gui_t *krad_gui_create_with_internal_surface(int width, int height) {

	krad_gui_t *krad_gui;

	krad_gui = krad_gui_create(width, height);
		
	krad_gui_create_internal_surface(krad_gui);
	
	//incase calloc takes time
	krad_gui_reset_elapsed_time(krad_gui);
	clock_gettime(CLOCK_MONOTONIC, &krad_gui->start_time);

	return krad_gui;

}

krad_gui_t *krad_gui_create_with_external_surface (int width, int height, unsigned char *pixels) {

	krad_gui_t *krad_gui;

	krad_gui = krad_gui_create (width, height);
		
	krad_gui->pixels = pixels;
	krad_gui_create_external_surface(krad_gui);
	
	//incase calloc takes time
	krad_gui_reset_elapsed_time(krad_gui);
	clock_gettime(CLOCK_MONOTONIC, &krad_gui->start_time);

	return krad_gui;

}

void krad_gui_create_external_surface(krad_gui_t *krad_gui) {

	krad_gui->stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, krad_gui->width);
	krad_gui->bytes = krad_gui->stride * krad_gui->height;
	krad_gui->cst = cairo_image_surface_create_for_data (krad_gui->pixels, CAIRO_FORMAT_ARGB32,
														krad_gui->width, krad_gui->height, krad_gui->stride);
	krad_gui->cr = cairo_create (krad_gui->cst);
	krad_gui->external_surface = 1;
	
}

void krad_gui_destroy_external_surface (krad_gui_t *krad_gui) {

	if (krad_gui->external_surface == 1) {
		if (krad_gui->cr != NULL) {
			cairo_destroy (krad_gui->cr);
			krad_gui->cr = NULL;
		}
		cairo_surface_destroy (krad_gui->cst);
		krad_gui->cst = NULL;
		krad_gui->pixels = NULL;
		krad_gui->stride = 0;
		krad_gui->bytes = 0;
		krad_gui->external_surface = 0;
	}
}


void krad_gui_create_internal_surface(krad_gui_t *krad_gui) {

	krad_gui->stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, krad_gui->width);
	krad_gui->bytes = krad_gui->stride * krad_gui->height;
	krad_gui->pixels = calloc (1, krad_gui->bytes);
	krad_gui->cst = cairo_image_surface_create_for_data (krad_gui->pixels, CAIRO_FORMAT_ARGB32,
														krad_gui->width, krad_gui->height, krad_gui->stride);
	krad_gui->cr = cairo_create(krad_gui->cst);
	krad_gui->internal_surface = 1;
	
}

void krad_gui_destroy_internal_surface(krad_gui_t *krad_gui) {

	if (krad_gui->internal_surface == 1) {
		if (krad_gui->cr != NULL) {
			cairo_destroy (krad_gui->cr);
			krad_gui->cr = NULL;			
		}
		cairo_surface_destroy (krad_gui->cst);
		free(krad_gui->pixels);
		krad_gui->cst = NULL;
		krad_gui->pixels = NULL;
		krad_gui->stride = 0;
		krad_gui->bytes = 0;
		krad_gui->internal_surface = 0;
	}
}

void krad_gui_set_size(krad_gui_t *krad_gui, int width, int height) {

	krad_gui->width = width;
	krad_gui->height = height;

}


void krad_gui_set_background_color(krad_gui_t *krad_gui, float r, float g, float b, float a) {


	//BGCOLOR_TRANS


}

void krad_gui_destroy(krad_gui_t *krad_gui) {

	if (krad_gui->reel_to_reel != NULL) {
		krad_gui_reel_to_reel_destroy(krad_gui->reel_to_reel);
	}
	
	if (krad_gui->playback_state_status != NULL) {
		krad_gui_playback_state_status_destroy(krad_gui->playback_state_status);
	}
	
	if (krad_gui->bug != NULL) {
		cairo_surface_destroy ( krad_gui->bug );
	}
	
	if (krad_gui->internal_surface == 1) {
		krad_gui_destroy_internal_surface(krad_gui);
	}
	
	if (krad_gui->external_surface == 1) {
		krad_gui_destroy_external_surface(krad_gui);
	}	

	free(krad_gui);

}

void krad_gui_add_item(krad_gui_t *krad_gui, krad_gui_item_t item) {

	switch (item) {
	
	case REEL_TO_REEL:
		krad_gui->reel_to_reel = krad_gui_reel_to_reel_create(krad_gui);
		break;

	case PLAYBACK_STATE_STATUS:
		krad_gui->playback_state_status = krad_gui_playback_state_status_create(krad_gui);
		break;
	}

}

void krad_gui_remove_item(krad_gui_t *krad_gui, krad_gui_item_t item) {

	switch (item) {
	
	case REEL_TO_REEL:
		krad_gui_reel_to_reel_destroy(krad_gui->reel_to_reel);
		krad_gui->reel_to_reel = NULL;
		break;
	
	case PLAYBACK_STATE_STATUS:
		krad_gui_playback_state_status_destroy(krad_gui->playback_state_status);
		krad_gui->playback_state_status = NULL;
		break;
	}

}


krad_gui_reel_to_reel_t *krad_gui_reel_to_reel_create(krad_gui_t *krad_gui) {

	krad_gui_reel_to_reel_t *krad_gui_reel_to_reel;

	if ((krad_gui_reel_to_reel = calloc (1, sizeof (krad_gui_reel_to_reel_t))) == NULL) {
		failfast ("Krad GUI reel to reel mem alloc fail");
	}
	
	krad_gui_reel_to_reel->krad_gui = krad_gui;
	
	krad_gui_reel_to_reel->reel_size = NORMAL_REEL_SIZE;
	krad_gui_reel_to_reel->reel_speed = SLOW_NORMAL_REEL_SPEED;
	//krad_gui->reel_size = PRO_REEL_SIZE;
	//krad_gui->reel_speed = PRO_REEL_SPEED;
	
	krad_gui_update_reel_to_reel_information(krad_gui_reel_to_reel);
	
	return krad_gui_reel_to_reel;

}


void krad_gui_reel_to_reel_destroy(krad_gui_reel_to_reel_t *krad_gui_reel_to_reel) {

	free(krad_gui_reel_to_reel);

}

krad_gui_playback_state_status_t *krad_gui_playback_state_status_create (krad_gui_t *krad_gui) {

	krad_gui_playback_state_status_t *krad_gui_playback_state_status;

	if ((krad_gui_playback_state_status = calloc (1, sizeof (krad_gui_playback_state_status_t))) == NULL) {
		failfast ("Krad GUI mem alloc fail");
	}
	
	krad_gui_playback_state_status->krad_gui = krad_gui;

	return krad_gui_playback_state_status;

}

void krad_gui_playback_state_status_destroy(krad_gui_playback_state_status_t *krad_gui_playback_state_status) {

	free(krad_gui_playback_state_status);	

}


void krad_gui_render_playback_state_status(krad_gui_playback_state_status_t *krad_gui_playback_state_status) {

	cairo_t *cr;
	cr = krad_gui_playback_state_status->krad_gui->cr;
	
	cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 20.0);
	
	switch (krad_gui_playback_state_status->krad_gui->playback_state) {
		case krad_gui_PLAYING:
			cairo_set_source_rgb (cr, GREEN);
			break;
			
		case krad_gui_PAUSED:
			cairo_set_source_rgb (cr, BLUE);
			break;
			
		case krad_gui_STOPPED:
			cairo_set_source_rgb (cr, GREY);
			break;
	}
	
	cairo_move_to (cr, krad_gui_playback_state_status->krad_gui->width/8.0 * 6, krad_gui_playback_state_status->krad_gui->height - 232.0);
	
	cairo_show_text (cr, krad_gui_playback_state_status->krad_gui->playback_state_status_string);

}

void krad_gui_set_playback_state(krad_gui_t *krad_gui, krad_gui_playback_state_t playback_state) {

	krad_gui->playback_state = playback_state;
	krad_gui_update_playback_state_status_string(krad_gui);

}


void krad_gui_control_speed_up(krad_gui_t *krad_gui) {

	if (krad_gui->control_speed_up_callback != NULL) {
		krad_gui->control_speed_up_callback(krad_gui->callback_pointer);
	}


}

void krad_gui_control_speed_down(krad_gui_t *krad_gui) {

	if (krad_gui->control_speed_down_callback != NULL) {
		krad_gui->control_speed_down_callback(krad_gui->callback_pointer);
	}

}

void krad_gui_control_speed(krad_gui_t *krad_gui, float value) {

	if (krad_gui->control_speed_callback != NULL) {
		krad_gui->control_speed_callback(krad_gui->callback_pointer, value);
	}

}


void krad_gui_set_control_speed_callback(krad_gui_t *krad_gui, void control_speed_callback(void *, float)) {

	krad_gui->control_speed_callback = control_speed_callback;


}

void krad_gui_set_control_speed_down_callback(krad_gui_t *krad_gui, void control_speed_down_callback(void *)) {

	krad_gui->control_speed_down_callback = control_speed_down_callback;


}

void krad_gui_set_control_speed_up_callback(krad_gui_t *krad_gui, void control_speed_up_callback(void *)) {

	krad_gui->control_speed_up_callback = control_speed_up_callback;


}

void krad_gui_set_callback_pointer(krad_gui_t *krad_gui, void *callback_pointer) {

	krad_gui->callback_pointer = callback_pointer;


}


void krad_gui_update_reel_to_reel_information(krad_gui_reel_to_reel_t *krad_gui_reel_to_reel) {

	// calc rotation example
	// determine rotation in degrees per millisecond
	// pro is 0.0381cm per millisecond 38.1 / 1000
	// 1 degree in pro is 0.074083333cm 26.67 / 360 
	// time ms * 0.381 = distance
	// distance / 0.74083333 = rotation
	
	krad_gui_reel_to_reel->distance = 
		krad_gui_reel_to_reel->krad_gui->current_track_time_ms * (krad_gui_reel_to_reel->reel_speed / 1000);
	krad_gui_reel_to_reel->total_distance = 
		krad_gui_reel_to_reel->krad_gui->total_track_time_ms * (krad_gui_reel_to_reel->reel_speed / 1000);
	if (krad_gui_reel_to_reel->distance >= krad_gui_reel_to_reel->total_distance) {
		krad_gui_reel_to_reel->distance = krad_gui_reel_to_reel->total_distance;
	}
	
	krad_gui_reel_to_reel->angle = 
		(unsigned int)(krad_gui_reel_to_reel->distance / (krad_gui_reel_to_reel->reel_size / 360)) % 360;
	
	//printk ("MS: %zu Reel Size: %fcm Reel Speed: %fcm/s Distance: %fcm Rotation: %fdegrees\n", 
	//krad_gui_reel_to_reel->krad_gui->current_track_time_ms, krad_gui_reel_to_reel->reel_size, 
	//krad_gui_reel_to_reel->reel_speed, krad_gui_reel_to_reel->distance, krad_gui_reel_to_reel->angle);
	
}

void krad_gui_render_circles (krad_gui_t *krad_gui, int w, int h) {

	cairo_t *cr;
	
	cr = krad_gui->cr;


	cairo_set_source_rgb(cr, GREEN);
	cairo_arc (cr, w, h, 0.4 * h, 0, 2*M_PI);
	cairo_stroke (cr);


	cairo_set_source_rgb(cr, ORANGE);
	cairo_arc (cr, w, h, 0.8 * h, 0, 2*M_PI);
	cairo_stroke (cr);
	
}

void krad_gui_set_bug (krad_gui_t *krad_gui, char *filename, int x, int y) {


	if ( filename != NULL ) {
	
		krad_gui->bug_x = x;
		krad_gui->bug_y = y;
		//krad_gui_load_bug ( krad_gui, filename );
		krad_gui->render_bug = 1;
		krad_gui->next_bug = strdup ( filename );
	}

}

void krad_gui_remove_bug (krad_gui_t *krad_gui) {

	//krad_gui_load_bug ( krad_gui, NULL );
	krad_gui->next_bug = strdup ("none");
}

void krad_gui_load_bug (krad_gui_t *krad_gui, char *filename) {

	if (krad_gui->bug_alpha == 0.0f) {
		krad_gui->bug_alpha = 1.0f;
	}

	
	if (krad_gui->bug != NULL) {
		cairo_surface_destroy ( krad_gui->bug );
		krad_gui->bug = NULL;
	}
	
	if ( filename != NULL ) {
		krad_gui->bug = cairo_image_surface_create_from_png ( filename );
		krad_gui->bug_width = cairo_image_surface_get_width ( krad_gui->bug );
		krad_gui->bug_height = cairo_image_surface_get_height ( krad_gui->bug );
		//krad_gui->start_frame = krad_gui->frame;
		krad_gui->bug_fade = 0;
		krad_gui->bug_fade_speed = 0.04;
		krad_gui->bug_fader = -1.3f;
		krad_gui->render_bug = 1;
	} else {
		krad_gui->render_bug = 0;
		krad_gui->bug_x = 0;
		krad_gui->bug_y = 0;
		krad_gui->bug_width = 0;
		krad_gui->bug_height = 0;
		krad_gui->bug_fade = 0;
		krad_gui->bug = NULL;
	}

}


void krad_gui_render_bug (krad_gui_t *krad_gui) {

	if (krad_gui->next_bug != NULL) {
		
		if (krad_gui->bug_fade <= 0.0f) {
			//printk ("%f %f\n", krad_gui->bug_fade, krad_gui->bug_fader);
			krad_gui->bug_fade = 0.0f;
			if (strncmp(krad_gui->next_bug, "none", 4) == 0) {
				krad_gui_load_bug (krad_gui, NULL);
			} else {
				krad_gui_load_bug (krad_gui, krad_gui->next_bug);
			}
			free (krad_gui->next_bug);			
			krad_gui->next_bug = NULL;
		} else {
		
			//krad_gui->bug_fade -= 0.01f;
	
			krad_gui->bug_fade = (0.5f) + 1.0f * sin(krad_gui->bug_fader / 2);
			krad_gui->bug_fader -= krad_gui->bug_fade_speed;
		
		}
	} else {
	
		if (krad_gui->bug_fade < krad_gui->bug_alpha) {
			//krad_gui->bug_fade += 0.01f;
			krad_gui->bug_fade = (0.5f) + 1.0f * sin(krad_gui->bug_fader / 2);
			krad_gui->bug_fader += krad_gui->bug_fade_speed;
		} else {
			if (krad_gui->bug_fade > krad_gui->bug_alpha + 0.1f) {
				//krad_gui->bug_fade += 0.01f;
				krad_gui->bug_fade = (0.5f) + 1.0f * sin(krad_gui->bug_fader / 2);
				krad_gui->bug_fader -= krad_gui->bug_fade_speed;
			} 
		}
	
	}
	
	//printk ("%f %f\n", krad_gui->bug_fade, krad_gui->bug_fader);
	
	if (krad_gui->bug_fader >= 2 * M_PI) {
		krad_gui->bug_fader -= 2 * M_PI;
	}

	if (krad_gui->render_bug == 1) {
		cairo_set_source_surface ( krad_gui->cr, krad_gui->bug, krad_gui->bug_x, krad_gui->bug_y );
		cairo_paint_with_alpha ( krad_gui->cr , krad_gui->bug_fade);
	}
}



void krad_gui_render_meter (krad_gui_t *krad_gui, int x, int y, int size, float pos) {

	cairo_t *cr;
	
	cr = krad_gui->cr;


	pos = pos * 1.8f - 90.0f;
//	printk ("peak %f %f\n", pos);

	cairo_save(cr);
	
	cairo_new_path ( cr );
	
	cairo_translate (cr, x, y);
	cairo_set_line_width(cr, 0.05 * size);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
	cairo_set_source_rgb(cr, GREY3);
	cairo_arc (cr, 0, 0, 0.8 * size, M_PI, 0);
	cairo_stroke (cr);
	
	cairo_set_source_rgb(cr, ORANGE);
	cairo_arc (cr, 0, 0, 0.65 * size, 1.8 * M_PI, 0);
	cairo_stroke (cr);

	cairo_arc (cr, size - 0.56 * size, -0.15 * size, 0.07 * size, 0, 2 * M_PI);
	cairo_fill(cr);

	cairo_save(cr);
	cairo_translate (cr, 0.05 * size, 0);
	cairo_rotate (cr, pos * (M_PI/180.0));	
	/*
	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (0, 0, 0.11 * size, 0);
	cairo_pattern_add_color_stop_rgba (pat, 0, 0, 0, 0, 0);
	cairo_pattern_add_color_stop_rgba (pat, 0.3, 0, 0, 0, 0);	
	cairo_pattern_add_color_stop_rgba (pat, 0.4, 0, 0, 0, 1);	
	cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 0);
	cairo_set_source (cr, pat);
	cairo_rectangle (cr, 0, 0, 0.11 * size, -size);
	cairo_fill (cr);
	*/
	cairo_set_source_rgb(cr, WHITE);
	cairo_move_to (cr, 0, 0);
	cairo_line_to (cr, 0, -size * 0.93);
	cairo_stroke (cr);
	
	cairo_restore(cr);
	cairo_set_source_rgb(cr, WHITE);
	cairo_arc (cr, 0.05 * size, 0, 0.1 * size, 0, 2 * M_PI);
	cairo_fill(cr);
	
	cairo_restore(cr);
	
}

void krad_gui_render_curve (krad_gui_t *krad_gui, int w, int h) {

	cairo_t *cr;

	cr = krad_gui->cr;

	cairo_set_line_width(cr, 3.5);
	cairo_set_source_rgb(cr, BLUE);

	static float pointy_positioner = 0.0f;
	int pointy;
	int pointy_start = 15;
	int pointy_range = 83;
	float pointy_final;
	float pointy_final_adj;
	float speed = 0.021;
	
	pointy = pointy_start + (pointy_range / 2) + round(pointy_range * sin(pointy_positioner) / 2);

	pointy_positioner += speed;

	if (pointy_positioner >= 2 * M_PI) {
		pointy_positioner -= 2 * M_PI;
		//new_speed = 1;
	}
	
	pointy_final = pointy * 0.01f;
	
	pointy_final_adj = pointy_final / 3.0f;
	
	int point1_x, point1_y, point2_x, point2_y;
	
	point1_x =  (0.250 + pointy_final_adj) * w;
	point1_y =  0.25 * h;
	point2_x =  (0.75 - pointy_final_adj) * w;
	point2_y =  0.25 * h;
	
	point1_y =  pointy_final * h;
	point2_y =  pointy_final * h;
		
	///printk ("f1 %f f2 %f\n", pointy_final, pointy_final_adj);
	cairo_move_to (cr, 20, 70);
	cairo_curve_to (cr, point1_x, point1_y, point2_x, point2_y, 0.85 * w, 0.85 * h);
	cairo_stroke (cr);
	
}

void krad_gui_render_grid (krad_gui_t *krad_gui, int x, int y, int w, int h, int lines) {


	cairo_t *cr;
	cr = krad_gui->cr;
	int count;

	//srand(time(NULL));
	cairo_save(cr);
	cairo_translate (cr, x + w/2, y + h/2);
	
	cairo_translate (cr, -x + -w/2, -y + -h/2);
	cairo_set_line_width(cr, 1.5);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
	cairo_set_source_rgb(cr, GREEN);
	
	cairo_save(cr);

	for (count = 1; count != lines; count++) {
	
		cairo_move_to (cr, x + (w/lines) * count, y);
		cairo_line_to (cr, x + (w/lines) * count, y + h);
		cairo_stroke (cr);

	}

	cairo_restore(cr);
	
	for (count = 1; count != lines; count++) {
		
		cairo_move_to (cr, x, y + (h/lines) * count);
		cairo_line_to (cr, x + w, y + (h/lines) * count);
		cairo_stroke (cr);
		
	}
	
	
	cairo_rectangle (cr, x, y, w, h);
	cairo_stroke (cr);
	
	cairo_restore(cr);
}

void krad_gui_render_viper (krad_gui_t *krad_gui, int x, int y, int size, int direction) {

	cairo_t *cr;

	cr = krad_gui->cr;

	cairo_save(cr);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);

	cairo_set_line_width(cr, size/7);
	cairo_set_source_rgb(cr, WHITE);
	cairo_translate (cr, x, y);

	cairo_arc (cr, 0.0, 0.0, size/2, 0, 2*M_PI);
	cairo_stroke(cr);
	
	cairo_set_source_rgb(cr, GREEN);

	cairo_arc (cr, 0.0, -size/5, size/14, 0, 2*M_PI);
	cairo_stroke(cr);

	cairo_rotate (cr, 40 * (M_PI/180.0));
	cairo_arc (cr, size/3.8, 0.0, size/14, 0, 2*M_PI);
	cairo_stroke(cr);
	cairo_move_to(cr, size/1.8,  0.0);
	cairo_rel_line_to (cr, size/2.3, 0.0);
	cairo_stroke(cr);

	cairo_rotate (cr, 100 * (M_PI/180.0));
	cairo_arc (cr, size/3.8, 0.0, size/14, 0, 2*M_PI);
	cairo_stroke(cr);
	cairo_move_to(cr, size/1.8,  0.0);
	cairo_rel_line_to (cr, size/2.3, 0.0);
	cairo_stroke(cr);

	cairo_rotate (cr, 130 * (M_PI/180.0));
	cairo_move_to(cr, size/1.8,  0.0);
	cairo_rel_line_to (cr, size/2.3, 0.0);
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, WHITE);
	cairo_rotate (cr, (180 + direction - 90) * (M_PI/180.0));
	cairo_move_to(cr, size/1.6,  size/6);
	cairo_rel_line_to (cr, size/6, -size/6);
	cairo_rel_line_to (cr, -size/6, -size/6);
	//cairo_rel_line_to(cr, size/1.6,  size/6);
	cairo_fill(cr);

	cairo_set_line_width(cr, size/7);
	cairo_rotate (cr, 180 * (M_PI/180.0));
	cairo_move_to(cr, size/1.6, 0.0);
	cairo_rel_line_to (cr, size/8, 0.0);
	cairo_stroke(cr);
	
	cairo_restore(cr);

}

void krad_gui_render_selector (krad_gui_t *krad_gui, int x, int y, int w) {

	cairo_t *cr;
	
	cr = krad_gui->cr;	
	
	cairo_save (cr);
		
	cairo_set_line_width (cr, 1.5);
	cairo_set_source_rgb (cr, BLUE);

	cairo_translate (cr, x, y);

	cairo_move_to (cr, 0, 0);
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w/4, 0);
	cairo_rotate (cr, -60 * (M_PI/180.0));	
	cairo_rel_line_to (cr, 0, w/1.5);
	cairo_rotate (cr, 120 * (M_PI/180.0));
	cairo_rel_line_to (cr, w/4, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));	
	cairo_rel_line_to (cr, w, 0);
	cairo_close_path (cr);
	cairo_stroke_preserve (cr);
	
	cairo_set_source_rgba (cr, BLUE_TRANS);	
	
	cairo_fill (cr);	

	cairo_restore (cr);

}

void krad_gui_set_click (krad_gui_t *krad_gui, int click) {
	krad_gui->click = click;
}

int krad_gui_render_selector_selected (krad_gui_t *krad_gui, int x, int y, int size, char *label) {

	cairo_t *cr;
	double px;
	double py;
	cairo_text_extents_t extents;
	int width;
	int height;
	
	px = krad_gui->cursor_x;
	py = krad_gui->cursor_y;
	
	cr = krad_gui->cr;	

	char *font = "sans";
	
	if (label != NULL) {
		cairo_select_font_face (cr, font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size (cr, size);
		cairo_text_extents (cr, label, &extents);
		width = extents.width + size;
		height = size * 1.2;
	}

	cairo_save (cr);
		
	cairo_set_line_width (cr, 3);
	//cairo_set_source_rgb (cr, ORANGE);

	cairo_translate (cr, x, y);

	cairo_move_to (cr, 0, 0);
	cairo_rel_line_to (cr, width, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, height/3, 0);
	cairo_rotate (cr, -60 * (M_PI/180.0));	
	cairo_rel_line_to (cr, 0, height);
	cairo_rotate (cr, 120 * (M_PI/180.0));
	cairo_rel_line_to (cr, height/3, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));	
	cairo_rel_line_to (cr, width, 0);
	cairo_close_path (cr);
	
	cairo_user_to_device (cr, &px, &py);
	//cairo_set_source_rgba (cr, BLUE_TRANS);
	//cairo_fill (cr);	
	
	if ((cairo_in_fill(cr, px, py) != 0) || (cairo_in_stroke(cr, px, py) != 0)) {
		if (krad_gui->click) {
			krad_gui->render_ftest = 1;
			cairo_set_source_rgb (cr, ORANGE);
			cairo_fill_preserve (cr);
			cairo_set_source_rgb (cr, GREEN);
			cairo_stroke (cr);
		} else {
			//cairo_set_source_rgb (cr, BLUE);
			//cairo_fill_preserve (cr);			
			cairo_set_source_rgb (cr, GREEN);
			cairo_stroke (cr);

		}
	} else {
		cairo_set_source_rgba (cr, BLUE_TRANS);
		cairo_fill (cr);		
	}
	
	cairo_restore (cr);
	
	if (label != NULL) {
		cairo_save (cr);
		cairo_translate (cr, x, y);	
		cairo_select_font_face (cr, font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size (cr, size);
		cairo_set_source_rgb (cr, GREY3);
		cairo_move_to (cr, size/2, height + height/8);	
		cairo_show_text (cr, label);
		cairo_stroke (cr);
		cairo_restore (cr);
	}
	
	
	return width + height/3;

}




void krad_gui_render_hex (krad_gui_t *krad_gui, int x, int y, int w) {


	cairo_t *cr;
	cairo_pattern_t *pat;
	
	static float hexrot = 0;
	
	cr = krad_gui->cr;	
	
	cairo_save(cr);
		
	cairo_set_line_width(cr, 1);
	cairo_set_source_rgb(cr, BLUE);

	int r1;
	float scale;

	scale = 2.5;
		
	r1 = ((w)/2 * sqrt(3));

	cairo_translate (cr, x, y);
	cairo_rotate (cr, hexrot * (M_PI/180.0));
	cairo_translate (cr, -(w/2), -r1);
	
	// draw radius 
	//cairo_move_to (cr, w/2, 0);
	//cairo_line_to (cr, w/2, r1);
	//cairo_stroke (cr);

	cairo_move_to (cr, 0, 0);
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	//cairo_stroke_preserve (cr);


	hexrot += 1.5;
	cairo_fill (cr);
	
	cairo_restore(cr);


//-----------------------

	cairo_save(cr);
		
	cairo_set_line_width(cr, 1.5);
	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	cairo_set_source_rgb(cr, GREY);


	cairo_translate (cr, x, y);
	cairo_rotate (cr, hexrot * (M_PI/180.0));
	cairo_translate (cr, -((w * scale)/2), -r1 * scale);
	cairo_scale(cr, scale, scale);
	//hexrot += 0.11;

	cairo_move_to (cr, 0, 0);
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);
	cairo_rotate (cr, 60 * (M_PI/180.0));
	cairo_rel_line_to (cr, w, 0);

	//cairo_stroke_preserve (cr);
	
	cairo_rotate (cr, 60 * (M_PI/180.0));
	
	
	// draw radius 
	//cairo_move_to (cr, w/2, 0);
	//cairo_line_to (cr, w/2, r1);
	//cairo_stroke_preserve (cr);
	
	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	pat = cairo_pattern_create_radial (w/2, r1, 3, w/2, r1, r1*scale);
	cairo_pattern_add_color_stop_rgba (pat, 0, 0, 0, 1, 1);
	//cairo_pattern_add_color_stop_rgba (pat, 0.6, 0, 0, 1, 0.3);
	//cairo_pattern_add_color_stop_rgba (pat, 0.8, 0, 0, 1, 0.05);
	cairo_pattern_add_color_stop_rgba (pat, 0.4, 0, 0, 0, 0);
	cairo_set_source (cr, pat);
	
	cairo_fill (cr);
	cairo_pattern_destroy (pat);


	
	cairo_restore(cr);

}


void krad_gui_render_cube (krad_gui_t *krad_gui, int x, int y, int w, int h) {

	cairo_t *cr;
	
	static float shear = -0.579595f;
	static float scale = 0.86062;
	static float rot = 30.0f;
	
	cr = krad_gui->cr;
	
	int lines;
	
	lines = 5;
	
	cairo_matrix_t matrix_rot;
	cairo_matrix_t matrix_rot2;
	cairo_matrix_t matrix_rot3;

	cairo_matrix_t matrix_trans;
	cairo_matrix_t matrix_trans2;
	cairo_matrix_t matrix_trans3;
	
	cairo_matrix_t matrix_sher;
	cairo_matrix_t matrix_sher2;
	cairo_matrix_t matrix_sher3;

	cairo_matrix_t matrix_scal;
	cairo_matrix_t matrix_scal2;
	cairo_matrix_t matrix_scal3;
	cairo_matrix_t matrix_scal4;
	
	cairo_matrix_t matrix_scal_sher;
	cairo_matrix_t matrix_scal_sher_rot;
	cairo_matrix_t matrix_scal_sher_rot_trans;

	
	cairo_matrix_t matrix_scal_sher2;
	cairo_matrix_t matrix_scal_sher_rot2;
	cairo_matrix_t matrix_scal_sher_rot_trans2;
	
	cairo_matrix_t matrix_scal_sher3;
	cairo_matrix_t matrix_scal_sher_rot3;
	cairo_matrix_t matrix_scal_sher_rot_trans3;
	
	cairo_matrix_init_translate(&matrix_trans, x - 1, y + 0.5);
	cairo_matrix_init_translate(&matrix_trans2, x, y - h);
	cairo_matrix_init_translate(&matrix_trans3, x + 1, y + 0.5);
	
	cairo_matrix_init_scale(&matrix_scal, 1.0, scale);
	
	cairo_matrix_init_scale(&matrix_scal2, 1.0, 1.0);
	cairo_matrix_init_scale(&matrix_scal3, 1.0, 1.0);
	cairo_matrix_init_scale(&matrix_scal4, 1.0, 1.0);
		
	cairo_matrix_init_rotate(&matrix_rot, (90.0f) * (M_PI/180.0));
	cairo_matrix_init_rotate(&matrix_rot2, (rot) * (M_PI/180.0));
	cairo_matrix_init_rotate(&matrix_rot3, (-30.0f) * (M_PI/180.0));
	
	cairo_matrix_init(&matrix_sher, 1.0, 0.0, shear, 1.0, 0.0, 0.0);
	cairo_matrix_init(&matrix_sher2, 1.0, 0.0, shear, 1.0, 0.0, 0.0);
	cairo_matrix_init(&matrix_sher3, 1.0, 0.0, shear, 1.0, 0.0, 0.0);

	
	cairo_matrix_multiply (&matrix_scal_sher, &matrix_scal, &matrix_sher);
	cairo_matrix_multiply (&matrix_scal_sher2, &matrix_scal, &matrix_sher2);
	cairo_matrix_multiply (&matrix_scal_sher3, &matrix_scal, &matrix_sher3);
	
	
	cairo_matrix_multiply (&matrix_scal_sher_rot, &matrix_scal_sher, &matrix_rot);
	cairo_matrix_multiply (&matrix_scal_sher_rot2, &matrix_scal_sher2, &matrix_rot2);
	cairo_matrix_multiply (&matrix_scal_sher_rot3, &matrix_scal_sher3, &matrix_rot3);	

	cairo_matrix_multiply (&matrix_scal_sher_rot_trans, &matrix_scal_sher_rot, &matrix_trans);
	
	cairo_matrix_multiply (&matrix_scal_sher_rot_trans2, &matrix_scal_sher_rot2, &matrix_trans2);
	cairo_matrix_multiply (&matrix_scal_sher_rot_trans3, &matrix_scal_sher_rot3, &matrix_trans3);
	
	cairo_save(cr);
	cairo_set_line_width(cr, 2);
	cairo_set_source_rgb(cr, GREEN);
	
	cairo_save(cr);
	
	cairo_transform(cr, &matrix_scal_sher_rot_trans2);
	//cairo_transform(cr, &matrix_scal4);
	cairo_set_source_rgba(cr, LGREEN);
	//krad_gui_render_grid (krad_gui, w, h, w, h, lines);
	cairo_stroke (cr);
	
	cairo_set_source_rgb(cr, GREEN);
	cairo_rectangle (cr, 0, 0, w, h);
	//cairo_fill (cr);
	cairo_stroke (cr);
	
	cairo_restore(cr);
	cairo_save(cr);

	//cairo_set_source_rgb(cr, ORANGE);
	cairo_transform(cr, &matrix_scal_sher_rot_trans);
	//cairo_transform(cr, &matrix_scal2);
	//cairo_transform(cr, &matrix_scal4);
	
	cairo_set_source_rgba(cr, LGREEN);
	cairo_move_to(cr, 0, 0);
	cairo_line_to (cr, w, h);
	cairo_stroke (cr);
	
	cairo_set_source_rgb(cr, GREEN);
	krad_gui_render_grid (krad_gui, 0, 0, w, h, lines);

	cairo_rectangle (cr, 0, 0, w, h);
	//cairo_fill (cr);
	cairo_stroke (cr);

	cairo_restore(cr);
	cairo_save(cr);

	//cairo_set_source_rgb(cr, BLUE);
	cairo_transform(cr, &matrix_scal_sher_rot_trans3);
	//cairo_transform(cr, &matrix_scal3);
	//cairo_transform(cr, &matrix_scal4);
	
	cairo_set_source_rgba(cr, LGREEN);
	cairo_move_to(cr, 0, 0);
	cairo_line_to (cr, w, h);
	cairo_stroke (cr);
	
	
	cairo_set_source_rgb(cr, GREEN);
	//krad_gui_render_grid (krad_gui, 0, 0, w, h, lines);
	cairo_rectangle (cr, 0, 0, w, h);
	//cairo_fill (cr);	
	cairo_stroke (cr);

	cairo_restore(cr);
	cairo_restore(cr);


	// other method
	if (0) {

		cairo_save(cr);

		cairo_transform(cr, &matrix_scal2);	
		cairo_transform(cr, &matrix_scal_sher_rot_trans2);
	
		static float h_test = 170;

		int t_x, t_y, t_w, t_h;
		int t2_x, t2_y, t2_w, t2_h;
	
		t_x = 500;
		t_y = 200;
	
		t_x = 0;
		t_y = 0;
	
	
		t_w = 200;
		t_h = 200;
	
		t2_w = t_w;
		t2_h = t_h;
	
		t2_x = (t_x + t2_w/2) - h_test;
		t2_y = (t_y + t2_h/2) - h_test;
	
	
		cairo_set_line_width(cr, 3);
		cairo_set_source_rgb(cr, ORANGE);
	
	
		cairo_move_to (cr, t_x, t_y);
		cairo_line_to (cr, t2_x, t2_y);	
	
		cairo_move_to (cr, t_x + t_w, t_y);
		cairo_line_to (cr, t2_x + t2_w, t2_y);	
	
		cairo_move_to (cr, t_x, t_y + t_h);
		cairo_line_to (cr, t2_x, t2_y + t2_h);	
	
		cairo_move_to (cr, t_x + t_w, t_y + t_h);
		cairo_line_to (cr, t2_x + t2_w, t2_y + t2_h);	
	
		cairo_stroke (cr);
	
		cairo_rectangle (cr, t_x, t_y, t_w, t_h);


		cairo_rectangle (cr, t2_x, t2_y, t2_w, t2_h);
	
		cairo_stroke (cr);

		cairo_restore(cr);

	}

}

void krad_gui_render_vtest (krad_gui_t *krad_gui) {

	
	//#ifdef SDLGUI
	//SDL_ShowCursor(0);

	//#endif
	//printk ("x %d y %d\n", mx, my);

/*
	krad_gui_render_cube (krad_gui, 590, 230, 100, 100);
	krad_gui_render_cube (krad_gui, 375, 260, 100, 100);
	krad_gui_render_cube (krad_gui,  550,  430, 20, 20);
	
	krad_gui_render_curve(krad_gui, mx, my);
	
	krad_gui_render_circles (krad_gui, mx,  my);
*/
static int g = 0;

g += (rand() % 5);
g -= (rand() % 5);

if (g > 100) {
	g = 100;
}

if (g < 0) {
	g = 0;
}

g = (krad_gui->cursor_x * 100) / krad_gui->width;


	krad_gui_render_hex (krad_gui, krad_gui->cursor_x, krad_gui->cursor_y, 14);


static int vposx = 0;
static int vposy = 0;
static int vdir = 0;
if (krad_gui->cursor_x > vposx) {
vposx++;
} else {
vposx--;
}

if (krad_gui->cursor_y > vposy) {
vposy++;
} else {
vposy--;
}
vdir = atan2 (krad_gui->cursor_y - vposy, krad_gui->cursor_x - vposx) * 180 / M_PI;

static float vposx2 = 666;
static float vposy2 = 666;
static int vdir2 = 0;
if (vposx > vposx2) {
vposx2 += 0.2;
} else {
vposx2 -= 0.2;
}

if (vposy > vposy2) {
vposy2 += 0.3;
} else {
vposy2 -= 0.7;
}
vdir2 = atan2 (vposy - vposy2, vposx - vposx2) * 180 / M_PI;


	//printk ("\nvdr is --%d--\n", vdir);
	krad_gui_render_viper (krad_gui, vposx, vposy, 32, vdir);	
	krad_gui_render_viper (krad_gui, vposx2, vposy2, 32, vdir2);

	
}

void krad_gui_set_pointer (krad_gui_t *krad_gui, int x, int y) {

	krad_gui->cursor_x = x;
	krad_gui->cursor_y = y;

}

void krad_gui_render_ftest (krad_gui_t *krad_gui) {

	//int w, h;
	cairo_t *cr;
	
	cr = krad_gui->cr;

	//w = krad_gui->width;
	//h = krad_gui->height;

	cairo_set_line_width(cr, 22);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	
	//#ifdef SDLGUI
	//SDL_ShowCursor(0);

	//#endif
	//printk ("m x %d y %d\n", krad_gui->cursor_x, krad_gui->cursor_y);

/*
	krad_gui_render_cube (krad_gui, 590, 230, 100, 100);
	krad_gui_render_cube (krad_gui, 375, 260, 100, 100);
	krad_gui_render_cube (krad_gui,  550,  430, 20, 20);
	
	krad_gui_render_curve(krad_gui, mx, my);
	
	krad_gui_render_circles (krad_gui, mx,  my);
*/
static int g = 0;

g += (rand() % 5);
g -= (rand() % 5);

if (g > 100) {
	g = 100;
}

if (g < 0) {
	g = 0;
}

g = (krad_gui->cursor_x * 100) / krad_gui->width;


	krad_gui_render_hex (krad_gui, krad_gui->cursor_x, krad_gui->cursor_y, 33);


static int vposx = 0;
static int vposy = 0;
static int vdir = 0;
if (krad_gui->cursor_x > vposx) {
vposx++;
} else {
vposx--;
}

if (krad_gui->cursor_y > vposy) {
vposy++;
} else {
vposy--;
}
vdir = atan2 (krad_gui->cursor_y - vposy, krad_gui->cursor_x - vposx) * 180 / M_PI;

static float vposx2 = 666;
static float vposy2 = 666;
static int vdir2 = 0;
if (vposx > vposx2) {
vposx2 += 0.2;
} else {
vposx2 -= 0.2;
}

if (vposy > vposy2) {
vposy2 += 0.3;
} else {
vposy2 -= 0.7;
}
vdir2 = atan2 (vposy - vposy2, vposx - vposx2) * 180 / M_PI;


	//printk ("\nvdr is --%d--\n", vdir);
	krad_gui_render_viper (krad_gui, vposx, vposy, 32, vdir);
	
	krad_gui_render_viper (krad_gui, vposx2, vposy2, 32, vdir2);
//	krad_gui_render_viper (krad_gui, 474, 333, 13, 99);

//	krad_gui_render_viper (krad_gui, 544, 555, 53, 222);
	
/*
	krad_gui_render_hex (krad_gui, 444, 222, 33);
	krad_gui_render_hex (krad_gui, 474, 333, 13);
	krad_gui_render_hex (krad_gui, mx, my, 44);
	krad_gui_render_hex (krad_gui, 544, 555, 53);
	krad_gui_render_hex (krad_gui, 644, 290, 63);
	krad_gui_render_hex (krad_gui, 744, 410, 23);
*/	
//	krad_gui_render_meter (krad_gui, 63, 33, 33, g);

	//krad_gui_render_meter (krad_gui, 233, 233, 44, g);
	
	//krad_gui_render_meter (krad_gui, 233, 533, 88, g);
	
//	krad_gui_render_meter (krad_gui, 633, 533, 188, g);


}

void krad_gui_render_wheel (krad_gui_t *krad_gui) {

	int diameter;
	int inner_diameter;

	int s;
	int num_spokes;
	int spoke_distance;
	
	num_spokes = 10;
	spoke_distance = 360 / num_spokes;
	

	diameter = (krad_gui->height/4 * 3.5) / 2;
	inner_diameter = diameter/8;

	cairo_t *cr;
	cr = krad_gui->cr;

	cairo_save(cr);
	cairo_new_path (cr);
	// wheel
	cairo_set_line_width(cr, 5);
	cairo_set_source_rgb(cr, WHITE);
	cairo_translate (cr, krad_gui->width/3, krad_gui->height/2);
	cairo_arc (cr, 0.0, 0.0, inner_diameter, 0, 2*M_PI);
	cairo_stroke(cr);
	cairo_arc(cr, 0.0, 0.0, diameter, 0, 2 * M_PI);
	cairo_stroke(cr);

	// spokes
	cairo_set_source_rgb(cr, WHITE);
	cairo_set_line_width(cr, 35);
	
	cairo_rotate (cr, (krad_gui->wheel_angle % 360) * (M_PI/180.0));
	
	for (s = 0; s < num_spokes; s++) {
		cairo_rotate (cr, spoke_distance * (M_PI/180.0));
		cairo_move_to(cr, inner_diameter,  0.0);
		cairo_line_to (cr, diameter, 0.0);
		cairo_move_to(cr, 0.0, 0.0);
		cairo_stroke(cr);
	}

	krad_gui->wheel_angle += 1;

	cairo_restore(cr);

}


void krad_gui_render_reel(krad_gui_reel_to_reel_t *krad_gui_reel_to_reel, int x, int y) {


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
	
	cr = krad_gui_reel_to_reel->krad_gui->cr;
	
	reel_inner_diameter = 13;
	reel_diameter = 45;
	
	max_tape_on_reel = reel_diameter - 5 - (reel_inner_diameter + 2);
	//min_tape_on_reel = 2 + reel_inner_diameter;
	
	tape_on_dest_reel = (krad_gui_reel_to_reel->krad_gui->current_track_progress * 0.01) * max_tape_on_reel;
	tape_on_source_reel = ((100.0 - krad_gui_reel_to_reel->krad_gui->current_track_progress) * 0.01) * max_tape_on_reel;
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
	cairo_arc (cr, 0.0, 0.0, 9.0, 0, 2*M_PI);
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
//	cairo_set_source_rgba(cr, WHITE_TRANS);
	cairo_set_source_rgb(cr, WHITE);
	cairo_set_line_width(cr, 2);
	cairo_rotate (cr, (krad_gui_reel_to_reel->angle + rotation_offset % 360) * (M_PI/180.0));
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
	if ((x > 200) && (krad_gui_reel_to_reel->krad_gui->current_track_progress < 100.0)) {
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

void krad_gui_render_reel_to_reel(krad_gui_reel_to_reel_t *krad_gui_reel_to_reel) {

	cairo_t *cr;
	
	cr = krad_gui_reel_to_reel->krad_gui->cr;

	cairo_save(cr);
	cairo_scale(cr, 3.0, 3.0);

	// reels
	krad_gui_render_reel(krad_gui_reel_to_reel, 82, 57);
	krad_gui_render_reel(krad_gui_reel_to_reel, 235, 57);

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
	
	cairo_restore(cr);
	
}

void krad_gui_render_elapsed_timecode(krad_gui_t *krad_gui) {

	krad_gui_update_elapsed_time(krad_gui);

	cairo_select_font_face (krad_gui->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (krad_gui->cr, 22.0);
	cairo_set_source_rgb (krad_gui->cr, ORANGE);
	
	cairo_move_to (krad_gui->cr, krad_gui->width/4.0, krad_gui->height - 174.0);	
	cairo_show_text (krad_gui->cr, "Elapsed Time");
	
	cairo_move_to (krad_gui->cr, krad_gui->width/4.0, krad_gui->height - 131.0);
	cairo_show_text (krad_gui->cr, krad_gui->elapsed_time_timecode_string);
	
	cairo_stroke (krad_gui->cr);
	
}

void krad_gui_render_timecode(krad_gui_t *krad_gui) {

	cairo_select_font_face (krad_gui->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (krad_gui->cr, 22.0);
	cairo_set_source_rgb (krad_gui->cr, ORANGE);

	cairo_move_to (krad_gui->cr, krad_gui->width/4.0, 90.0);
	cairo_show_text (krad_gui->cr, "Track Time");

	
	cairo_move_to (krad_gui->cr, krad_gui->width/4.0, 144.0);
	cairo_show_text (krad_gui->cr, krad_gui->current_track_time_timecode_string);
}

void krad_gui_render_rgb(krad_gui_t *krad_gui) {

	int size;
	int xoffset;
	int yoffset;
	
	size = krad_gui->width / 7;
	xoffset = size;
	yoffset = krad_gui->height / 7;
	
	cairo_set_source_rgb (krad_gui->cr, 1.0, 0.0, 0.0);
	cairo_rectangle (krad_gui->cr, xoffset, yoffset, size, size);
	cairo_fill (krad_gui->cr);
	
	cairo_set_source_rgb (krad_gui->cr, 0.0, 1.0, 0.0);
	cairo_rectangle (krad_gui->cr, xoffset + size, yoffset, size, size);
	cairo_fill (krad_gui->cr);
	
	cairo_set_source_rgb (krad_gui->cr, 0.0, 0.0, 1.0);
	cairo_rectangle (krad_gui->cr, xoffset + (size * 2), yoffset, size, size);
	cairo_fill (krad_gui->cr);
		
}


void krad_gui_render_tearbar(krad_gui_t *krad_gui) {

	if (krad_gui->tearbar_speed == 0.0) {		
		krad_gui->tearbar_speed = 0.1;
		krad_gui->tearbar_speed_adj = 0.01;
		krad_gui->tearbar_width = krad_gui->width / 15;
		krad_gui->movement_range = krad_gui->width - krad_gui->tearbar_width;
	}

	krad_gui->tearbar_position =
		0 + (krad_gui->movement_range / 2) + round(krad_gui->movement_range * sin(krad_gui->tearbar_positioner) / 2);

	//printk ("speed is %f adj is %f position is: %d positioner is %f\n", krad_gui->tearbar_speed, 
	//krad_gui->tearbar_speed_adj, krad_gui->tearbar_position, krad_gui->tearbar_positioner);

	krad_gui->tearbar_positioner += krad_gui->tearbar_speed;

	if (krad_gui->tearbar_positioner >= 2 * M_PI) {
		krad_gui->tearbar_positioner -= 2 * M_PI;
		krad_gui->new_speed = 1;
	}
	
	if ((krad_gui->new_speed) && (krad_gui->tearbar_position < krad_gui->last_tearbar_position)) {

		if (krad_gui->tearbar_speed > 0.17) {
			krad_gui->tearbar_speed_adj = -0.01;
		}
	
		if (krad_gui->tearbar_speed <= 0.02) {
			krad_gui->tearbar_speed_adj = 0.01;
		}
		
		krad_gui->tearbar_speed += krad_gui->tearbar_speed_adj;
		krad_gui->new_speed = 0;
	}
	
	krad_gui->last_tearbar_position = krad_gui->tearbar_position;
	
	cairo_set_source_rgb (krad_gui->cr, 1.0, 1.0, 1.0);
	cairo_rectangle (krad_gui->cr, krad_gui->tearbar_position, 0, krad_gui->tearbar_width, krad_gui->height);
	cairo_fill (krad_gui->cr);

}


void krad_gui_test_screen (krad_gui_t *krad_gui, char *info) {

	time_t t;

	t = time(0);
	sprintf(krad_gui->test_start_time, "Test Started: %s", ctime(&t));
	krad_gui->test_start_time[strlen(krad_gui->test_start_time) - 1] = '\0';

	if (info != NULL) {
		strncpy(krad_gui->test_info_text, info, 510);
	} else {
		strcpy(krad_gui->test_info_text, "No Test Information");
	}
	
	
	krad_gui_go_live (krad_gui);
	krad_gui->render_rgb = 1;
	krad_gui->render_test_text = 1;
	krad_gui->render_rotator = 1;

}


void krad_gui_render_text (krad_gui_t *krad_gui, int x, int y, int size, char *string) {

	cairo_select_font_face (krad_gui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (krad_gui->cr, size);
	cairo_set_source_rgb (krad_gui->cr, 0.9, 0.9, 0.9);
	cairo_move_to (krad_gui->cr, x, y);
	cairo_show_text (krad_gui->cr, string);
	
}	


void krad_gui_render_test_text(krad_gui_t *krad_gui) {

	time_t t;

	cairo_select_font_face (krad_gui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (krad_gui->cr, 62.0);
	cairo_set_source_rgb (krad_gui->cr, 1.0, 0.1, 0.1);
	cairo_move_to (krad_gui->cr, krad_gui->width/10, krad_gui->height - 360);
	cairo_show_text (krad_gui->cr, "KRAD TEST SIGNAL");

	cairo_set_font_size (krad_gui->cr, 20.0);
	cairo_set_source_rgb (krad_gui->cr, 0.9, 0.9, 0.9);
	cairo_move_to (krad_gui->cr, 50, krad_gui->height - 280);
	cairo_show_text (krad_gui->cr, krad_gui->test_info_text);


	cairo_select_font_face (krad_gui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_source_rgb (krad_gui->cr, 0.8, 0.8, 0.8);
	cairo_set_font_size (krad_gui->cr, 32.0);

	t = time(0);
	sprintf(krad_gui->test_text_time, "%s", ctime(&t));
	krad_gui->test_text_time[strlen(krad_gui->test_text_time) - 1] = '\0';
	cairo_move_to (krad_gui->cr, krad_gui->width/10.0, krad_gui->height - 84.0);
	cairo_show_text (krad_gui->cr, krad_gui->test_text_time);

	cairo_set_source_rgb (krad_gui->cr, 0.4, 0.4, 0.4);
	cairo_set_font_size (krad_gui->cr, 26.0);
	cairo_move_to (krad_gui->cr, 44, 50);
	cairo_show_text (krad_gui->cr, krad_gui->test_start_time);

	cairo_set_source_rgb (krad_gui->cr, 0.8, 0.8, 0.8);
	cairo_set_font_size (krad_gui->cr, 32.0);
	sprintf(krad_gui->test_text, "W: %d H: %d F: %llu", krad_gui->width, krad_gui->height, krad_gui->frame);
	cairo_move_to (krad_gui->cr, krad_gui->width/2, krad_gui->height - 84.0);
	cairo_show_text (krad_gui->cr, krad_gui->test_text);

}

void krad_gui_render_rotator(krad_gui_t *krad_gui) {

	int width;
	
	
	width = 16;
	krad_gui->rotator_speed = 2;
	
	
	cairo_set_source_rgb (krad_gui->cr, 0.5, 0.5, 0.5);
	
	cairo_save (krad_gui->cr);

	cairo_translate (krad_gui->cr, krad_gui->width/4 * 3, krad_gui->height/5 * 3);
	cairo_rotate (krad_gui->cr, (krad_gui->rotator_angle % 360) * (M_PI/180.0));
	cairo_translate (krad_gui->cr, width/2 * -1, (width * 16)/2 * -1);
	cairo_rectangle (krad_gui->cr, 0, 0, width, width * 16);
	cairo_fill (krad_gui->cr);
	cairo_restore (krad_gui->cr);

	krad_gui->rotator_angle += krad_gui->rotator_speed;

}

void krad_gui_render_recording(krad_gui_t *krad_gui) {

	krad_gui->recording_elapsed_time = timespec_diff(krad_gui->recording_start_time, krad_gui->current_time);
	sprintf(krad_gui->recording_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld", krad_gui->recording_elapsed_time.tv_sec / 86400 % 365, krad_gui->recording_elapsed_time.tv_sec / 3600 % 24, krad_gui->recording_elapsed_time.tv_sec / 60 % 60, krad_gui->recording_elapsed_time.tv_sec % 60, krad_gui->recording_elapsed_time.tv_nsec / 1000000);

	cairo_set_source_rgba (krad_gui->cr, 0.7, 0.0, 0.0, 0.3);
	cairo_rectangle (krad_gui->cr, krad_gui->recording_box_margin, krad_gui->recording_box_margin, krad_gui->recording_box_width, krad_gui->recording_box_height);
	cairo_fill (krad_gui->cr);

	cairo_select_font_face (krad_gui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (krad_gui->cr, krad_gui->recording_box_font_size);
	cairo_set_source_rgb (krad_gui->cr, 1.0, 0.0, 0.0);

	cairo_move_to (krad_gui->cr, krad_gui->recording_box_margin + krad_gui->recording_box_padding,  krad_gui->recording_box_margin + (krad_gui->recording_box_height - krad_gui->recording_box_padding));
	cairo_show_text (krad_gui->cr, "RECORDING");


//	cairo_move_to (krad_gui->cr, 50, 22);
//	cairo_show_text (krad_gui->cr, krad_gui->recording_time_timecode_string);

}

void krad_gui_render_live(krad_gui_t *krad_gui) {

	krad_gui->live_elapsed_time = timespec_diff(krad_gui->live_start_time, krad_gui->current_time);
	sprintf(krad_gui->live_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld",
		krad_gui->live_elapsed_time.tv_sec / 86400 % 365, krad_gui->live_elapsed_time.tv_sec / 3600 % 24,
		krad_gui->live_elapsed_time.tv_sec / 60 % 60, krad_gui->live_elapsed_time.tv_sec % 60,
		krad_gui->live_elapsed_time.tv_nsec / 1000000);

	cairo_set_source_rgba (krad_gui->cr, 0.0, 0.5, 0.0, 0.1);
	cairo_rectangle (krad_gui->cr, krad_gui->width - (krad_gui->live_box_margin + krad_gui->live_box_width),
					 krad_gui->live_box_margin, krad_gui->live_box_width, krad_gui->live_box_height);
	cairo_fill (krad_gui->cr);

	cairo_select_font_face (krad_gui->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size (krad_gui->cr, krad_gui->live_box_font_size);
	cairo_set_source_rgb (krad_gui->cr, GREEN);

	cairo_move_to (krad_gui->cr,
		krad_gui->width - (krad_gui->live_box_width + krad_gui->live_box_margin) + krad_gui->live_box_padding,
		krad_gui->live_box_margin + (krad_gui->live_box_height - krad_gui->live_box_padding));
	cairo_show_text (krad_gui->cr, "LIVE");

	cairo_set_font_size (krad_gui->cr, krad_gui->live_box_font_size / 4);
	cairo_move_to (krad_gui->cr,
				(krad_gui->width - (krad_gui->live_box_width + krad_gui->live_box_margin)) + krad_gui->live_box_padding,
				krad_gui->live_box_height + (krad_gui->live_box_padding + krad_gui->live_box_margin * 2));
	cairo_show_text (krad_gui->cr, krad_gui->live_time_timecode_string);

}

void krad_gui_go_live(krad_gui_t *krad_gui) {

	clock_gettime(CLOCK_MONOTONIC, &krad_gui->live_start_time);
	
	krad_gui->live_box_width = 270;
	krad_gui->live_box_height = 114;
	krad_gui->live_box_margin = 44;
	krad_gui->live_box_padding = (krad_gui->live_box_height / 5);
	krad_gui->live_box_font_size = (krad_gui->live_box_height / 8) * 6.5;
	krad_gui->live = 1;

}

void krad_gui_go_off(krad_gui_t *krad_gui) {

	krad_gui->live = 0;

}

void krad_gui_start_recording(krad_gui_t *krad_gui) {

	clock_gettime(CLOCK_MONOTONIC, &krad_gui->recording_start_time);
	
	krad_gui->recording_box_width = 654;
	krad_gui->recording_box_height = 114;
	krad_gui->recording_box_margin = 44;
	krad_gui->recording_box_padding = (krad_gui->recording_box_height / 5);
	krad_gui->recording_box_font_size = (krad_gui->recording_box_height / 8) * 6.5;
	
	
	krad_gui->recording = 1;

}

void krad_gui_stop_recording(krad_gui_t *krad_gui) {

	krad_gui->recording = 0;
	
}



void krad_gui_update_current_track_progress(krad_gui_t *krad_gui) {
	
	if ((krad_gui->total_track_time_ms != 0) && (krad_gui->current_track_time_ms != 0)) {
		krad_gui->current_track_progress = (krad_gui->current_track_time_ms * 1000 / krad_gui->total_track_time_ms) / 10.0;
	} else {
		krad_gui->current_track_progress = 0;
	}
	
	if (krad_gui->current_track_progress > 100.0) {
		krad_gui->current_track_progress = 100.0;
	}
	
	if (krad_gui->current_track_progress < 0.0) {
		krad_gui->current_track_progress = 0.0;
	}
	
	//printk ("Progress is: %f%%\n", krad_gui->current_track_progress);
}

void krad_gui_update_current_track_time_ms(krad_gui_t *krad_gui) {
	
	krad_gui->current_track_time_ms = 
		((krad_gui->current_track_time.tv_sec * 1000) + (krad_gui->current_track_time.tv_nsec / 1000000));

}

void krad_gui_update_total_track_time_ms(krad_gui_t *krad_gui) {
	
	krad_gui->total_track_time_ms = 
		((krad_gui->total_track_time.tv_sec * 1000) + (krad_gui->total_track_time.tv_nsec / 1000000));

}

void krad_gui_update_elapsed_time_ms(krad_gui_t *krad_gui) {
	
	krad_gui->elapsed_time_ms = ((krad_gui->elapsed_time.tv_sec * 1000) + (krad_gui->elapsed_time.tv_nsec / 1000000));

}


void krad_gui_update_current_track_time_timecode_string(krad_gui_t *krad_gui) {

	sprintf(krad_gui->current_track_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld",
	krad_gui->current_track_time.tv_sec / 86400 % 365, krad_gui->current_track_time.tv_sec / 3600 % 24,
	krad_gui->current_track_time.tv_sec / 60 % 60, krad_gui->current_track_time.tv_sec % 60,
	krad_gui->current_track_time.tv_nsec / 1000000);

}

void krad_gui_update_total_track_time_timecode_string(krad_gui_t *krad_gui) {

	sprintf(krad_gui->total_track_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld",
	krad_gui->total_track_time.tv_sec / 86400 % 365, krad_gui->total_track_time.tv_sec / 3600 % 24,
	krad_gui->total_track_time.tv_sec / 60 % 60, krad_gui->total_track_time.tv_sec % 60,
	krad_gui->total_track_time.tv_nsec / 1000000);

}

void krad_gui_update_playback_state_status_string(krad_gui_t *krad_gui) {

	switch (krad_gui->playback_state) {
		case krad_gui_PLAYING:
			strcpy(krad_gui->playback_state_status_string, "Playing");
			break;
			
		case krad_gui_PAUSED:
			strcpy(krad_gui->playback_state_status_string, "Paused");
			break;
			
		case krad_gui_STOPPED:
			strcpy(krad_gui->playback_state_status_string, "Stopped");
			break;
	}

}

void krad_gui_update_elapsed_time_timecode_string(krad_gui_t *krad_gui) {

	sprintf(krad_gui->elapsed_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld",
	krad_gui->elapsed_time.tv_sec / 86400 % 365, krad_gui->elapsed_time.tv_sec / 3600 % 24,
	krad_gui->elapsed_time.tv_sec / 60 % 60, krad_gui->elapsed_time.tv_sec % 60,
	krad_gui->elapsed_time.tv_nsec / 1000000);

}


void krad_gui_start_draw_time(krad_gui_t *krad_gui) {

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &krad_gui->start_draw_time);
	sprintf(krad_gui->elapsed_time_timecode_string, "%03ld:%02ld:%02ld:%02ld:%03ld",
	krad_gui->elapsed_time.tv_sec / 86400 % 365, krad_gui->elapsed_time.tv_sec / 3600 % 24,
	krad_gui->elapsed_time.tv_sec / 60 % 60, krad_gui->elapsed_time.tv_sec % 60, krad_gui->elapsed_time.tv_nsec / 1000000);

}

void krad_gui_end_draw_time(krad_gui_t *krad_gui) {

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &krad_gui->end_draw_time);
	krad_gui->draw_time = timespec_diff(krad_gui->start_draw_time, krad_gui->end_draw_time);
	sprintf(krad_gui->draw_time_string, "%02ld:%03ld", krad_gui->draw_time.tv_sec % 60,
			krad_gui->draw_time.tv_nsec / 1000000);

}

void krad_gui_reset_elapsed_time(krad_gui_t *krad_gui) {

	clock_gettime(CLOCK_MONOTONIC, &krad_gui->start_time);
	memcpy(&krad_gui->current_time, &krad_gui->start_time, sizeof(struct timespec));
	krad_gui->elapsed_time = timespec_diff(krad_gui->start_time, krad_gui->current_time);
	krad_gui_update_elapsed_time_ms(krad_gui);
	krad_gui_update_elapsed_time_timecode_string(krad_gui);

}

void krad_gui_update_elapsed_time(krad_gui_t *krad_gui) {

	clock_gettime(CLOCK_MONOTONIC, &krad_gui->current_time);
	krad_gui->elapsed_time = timespec_diff(krad_gui->start_time, krad_gui->current_time);
	krad_gui_update_elapsed_time_ms(krad_gui);
	krad_gui_update_elapsed_time_timecode_string(krad_gui);

}

void krad_gui_set_current_track_time(krad_gui_t *krad_gui, struct timespec current_track_time) {

	krad_gui->current_track_time = current_track_time;
	krad_gui_update_current_track_time_ms(krad_gui);
	krad_gui_update_current_track_progress(krad_gui);
	krad_gui_update_current_track_time_timecode_string(krad_gui);

}

void krad_gui_set_total_track_time(krad_gui_t *krad_gui, struct timespec total_track_time) {

	krad_gui->total_track_time = total_track_time;
	krad_gui_update_current_track_progress(krad_gui);
	krad_gui_update_total_track_time_timecode_string(krad_gui);
}

void krad_gui_set_current_track_time_ms(krad_gui_t *krad_gui, unsigned int current_track_time_ms) {

	//printk ("Set current track time to: %zums\n", current_track_time_ms);

	krad_gui->current_track_time.tv_sec = (current_track_time_ms - (current_track_time_ms % 1000)) / 1000;
	krad_gui->current_track_time.tv_nsec = (current_track_time_ms % 1000) * 1000000;
	krad_gui_update_current_track_time_ms(krad_gui);
	krad_gui_update_current_track_progress(krad_gui);
	krad_gui_update_current_track_time_timecode_string(krad_gui);

}

void krad_gui_add_current_track_time_ms(krad_gui_t *krad_gui, unsigned int additional_ms) {

	unsigned int current_track_time_ms;
	
	current_track_time_ms = 
	((krad_gui->current_track_time.tv_sec * 1000) + (krad_gui->current_track_time.tv_nsec / 1000000));

	current_track_time_ms += additional_ms;

	//printk ("Set current track time to: %zums", current_track_time_ms);

	krad_gui->current_track_time.tv_sec = (current_track_time_ms - (current_track_time_ms % 1000)) / 1000;
	krad_gui->current_track_time.tv_nsec = (current_track_time_ms % 1000) * 1000000;
	krad_gui_update_current_track_time_ms(krad_gui);
	krad_gui_update_current_track_progress(krad_gui);
	krad_gui_update_current_track_time_timecode_string(krad_gui);

}

void krad_gui_set_total_track_time_ms(krad_gui_t *krad_gui, unsigned int total_track_time_ms) {

	krad_gui->total_track_time.tv_sec = (total_track_time_ms - (total_track_time_ms % 1000)) / 1000;
	krad_gui->total_track_time.tv_nsec = (total_track_time_ms % 1000) * 1000000;
	krad_gui_update_total_track_time_ms(krad_gui);
	krad_gui_update_current_track_progress(krad_gui);
	krad_gui_update_total_track_time_timecode_string(krad_gui);

}

void krad_gui_overlay_clear (krad_gui_t *krad_gui) {
	cairo_save (krad_gui->cr);
	cairo_set_source_rgba (krad_gui->cr, BGCOLOR_TRANS);
	cairo_set_operator (krad_gui->cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (krad_gui->cr);
	cairo_restore (krad_gui->cr);
}

void krad_gui_clear (krad_gui_t *krad_gui) {
	cairo_save (krad_gui->cr);
	cairo_set_source_rgba (krad_gui->cr, BGCOLOR_CLR);
	cairo_set_operator (krad_gui->cr, CAIRO_OPERATOR_SOURCE);	
	cairo_paint (krad_gui->cr);
	cairo_restore (krad_gui->cr);
}

void krad_gui_render (krad_gui_t *krad_gui) {

	if (krad_gui->update_drawtime) {
		krad_gui_start_draw_time(krad_gui);
	}

	if (krad_gui->clear == 1) {
		if (krad_gui->overlay) {
			krad_gui_overlay_clear (krad_gui);
		} else {
			krad_gui_clear (krad_gui);
		}
	}

	if (krad_gui->reel_to_reel != NULL) {
		krad_gui_update_reel_to_reel_information(krad_gui->reel_to_reel);
		krad_gui_render_reel_to_reel(krad_gui->reel_to_reel);
	}
	
	if (krad_gui->playback_state_status != NULL) {
		krad_gui_render_playback_state_status(krad_gui->playback_state_status);
	}
	
	if (krad_gui->render_timecode) {
		krad_gui_render_timecode(krad_gui);
		krad_gui_render_elapsed_timecode(krad_gui);
	}
	
	if ((krad_gui->render_test_text) || (krad_gui->live)) {
		krad_gui_update_elapsed_time(krad_gui);
	}
	
	if (krad_gui->live) {
		krad_gui_render_live(krad_gui);
	}
	
	if (krad_gui->recording) {
		krad_gui_render_recording(krad_gui);
	}
	
	if (krad_gui->render_rgb) {
		krad_gui_render_rgb(krad_gui);
	}
	
	if (krad_gui->render_rotator) {
		krad_gui_render_rotator(krad_gui);
	}
	
	if (krad_gui->render_test_text) {
		krad_gui_render_test_text(krad_gui);
	}
	
	if (krad_gui->render_tearbar) {
		krad_gui_render_tearbar(krad_gui);
	}
	
	if (krad_gui->render_wheel) {
		krad_gui_render_wheel(krad_gui);
	}
	
	if (krad_gui->render_ftest) {
		krad_gui_render_ftest(krad_gui);
	}
	
	if ( krad_gui->render_bug ) {
		krad_gui_render_bug ( krad_gui );
	}
	
	if (krad_gui->update_drawtime) {
		krad_gui_end_draw_time(krad_gui);
	}
	
	if (krad_gui->print_drawtime) {
		printk ("Frame: %llu :: %s", krad_gui->frame, krad_gui->draw_time_string);
	}
	
	krad_gui->frame++;

}


struct timespec timespec_diff(struct timespec start, struct timespec end) {

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

