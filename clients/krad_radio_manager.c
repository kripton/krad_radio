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

#include "kr_client.h"

#define PROGRAM_NAME "Krad Radio Manager"

typedef struct kr_manager_St kr_manager_t;

enum {
  LIST_ITEM = 0,
  N_COLUMNS
};

struct kr_manager_St {

  GtkWidget *statusbar;
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *buttonbox;
  GtkWidget *buttonbox_bottom;  
  GtkWidget *button;
  GtkWidget *new_button;
  GtkWidget *list;

  GtkStatusIcon *status_icon;

  char *running_stations;
  char *last_running_stations;

  GtkEntryBuffer *new_station_name;

};

static gboolean on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data) {
  return FALSE;
}

static void add_to_list (kr_manager_t *kr_manager, const gchar *str) {

  GtkListStore *store;
  GtkTreeIter iter;

  store = GTK_LIST_STORE(gtk_tree_view_get_model
      (GTK_TREE_VIEW(kr_manager->list)));

  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, LIST_ITEM, str, -1);
}

static void clear_list (kr_manager_t *kr_manager) {

  GtkListStore *store;
  store = GTK_LIST_STORE(gtk_tree_view_get_model
      (GTK_TREE_VIEW(kr_manager->list)));
  gtk_list_store_clear(store);
}

void check_uptime (kr_manager_t *kr_manager) {

  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;  
  char *name;
  char *result;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(kr_manager->list));
  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {

    gtk_tree_model_get(model, &iter, LIST_ITEM, &name,  -1);

    result = kr_station_uptime (name);

    if (result != NULL) {
      gtk_statusbar_push(GTK_STATUSBAR(kr_manager->statusbar),
          gtk_statusbar_get_context_id(GTK_STATUSBAR(kr_manager->statusbar), 
              name), result);
    }
    g_free(name);
  }
}

static gboolean destroy_selected (gpointer data) {

  kr_manager_t *kr_manager = (kr_manager_t *)data;
  int ret;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  gchar *name;
  char *result;

  selection = NULL;
  name = NULL;
  result = NULL;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(kr_manager->list));

  if ((selection != NULL) && (gtk_tree_selection_get_selected(selection, &model, &iter))) {
    gtk_tree_model_get (model, &iter, LIST_ITEM, &name, -1);
    ret = krad_radio_destroy (name);
    if (ret == 0) {
      result = "Daemon shutdown";
    }
    if (ret == 1) {
      result = "Daemon was killed";
    }
    if (ret == -1) {
      result = "Daemon was not running";
    }

    gtk_statusbar_push(GTK_STATUSBAR(kr_manager->statusbar),
          gtk_statusbar_get_context_id(GTK_STATUSBAR(kr_manager->statusbar), 
              name), result);

    if (name != NULL) {
      g_free (name);
    }
  }

  return TRUE;
}

static gboolean kr_manager_check_running_stations (gpointer data) {

  kr_manager_t *kr_manager = (kr_manager_t *)data;
  char *station;
  
  kr_manager->running_stations = krad_radio_running_stations ();

  if (kr_manager->last_running_stations != NULL) {
    if ((strlen(kr_manager->running_stations) == strlen(kr_manager->last_running_stations)) &&
        (strncmp(kr_manager->running_stations,
                 kr_manager->last_running_stations,
                 strlen(kr_manager->running_stations)) == 0)) {
        check_uptime (kr_manager);
        return TRUE;
    }
    free (kr_manager->last_running_stations);
  }

  check_uptime (kr_manager);

  kr_manager->last_running_stations = strdup (kr_manager->running_stations);
  if (kr_manager->status_icon != NULL) {
    gtk_status_icon_set_tooltip_text(GTK_STATUS_ICON(kr_manager->status_icon), kr_manager->running_stations);
  }

  clear_list (kr_manager);
  station = strtok(kr_manager->running_stations, "\n");

  while (station != NULL) {
    add_to_list (kr_manager, station);
    station = strtok(NULL, "\n");    
  }

  return TRUE;
}

