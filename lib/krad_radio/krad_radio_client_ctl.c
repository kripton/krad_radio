#include "krad_radio_client_ctl.h"
#include "krad_radio_client.h"
#include "krad_radio_client_internal.h"

static int krad_radio_pid (char *sysname);

char *krad_radio_running_stations () {

	char *unix_sockets;
	int fd;
	int bytes;
	int pos;
	int flag_check;
	int flag_pos;
	static char list[4096];
	
	memset (list, '\0', sizeof(list));
	
	fd = open ( "/proc/net/unix", O_RDONLY );
	
	if (fd < 1) {
		printke ("krad_radio_list_running_daemons: Could not open /proc/net/unix");
		return NULL;
	}
	
	unix_sockets = malloc (512000);
	
	bytes = read (fd, unix_sockets, 512000);	
	
	if (bytes > 512000) {
		printke("lots of unix sockets oh my");
	}
	
	for (pos = 0; pos < bytes - 12; pos++) {	
		if (unix_sockets[pos] == '@') {
			if (memcmp(unix_sockets + pos, "@krad_radio_", 12) == 0) {
			
				/* back up a few spaces and check that its a listening socket */
				flag_pos = 0;
				flag_check = 5;
				while (flag_check != 0) {
					flag_pos--;
					if (unix_sockets[pos + flag_pos] == ' ') {
						flag_check--;
					}
				}
				flag_pos++;
				if (memcmp(unix_sockets + (pos + flag_pos), "00010000", 8) == 0) {
					strncat(list, unix_sockets + pos + 12, strcspn(unix_sockets + pos + 12, "_"));
					strcat(list, "\n");
				}
			}
		}
	}
	
	list[strlen(list) - 1] = '\0';
	
	free (unix_sockets);
	
	return list;
	
}

static int krad_radio_pid (char *sysname) {

	DIR *dp;
	struct dirent *ep;
	char cmdline[512];
	char search[128];
	int searchlen;
	char cmdline_file[128];	
	int fd;
	int bytes;
	int pid;
	
  if (!(krad_valid_sysname(sysname))) {
    return 0;
  }
	
	pid = 0;
	memset(search, '\0', sizeof(search));	
	strcpy (search, "krad_radio_daemon");
	strcpy (search + 18, sysname);
	searchlen = 18 + strlen(sysname);
	memset(cmdline, '\0', sizeof(cmdline));
	memset(cmdline_file, '\0', sizeof(cmdline_file));
	
	dp = opendir ("/proc");
	
	if (dp == NULL) {
		printke ("Couldn't open the /proc directory");
		return 0;
	}
	
	while ((ep = readdir(dp))) {
		if (isdigit(ep->d_name[0])) {
			sprintf (cmdline_file, "/proc/%s/cmdline", ep->d_name);
			fd = open ( cmdline_file, O_RDONLY );
			if (fd > 0) {
				bytes = read (fd, cmdline, sizeof(cmdline));
				if (bytes > 0) {
					if (bytes == searchlen + 1) {
						if (memcmp(cmdline, search, searchlen) == 0) {
							pid = strtoul(ep->d_name, NULL, 10);
						}
					}
				}
				close (fd);
				if (pid != 0) {
					return pid;
				}
			}
		}
	}
	closedir (dp);

	return 0;

}

int krad_radio_running (char *sysname) {
  if ((krad_radio_pid (sysname)) > 0) {
    return 1;
  }
  
  return 0; 
}

int krad_radio_destroy (char *sysname) {

	int pid;
	int wait_time_total;
	int wait_time_interval;	
	int clean_shutdown_wait_time_limit;
	
	pid = 0;
	wait_time_total = 0;
	clean_shutdown_wait_time_limit = 3000000;
	wait_time_interval = clean_shutdown_wait_time_limit / 40;
		
	pid = krad_radio_pid (sysname);
	
	if (pid != 0) {
		kill (pid, 15);
		while ((pid != 0) && (wait_time_total < clean_shutdown_wait_time_limit)) {
			usleep (wait_time_interval);
			wait_time_total += wait_time_interval;
			pid = krad_radio_pid (sysname);
		}
		pid = krad_radio_pid (sysname);
		if (pid != 0) {
			kill (pid, 9);
			return 1;
		} else {
			return 0;
		}
	}
	
	return -1;
}

void krad_radio_launch (char *sysname) {

	pid_t pid;
	FILE *refp;

	pid = fork();

	if (pid < 0) {
		exit (1);
	}

	if (pid > 0) {
		if (waitpid(pid, NULL, 0) != pid) {
			failfast ("waitpid error launching daemon!");
		}
		return;
	}

	pid = fork();

	if (pid < 0) {
		exit (1);
	}

	if (pid > 0) {
		exit (0);
	}
	
	refp = freopen ("/dev/null", "r", stdin);
  if (refp == NULL) {
	  exit(1);
  }	
	refp = freopen ("/dev/null", "w", stdout);
  if (refp == NULL) {
	  exit(1);
  }	
	refp = freopen ("/dev/null", "w", stderr);	
  if (refp == NULL) {
	  exit(1);
  }	

	execlp ("krad_radio_daemon", "krad_radio_daemon", sysname, (char *)NULL);

}

