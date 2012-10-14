#include "krad_system.h"

int do_shutdown;
int verbose;
int krad_system_initialized;
krad_system_t krad_system;

int krad_system_get_cpu_usage () {
	return krad_system.system_cpu_usage;
}

void krad_system_set_monitor_cpu_interval (int ms) {
	krad_system.kcm.interval = ms;
}

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

void krad_system_monitor_cpu_off_fast () {

	if (krad_system.kcm.on == 1) {
		pthread_cancel (krad_system.kcm.monitor_thread);
		krad_system.kcm.on = 0;
		if (krad_system.kcm.fd != 0) {
			close (krad_system.kcm.fd);
			krad_system.kcm.fd = 0;
		}
	}
}

void krad_system_log_on (char *filename) {

	if (krad_system.log_fd > 0) {
		krad_system_log_off ();
	}

	pthread_mutex_lock (&krad_system.log_lock);
	krad_system.log_fd = open (filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (krad_system.log_fd > 0) {
		dprintf (krad_system.log_fd, "Logging started\n");
	} else {
		failfast ("frak");
	}
	pthread_mutex_unlock (&krad_system.log_lock);
}

void krad_system_log_off () {

	pthread_mutex_lock (&krad_system.log_lock);
	if (krad_system.log_fd > 0) {
		fsync (krad_system.log_fd);
		close (krad_system.log_fd);
		krad_system.log_fd = 0;
	}
	pthread_mutex_unlock (&krad_system.log_lock);

}

void *krad_system_monitor_cpu_thread (void *arg) {

	krad_system_cpu_monitor_t *kcm;
	
	krad_system_set_thread_name ("kr_cpu_mon");	
	
	printk ("Krad System CPU Monitor On");

	kcm = &krad_system.kcm;

	kcm->fd = open ( "/proc/stat", O_RDONLY );

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
			
			if (kcm->usage < 0) {
				kcm->usage = 0;
			} else {
				if (kcm->usage > 100) {
					kcm->usage = 100;
				}
			} 

			krad_system.system_cpu_usage = kcm->usage;

			//printk ("user %d nice %d system %d idle %d usage %d\n", 
			//		kcm->user, kcm->nice, kcm->system, kcm->idle, kcm->usage);

			kcm->last_idle = kcm->idle;
			kcm->last_total = kcm->total;
			
			if (kcm->unset_cpu_monitor_callback == 1) {
				kcm->callback_pointer = NULL;
				kcm->cpu_monitor_callback = NULL;
				kcm->unset_cpu_monitor_callback = 0;
			}
			
			if (kcm->cpu_monitor_callback) {
				kcm->cpu_monitor_callback (kcm->callback_pointer, krad_system.system_cpu_usage);
			}

		}

		usleep (kcm->interval * 1000);
	}
	
	close (kcm->fd);
	
	kcm->fd = 0;
	
	printk ("Krad System CPU Monitor Off");
	
	return NULL;

}

void krad_system_unset_monitor_cpu_callback () {
	
	krad_system_cpu_monitor_t *kcm;
	kcm = &krad_system.kcm;
	kcm->unset_cpu_monitor_callback = 1;
									 
}
									 

void krad_system_set_monitor_cpu_callback (void *callback_pointer, 
									 void (*cpu_monitor_callback)( void *, uint32_t)) {
	
	krad_system_cpu_monitor_t *kcm;
	kcm = &krad_system.kcm;
		
	kcm->callback_pointer = callback_pointer;
	kcm->cpu_monitor_callback = cpu_monitor_callback;							 

}


char *krad_system_info () {

	return krad_system.info_string;

}

