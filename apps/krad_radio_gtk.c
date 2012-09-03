#include <gtk/gtk.h>

#include "krad_ipc_client.h"
#include "krad_osc.h"
#include "krad_alsa_seq.h"

#define PORTGROUPS_MAX 25

typedef struct krad_radio_gtk_St krad_radio_gtk_t;
typedef struct krad_radio_gtk_portgroup_St krad_radio_gtk_portgroup_t;

struct krad_radio_gtk_portgroup_St {

	char name[128];
	int active;
	float volume;

	char crossfade_name[128];
	float crossfade;
	GtkWidget *crossfade_scale;
	GtkWidget *crossfade_label;
	GtkWidget *crossfade_vbox;	
	
	GtkWidget *volume_scale;
	GtkWidget *label;
	
	GtkWidget *label_now_playing;
	GtkWidget *label_playtime;
	
	GtkWidget *vbox;
	krad_radio_gtk_t *krad_radio_gtk;
};

struct krad_radio_gtk_St {

	krad_ipc_client_t *client;
	
	krad_alsa_seq_t *krad_alsa_seq;
	krad_osc_t *krad_osc;
	
	char sysname[128];
	
	GtkWidget *window;
	GtkWidget *label;	
	GtkWidget *vbox;
	GtkWidget *hbox_crossfades;	
	GtkWidget *hbox_volumes;	

	krad_radio_gtk_portgroup_t *portgroups;

};

static void set_volume_value (GtkWidget *widget, gpointer data);
static void set_crossfade_value (GtkWidget *widget, gpointer data);

static void remove_portgroup_crossfade (krad_radio_gtk_t *krad_radio_gtk, char *name) {

	int p;
	krad_radio_gtk_portgroup_t *portgroup;
	
	for (p = 0; p < PORTGROUPS_MAX; p++) {
		portgroup = &krad_radio_gtk->portgroups[p];
		if ((portgroup->active == 1) && (strcmp(name, portgroup->name) == 0)) {
			break;
		}
	}
	
	if (p == PORTGROUPS_MAX) {
		return;
	}

	if (portgroup->crossfade_label != NULL) {
		gtk_widget_destroy (portgroup->crossfade_label);
		gtk_widget_destroy (portgroup->crossfade_scale);
		gtk_widget_destroy (portgroup->crossfade_vbox);
	}
	
	portgroup->crossfade_label = NULL;

}

static void remove_portgroup (krad_radio_gtk_t *krad_radio_gtk, char *name) {

	int p;
	krad_radio_gtk_portgroup_t *portgroup;
	
	for (p = 0; p < PORTGROUPS_MAX; p++) {
		portgroup = &krad_radio_gtk->portgroups[p];
		if ((portgroup->active == 1) && (strcmp(name, portgroup->name) == 0)) {
			break;
		}
	}
	
	if (p == PORTGROUPS_MAX) {
		return;
	}

	gtk_widget_hide (portgroup->vbox);

	gtk_widget_destroy (portgroup->label);
	gtk_widget_destroy (portgroup->volume_scale);
	gtk_widget_destroy (portgroup->vbox);

	remove_portgroup_crossfade (krad_radio_gtk, name);
	
	strcpy (portgroup->name, "");
	
	portgroup->active = 0;

}

static void add_portgroup_crossfade (krad_radio_gtk_t *krad_radio_gtk, char *name, float volume, char *crossfade_name, float crossfade) {

	char tempname[256];

	int p;
	krad_radio_gtk_portgroup_t *portgroup;
	
	for (p = 0; p < PORTGROUPS_MAX; p++) {
		portgroup = &krad_radio_gtk->portgroups[p];
		if ((portgroup->active == 1) && (strcmp(name, portgroup->name) == 0)) {
			break;
		}
	}
	
	if (p == PORTGROUPS_MAX) {
		return;
	}

	if (strlen(crossfade_name) > 0) {

		sprintf(tempname, "Fade %s - %s", name, crossfade_name);
	

		portgroup->crossfade_vbox = gtk_box_new ( GTK_ORIENTATION_VERTICAL, 5 );
		portgroup->crossfade_label = gtk_label_new (tempname);
		portgroup->crossfade_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, -100.0, 100.0, 1.0);
		gtk_scale_set_value_pos ( GTK_SCALE (portgroup->crossfade_scale), GTK_POS_BOTTOM );
		//gtk_range_set_inverted (GTK_RANGE (portgroup->crossfade_scale), TRUE);
		g_signal_connect (portgroup->crossfade_scale, "value-changed", G_CALLBACK (set_crossfade_value), portgroup);
		gtk_box_pack_start (GTK_BOX(portgroup->crossfade_vbox), portgroup->crossfade_label, FALSE, FALSE, 5);
		gtk_box_pack_end (GTK_BOX(portgroup->crossfade_vbox), portgroup->crossfade_scale, TRUE, TRUE, 5);
		gtk_box_pack_start (GTK_BOX(portgroup->krad_radio_gtk->hbox_crossfades), portgroup->crossfade_vbox, TRUE, TRUE, 15);
		gtk_range_set_value (GTK_RANGE(portgroup->crossfade_scale), portgroup->crossfade);
	
		gtk_widget_show_all (portgroup->crossfade_vbox);	
	
	}

}


