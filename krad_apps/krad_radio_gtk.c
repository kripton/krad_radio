#include <gtk/gtk.h>

#include "krad_ipc_client.h"
#include "krad_radio_ipc.h"

#define PORTGROUPS_MAX 25

typedef struct krad_radio_gtk_St krad_radio_gtk_t;
typedef struct krad_radio_gtk_portgroup_St krad_radio_gtk_portgroup_t;

struct krad_radio_gtk_portgroup_St {

	char name[512];
	int active;
	float volume;	
	GtkWidget *volume_scale;	
	krad_radio_gtk_t *krad_radio_gtk;
};

struct krad_radio_gtk_St {

	krad_ipc_client_t *client;
	
	GtkWidget *window;
	GtkWidget *hbox;	

	krad_radio_gtk_portgroup_t *portgroups;

};

static void set_control_value (GtkWidget *widget, gpointer data);


static void add_portgroup (krad_radio_gtk_t *krad_radio_gtk, char *name, float value) {

	int p;
	krad_radio_gtk_portgroup_t *portgroup;
	
	for (p = 0; p < PORTGROUPS_MAX; p++) {
		portgroup = &krad_radio_gtk->portgroups[p];
		if ((portgroup != NULL) && (portgroup->active == 0)) {
			break;
		}
	}
	
	portgroup->krad_radio_gtk = krad_radio_gtk;
	portgroup->active = 1;
	strcpy (portgroup->name, name);
	portgroup->volume = value;

	portgroup->volume_scale = gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL, 0.0, 100.0, 1.0);
	gtk_scale_set_value_pos ( GTK_SCALE (portgroup->volume_scale), GTK_POS_BOTTOM );
	gtk_range_set_inverted (GTK_RANGE (portgroup->volume_scale), TRUE);
	g_signal_connect (portgroup->volume_scale, "value-changed", G_CALLBACK (set_control_value), portgroup);
	gtk_box_pack_start (GTK_BOX(portgroup->krad_radio_gtk->hbox), portgroup->volume_scale, TRUE, TRUE, 15);
	gtk_range_set_value (GTK_RANGE(portgroup->volume_scale), portgroup->volume);	
	gtk_widget_show_all (portgroup->volume_scale);
}


static void set_control_value (GtkWidget *widget, gpointer data) {
	
	krad_radio_gtk_portgroup_t *portgroup = (krad_radio_gtk_portgroup_t *)data;

	float volume;

	volume = gtk_range_get_value (GTK_RANGE(widget));
	
	if (volume != portgroup->volume) {
		portgroup->volume = volume;
		krad_ipc_set_control (portgroup->krad_radio_gtk->client, portgroup->name, "volume", portgroup->volume);
	}
}

static gboolean krad_ipc_poll (gpointer data) {
	
	krad_radio_gtk_t *krad_radio_gtk = (krad_radio_gtk_t *)data;
	
	krad_ipc_client_poll (krad_radio_gtk->client);
	
	return TRUE;
	
}

static gboolean on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data) {

	krad_radio_gtk_t *krad_radio_gtk = (krad_radio_gtk_t *)data;
	krad_ipc_disconnect (krad_radio_gtk->client);

	return FALSE;
}

void krad_radio_gtk_set_control ( krad_radio_gtk_t *krad_radio_gtk, char *portname, char *controlname, float floatval) {

	printf("yay I set control to %s portname %s controlname %f floatval\n", portname, controlname, floatval);

	int p;
	krad_radio_gtk_portgroup_t *portgroup;
	
	for (p = 0; p < PORTGROUPS_MAX; p++) {
		portgroup = &krad_radio_gtk->portgroups[p];
		if ((portgroup != NULL) && (portgroup->active == 1)) {
			if (strcmp(portname, portgroup->name) == 0) {
				portgroup->volume = floatval;
				gtk_range_set_value (GTK_RANGE(portgroup->volume_scale), floatval);
			}
		}
	}
}


