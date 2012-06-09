#include "krad_gui.h"
#include "krad_wayland.h"
#include "krad_system.h"

typedef struct cpu_meter_St cpu_meter_t;

struct cpu_meter_St {

	kradgui_t *krad_gui;
	krad_wayland_t *krad_wayland;
	int width;
	int height;
	
	int x;
	int y;
	int size;
	int cpu_usage;
	int displayed_cpu_usage;	
	int current_cpu_usage;
	
};

static void show_cpu_meter_window ();

int cpu_meter_render_callback (void *buffer, void *pointer) {

	cpu_meter_t *cpu_meter;
	int updated;

	cpu_meter = (cpu_meter_t *)pointer;	
	updated = 0;
	
	kradgui_render (cpu_meter->krad_gui);
	
	cpu_meter->current_cpu_usage = krad_system_get_cpu_usage ();
	
	if (cpu_meter->current_cpu_usage > cpu_meter->cpu_usage) {
		if ((cpu_meter->current_cpu_usage - cpu_meter->cpu_usage) > 5) {
			cpu_meter->cpu_usage = cpu_meter->cpu_usage + 5;
		} else {
			cpu_meter->cpu_usage++;
		}
	}
	if (cpu_meter->current_cpu_usage < cpu_meter->cpu_usage) {
	
		if ((cpu_meter->cpu_usage - cpu_meter->current_cpu_usage) > 5) {
			cpu_meter->cpu_usage = cpu_meter->cpu_usage - 5;
		} else {
			cpu_meter->cpu_usage--;
		}
		
		
	}		
	
	if (cpu_meter->displayed_cpu_usage != cpu_meter->cpu_usage) {
		kradgui_render_meter ( cpu_meter->krad_gui, cpu_meter->x, cpu_meter->y, cpu_meter->size, cpu_meter->cpu_usage );
		cpu_meter->displayed_cpu_usage = cpu_meter->cpu_usage;
	}
	
	updated = 1;
	return updated;
}


static void show_cpu_meter_window () {

	cpu_meter_t *cpu_meter;


	cpu_meter = calloc (1, sizeof (cpu_meter_t));

	cpu_meter->displayed_cpu_usage = -1;
	
	cpu_meter->width = 960;
	cpu_meter->height = 540;

	//krad_gui = kradgui_create_with_external_surface (width, height, krad_x11->pixels);
	cpu_meter->krad_gui = kradgui_create_with_internal_surface (cpu_meter->width, cpu_meter->height);

	cpu_meter->krad_gui->update_drawtime = 1;
	cpu_meter->krad_gui->print_drawtime = 1;
	
	//krad_gui->render_wheel = 1;
	//krad_gui->render_ftest = 1;
	//krad_gui->render_tearbar = 1;
	//kradgui_test_screen (krad_gui, "Gtk test");
	
	cpu_meter->x = cpu_meter->width / 2;
	cpu_meter->y = cpu_meter->height - (cpu_meter->height / 3);
	cpu_meter->size = cpu_meter->width / 4;
	
	cpu_meter->cpu_usage = krad_system_get_cpu_usage ();
	
	
	cpu_meter->krad_wayland = krad_wayland_create ();

	krad_wayland_set_frame_callback (cpu_meter->krad_wayland, cpu_meter_render_callback, cpu_meter);
	krad_wayland_open_window (cpu_meter->krad_wayland, cpu_meter->width, cpu_meter->height);
	
	while (cpu_meter->krad_wayland->running == 1) {
		
		usleep (300000);

	}
	
	krad_wayland_destroy (cpu_meter->krad_wayland);

	kradgui_destroy (cpu_meter->krad_gui);

	free (cpu_meter);

}

int main (int argc, char *argv[]) {

	krad_system_init ();
	krad_system_set_monitor_cpu_interval (800);
	krad_system_monitor_cpu_on ();
	
	show_cpu_meter_window ();

	krad_system_monitor_cpu_off ();

	printf ("Clean Exit\n");

    return 0;
}
