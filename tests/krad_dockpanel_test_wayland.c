#include "krad_gui.h"
#include "krad_wayland.h"
#include "krad_system.h"

typedef struct dockpanel_St dockpanel_t;

struct dockpanel_St {

	krad_gui_t *krad_gui;
	krad_wayland_t *krad_wayland;

	void *buffer;

	int width;
	int height;

	int x;
	int y;
	int size;
	int cpu_usage;
	int displayed_cpu_usage;
	int current_cpu_usage;

	int margin;

};

static void show_dockpanel_window (int width, int height);

int dockpanel_render_callback (void *pointer, uint32_t xtime) {

	dockpanel_t *dockpanel;
	int updated;
	int cur_width;
	int motion;
	
	motion = 0;

	dockpanel = (dockpanel_t *)pointer;
	updated = 0;

	dockpanel->current_cpu_usage = krad_system_get_cpu_usage ();

	if (dockpanel->current_cpu_usage > dockpanel->cpu_usage) {
		dockpanel->cpu_usage++;
	}
	if (dockpanel->current_cpu_usage < dockpanel->cpu_usage) {
		dockpanel->cpu_usage--;
	}


	if ((dockpanel->krad_wayland->display->pointer_x != dockpanel->krad_gui->cursor_x) ||
		(dockpanel->krad_wayland->display->pointer_y != dockpanel->krad_gui->cursor_y) ||
		(dockpanel->krad_wayland->click != dockpanel->krad_gui->click)) {

		motion = 1;
		krad_gui_set_pointer (dockpanel->krad_gui,
							  dockpanel->krad_wayland->display->pointer_x,
							  dockpanel->krad_wayland->display->pointer_y);

	}
	
	krad_gui_set_click (dockpanel->krad_gui, dockpanel->krad_wayland->click);

	//printf ("x %d y %d\n", dockpanel->krad_wayland->display->pointer_x,
	//					   dockpanel->krad_wayland->display->pointer_y);


	//if ((dockpanel->krad_wayland->mousein) ||
	if ((0) ||	
		(motion == 1) || 
		(dockpanel->krad_gui->mousein != dockpanel->krad_wayland->mousein) ||
		(dockpanel->displayed_cpu_usage != dockpanel->cpu_usage)) {


		dockpanel->krad_gui->mousein = dockpanel->krad_wayland->mousein;
	
		krad_gui_render (dockpanel->krad_gui);
		
		dockpanel->size = 22;
		dockpanel->margin = dockpanel->size/8;	
		cur_width = dockpanel->margin;
	
		cur_width += krad_gui_render_selector_selected (dockpanel->krad_gui, dockpanel->margin, dockpanel->margin, dockpanel->size, "WAYRAD");
		cur_width += dockpanel->margin;	
		cur_width += krad_gui_render_selector_selected (dockpanel->krad_gui, cur_width, dockpanel->margin, dockpanel->size, "ACTIVATE");
		cur_width += dockpanel->margin;		
		cur_width += krad_gui_render_selector_selected (dockpanel->krad_gui, cur_width, dockpanel->margin, dockpanel->size, "OVERRIDE");
		cur_width += dockpanel->margin;		
		cur_width += krad_gui_render_selector_selected (dockpanel->krad_gui, cur_width, dockpanel->margin, dockpanel->size, "ON");
		cur_width += dockpanel->margin;		
		cur_width += krad_gui_render_selector_selected (dockpanel->krad_gui, cur_width, dockpanel->margin, dockpanel->size, "ENGAGE");
		cur_width += dockpanel->margin;		
		krad_gui_render_meter ( dockpanel->krad_gui, cur_width + dockpanel->size * 2, dockpanel->size * 2, dockpanel->size * 2, dockpanel->cpu_usage );
		//memcpy (buffer, dockpanel->krad_gui->data, dockpanel->width * dockpanel->height * 4);
		cairo_select_font_face (dockpanel->krad_gui->cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size (dockpanel->krad_gui->cr, dockpanel->size);
		cairo_set_source_rgb (dockpanel->krad_gui->cr, GREY3);
		cairo_move_to (dockpanel->krad_gui->cr, cur_width + dockpanel->size * 5,  dockpanel->margin*4 + dockpanel->size);
		
		krad_gui_render_hex (dockpanel->krad_gui, cur_width + dockpanel->size * 6,  dockpanel->margin*4 + dockpanel->size, 12);	
		
		cairo_show_text (dockpanel->krad_gui->cr, "The Present.");
		cairo_stroke (dockpanel->krad_gui->cr);
		dockpanel->displayed_cpu_usage = dockpanel->cpu_usage;
		updated = 1;
	}

	return updated;
}


static void show_dockpanel_window (int width, int height) {

	dockpanel_t *dockpanel;

	dockpanel = calloc (1, sizeof (dockpanel_t));

	dockpanel->displayed_cpu_usage = -1;

	dockpanel->width = width;
	dockpanel->height = height;


	dockpanel->x = dockpanel->width / 2;
	dockpanel->y = dockpanel->height - (dockpanel->height / 3);
	dockpanel->size = dockpanel->width / 4;

	dockpanel->cpu_usage = krad_system_get_cpu_usage ();


	dockpanel->krad_wayland = krad_wayland_create ();

	krad_wayland_prepare_window (dockpanel->krad_wayland, dockpanel->width, dockpanel->height, &dockpanel->buffer);

	dockpanel->krad_gui = krad_gui_create_with_external_surface (dockpanel->width, dockpanel->height, dockpanel->buffer);

	dockpanel->krad_gui->update_drawtime = 0;
	dockpanel->krad_gui->print_drawtime = 0;

	//dockpanel->krad_gui->render_wheel = 1;
	//dockpanel->krad_gui->render_ftest = 1;
	//dockpanel->krad_gui->render_tearbar = 1;
	//krad_gui_test_screen (dockpanel->krad_gui, "WAYRAD Test");


	krad_wayland_set_frame_callback (dockpanel->krad_wayland, dockpanel_render_callback, dockpanel);
	krad_wayland_open_window (dockpanel->krad_wayland);

	while (dockpanel->krad_wayland->running == 1) {
		usleep (300000);

	}

	krad_wayland_destroy (dockpanel->krad_wayland);

	krad_gui_destroy (dockpanel->krad_gui);

	free (dockpanel);

}

int main (int argc, char *argv[]) {

	krad_system_init ();
	krad_system_set_monitor_cpu_interval (800);
	krad_system_monitor_cpu_on ();

	if (argc == 3) {
		show_dockpanel_window (atoi(argv[1]), atoi(argv[2]));
	} else {
		show_dockpanel_window (360, 200);
	}
	krad_system_monitor_cpu_off ();

	printf ("Clean Exit\n");

    return 0;
}