int krad_radio_gtk_ipc_handler ( krad_ipc_client_t *krad_ipc, void *ptr ) {

	krad_radio_gtk_t *krad_radio_gtk = (krad_radio_gtk_t *)ptr;

	uint32_t message;
	uint32_t ebml_id;	
	uint64_t ebml_data_size;
	//uint64_t element;
	int list_size;
	//int p;
	float floatval;	
	char portname_actual[256];
	char controlname_actual[1024];
	int bytes_read;
	portname_actual[0] = '\0';
	controlname_actual[0] = '\0';
	
	char *portname = portname_actual;
	char *controlname = controlname_actual;
	bytes_read = 0;
	ebml_id = 0;
	list_size = 0;	
	floatval = 0;
	message = 0;
	ebml_data_size = 0;
	//element = 0;
	//p = 0;
	
	krad_ebml_read_element ( krad_ipc->krad_ebml, &message, &ebml_data_size);

	switch ( message ) {
	
		case EBML_ID_KRAD_RADIO_MSG:
			printf("krad_radio_gtk_ipc_handler got message from krad radio\n");
			break;
	
		case EBML_ID_KRAD_LINK_MSG:
			printf("krad_radio_gtk_ipc_handler got message from krad link\n");
			break;
			
		case EBML_ID_KRAD_MIXER_MSG:
			printf("krad_radio_gtk_ipc_handler got message from krad mixer\n");
//			krad_ipc_server_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_CONTROL, portname, controlname, floatval);
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);
			
			switch ( ebml_id ) {
				case EBML_ID_KRAD_MIXER_CONTROL:
					printf("Received mixer control list %zu bytes of data.\n", ebml_data_size);

					krad_ipc_client_read_mixer_control ( krad_ipc, &portname, &controlname, &floatval );
					
					krad_radio_gtk_set_control ( krad_radio_gtk, portname, controlname, floatval);
					
					break;	
				case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
					printf("Received PORTGROUP list %zu bytes of data.\n", ebml_data_size);
					list_size = ebml_data_size;
					while ((list_size) && ((bytes_read += krad_ipc_client_read_portgroup ( krad_ipc, portname, &floatval )) <= list_size)) {
						add_portgroup (krad_radio_gtk, portname, floatval);
						if (bytes_read == list_size) {
							break;
						}
					}	
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
	
	krad_radio_gtk_t *krad_radio_gtk = calloc (1, sizeof(krad_radio_gtk_t));
	
	krad_radio_gtk->portgroups = calloc (PORTGROUPS_MAX, sizeof(krad_radio_gtk_portgroup_t));
	
	krad_radio_gtk->client = krad_ipc_connect (argv[1]);
	
	krad_ipc_set_handler_callback (krad_radio_gtk->client, krad_radio_gtk_ipc_handler, krad_radio_gtk);
	
	krad_ipc_get_portgroups (krad_radio_gtk->client);
	
	gtk_init (&argc, &argv);

	krad_radio_gtk->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (krad_radio_gtk->window), "Krad Radio");

	gtk_window_resize (GTK_WINDOW (krad_radio_gtk->window), 300, 400);

	g_signal_connect (krad_radio_gtk->window, "delete-event", G_CALLBACK (on_delete_event), krad_radio_gtk);
	g_signal_connect (krad_radio_gtk->window, "destroy", G_CALLBACK (gtk_main_quit), krad_radio_gtk);

	krad_radio_gtk->hbox = gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 15 );
	
	gdk_threads_add_timeout (25, krad_ipc_poll, krad_radio_gtk);


	gtk_container_add (GTK_CONTAINER (krad_radio_gtk->window), krad_radio_gtk->hbox);
	
	gtk_container_set_border_width (GTK_CONTAINER (krad_radio_gtk->window), 20);

	gtk_widget_show_all (krad_radio_gtk->window);

	gtk_main ();

	free (krad_radio_gtk);

	return 0;
}