static void add_portgroup (krad_radio_gtk_t *krad_radio_gtk, char *name, float volume, char *crossfade_name, float crossfade) {

	int p;
	krad_radio_gtk_portgroup_t *portgroup;
	
	for (p = 0; p < PORTGROUPS_MAX; p++) {
		portgroup = &krad_radio_gtk->portgroups[p];
		if ((portgroup != NULL) && (portgroup->active == 0)) {
			break;
		}
	}
	
	if (p == PORTGROUPS_MAX) {
		return;
	}	
	
	portgroup->krad_radio_gtk = krad_radio_gtk;
	portgroup->active = 1;
	strcpy (portgroup->name, name);
	portgroup->volume = volume;

	strcpy (portgroup->crossfade_name, crossfade_name);
	portgroup->crossfade = crossfade;

	portgroup->vbox = gtk_box_new ( GTK_ORIENTATION_VERTICAL, 5 );
	portgroup->label = gtk_label_new (portgroup->name);
	portgroup->label_now_playing = gtk_label_new ("");
	portgroup->label_playtime = gtk_label_new ("");
	portgroup->volume_scale = gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL, 0.0, 100.0, 1.0);
	gtk_scale_set_value_pos ( GTK_SCALE (portgroup->volume_scale), GTK_POS_BOTTOM );
	gtk_range_set_inverted (GTK_RANGE (portgroup->volume_scale), TRUE);
	g_signal_connect (portgroup->volume_scale, "value-changed", G_CALLBACK (set_volume_value), portgroup);
	gtk_box_pack_start (GTK_BOX(portgroup->vbox), portgroup->label, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX(portgroup->vbox), portgroup->label_now_playing, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX(portgroup->vbox), portgroup->label_playtime, FALSE, FALSE, 5);
	gtk_box_pack_end (GTK_BOX(portgroup->vbox), portgroup->volume_scale, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX(portgroup->krad_radio_gtk->hbox_volumes), portgroup->vbox, TRUE, TRUE, 15);
	gtk_range_set_value (GTK_RANGE(portgroup->volume_scale), portgroup->volume);
	
	
	add_portgroup_crossfade (krad_radio_gtk, name, volume, crossfade_name, crossfade);
	
	krad_ipc_get_tags (krad_radio_gtk->client, portgroup->name);
	gtk_widget_show_all (portgroup->vbox);
}

// Sending the control command after using a gtk widget
static void set_volume_value (GtkWidget *widget, gpointer data) {
	
	krad_radio_gtk_portgroup_t *portgroup = (krad_radio_gtk_portgroup_t *)data;

	float volume;

	volume = gtk_range_get_value (GTK_RANGE(widget));
	
	if (volume != portgroup->volume) {
		portgroup->volume = volume;
		krad_ipc_set_control (portgroup->krad_radio_gtk->client, portgroup->name, "volume", portgroup->volume);
	}
}

// Sending the control command after using a gtk widget
static void set_crossfade_value (GtkWidget *widget, gpointer data) {
	
	krad_radio_gtk_portgroup_t *portgroup = (krad_radio_gtk_portgroup_t *)data;

	float crossfade;

	crossfade = gtk_range_get_value (GTK_RANGE(widget));
	
	if (crossfade != portgroup->crossfade) {
		portgroup->crossfade = crossfade;
		krad_ipc_set_control (portgroup->krad_radio_gtk->client, portgroup->name, "crossfade", portgroup->crossfade);
	}
}

