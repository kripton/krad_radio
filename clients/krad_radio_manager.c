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

/*  Somehow I enjoy writing sloppy GTK code 
    Its just an example..
*/


typedef struct kr_manager_St kr_manager_t;

enum
{
  LIST_ITEM = 0,
  N_COLUMNS
};

struct kr_manager_St {

  GtkWidget *statusbar;
  GtkWidget *window;
  GtkWidget *stations_label;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox_stations;
  GtkWidget *button;
  
  GtkWidget *list;

  char *running_stations;

  char *last_running_stations;

};

static gboolean on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data) {
  return FALSE;
}

static void
add_to_list (kr_manager_t *kr_manager, const gchar *str) {
  GtkListStore *store;
  GtkTreeIter iter;

  store = GTK_LIST_STORE(gtk_tree_view_get_model
      (GTK_TREE_VIEW(kr_manager->list)));

  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, LIST_ITEM, str, -1);
}

static void
clear_list (kr_manager_t *kr_manager) {
  GtkListStore *store;

  store = GTK_LIST_STORE(gtk_tree_view_get_model
      (GTK_TREE_VIEW(kr_manager->list)));

  gtk_list_store_clear(store);

}

void check_uptime (kr_manager_t *kr_manager) 
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;  
  char *name;
  char *result;


  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(kr_manager->list));
  if (gtk_tree_selection_get_selected(selection, &model, &iter))
  {

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
      
  name = NULL;
  result = NULL;


  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(kr_manager->list));
  if (gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gtk_tree_model_get (model, &iter, LIST_ITEM, &name, -1);

    //g_print ("selected row is: %s\n", name);

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

  }
  else
  {
    result = "Nothing Selected";
  }
  
  
  gtk_statusbar_push(GTK_STATUSBAR(kr_manager->statusbar),
        gtk_statusbar_get_context_id(GTK_STATUSBAR(kr_manager->statusbar), 
            name), result);  
  
  if (name != NULL) {
    g_free(name);
  }
  
  return TRUE;  
  
}

static gboolean krad_radio_manager_check_running_stations (gpointer data) {

  kr_manager_t *kr_manager = (kr_manager_t *)data;
  //char *markup;

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

  //markup = g_markup_printf_escaped ("<span size=\"26000\">%s</span>", kr_manager->running_stations);
  //gtk_label_set_markup (GTK_LABEL (kr_manager->stations_label), markup);

  //g_free (markup);

  clear_list (kr_manager);

  char *station;
  station = strtok(kr_manager->running_stations, "\n");

  while (station != NULL) {
    //printf("station found: --%s--\n", station);
    add_to_list (kr_manager, station);
    station = strtok(NULL, "\n");    
  }

  return TRUE;

}

int main (int argc, char *argv[]) {

  kr_manager_t *kr_manager = calloc (1, sizeof(kr_manager_t));

  gtk_init (&argc, &argv);

  kr_manager->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (kr_manager->window), PROGRAM_NAME);

  gtk_window_resize (GTK_WINDOW (kr_manager->window), 600, 400);

  g_signal_connect (kr_manager->window, "delete-event", G_CALLBACK (on_delete_event), kr_manager);
  g_signal_connect (kr_manager->window, "destroy", G_CALLBACK (gtk_main_quit), kr_manager);

  kr_manager->label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (kr_manager->label), "<span size=\"34000\"><b>"PROGRAM_NAME"</b></span>");

  kr_manager->vbox = gtk_box_new ( GTK_ORIENTATION_VERTICAL, 5 );
  kr_manager->hbox_stations = gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 5 );
  kr_manager->stations_label = gtk_label_new ("");

  gtk_box_pack_start (GTK_BOX(kr_manager->vbox), kr_manager->label, FALSE, FALSE, 15);
  gtk_box_pack_start (GTK_BOX(kr_manager->vbox), kr_manager->hbox_stations, TRUE, TRUE, 15);  
  gtk_box_pack_start (GTK_BOX(kr_manager->hbox_stations), kr_manager->stations_label, FALSE, FALSE, 5);

  kr_manager->list = gtk_tree_view_new();

  gtk_box_pack_start (GTK_BOX(kr_manager->hbox_stations), kr_manager->list, TRUE, TRUE, 15);

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *store;
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Active Stations",
          renderer, "text", LIST_ITEM, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(kr_manager->list), column);
  store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(kr_manager->list), 
  GTK_TREE_MODEL(store));
  g_object_unref(store);

  krad_radio_manager_check_running_stations (kr_manager);

  kr_manager->button = gtk_button_new_with_label ("Destroy Selected");

  g_signal_connect_swapped (kr_manager->button, "clicked",
          G_CALLBACK (destroy_selected),
          kr_manager);

  gtk_box_pack_start (GTK_BOX(kr_manager->vbox), kr_manager->button, FALSE, FALSE, 5);  

  kr_manager->statusbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(kr_manager->vbox), kr_manager->statusbar, FALSE, TRUE, 1);

  gdk_threads_add_timeout (1000, krad_radio_manager_check_running_stations, kr_manager);

  gtk_container_add (GTK_CONTAINER (kr_manager->window), kr_manager->vbox);

  gtk_container_set_border_width (GTK_CONTAINER (kr_manager->window), 10);

  gtk_widget_show_all (kr_manager->window);

  gtk_main ();

  free (kr_manager);

  return 0;
}
