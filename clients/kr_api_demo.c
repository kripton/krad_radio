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

void my_mixer_print (kr_mixer_t *mixer, void *user_ptr) {

  printf ("Mixer Sample Rate: %d\n",
					 mixer->sample_rate);
}

void my_print (kr_address_t *address, kr_rep_t *rep) {

  void *user_ptr;
  
  user_ptr = NULL;

  if ((address->path.unit == KR_MIXER) && (address->path.subunit.zero == KR_UNIT)) {
    my_mixer_print (rep->rep_ptr.actual, user_ptr);
  }
  if ((address->path.unit == KR_MIXER) && (address->path.subunit.mixer_subunit == KR_PORTGROUP)) {
    my_portgroup_print (rep->rep_ptr.portgroup, user_ptr);
  }

}

void get_delivery (kr_client_t *client) {

  kr_crate_t *crate;
  kr_address_t *address;
  kr_rep_t *rep;
  char *string;
  int wait_time_ms;
  int integer;
  float real;

  integer = 0;
  real = 0.0f;
  string = NULL;
  crate = NULL;
  rep = NULL;
  wait_time_ms = 250;

  if (!kr_delivery_wait (client, wait_time_ms)) {
    printf ("No delivery after waiting %dms\n", wait_time_ms);
    return;
  }

  printf ("*** Get Delivery Start: \n");

  kr_delivery_get (client, &crate);

  if (crate != NULL) {

    kr_address_get (crate, &address);
    kr_address_debug_print (address); 

    /* Crate sometimes can be converted
       to a integer, float or string */
    
    if (kr_uncrate_string (crate, &string)) {
      printf ("String: \n%s\n", string);
      kr_string_recycle (&string);
    }
    
    if (kr_uncrate_int (crate, &integer)) {
      printf ("Int: %d\n", integer);
    }
    
    if (kr_uncrate_float (crate, &real)) {
      printf ("Float: %f\n", real);
    }
    
    /* Crate has a rep struct */
    
    if (kr_uncrate (crate, &rep)) {
      printf ("Rep\n");
      my_print (address, rep);
      kr_rep_free (&rep);
    }

    kr_crate_recycle (&crate);
    printf ("*** Get Delivery End\n\n\n");
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
    ret = kr_wait (client, timeout_ms);
    if (ret > 0) {
      get_delivery (client);
    } else {
      printf (".");
      fflush (stdout);
    }
  }
}

void kr_api_test (kr_client_t *client) {

  kr_system_info (client);
  get_delivery (client);

  //kr_remote_list (client);
  //get_delivery (client);

  //kr_compositor_info (client);
  //get_delivery (client);
  
  kr_mixer_info (client);
  get_delivery (client);

  kr_mixer_portgroups_list (client);
  get_delivery (client);
  
  printf ("Subscribing to all broadcasts\n");
  kr_subscribe (client, ALL_BROADCASTS);
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
