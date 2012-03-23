#include <gtk/gtk.h>

#include "krad_ipc_client.h"
#include "krad_radio_ipc.h"


typedef struct krad_mixer_gtk_St krad_mixer_gtk_t;

struct krad_mixer_gtk_St {

	int intval;
	float floatval;
	krad_ipc_client_t *client;
	
	GtkWidget *window;
	GtkWidget *scale;
	GtkWidget *hbox;	

};

static void set_test_value (GtkWidget *widget, gpointer data) {
	
	krad_mixer_gtk_t *krad_mixer_gtk = (krad_mixer_gtk_t *)data;

	int intval2;
	float floatval;	
	intval2 = gtk_range_get_value (GTK_RANGE(widget));
	
	floatval = intval2;
	
	if (intval2 != krad_mixer_gtk->intval) {
		krad_mixer_gtk->intval = intval2;
		krad_ipc_set_control (krad_mixer_gtk->client, "Music2", "volume", floatval);
		//krad_ipc_cmd2 (krad_mixer_gtk->client, krad_mixer_gtk->intval);
	}
}

static gboolean get_test_value (gpointer data) {
	
	krad_mixer_gtk_t *krad_mixer_gtk = (krad_mixer_gtk_t *)data;
	
	int intval2 = -1;
	
	krad_ipc_client_check (krad_mixer_gtk->client, &intval2);
	
	if (intval2 != -1) {
		krad_mixer_gtk->intval = intval2;	
		gtk_range_set_value (GTK_RANGE(krad_mixer_gtk->scale), intval2);
	}
	
	return TRUE;
	
}

static gboolean on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data) {

	krad_mixer_gtk_t *krad_mixer_gtk = (krad_mixer_gtk_t *)data;
	krad_ipc_disconnect (krad_mixer_gtk->client);

	return FALSE;
}

int main (int argc, char *argv[]) {
	
	krad_mixer_gtk_t *krad_mixer_gtk = calloc (1, sizeof(krad_mixer_gtk_t));;
	
	krad_mixer_gtk->client = krad_ipc_connect (argv[1]);
	
	krad_ipc_get_portgroups (krad_mixer_gtk->client);
	
	gtk_init (&argc, &argv);

	krad_mixer_gtk->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (krad_mixer_gtk->window), "Krad Radio");

	gtk_window_resize (GTK_WINDOW (krad_mixer_gtk->window), 800, 600);

	g_signal_connect (krad_mixer_gtk->window, "delete-event", G_CALLBACK (on_delete_event), krad_mixer_gtk);
	g_signal_connect (krad_mixer_gtk->window, "destroy", G_CALLBACK (gtk_main_quit), krad_mixer_gtk);

	krad_mixer_gtk->hbox = gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 25 );

	krad_mixer_gtk->scale = gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL, 0.0, 100.0, 1.0);
	gtk_scale_set_value_pos ( GTK_SCALE (krad_mixer_gtk->scale), GTK_POS_BOTTOM );
	gtk_range_set_inverted (GTK_RANGE (krad_mixer_gtk->scale), TRUE);

	gdk_threads_add_timeout (25, get_test_value, krad_mixer_gtk);

	g_signal_connect (krad_mixer_gtk->scale, "value-changed", G_CALLBACK (set_test_value), krad_mixer_gtk);

	gtk_box_pack_start (GTK_BOX(krad_mixer_gtk->hbox), krad_mixer_gtk->scale, TRUE, TRUE, 15);

	gtk_container_add (GTK_CONTAINER (krad_mixer_gtk->window), krad_mixer_gtk->hbox);
	
	gtk_container_set_border_width (GTK_CONTAINER (krad_mixer_gtk->window), 20);

	gtk_widget_show_all (krad_mixer_gtk->window);

	gtk_main ();

	free (krad_mixer_gtk);

	return 0;
}
