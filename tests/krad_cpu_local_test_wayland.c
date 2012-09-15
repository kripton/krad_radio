#include "krad_gui.h"
#include "krad_wayland.h"
#include "krad_system.h"

typedef struct cpu_meter_St cpu_meter_t;

struct cpu_meter_St {

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

};

static void show_cpu_meter_window (int width, int height);

int cpu_meter_render_callback (void *pointer, uint32_t time) {

	cpu_meter_t *cpu_meter;
	int updated;

	cpu_meter = (cpu_meter_t *)pointer;
	updated = 0;

	cpu_meter->current_cpu_usage = krad_system_get_cpu_usage ();

	if (cpu_meter->current_cpu_usage > cpu_meter->cpu_usage) {
		cpu_meter->cpu_usage++;
	}
	if (cpu_meter->current_cpu_usage < cpu_meter->cpu_usage) {
		cpu_meter->cpu_usage--;
	}

	if (cpu_meter->displayed_cpu_usage != cpu_meter->cpu_usage) {
		krad_gui_render (cpu_meter->krad_gui);
		krad_gui_render_meter ( cpu_meter->krad_gui, cpu_meter->x, cpu_meter->y, cpu_meter->size, cpu_meter->cpu_usage );
		//memcpy (buffer, cpu_meter->krad_gui->data, cpu_meter->width * cpu_meter->height * 4);
		cpu_meter->displayed_cpu_usage = cpu_meter->cpu_usage;
		updated = 1;
	}

	return updated;
}


static void show_cpu_meter_window (int width, int height) {

	cpu_meter_t *cpu_meter;

	cpu_meter = calloc (1, sizeof (cpu_meter_t));

	cpu_meter->displayed_cpu_usage = -1;

	cpu_meter->width = width;
	cpu_meter->height = height;


	cpu_meter->x = cpu_meter->width / 2;
	cpu_meter->y = cpu_meter->height - (cpu_meter->height / 3);
	cpu_meter->size = cpu_meter->width / 4;

	cpu_meter->cpu_usage = krad_system_get_cpu_usage ();


	cpu_meter->krad_wayland = krad_wayland_create ();

	krad_wayland_prepare_window (cpu_meter->krad_wayland, cpu_meter->width, cpu_meter->height, &cpu_meter->buffer);

	cpu_meter->krad_gui = krad_gui_create_with_external_surface (cpu_meter->width, cpu_meter->height, cpu_meter->buffer);

	cpu_meter->krad_gui->update_drawtime = 0;
	cpu_meter->krad_gui->print_drawtime = 0;

	//krad_gui->render_wheel = 1;
	//krad_gui->render_ftest = 1;
	//krad_gui->render_tearbar = 1;
	//krad_gui_test_screen (krad_gui, "Gtk test");


	krad_wayland_set_frame_callback (cpu_meter->krad_wayland, cpu_meter_render_callback, cpu_meter);
	krad_wayland_open_window (cpu_meter->krad_wayland);

	while (cpu_meter->krad_wayland->running == 1) {
		usleep (300000);

	}

	krad_wayland_destroy (cpu_meter->krad_wayland);

	krad_gui_destroy (cpu_meter->krad_gui);

	free (cpu_meter);

}

int main (int argc, char *argv[]) {

	krad_system_init ();
	krad_system_set_monitor_cpu_interval (800);
	krad_system_monitor_cpu_on ();

	if (argc == 3) {
		show_cpu_meter_window (atoi(argv[1]), atoi(argv[2]));
	} else {
		show_cpu_meter_window (360, 200);
	}
	krad_system_monitor_cpu_off ();

	printf ("Clean Exit\n");

    return 0;
}
