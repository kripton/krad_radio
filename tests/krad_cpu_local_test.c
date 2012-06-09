#include "krad_gui.h"
#include "krad_x11.h"
#include "krad_system.h"


static void show_cpu_meter_window ();

static void show_cpu_meter_window () {

	kradgui_t *krad_gui;
	krad_x11_t *krad_x11;
	int width;
	int height;
	
	int x;
	int y;
	int size;
	int cpu_usage;
	int displayed_cpu_usage;	
	int current_cpu_usage;	
	
	cpu_usage = 0;
	displayed_cpu_usage = -1;
	current_cpu_usage = 0;
	
	width = 960;
	height = 540;
	
	krad_x11 = krad_x11_create ();
	
	krad_x11_create_glx_window (krad_x11, "CPU Meter", width, height, NULL);
	
	krad_gui = kradgui_create_with_external_surface (width, height, krad_x11->pixels);

	krad_gui->update_drawtime = 1;
	krad_gui->print_drawtime = 1;

	//krad_gui->render_wheel = 1;
	//krad_gui->render_ftest = 1;
	//krad_gui->render_tearbar = 1;
	//kradgui_test_screen (krad_gui, "Gtk test");
	
	x = width / 2;
	y = height - (height / 3);
	size = width / 4;
	
	cpu_usage = krad_system_get_cpu_usage ();
	
	while (krad_x11->close_window == 0) {								 

		kradgui_render (krad_gui);
		
		current_cpu_usage = krad_system_get_cpu_usage ();
		
		if (current_cpu_usage > cpu_usage) {
			if ((current_cpu_usage - cpu_usage) > 5) {
				cpu_usage = cpu_usage + 5;
			} else {
				cpu_usage++;
			}
		}
		if (current_cpu_usage < cpu_usage) {
		
			if ((cpu_usage - current_cpu_usage) > 5) {
				cpu_usage = cpu_usage - 5;
			} else {
				cpu_usage--;
			}
			
			
		}		
		
		if (displayed_cpu_usage != cpu_usage) {
			kradgui_render_meter ( krad_gui, x, y, size, cpu_usage );
			krad_x11_glx_render (krad_x11);
			displayed_cpu_usage = cpu_usage;
		}
		
		usleep (350000);

	}
	
	krad_x11_destroy_glx_window (krad_x11);

	krad_x11_destroy (krad_x11);

	kradgui_destroy (krad_gui);

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
