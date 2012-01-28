#include <cairo/cairo.h>
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

#define KRADGUI_GTK_DEFAULT_FPS 60

typedef struct kradgui_gtk_St kradgui_gtk_t;

struct kradgui_gtk_St {

	kradgui_t *kradgui;
	GtkWidget *window;
	pthread_t gui_thread;
	pthread_t drawing_thread;
	int shutdown;
	
	int width;
	int height;
	
	int rest_time;
	int timeout_status;
	
	int first_time;
};


void kradgui_gtk_set_fps(kradgui_t *kradgui, int fps);

void kradgui_gtk_start(kradgui_t *kradgui);
void kradgui_gtk_end(kradgui_t *kradgui);

void *kradgui_gtk_init();
gboolean on_window_configure_event(GtkWidget * da, GdkEventConfigure * event, gpointer user_data);
gboolean on_window_expose_event(GtkWidget * da, GdkEventExpose * event, gpointer user_data); 
void *do_draw(void *ptr);
void *do_draw_alt(void *ptr);
void *do_draw_alt2(void *ptr);

