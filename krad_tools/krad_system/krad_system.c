#include "krad_system.h"

int do_shutdown;
int verbose;

void failfast (char* format, ...) {

	va_list args;


	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);

	exit (1);
	
}

void printke (char* format, ...) {

	va_list args;

	printf ("\nERROR!\n");

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
}

void printk (char* format, ...) {

	va_list args;

	if (verbose) {
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
	}
}


void krad_system_init () {

	do_shutdown = 0;
	verbose = 1;

}

void krad_system_daemonize () {

	pid_t pid, sid;

	close (STDIN_FILENO);
	close (STDOUT_FILENO);
	close (STDERR_FILENO);

	pid = fork();

	if (pid < 0) {
		exit (EXIT_FAILURE);
	}

	if (pid > 0) {
		exit (EXIT_SUCCESS);
	}
	
	umask(0);
 
	sid = setsid();
	
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}
        
}
