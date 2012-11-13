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
#include <cairo/cairo.h>
#include <gtk/gtk.h>

#include "krad_gui.h"

typedef struct krad_gui_gtk_St krad_gui_gtk_t;

struct krad_gui_gtk_St {

	krad_gui_t *krad_gui;
	
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *da;
	
	GdkDevice *pointer;
	GdkWindow *gdk_window;
	
	pthread_t gui_thread;
	
	int width;
	int height;
	
};



void krad_gui_gtk_start (krad_gui_t *krad_gui);
void krad_gui_gtk_end (krad_gui_t *krad_gui);