void create_station_window (gpointer data) {

  kr_manager_t *kr_manager = (kr_manager_t *)data;

  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *content_area;
  GtkWidget *station_name;
  gint result;
  char *name;  
  
  dialog = gtk_dialog_new_with_buttons ("Create Station",
                                         GTK_WINDOW(kr_manager->window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "Cancel",
                                         GTK_RESPONSE_NONE,
                                         "Create",
                                         GTK_RESPONSE_YES,
                                         NULL);
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  label = gtk_label_new ("Station Name");
  gtk_entry_buffer_set_text (kr_manager->new_station_name, "", 0);
  station_name = gtk_entry_new_with_buffer (kr_manager->new_station_name);

  gtk_container_add (GTK_CONTAINER (content_area), label);
  gtk_container_add (GTK_CONTAINER (content_area), station_name);
  gtk_window_set_modal (GTK_WINDOW(dialog), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (content_area), 20);  
  gtk_widget_show_all (content_area);

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  switch (result) {
    case GTK_RESPONSE_YES:
      //printf("%s\n", gtk_entry_buffer_get_text (kr_manager->new_station_name));
      name = (char *)gtk_entry_buffer_get_text (kr_manager->new_station_name);
      if (strlen(name) > 3) {
        krad_radio_launch (name);
      }      
      break;
    default:
      break;
  }
  gtk_widget_destroy (dialog);
}

int main (int argc, char *argv[]) {

  kr_manager_t *kr_manager = calloc (1, sizeof(kr_manager_t));

  gtk_init (&argc, &argv);

  kr_manager->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (kr_manager->window), PROGRAM_NAME);
  gtk_window_set_position (GTK_WINDOW (kr_manager->window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (kr_manager->window), 600, 400);
  g_signal_connect (kr_manager->window, "delete-event", G_CALLBACK (on_delete_event), kr_manager);
  g_signal_connect (kr_manager->window, "destroy", G_CALLBACK (gtk_main_quit), kr_manager);
  kr_manager->label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (kr_manager->label), "<span size=\"34000\"><b>"PROGRAM_NAME"</b></span>");
  kr_manager->vbox = gtk_box_new ( GTK_ORIENTATION_VERTICAL, 5 );
  kr_manager->buttonbox = gtk_button_box_new ( GTK_ORIENTATION_HORIZONTAL );
  gtk_button_box_set_layout ( GTK_BUTTON_BOX(kr_manager->buttonbox), GTK_BUTTONBOX_START );
  kr_manager->buttonbox_bottom = gtk_button_box_new ( GTK_ORIENTATION_HORIZONTAL );
  gtk_button_box_set_layout ( GTK_BUTTON_BOX(kr_manager->buttonbox_bottom), GTK_BUTTONBOX_START );  
  kr_manager->new_button = gtk_button_new_with_label ("New");
  gtk_box_pack_start (GTK_BOX(kr_manager->vbox), kr_manager->label, FALSE, FALSE, 15);
  gtk_box_pack_start (GTK_BOX(kr_manager->vbox), kr_manager->buttonbox, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX(kr_manager->buttonbox), kr_manager->new_button, FALSE, TRUE, 15);

  kr_manager->list = gtk_tree_view_new ();
  gtk_box_pack_start (GTK_BOX(kr_manager->vbox), kr_manager->list, TRUE, TRUE, 0);

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *store;

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Active Stations",
                                                    renderer, "text", LIST_ITEM, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (kr_manager->list), column);
  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (kr_manager->list), GTK_TREE_MODEL (store));
  g_object_unref (store);

  kr_manager_check_running_stations (kr_manager);

  kr_manager->button = gtk_button_new_with_label ("Destroy Selected");

  g_signal_connect_swapped (kr_manager->button, "clicked",
          G_CALLBACK (destroy_selected),
          kr_manager);

  g_signal_connect_swapped (kr_manager->new_button, "clicked",
          G_CALLBACK (create_station_window),
          kr_manager);

  gtk_box_pack_start (GTK_BOX(kr_manager->vbox), kr_manager->buttonbox_bottom, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX(kr_manager->buttonbox_bottom), kr_manager->button, FALSE, FALSE, 5);
  kr_manager->statusbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(kr_manager->vbox), kr_manager->statusbar, FALSE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (kr_manager->window), kr_manager->vbox);
  gtk_container_set_border_width (GTK_CONTAINER (kr_manager->window), 20);

  kr_manager->new_station_name = gtk_entry_buffer_new ("", 0);

  //kr_manager->status_icon = gtk_status_icon_new_from_stock(GTK_STOCK_MEDIA_RECORD);
  //gtk_status_icon_set_tooltip_text(GTK_STATUS_ICON(kr_manager->status_icon), "Krad Radio Manager");
  //gtk_status_icon_set_visible(kr_manager->status_icon, TRUE);
  //printf("embedded: %s", gtk_status_icon_is_embedded(kr_manager->status_icon) ? "yes" : "no");

  gtk_widget_show_all (kr_manager->window);
  gdk_threads_add_timeout (1000, kr_manager_check_running_stations, kr_manager);
  gtk_main ();
  free (kr_manager);
  return 0;
}
