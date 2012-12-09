#include "kr_client.h"

int main (int argc, char *argv[]) {

	kr_client_t *client;
	char *sysname;

	sysname = NULL;
  client = NULL;

	if (argc < 2) {
    fprintf (stderr, "Specify a station sysname!\n");
    return 1;
	}

	if (!krad_valid_host_and_port (argv[1])) {
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

  printf ("Disconnecting from %s..\n", sysname);
  kr_disconnect (&client);
  printf ("Disconnected from %s.\n", sysname);

	return 0;

}
