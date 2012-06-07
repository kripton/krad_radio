#include "krad_system.h"

int do_shutdown;
int verbose;
int krad_system_initialized;
krad_system_t krad_system;

void krad_system_monitor_cpu_on () {

	if (krad_system.kcm.on == 0) {
		krad_system.kcm.on = 1;
		pthread_create (&krad_system.kcm.monitor_thread, NULL, krad_system_monitor_cpu_thread, NULL);
	}
}

void krad_system_monitor_cpu_off () {

	if (krad_system.kcm.on == 1) {
		krad_system.kcm.on = 2;	
		pthread_join (krad_system.kcm.monitor_thread, NULL);
		krad_system.kcm.on = 0;
	}
}

void *krad_system_monitor_cpu_thread (void *arg) {

	krad_system_cpu_monitor_t *kcm;
	
	printk ("Krad System CPU Monitor On\n");

	kcm = &krad_system.kcm;

	kcm->fd = open ( "/proc/stat", O_RDONLY );

	kcm->interval = KRAD_CPU_MONITOR_INTERVAL;

	while (kcm->on == 1) {
	
		kcm->ret = lseek (kcm->fd, 0, SEEK_SET);

		if (kcm->ret != 0) {
			break;
		}
		
		kcm->ret = read (kcm->fd, kcm->buffer, KRAD_BUFLEN_CPUSTAT);

		if (kcm->ret != KRAD_BUFLEN_CPUSTAT) {
			break;
		}

		sscanf (kcm->buffer + 3, "%d %d %d %d", &kcm->user, &kcm->nice, &kcm->system, &kcm->idle );

		kcm->total = kcm->user + kcm->nice + kcm->system + kcm->idle;

		if (kcm->total != kcm->last_total) {

			kcm->diff_idle = kcm->idle - kcm->last_idle;
			kcm->diff_total = kcm->total - kcm->last_total;

			kcm->usage = (1000 * (kcm->diff_total - kcm->diff_idle) / kcm->diff_total + 5) / 10;

			krad_system.system_cpu_usage = kcm->usage;

			//printk ("user %d nice %d system %d idle %d usage %d\n", 
			//		kcm->user, kcm->nice, kcm->system, kcm->idle, kcm->usage);

			kcm->last_idle = kcm->idle;
			kcm->last_total = kcm->total;

		}

		usleep (kcm->interval * 1000);
	}
	
	close (kcm->fd);
	
	printk ("Krad System CPU Monitor Off\n");
	
	return NULL;

}

char *krad_system_daemon_info () {

	return krad_system.info_string;

}

void krad_system_info_collect () {
	
	krad_system.info_string_len = 0;
	
	krad_system.krad_start_time = time (NULL);
	
	uname (&krad_system.unix_info);

	krad_system.info_string_len += sprintf (krad_system.info_string, "Host: %s\n", krad_system.unix_info.nodename);
	krad_system.info_string_len += sprintf (krad_system.info_string + krad_system.info_string_len, 
											"System: %s %s (%s)\n", krad_system.unix_info.machine, 
											krad_system.unix_info.sysname, krad_system.unix_info.release);

	krad_system.info_string_len += sprintf (krad_system.info_string + krad_system.info_string_len, 
											"Krad Start Time: %llu\n", (unsigned long long)krad_system.krad_start_time);

}

uint64_t krad_system_daemon_uptime () {

	uint64_t now;

	now = time (NULL);
	krad_system.uptime = now - krad_system.krad_start_time;
	
	return krad_system.uptime;

}
//FIXME these prints need locks or single/reader/writer buffers/handles
void failfast (char* format, ...) {

	va_list args;


	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	printf ("\n");
	exit (1);
	
}

void printke (char* format, ...) {

	va_list args;

	printf ("\nERROR!\n");

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
}

void printkd (char* format, ...) {

	va_list args;

	if (verbose) {
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
		printf ("\n");		
	}
}

void printk (char* format, ...) {

	va_list args;

	if (verbose) {
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
		printf ("\n");
	}
}


void krad_system_init () {

	if (krad_system_initialized != 31337) {
		krad_system_initialized = 31337;
		do_shutdown = 0;
		verbose = 1;
		krad_system_info_collect ();
	  	srand (time(NULL));
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
	char host[128];
			
	if (strchr(string, ':') != NULL) {
			
		port = atoi (strchr(string, ':') + 1);
		memset (host, '\0', 128);
		memcpy (host, string, strcspn (string, ":"));

		if (((port > 1) && (port < 65000)) && (strlen(host) > 3)) {
			//printk ("Got host %s and port %d\n", host, port);
			return 1;
		} else {
			printke ("INVALID host %s port %d\n", host, port);
		}
	}
	
	return 0;

}