// IPC message updating GTK widget
void krad_radio_gtk_set_control ( krad_radio_gtk_t *krad_radio_gtk, char *portname, char *controlname, float floatval) {

	//printd ("yay I set control to %s portname %s controlname %f floatval\n", portname, controlname, floatval);

	int p;
	krad_radio_gtk_portgroup_t *portgroup;
	
	for (p = 0; p < PORTGROUPS_MAX; p++) {
		portgroup = &krad_radio_gtk->portgroups[p];
		if ((portgroup != NULL) && (portgroup->active == 1)) {
			if (strcmp(portname, portgroup->name) == 0) {
				if (strcmp(controlname, "crossfade") == 0) {
					portgroup->crossfade = floatval;
					gtk_range_set_value (GTK_RANGE(portgroup->crossfade_scale), floatval);
				}
				if (strcmp(controlname, "volume") == 0) {
					portgroup->volume = floatval;
					gtk_range_set_value (GTK_RANGE(portgroup->volume_scale), floatval);
				}
			}
		}
	}
}

// IPC message updating GTK widget
void krad_radio_gtk_set_tag ( krad_radio_gtk_t *krad_radio_gtk, char *portname, char *tag_name, char *tag_value) {

	//printd ("yay I set control to %s portname %s controlname %f floatval\n", portname, controlname, floatval);

	int p;
	krad_radio_gtk_portgroup_t *portgroup;
	
	for (p = 0; p < PORTGROUPS_MAX; p++) {
		portgroup = &krad_radio_gtk->portgroups[p];
		if ((portgroup != NULL) && (portgroup->active == 1)) {
			if (strcmp(portname, portgroup->name) == 0) {
				if (strcmp(tag_name, "now_playing") == 0) {
					gtk_label_set_text (GTK_LABEL(portgroup->label_now_playing), tag_value);
				}
				if (strcmp(tag_name, "playtime") == 0) {
					gtk_label_set_text (GTK_LABEL(portgroup->label_playtime), tag_value);
				}
			}
		}
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

int krad_radio_gtk_ipc_handler ( krad_ipc_client_t *krad_ipc, void *ptr ) {

	krad_radio_gtk_t *krad_radio_gtk = (krad_radio_gtk_t *)ptr;

	uint32_t message;
	uint32_t ebml_id;	
	uint64_t ebml_data_size;
	//uint64_t element;
	int list_size;
	int i;
	float floatval;	
	char portname_actual[256];
	char controlname_actual[1024];
	char crossfadename_actual[1024];
	char string_actual[1024];
	int bytes_read;
	string_actual[0] = '\0';
	portname_actual[0] = '\0';
	controlname_actual[0] = '\0';

	char tag_item_actual[256];	
	char tag_name_actual[256];
	char tag_value_actual[1024];
	
	tag_name_actual[0] = '\0';
	tag_item_actual[0] = '\0';
	tag_value_actual[0] = '\0';
	
	char *tag_item = tag_item_actual;
	char *tag_name = tag_name_actual;
	char *tag_value = tag_value_actual;
	
	uint64_t number;	
	
	char *portname = portname_actual;
	char *controlname = controlname_actual;
	char *crossfadename = crossfadename_actual;
	char *string = string_actual;
	float crossfade;
	bytes_read = 0;
	ebml_id = 0;
	list_size = 0;	
	floatval = 0;
	message = 0;
	ebml_data_size = 0;
	i = 0;
	number = 0;
	//element = 0;
	//p = 0;
	
	krad_ebml_read_element ( krad_ipc->krad_ebml, &message, &ebml_data_size);

	switch ( message ) {

		case EBML_ID_KRAD_COMPOSITOR_MSG:
			//printkd ("krad_radio_gtk_ipc_handler got message from krad compositor\n");
			break;
			
		case EBML_ID_KRAD_RADIO_MSG:
				//printf("Received KRAD_RADIO_MSG %zu bytes of data.\n", ebml_data_size);		
		
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);
			switch ( ebml_id ) {

				case EBML_ID_KRAD_RADIO_TAG_LIST:
					//printf("Received Tag list %"PRIu64" bytes of data.\n", ebml_data_size);
					list_size = ebml_data_size;
					while ((list_size) && ((bytes_read += krad_ipc_client_read_tag ( krad_ipc, &tag_item, &tag_name, &tag_value )) <= list_size)) {
						printk ("%s: %s - %s", tag_item, tag_name, tag_value);
						krad_radio_gtk_set_tag (krad_radio_gtk, tag_item, tag_name, tag_value);
						if (bytes_read == list_size) {
							break;
						}
					}
					break;
				case EBML_ID_KRAD_RADIO_TAG:
					krad_ipc_client_read_tag_inner ( krad_ipc, &tag_item, &tag_name, &tag_value );
					krad_radio_gtk_set_tag (krad_radio_gtk, tag_item, tag_name, tag_value);					
					printk ("%s: %s - %s", tag_item, tag_name, tag_value);
					break;

				case EBML_ID_KRAD_RADIO_UPTIME:
					number = krad_ebml_read_number (krad_ipc->krad_ebml, ebml_data_size);
					printk ("Uptime: %"PRIu64"", number);
					break;
				case EBML_ID_KRAD_RADIO_SYSTEM_INFO:
					krad_ebml_read_string (krad_ipc->krad_ebml, string, ebml_data_size);
					printk ("%s", string);
					break;

			}
	
	
			break;
	
		case EBML_ID_KRAD_LINK_MSG:
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);

			switch ( ebml_id ) {
				case EBML_ID_KRAD_LINK_LINK_LIST:
					//printkd ("Received LINK control list %"PRIu64" bytes of data.\n", ebml_data_size);

					list_size = ebml_data_size;
					i = 0;
					while ((list_size) && ((bytes_read += krad_ipc_client_read_link ( krad_ipc, string, NULL)) <= list_size)) {
						printkd ("%d: %s\n", i, string);
						i++;
						if (bytes_read == list_size) {
							break;
						}
					}	
					break;

				default:
					printkd ("Received KRAD_LINK_MSG %"PRIu64" bytes of data.\n", ebml_data_size);
					break;
			}

			break;
			
		case EBML_ID_KRAD_MIXER_MSG:
			//printkd ("krad_radio_gtk_ipc_handler got message from krad mixer\n");
//			krad_ipc_server_broadcast ( krad_ipc, EBML_ID_KRAD_MIXER_MSG, EBML_ID_KRAD_MIXER_CONTROL, portname, controlname, floatval);
			krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);
			
			switch ( ebml_id ) {
				case EBML_ID_KRAD_MIXER_CONTROL:
					//printkd ("Received mixer control list %zu bytes of data.\n", ebml_data_size);

					krad_ipc_client_read_mixer_control ( krad_ipc, &portname, &controlname, &floatval );
					
					krad_radio_gtk_set_control ( krad_radio_gtk, portname, controlname, floatval);
					
					break;	
				case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
					//printkd ("Received PORTGROUP list %zu bytes of data.\n", ebml_data_size);
					list_size = ebml_data_size;
					while ((list_size) && ((bytes_read += krad_ipc_client_read_portgroup ( krad_ipc, portname, &floatval, crossfadename, &crossfade )) <= list_size)) {
						add_portgroup (krad_radio_gtk, portname, floatval, crossfadename, crossfade);
						if (bytes_read == list_size) {
							break;
						}
					}	
					break;
				case EBML_ID_KRAD_MIXER_PORTGROUP_CREATED:
					//printkd ("PORTGROUP_CREATED msg %zu bytes  \n", ebml_data_size );
					
					krad_ipc_client_read_portgroup ( krad_ipc, portname, &floatval, crossfadename, &crossfade );
					add_portgroup (krad_radio_gtk, portname, floatval, crossfadename, crossfade);
					
					break;
				case EBML_ID_KRAD_MIXER_PORTGROUP_DESTROYED:
				
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);	

					if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
						//printkd ("hrm wtf2\n");
					} else {
						//printkd ("tag name size %zu\n", ebml_data_size);
					}
				
					krad_ebml_read_string (krad_ipc->krad_ebml, portname_actual, ebml_data_size);
					//printkd ("PORTGROUP_DESTROYED msg %zu bytes  \n", ebml_data_size );
					
					remove_portgroup (krad_radio_gtk, portname_actual);
					
					break;
				case EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED:
					//printkd ("PORTGROUP_UPDATED msg %"PRIu64" bytes  \n", ebml_data_size );
				
					krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);

					if (ebml_id == EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {				
			
						krad_ebml_read_string (krad_ipc->krad_ebml, portname_actual, ebml_data_size);
			
			
						krad_ebml_read_element (krad_ipc->krad_ebml, &ebml_id, &ebml_data_size);	

						if (ebml_id == EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
					
							krad_ebml_read_string (krad_ipc->krad_ebml, crossfadename, ebml_data_size);
					
							//printkd ("ya %s %s\n", portname_actual, crossfadename);
					
						}
					}
				
				
					break;
					
			}
		
		
			break;
	
	}

	return 0;

}


