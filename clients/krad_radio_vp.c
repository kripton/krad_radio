#include <cairo/cairo.h>

#include "krad_radio_client.h"

#define GREY  0.197 / 0.255 * 1.0, 0.203 / 0.255 * 1.0, 0.203 / 0.255   * 1.0
#define BLUE 0.0, 0.152 / 0.255 * 1.0, 0.212 / 0.255 * 1.0
#define BGCOLOR_CLR  0.0 / 0.255 * 1.0, 0.0 / 0.255 * 1.0, 0.0 / 0.255   * 1.0, 0.255 / 0.255   * 1.0
#define ORANGE  0.255 / 0.255 * 1.0, 0.080 / 0.255 * 1.0, 0.0

void render_hex (cairo_t *cr, int x, int y, int w) {

	cairo_pattern_t *pat;
	static float hexrot = 0;
	int r1;
	float scale;
		
	cairo_save(cr);
	cairo_set_line_width(cr, 1);
	cairo_set_source_rgb(cr, ORANGE);

	scale = 2.5;
	r1 = ((w)/2 * sqrt(3));

	cairo_translate (cr, x, y);
	cairo_rotate (cr, hexrot * (M_PI/180.0));
	cairo_translate (cr, -(w/2), -r1);

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
	hexrot += 1.5;
	cairo_fill (cr);
	
	cairo_restore(cr);
	cairo_save(cr);
		
	cairo_set_line_width(cr, 1.5);
	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	cairo_set_source_rgb(cr, GREY);


	cairo_translate (cr, x, y);
	cairo_rotate (cr, hexrot * (M_PI/180.0));
	cairo_translate (cr, -((w * scale)/2), -r1 * scale);
	cairo_scale(cr, scale, scale);

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
	
	cairo_rotate (cr, 60 * (M_PI/180.0));

	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	pat = cairo_pattern_create_radial (w/2, r1, 3, w/2, r1, r1*scale);
	cairo_pattern_add_color_stop_rgba (pat, 0, 0, 0, 1, 1);
	cairo_pattern_add_color_stop_rgba (pat, 0.4, 0, 0, 0, 0);
	cairo_set_source (cr, pat);
	
	cairo_fill (cr);
	cairo_pattern_destroy (pat);
	cairo_restore(cr);

}


int videoport_process (void *buffer, void *arg) {

	cairo_surface_t *cst;
	cairo_t *cr;

	cst = cairo_image_surface_create_for_data ((unsigned char *)buffer,
												 CAIRO_FORMAT_ARGB32,
												 1280,
												 720,
												 1280 * 4);
	
	cr = cairo_create (cst);
	cairo_save (cr);
	cairo_set_source_rgba (cr, BGCOLOR_CLR);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);	
	cairo_paint (cr);
	cairo_restore (cr);
	render_hex (cr, 640, 360, 66);
	cairo_surface_flush (cst);
	cairo_destroy (cr);
	cairo_surface_destroy (cst);

	return 0;

}

int main (int argc, char *argv[]) {

	kr_client_t *kr;
	kr_videoport_t *kr_videoport;

	if (argc != 2) {
		if (argc > 2) {
			fprintf (stderr, "Only takes station argument.\n");
		} else {
			fprintf (stderr, "No station specified.\n");
		}
		return 1;
	}
		
	kr = kr_connect (argv[1]);
	
	if (kr == NULL) {
		fprintf (stderr, "Could not connect to %s krad radio daemon.\n", argv[1]);
		return 1;
	}	

	kr_videoport = kr_videoport_create (kr);

	if (kr_videoport != NULL) {
		printf ("i worked real good\n");
	}
	
	kr_videoport_set_callback (kr_videoport, videoport_process, NULL);
	
	kr_videoport_activate (kr_videoport);
	
	usleep (8000000);
	
	kr_videoport_deactivate (kr_videoport);
	
	kr_videoport_destroy (kr_videoport);

	kr_disconnect (kr);

	return 0;
	
}
