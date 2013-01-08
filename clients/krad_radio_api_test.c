#include "kr_client.h"

void handle_response (kr_client_t *client) {

  kr_response_t *response;
  char *string;
  int wait_time_ms;
  int length;

  string = NULL;
  response = NULL;  
  wait_time_ms = 250;

  if (kr_poll (client, wait_time_ms)) {
    kr_client_response_get (client, &response);
  
    if (response != NULL) {
      length = kr_response_to_string (response, &string);
      printf ("Response Length: %d\n", length);
      if (length > 0) {
        printf ("Response: %s\n", string);
      }
      kr_response_free_string (&string);
      kr_response_free (&response);
    }
  } else {
    printf ("No response after waiting %dms\n", wait_time_ms);
  }
}


void kr_api_test (kr_client_t *client) {

  kr_system_info (client);
  handle_response (client);

  kr_logname (client);
  handle_response (client);

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

  kr_api_test (client);

  printf ("Disconnecting from %s..\n", sysname);
  kr_disconnect (&client);
  printf ("Disconnected from %s.\n", sysname);

  return 0;

}
