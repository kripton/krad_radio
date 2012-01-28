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


typedef struct kradgui_gtk_St kradgui_gtk_t;

struct kradgui_gtk_St {

	kradgui_t *kradgui;
	
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *da;
	
	pthread_t gui_thread;
	
	int width;
	int height;
	
};



void kradgui_gtk_start(kradgui_t *kradgui);
void kradgui_gtk_end(kradgui_t *kradgui);


