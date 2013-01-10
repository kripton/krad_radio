#include "kr_client.h"

void handle_response (kr_client_t *client) {

  kr_response_t *response;
  char *string;
  int wait_time_ms;
  int length;
  int number;

  number = 0;
  string = NULL;
  response = NULL;  
  wait_time_ms = 250;

  if (kr_poll (client, wait_time_ms)) {
    kr_client_response_get (client, &response);
  
    if (response != NULL) {
      length = kr_response_to_string (response, &string);
      printf ("Response Length: %d\n", length);
      if (length > 0) {
        printf ("Response String: %s\n", string);
        kr_response_free_string (&string);
      }
      if (kr_response_to_int (response, &number)) {
        printf ("Response Int: %d\n", number);
      }
      kr_response_free (&response);
    }
  } else {
    printf ("No response after waiting %dms\n", wait_time_ms);
  }

  printf ("\n");
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
