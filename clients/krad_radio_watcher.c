#include "kr_client.h"

void wait_for_broadcasts (kr_client_t *client) {

  int b;
  int ret;
  int max;
  unsigned int timeout_ms;
  
  ret = 0;
  b = 0;
  max = 100;
  timeout_ms = 1000;
  
  printf ("Waiting for up to %d broadcasts up to %ums each\n", max, timeout_ms);
  
  
  while (b < max) {

    ret = kr_poll (client, timeout_ms);

    if (ret > 0) {
      printf ("Got a broadcast! (%d)\n", b);
      kr_print_message_type (client);
    } else {
      printf ("No broadcast this time :/ (%d)\n", b);
    }

    b++;
  }

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

  client = kr_connect (sysname);

  if (client == NULL) {
    fprintf (stderr, "Could not connect to %s krad radio daemon\n", sysname);
    return 1;
  }

  printf ("Connected to %s!\n", sysname);

  printf ("Subscribing to all broadcasts\n");
  kr_broadcast_subscribe (client, ALL_BROADCASTS);
  printf ("Subscribed to all broadcasts\n");

  wait_for_broadcasts (client);
  
  printf ("Disconnecting from %s..\n", sysname);
  kr_disconnect (&client);
  printf ("Disconnected from %s.\n", sysname);
  return 0;

}
