#include "kr_client.h"

void my_remote_print (kr_remote_t *remote, void *user_ptr) {
  printf ("oh its a remote! %d on interface %s\n",
          remote->port,
          remote->interface);
}

void my_tag_print (kr_tag_t *tag, void *user_ptr) {
  printf ("The tag I wanted: %s - %s\n",
          tag->name,
          tag->value);
}

void my_portgroup_print (kr_mixer_portgroup_t *portgroup, void *user_ptr) {
  printf ("oh its a portgroup called %s and the volume is %0.2f%%\n",
           portgroup->sysname,
           portgroup->volume[0]);
}

void my_compositor_print (kr_compositor_t *compositor, void *user_ptr) {

  printf ("Compositor Resolution: %d x %d Frame Rate: %d / %d - %f\n",
					 compositor->width, compositor->height,
					 compositor->fps_numerator, compositor->fps_denominator,
					 ((float)compositor->fps_numerator / (float)compositor->fps_denominator));
}

void my_rep_print (kr_rep_t *rep) {

  void *user_ptr;
  
  user_ptr = NULL;

  switch ( rep->type ) {
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS:
      my_remote_print (rep->rep_ptr.actual, user_ptr);
      return;
    case EBML_ID_KRAD_RADIO_TAG:
      my_tag_print (rep->rep_ptr.tag, user_ptr);
      return;
    case EBML_ID_KRAD_MIXER_PORTGROUP:
      my_portgroup_print (rep->rep_ptr.mixer_portgroup, user_ptr);
      return;
    case EBML_ID_KRAD_COMPOSITOR_INFO:
      my_compositor_print (rep->rep_ptr.compositor, user_ptr);
      return;
  }
}

void handle_response (kr_client_t *client) {

  kr_response_t *response;
  kr_address_t *address;
  kr_rep_t *rep;
  char *string;
  int wait_time_ms;
  int length;
  int integer;
  float real;
  int i;
  int items;

  items = 0;
  i = 0;
  integer = 0;
  real = 0.0f;
  string = NULL;
  response = NULL;
  rep = NULL;
  wait_time_ms = 250;

  if (kr_poll (client, wait_time_ms)) {
    printf ("----- Handle Response Start: \n\n");
    kr_client_response_get (client, &response);
    if (response != NULL) {
      kr_response_address (response, &address);
      kr_address_debug_print (address); 
    
      /* Response sometimes is a list */
    
      if (kr_response_is_list (response)) {
        items = kr_response_list_length (response);
        printf ("Response is a list with %d items.\n", items);
        /*
        for (i = 0; i < items; i++) {
          if (kr_response_listitem_to_rep (response, i, &rep)) {
            if (rep != NULL) {
              my_rep_print (rep);
              kr_rep_free (&rep);
            }
          } else {
            printf ("Did not get list item %d rep\n", i);
          }
        }
        */
      }
      
      /* Response sometimes can be converted to a string or int */
      
      length = kr_response_to_string (response, &string);
      printf ("Response Length: %d\n", length);
      if (length > 0) {
        printf ("Response String: %s\n", string);
        kr_response_free_string (&string);
      }
      
      if (kr_response_to_int (response, &integer)) {
        printf ("Response Int: %d\n", integer);
      }
      
      if (kr_response_to_float (response, &real)) {
        printf ("Response Float: %f\n", real);
      }
      
      /* Response sometimes can be converted to a rep struct */
      
      if (kr_response_to_rep (response, &rep)) {
        printf ("Got rep from response!\n");
        my_rep_print (rep);
        kr_rep_free (&rep);
      } else {
        printf ("No rep from response :/\n");
      }

      kr_response_free (&response);
      printf ("----- Handle Response End\n\n\n\n\n");
    }
  } else {
    printf ("No response after waiting %dms\n", wait_time_ms);
  }
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
  
  printf ("Waiting for up to %"PRIu64" broadcasts up to %ums each\n",
          max, timeout_ms);
  
  for (b = 0; b < max; b++) {
    ret = kr_poll (client, timeout_ms);
    if (ret > 0) {
      handle_response (client);
    } else {
      printf (".");
      fflush (stdout);
    }
  }
}

void kr_api_test (kr_client_t *client) {

  //kr_system_info (client);
  //handle_response (client);

  //kr_remote_list (client);
  //handle_response (client);

  //kr_tags (client, NULL);
  //handle_response (client);

  //kr_compositor_info (client);
  //handle_response (client);
  
  kr_mixer_info (client);
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
