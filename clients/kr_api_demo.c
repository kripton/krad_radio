#include "kr_client.h"

void my_tag_print (kr_tag_t *tag) {

  printf ("The tag I wanted: %s - %s\n",
          tag->name,
          tag->value);

}

void my_remote_print (kr_remote_t *remote) {

  printf ("oh its a remote! %d on interface %s\n",
          remote->port,
          remote->interface);

}

void my_portgroup_print (kr_mixer_portgroup_t *portgroup) {

  printf ("oh its a portgroup called %s and the volume is %0.2f%%\n",
           portgroup->sysname,
           portgroup->volume[0]);

}

void my_rep_print (kr_rep_t *rep) {
  switch ( rep->type ) {
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS:
      my_remote_print (rep->rep_ptr.remote);
      return;
    case EBML_ID_KRAD_RADIO_TAG:
      my_tag_print (rep->rep_ptr.tag);
      return;
    case EBML_ID_KRAD_MIXER_PORTGROUP:
      my_portgroup_print (rep->rep_ptr.mixer_portgroup);
      return;
  }
}


void handle_response (kr_client_t *client) {

  kr_response_t *response;
  kr_item_t *item;
  kr_rep_t *rep;
  char *string;
  int wait_time_ms;
  int length;
  int number;
  int i;
  int items;
  

  items = 0;
  i = 0;
  number = 0;
  string = NULL;
  response = NULL;
  rep = NULL;
  wait_time_ms = 250;

  if (kr_poll (client, wait_time_ms)) {
    kr_client_response_get (client, &response);
  
    if (response != NULL) {
      if (kr_response_is_list (response)) {
        items = kr_response_list_length (response);
        printf ("Response is a list with %d items.\n", items);
        for (i = 0; i < items; i++) {
          if (kr_response_list_get_item (response, i, &item)) {
            printf ("Got item %d type is %s\n", i, kr_item_get_type_string (item));
            if (kr_item_to_string (item, &string)) {
              printf ("Item String: %s\n", string);
              kr_response_free_string (&string);
            }
            rep = kr_item_to_rep (item);
            if (rep != NULL) {
              my_rep_print (rep);
              kr_rep_free (&rep);
            }
          } else {
            printf ("Did not get item %d\n", i);
          }
        }
      }
      length = kr_response_to_string (response, &string);
      printf ("Response Length: %d\n", length);
      if (length > 0) {
        printf ("Response String: %s\n", string);
        kr_response_free_string (&string);
      }
      if (kr_response_to_int (response, &number)) {
        printf ("Response Int: %d\n", number);
      }
      if (kr_response_get_item (response, &item)) {
        printk ("Got item.. type is %s\n", kr_item_get_type_string (item));
        if (kr_item_to_string (item, &string)) {
          printf ("Item String: %s\n", string);
          kr_response_free_string (&string);
        }
        //rep = kr_item_to_rep (item);
        //if (rep != NULL) {
        //  rep_to_json (kr_ws_client, rep);
        //  kr_rep_free (&rep);
        //}
      }
      kr_response_free (&response);
    }
  } else {
    printf ("No response after waiting %dms\n", wait_time_ms);
  }

  printf ("\n");
}

void wait_for_broadcasts (kr_client_t *client) {

  int b;
  int ret;
  uint64_t max;
  unsigned int timeout_ms;
  
  ret = 0;
  b = 0;
  max = 10000000;
  timeout_ms = 3000;
  
  printf ("Waiting for up to %"PRIu64" broadcasts up to %ums each\n", max, timeout_ms);
  
  
  while (b < max) {

    ret = kr_poll (client, timeout_ms);

    if (ret > 0) {
      handle_response (client);
    } else {
      printf (".");
      fflush (stdout);
    }

    b++;
  }

}

void kr_api_test (kr_client_t *client) {

  kr_system_info (client);
  handle_response (client);

  kr_logname (client);
  handle_response (client);

  kr_uptime (client);
  handle_response (client);
  
  kr_system_cpu_usage (client);  
  handle_response (client);
    
  kr_remote_status (client);
  handle_response (client);
  
  kr_tags (client, NULL);
  handle_response (client);
  
  kr_mixer_portgroups_list (client);
  handle_response (client);
  
  printf ("Subscribing to all broadcasts\n");
  kr_broadcast_subscribe (client, ALL_BROADCASTS);
  printf ("Subscribed to all broadcasts\n");

  wait_for_broadcasts (client);
  
}

int main (int argc, char *argv[]) {

  kr_client_t *client;
  char *sysname;

  sysname = NULL;
  client = NULL;

  if (argc < 2) {
    fprintf (stderr, "Specify a station sysname!\n");
    return 1;
  }

  if (krad_valid_host_and_port (argv[1])) {
    sysname = argv[1];
  } else {
    if (!krad_valid_sysname(argv[1])) {
      fprintf (stderr, "Invalid station sysname!\n");
      return 1;
    } else {
      sysname = argv[1];
    }
  }

  client = kr_client_create ("krad api test");

  if (client == NULL) {
    fprintf (stderr, "Could create client\n");
    return 1;
  }

  if (!kr_connect (client, sysname)) {
    fprintf (stderr, "Could not connect to %s krad radio daemon\n", sysname);
    kr_client_destroy (&client);
    return 1;
  }

  printf ("Connected to %s!\n", sysname);

  kr_api_test (client);

  printf ("Disconnecting from %s..\n", sysname);
  kr_disconnect (client);
  printf ("Disconnected from %s.\n", sysname);
  printf ("Destroying client..\n");
  kr_client_destroy (&client);
  printf ("Client Destroyed.\n");
  
  return 0;

}
