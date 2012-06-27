#include "krad_gui_gtk.h"

static void *krad_gui_gtk_init(gpointer data);

static gboolean configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
  

	krad_gui_gtk_t *krad_gui_gtk = (krad_gui_gtk_t *)data;

	if (krad_gui_gtk->krad_gui->cst) {
		cairo_surface_destroy (krad_gui_gtk->krad_gui->cst);
		krad_gui_gtk->krad_gui->cst = NULL;
	}

	krad_gui_gtk->krad_gui->cst = gdk_window_create_similar_surface (gtk_widget_get_window (widget), CAIRO_CONTENT_COLOR,
                                           		 gtk_widget_get_allocated_width (widget),
                                                 gtk_widget_get_allocated_height (widget));

	krad_gui_gtk->pointer = 
	gdk_device_manager_get_client_pointer (gdk_display_get_device_manager (gtk_widget_get_display (widget)));

	krad_gui_gtk->gdk_window = gtk_widget_get_window (widget);

	return TRUE;
}


static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data) {

	krad_gui_gtk_t *krad_gui_gtk = (krad_gui_gtk_t *)data;

	gdk_window_get_device_position (krad_gui_gtk->gdk_window, krad_gui_gtk->pointer,
									&krad_gui_gtk->krad_gui->cursor_x, 
									&krad_gui_gtk->krad_gui->cursor_y, 
									NULL);

	krad_gui_gtk->krad_gui->cr = cairo_create (krad_gui_gtk->krad_gui->cst);
	krad_gui_render (krad_gui_gtk->krad_gui);
	cairo_destroy (krad_gui_gtk->krad_gui->cr);

	cairo_set_source_surface (cr, krad_gui_gtk->krad_gui->cst, 0, 0);
	cairo_paint (cr);

	return FALSE;
}


static gboolean update_gui (gpointer data) {

	krad_gui_gtk_t *krad_gui_gtk = (krad_gui_gtk_t *)data;

	if (krad_gui_gtk->krad_gui->shutdown == 1) {
		return FALSE;
	}

	gtk_widget_queue_draw_area (krad_gui_gtk->da, 0, 0, krad_gui_gtk->width, krad_gui_gtk->height);
	
	return TRUE;
}

static void close_window (gpointer data) {

	krad_gui_gtk_t *krad_gui_gtk = (krad_gui_gtk_t *)data;

	krad_gui_gtk_end (krad_gui_gtk->krad_gui);
	
}

void krad_gui_gtk_end (krad_gui_t *krad_gui) {

	gtk_main_quit ();
	
	exit(0);
}

void krad_gui_gtk_start(krad_gui_t *krad_gui) {

	krad_gui_gtk_t *krad_gui_gtk = calloc(1, sizeof(krad_gui_gtk_t));

	krad_gui->gui_ptr = krad_gui_gtk;
	krad_gui_gtk->krad_gui = krad_gui;

	krad_gui_gtk->width = krad_gui->width;
	krad_gui_gtk->height = krad_gui->height;

	pthread_create( &krad_gui_gtk->gui_thread, NULL, krad_gui_gtk_init, krad_gui_gtk);
	
}


static void *krad_gui_gtk_init(gpointer data) {

	krad_gui_gtk_t *krad_gui_gtk = (krad_gui_gtk_t *)data;

	gtk_init (NULL, NULL);

	krad_gui_gtk->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title (GTK_WINDOW (krad_gui_gtk->window), "Krad GUI");

	g_signal_connect (krad_gui_gtk->window, "destroy", G_CALLBACK (close_window), krad_gui_gtk);

	//gtk_container_set_border_width (GTK_CONTAINER (krad_gui_gtk->window), 8);

	//krad_gui_gtk->frame = gtk_frame_new (NULL);
	//gtk_frame_set_shadow_type (GTK_FRAME (krad_gui_gtk->frame), GTK_SHADOW_IN);
	//gtk_container_add (GTK_CONTAINER (krad_gui_gtk->window), krad_gui_gtk->frame);

	krad_gui_gtk->da = gtk_drawing_area_new ();
	/* set a minimum size */
	
	gtk_window_set_position (GTK_WINDOW(krad_gui_gtk->window), GTK_WIN_POS_CENTER);
    //gtk_window_set_decorated (GTK_WINDOW(krad_gui_gtk->window), FALSE);
    gtk_window_set_has_resize_grip (GTK_WINDOW(krad_gui_gtk->window), FALSE);
    gtk_window_set_resizable (GTK_WINDOW(krad_gui_gtk->window), FALSE);
    gtk_window_set_focus_on_map  (GTK_WINDOW(krad_gui_gtk->window), TRUE);
	gtk_widget_set_size_request (krad_gui_gtk->da, krad_gui_gtk->width, krad_gui_gtk->height);

	gtk_container_add (GTK_CONTAINER (krad_gui_gtk->window), krad_gui_gtk->da);

	/* Signals used to handle the backing surface */
	g_signal_connect (krad_gui_gtk->da, "draw",
		            G_CALLBACK (draw_cb), krad_gui_gtk);
		            
	g_signal_connect (krad_gui_gtk->da,"configure-event",
		            G_CALLBACK (configure_event_cb), krad_gui_gtk);

	gtk_widget_show_all (krad_gui_gtk->window);
	gtk_window_present (GTK_WINDOW(krad_gui_gtk->window));

	g_timeout_add (30, update_gui, krad_gui_gtk);

	gtk_main ();
	
	free(krad_gui_gtk->krad_gui->gui_ptr);
	krad_gui_gtk->krad_gui->gui_ptr = NULL;

	return 0;
}
