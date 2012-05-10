#include "krad_system.h"

int do_shutdown;
int verbose;
int krad_system_initialized;
krad_system_t krad_system;

void krad_system_info_print () {
		
	printk ("%s %s %s %s %s\n", krad_system.unix_info.sysname, krad_system.unix_info.nodename, krad_system.unix_info.release,
			krad_system.unix_info.version, krad_system.unix_info.machine);

	printk ("%llu\n", (unsigned long long)krad_system.krad_start_time);

}

void krad_system_info_collect () {
	
	krad_system.krad_start_time = time (NULL);
	
	uname (&krad_system.unix_info);

}

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

	if (krad_system_initialized != 31337) {
		krad_system_initialized = 31337;
		do_shutdown = 0;
		verbose = 1;
		krad_system_info_collect ();
		krad_system_info_print ();
	}
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

int krad_valid_sysname (char *sysname) {
	
	int i = 0;
	char j;
	
	char requirements[512];
	
	sprintf (requirements, "sysname's must be atleast %d characters long, only lowercase letters and numbers, "
						    "begin with a letter, and no longer than %d characters.",
						    KRAD_SYSNAME_MIN, KRAD_SYSNAME_MAX);
	
	
	if (strlen(sysname) < KRAD_SYSNAME_MIN) {
		fprintf (stderr, "sysname %s is invalid, too short!\n", sysname);
		fprintf (stderr, "%s\n", requirements);
		return 0;
	}
	
	if (strlen(sysname) > KRAD_SYSNAME_MAX) {
		fprintf (stderr, "sysname %s is invalid, too long!\n", sysname);
		fprintf (stderr, "%s\n", requirements);
		return 0;
	}
	
	j = sysname[i];
	if (!((isalpha (j)) && (islower (j)))) {
		fprintf (stderr, "sysname %s is invalid, must start with a lowercase letter!\n", sysname);
		fprintf (stderr, "%s\n", requirements);
		return 0;
	}
	i++;

	while (sysname[i]) {
		j = sysname[i];
			if (!isalnum (j)) {
				fprintf (stderr, "sysname %s is invalid, alphanumeric only!\n", sysname);
				fprintf (stderr, "%s\n", requirements);
				return 0;
			}
			if (isalpha (j)) {
				if (!islower (j)) {
					fprintf (stderr, "Sysname %s is invalid lowercase letters only!\n", sysname);
					fprintf (stderr, "%s\n", requirements);
					return 0;
				}
			}
		i++;
	}

	return 1;

}

void krad_get_host_and_port (char *string, char *host, int *port) {
		
	*port = atoi (strchr(string, ':') + 1);
	memset (host, '\0', 128);
	memcpy (host, string, strcspn (string, ":"));

}

//FIXME not actually any good

int krad_valid_host_and_port (char *string) {

	int port;
	struct in_addr ia;
	char host[128];
			
	if (strchr(string, ':') != NULL) {
			
		port = atoi (strchr(string, ':') + 1);
		memset (host, '\0', 128);
		memcpy (host, string, strcspn (string, ":"));

//		if (((port > 1) && (port < 65000)) && (inet_aton(host, &ia) != 0)) {
		if (((port > 1) && (port < 65000)) && (strlen(host) > 3)) {

			//printk ("Got host %s and port %d\n", host, port);
			return 1;
		} else {
			printke ("INVALID host %s port %d\n", host, port);
		}
	}
	
	return 0;

}
