#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>

#include "krad_ebml.h"

#define APPVERSION "Krad EBML Header Test 0.1"

int do_shutdown;
int debug;
int verbose;

void help() {

	printf("%s\n", APPVERSION);

	printf("--doctype=webm --doctype_version=2 --doctype_read_version=2\n");	
	exit (0);

}

void dbg (char* format, ...) {

	va_list args;

	if (debug) {
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
	}
}

void krad_shutdown (int signal) {

	if (do_shutdown == 0) {
		printf("\nShutting down..\n");
		do_shutdown = 1;
	} else {
		printf("\nTerminating.\n");
		exit(1);
	}
	
}


int main (int argc, char *argv[]) {

	int o;
	int option_index;

	char input_filename[512];

	krad_ebml_t *input;
	
	char doctype[512];
	int doctype_version;
	int doctype_read_version;	
	
	memset(input_filename, '\0', sizeof(input_filename));
	memset(doctype, '\0', sizeof(doctype));

	do_shutdown = 0;
	doctype_version = 0;	
	doctype_read_version = 0;
	verbose = 0;
	debug = 0;

	while (1) {

		static struct option long_options[] = {
			{"file",			required_argument, 0, 'f'},
			{"doctype",			required_argument, 0, 't'},			
			{"doctype_version",			required_argument, 0, 'V'},
			{"doctype_read_version",			required_argument, 0, 'r'},
			{"debug",			no_argument, 0, 'd'},
			{"verbose",			no_argument, 0, 'v'},
			{"help",			no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

		option_index = 0;

		o = getopt_long ( argc, argv, "vhfd", long_options, &option_index );

		if (o == -1) {
			break;
		}

		switch ( o ) {
			case 'f':
				strncpy (input_filename, optarg, sizeof(input_filename));
				break;
			case 't':
				strncpy (doctype, optarg, sizeof(doctype));
				break;
			case 'V':
				doctype_version = atoi (optarg);
				break;
			case 'r':
				doctype_read_version = atoi (optarg);
				break;				
			case 'd':
				debug = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				help ();
				break;
			default:
				abort ();
		}
	}

	signal(SIGTERM, krad_shutdown);
	signal(SIGINT, krad_shutdown);

	input = krad_ebml_open_file ( input_filename, KRAD_EBML_IO_READONLY );
	
	if (krad_ebml_check_doctype_header (input->header, doctype, doctype_version, doctype_read_version)) {
		printf("Matched\n");
	} else {
		printf("Did Not Match %s Version: %d Read Version: %d\n", doctype, doctype_version, doctype_read_version);
		krad_ebml_print_ebml_header (input->header);
	}
	
	krad_ebml_destroy ( input );
	
	dbg ("\nClean exit\n");
	
	return 0;

}
