#include "krad_gui.h"
#include "krad_gui_gtk.h"

static void *kradgui_gtk_init(gpointer data);

static gboolean configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
  

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)data;

	if (kradgui_gtk->kradgui->cst) {
		cairo_surface_destroy (kradgui_gtk->kradgui->cst);
		kradgui_gtk->kradgui->cst = NULL;
	}

	kradgui_gtk->kradgui->cst = gdk_window_create_similar_surface (gtk_widget_get_window (widget), CAIRO_CONTENT_COLOR,
                                           		 gtk_widget_get_allocated_width (widget),
                                                 gtk_widget_get_allocated_height (widget));

	return TRUE;
}


static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data) {

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)data;

	gtk_widget_get_pointer (widget, &kradgui_gtk->kradgui->cursor_x, &kradgui_gtk->kradgui->cursor_y);

	kradgui_gtk->kradgui->cr = cairo_create(kradgui_gtk->kradgui->cst);
	kradgui_render(kradgui_gtk->kradgui);
	cairo_destroy(kradgui_gtk->kradgui->cr);

	cairo_set_source_surface (cr, kradgui_gtk->kradgui->cst, 0, 0);
	cairo_paint (cr);

	return FALSE;
}


static gboolean update_gui (gpointer data) {

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)data;

	if (kradgui_gtk->kradgui->shutdown == 1) {
		return FALSE;
	}

	gtk_widget_queue_draw_area (kradgui_gtk->da, 0, 0, kradgui_gtk->width, kradgui_gtk->height);
	
	return TRUE;
}

static void close_window (gpointer data) {

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)data;

	kradgui_gtk_end(kradgui_gtk->kradgui);
	
}

void kradgui_gtk_end(kradgui_t *kradgui) {

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)kradgui->gui_ptr;

	gtk_main_quit ();
	
	exit(0);
}

void kradgui_gtk_start(kradgui_t *kradgui) {

	kradgui_gtk_t *kradgui_gtk = calloc(1, sizeof(kradgui_gtk_t));

	kradgui->gui_ptr = kradgui_gtk;
	kradgui_gtk->kradgui = kradgui;

	kradgui_gtk->width = kradgui->width;
	kradgui_gtk->height = kradgui->height;

	pthread_create( &kradgui_gtk->gui_thread, NULL, kradgui_gtk_init, kradgui_gtk);
	
}


static void *kradgui_gtk_init(gpointer data) {

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)data;

	gtk_init (NULL, NULL);

	kradgui_gtk->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title (GTK_WINDOW (kradgui_gtk->window), "Krad GUI");

	g_signal_connect (kradgui_gtk->window, "destroy", G_CALLBACK (close_window), kradgui_gtk);

	//gtk_container_set_border_width (GTK_CONTAINER (kradgui_gtk->window), 8);

	//kradgui_gtk->frame = gtk_frame_new (NULL);
	//gtk_frame_set_shadow_type (GTK_FRAME (kradgui_gtk->frame), GTK_SHADOW_IN);
	//gtk_container_add (GTK_CONTAINER (kradgui_gtk->window), kradgui_gtk->frame);

	kradgui_gtk->da = gtk_drawing_area_new ();
	/* set a minimum size */
	
	gtk_window_set_position (GTK_WINDOW(kradgui_gtk->window), GTK_WIN_POS_CENTER);
    //gtk_window_set_decorated (GTK_WINDOW(kradgui_gtk->window), FALSE);
    gtk_window_set_has_resize_grip (GTK_WINDOW(kradgui_gtk->window), FALSE);
	gtk_widget_set_size_request (kradgui_gtk->da, kradgui_gtk->width, kradgui_gtk->height);

	gtk_container_add (GTK_CONTAINER (kradgui_gtk->window), kradgui_gtk->da);

	/* Signals used to handle the backing surface */
	g_signal_connect (kradgui_gtk->da, "draw",
		            G_CALLBACK (draw_cb), kradgui_gtk);
		            
	g_signal_connect (kradgui_gtk->da,"configure-event",
		            G_CALLBACK (configure_event_cb), kradgui_gtk);

	gtk_widget_show_all (kradgui_gtk->window);

	g_timeout_add (30, update_gui, kradgui_gtk);

	gtk_main ();
	
	free(kradgui_gtk->kradgui->gui_ptr);
	kradgui_gtk->kradgui->gui_ptr = NULL;

	return 0;
}
