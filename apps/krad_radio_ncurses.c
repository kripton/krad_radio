#include <ncurses.h>

#include "krad_ipc_client.h"

static void krad_radio_ncurses (krad_ipc_client_t *client);

static void krad_radio_ncurses (krad_ipc_client_t *client) {

	//int row;
	//int col;
	char header[256];

	initscr ();
	start_color ();
	curs_set (2);
	init_pair (1, COLOR_RED, COLOR_BLACK);
	init_pair (2, COLOR_GREEN, COLOR_BLACK);
	cbreak ();
	noecho ();

	sprintf (header, "Krad Radio %s", client->sysname);
	
	attron (A_BOLD);
	//getmaxyx (stdscr, row, col);
	attron (COLOR_PAIR(1));
	mvprintw (2, 4, "%s", header);
	refresh ();
	attroff (A_BOLD);
	attroff (COLOR_PAIR(1));
	curs_set(0);

	while (1) {
		refresh();
		usleep(250000);
	}

}


int main (int argc, char *argv[]) {

	krad_ipc_client_t *client;
	
	if (argc == 1) {
		printf("Specify a station..\n");
	}
	
	
	if (argc == 2) {

		if (!krad_valid_host_and_port (argv[1])) {
			if (!krad_valid_sysname(argv[1])) {
				failfast ("");
			}
		}
	
		client = krad_ipc_connect (argv[1]);
	
		if (client != NULL) {
		
			krad_radio_ncurses (client);
		
			krad_ipc_disconnect (client);
		
		}
		
	}
	
	return 0;
	
}