void krad_system_info_collect () {
	
	krad_system.info_string_len = 0;
	
	krad_system.krad_start_time = time (NULL);
	
	uname (&krad_system.unix_info);

	krad_system.info_string_len += sprintf (krad_system.info_string, "Host: %s\n", krad_system.unix_info.nodename);
	krad_system.info_string_len += sprintf (krad_system.info_string + krad_system.info_string_len, 
											"System: %s %s (%s)", krad_system.unix_info.machine, 
											krad_system.unix_info.sysname, krad_system.unix_info.release);

	//krad_system.info_string_len += sprintf (krad_system.info_string + krad_system.info_string_len, 
	//										"Krad Start Time: %llu", (unsigned long long)krad_system.krad_start_time);

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
	if ((verbose) && (krad_system.log_fd > 0)) {
		pthread_mutex_lock (&krad_system.log_lock);
		dprintf (krad_system.log_fd, "\n***ERROR! FAILURE!\n");
		va_start(args, format);
		vdprintf(krad_system.log_fd, format, args);
		va_end(args);
		dprintf (krad_system.log_fd, "\n");
		pthread_mutex_unlock (&krad_system.log_lock);
		krad_system_log_off ();
	}
	exit (1);
}

void printke (char* format, ...) {

	va_list args;

	if ((verbose) && (krad_system.log_fd > 0)) {
		pthread_mutex_lock (&krad_system.log_lock);		
		dprintf (krad_system.log_fd, "***ERROR!: ");
		va_start(args, format);
		vdprintf(krad_system.log_fd, format, args);
		va_end(args);
		dprintf (krad_system.log_fd, "\n");
		pthread_mutex_unlock (&krad_system.log_lock);
	}
}

void printkd (char* format, ...) {

	va_list args;

	if ((verbose) && (krad_system.log_fd > 0)) {
		pthread_mutex_lock (&krad_system.log_lock);
		va_start(args, format);
		vdprintf(krad_system.log_fd, format, args);
		va_end(args);
		dprintf (krad_system.log_fd, "\n");
		pthread_mutex_unlock (&krad_system.log_lock);
	}
}

void printk (char* format, ...) {

	va_list args;

	if ((verbose) && (krad_system.log_fd > 0)) {
		pthread_mutex_lock (&krad_system.log_lock);
		va_start(args, format);
		vdprintf(krad_system.log_fd, format, args);
		va_end(args);
		dprintf (krad_system.log_fd, "\n");
		pthread_mutex_unlock (&krad_system.log_lock);
	}
}


void krad_system_init () {

	if (krad_system_initialized != 31337) {
		krad_system_initialized = 31337;
		krad_system.kcm.interval = KRAD_CPU_MONITOR_INTERVAL;
		do_shutdown = 0;
		verbose = 0;
		pthread_mutex_init (&krad_system.log_lock, NULL);
		krad_system_info_collect ();
	  	srand (time(NULL));
	}
}

void krad_system_daemonize () {

	pid_t pid, sid;

	pid = fork();

	if (pid < 0) {
		exit (EXIT_FAILURE);
	}

	if (pid > 0) {
		exit (EXIT_SUCCESS);
	}

	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
	
	umask(0);
 
	sid = setsid();
	
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}
        
}

void krad_system_set_thread_name (char *name) {
	if ((name == NULL || strlen (name) >= 15) ||
	    (prctl (PR_SET_NAME, (unsigned long) name, 0, 0, 0) != 0)) {
		printke ("Could not set thread name: %s", name);
	}
}

uint64_t ktime() {

	uint64_t seconds;
	struct timespec ts;

	clock_gettime (CLOCK_REALTIME, &ts);
	seconds = ts.tv_sec;
	
	return seconds;

}

int krad_system_set_socket_nonblocking (int sd) {

	int ret;
	int flags;
	
	flags = 0;
	ret = 0;

	flags = fcntl (sd, F_GETFL, 0);
	if (flags == -1) {
		failfast ("Krad System: error on syscall fcntl F_GETFL");
		return -1;		
	}

	flags |= O_NONBLOCK;
	
	ret = fcntl (sd, F_SETFL, flags);
	if (ret == -1) {
		failfast ("Krad System: error on syscall fcntl F_SETFL");
		return -1;
	}
	
	return sd;
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
			printke ("INVALID host %s port %d", host, port);
		}
	}
	
	return 0;

}
