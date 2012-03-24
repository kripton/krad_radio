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

void krad_mixer_gtk_set_control ( krad_mixer_gtk_t *krad_mixer_gtk, char *portname, char *controlname, float floatval) {

	printf("yay I set control to %s portname %s controlname %f floatval\n", portname, controlname, floatval);

}


int krad_mixer_gtk_ipc_handler ( krad_ipc_client_t *krad_ipc, void *ptr ) {

	krad_mixer_gtk_t *krad_mixer_gtk = (krad_mixer_gtk_t *)ptr;

	uint32_t message;
	uint32_t ebml_id;	
	uint64_t ebml_data_size;
	//uint64_t element;
	//int list_size;
	//int p;
	float floatval;	
	char portname_actual[256];
	char controlname_actual[1024];
	//int bytes_read;
	portname_actual[0] = '\0';
	controlname_actual[0] = '\0';
	
	char *portname = portname_actual;
	char *controlname = controlname_actual;
	//bytes_read = 0;
	ebml_id = 0;
	//list_size = 0;	
	floatval = 0;
	message = 0;
	ebml_data_size = 0;
	//element = 0;
	//p = 0;
	
	krad_ebml_read_element ( krad_ipc->krad_ebml, &message, &ebml_data_size);

	switch ( message ) {
	
		case EBML_ID_KRAD_RADIO_MSG:
			printf("krad_mixer_gtk_ipc_handler got message from krad radio\n");
			break;
	
		case EBML_ID_KRAD_LINK_MSG:
			printf("krad_mixer_gtk_ipc_handler got message from krad link\n");
			break;
			
		case EBML_ID_KRAD_MIXER_MSG:
			printf("krad_mixer_gtk_ipc_handler got message from krad mixer\n");
//			krad_ipc_server_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_CONTROL, portname, controlname, floatval);
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);
			
			switch ( ebml_id ) {
				case EBML_ID_KRAD_MIXER_CONTROL:
					printf("Received mixer control list %zu bytes of data.\n", ebml_data_size);

					krad_ipc_client_read_mixer_control ( krad_ipc, &portname, &controlname, &floatval );
					
					krad_mixer_gtk_set_control ( krad_mixer_gtk, portname, controlname, floatval);
					
					break;	
				case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
					printf("Received PORTGROUP list %zu bytes of data.\n", ebml_data_size);
					//list_size = ebml_data_size;
					//while ((list_size) && ((bytes_read += krad_ipc_client_read_portgroup ( krad_ipc )) <= list_size)) {
					//	//printf("Tag %s - %s.\n", tag_name, tag_value);
					//	if (bytes_read == list_size) {
					//		break;
					///	}
					//}	
					break;
				case EBML_ID_KRAD_MIXER_PORTGROUP:
					//krad_ipc_client_read_portgroup_inner ( client, &tag_name, &tag_value );
					printf("PORTGROUP %zu bytes  \n", ebml_data_size );
					break;
			}
		
		
			break;
	
	}

	return 0;

}


int main (int argc, char *argv[]) {
	
	krad_mixer_gtk_t *krad_mixer_gtk = calloc (1, sizeof(krad_mixer_gtk_t));
	
	krad_mixer_gtk->client = krad_ipc_connect (argv[1]);
	
	krad_ipc_set_handler_callback (krad_mixer_gtk->client, krad_mixer_gtk_ipc_handler, krad_mixer_gtk);
	
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