int main (int argc, char *argv[]) {
	
	krad_radio_gtk_t *krad_radio_gtk = calloc (1, sizeof(krad_radio_gtk_t));
	
	strcpy (krad_radio_gtk->sysname, argv[1]);
	
	krad_radio_gtk->portgroups = calloc (PORTGROUPS_MAX, sizeof(krad_radio_gtk_portgroup_t));
	
	krad_radio_gtk->krad_osc = krad_osc_create ();

	krad_radio_gtk->krad_alsa_seq = krad_alsa_seq_create ();
	
	if (argc == 3) {
		if (strncmp(argv[2], "alsa", 4) == 0) {
			krad_alsa_seq_run (krad_radio_gtk->krad_alsa_seq, argv[1]);
		} else {
	
			krad_osc_listen (krad_radio_gtk->krad_osc, atoi(argv[2]));
		}
	}
	
	if (!krad_valid_host_and_port (krad_radio_gtk->sysname)) {
		if (!krad_valid_sysname(krad_radio_gtk->sysname)) {
			failfast ("");
		}
	}
	
	krad_radio_gtk->client = krad_ipc_connect (krad_radio_gtk->sysname);
	
	if (krad_radio_gtk->client == NULL) {
		failfast ("");
	}
	//FIXME TEMP KLUDGE
	krad_alsa_seq_set_ipc_client (krad_radio_gtk->krad_alsa_seq, krad_radio_gtk->client);
	
	krad_ipc_set_handler_callback (krad_radio_gtk->client, krad_radio_gtk_ipc_handler, krad_radio_gtk);
	
	krad_ipc_get_portgroups (krad_radio_gtk->client);
	krad_ipc_list_links (krad_radio_gtk->client);
	krad_ipc_get_tags (krad_radio_gtk->client, NULL);
	gtk_init (&argc, &argv);

	krad_radio_gtk->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (krad_radio_gtk->window), "Krad Radio");

	gtk_window_resize (GTK_WINDOW (krad_radio_gtk->window), 600, 400);

	g_signal_connect (krad_radio_gtk->window, "delete-event", G_CALLBACK (on_delete_event), krad_radio_gtk);
	g_signal_connect (krad_radio_gtk->window, "destroy", G_CALLBACK (gtk_main_quit), krad_radio_gtk);

	krad_radio_gtk->label = gtk_label_new (krad_radio_gtk->sysname);

	krad_radio_gtk->vbox = gtk_box_new ( GTK_ORIENTATION_VERTICAL, 15 );
	krad_radio_gtk->hbox_volumes = gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 15 );
	krad_radio_gtk->hbox_crossfades = gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 15 );
	
	gtk_box_pack_start (GTK_BOX(krad_radio_gtk->vbox), krad_radio_gtk->label, FALSE, FALSE, 5);
	gtk_box_pack_end (GTK_BOX(krad_radio_gtk->vbox), krad_radio_gtk->hbox_crossfades, FALSE, FALSE, 5);	
	gtk_box_pack_end (GTK_BOX(krad_radio_gtk->vbox), krad_radio_gtk->hbox_volumes, TRUE, TRUE, 5);	
	
	gdk_threads_add_timeout (13, krad_ipc_poll, krad_radio_gtk);

	gtk_container_add (GTK_CONTAINER (krad_radio_gtk->window), krad_radio_gtk->vbox);
	
	gtk_container_set_border_width (GTK_CONTAINER (krad_radio_gtk->window), 20);

	//gtk_window_set_icon_from_file (GTK_WINDOW (krad_radio_gtk->window), "/home/oneman/kode/krad_radio_info_logo2.png", NULL);

	gtk_widget_show_all (krad_radio_gtk->window);

	gtk_main ();

	if (krad_radio_gtk->krad_osc != NULL) {
		krad_osc_destroy (krad_radio_gtk->krad_osc);
		krad_radio_gtk->krad_osc = NULL;
	}

	if (krad_radio_gtk->krad_alsa_seq != NULL) {
		krad_alsa_seq_destroy (krad_radio_gtk->krad_alsa_seq);
		krad_radio_gtk->krad_alsa_seq = NULL;
	}

	free (krad_radio_gtk);

	return 0;
}
