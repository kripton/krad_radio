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

/* the implementation of this is very silly */
char *kr_station_uptime (char *sysname) {

  krad_ipc_client_t *client;
  kr_client_t *kr_client;
  
  kr_client = kr_connect (sysname);

  if (kr_client == NULL) {
    fprintf (stderr, "Could not connect to %s krad radio daemon\n", sysname);
    return NULL;
  }
  
  kr_uptime (kr_client);
  
  static char uptime[256];
  
  strcat (uptime, "");
  
  client = kr_client->krad_ipc_client;

  uint32_t ebml_id;
  uint64_t ebml_data_size;
  fd_set set;

  int updays, uphours, upminutes;  
  uint64_t number;
  
  struct timeval tv;
  int ret;
  
  ret = 0;
  if (client->tcp_port) {
    tv.tv_sec = 1;
  } else {
      tv.tv_sec = 0;
  }
    tv.tv_usec = 500000;
  
  ebml_id = 0;
  ebml_data_size = 0;
  
  FD_ZERO (&set);
  FD_SET (client->sd, &set);  

  ret = select (client->sd+1, &set, NULL, NULL, &tv);
  
  if (ret > 0) {
    krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
    switch ( ebml_id ) {
      case EBML_ID_KRAD_RADIO_MSG:
        krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
        switch ( ebml_id ) {
          case EBML_ID_KRAD_RADIO_UPTIME:
            number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
            //printf("up ");
            updays = number / (60*60*24);
            if (updays) {
              sprintf (uptime, "Uptime: %d day%s, ", updays, (updays != 1) ? "s" : "");
            }
            upminutes = number / 60;
            uphours = (upminutes / 60) % 24;
            upminutes %= 60;
            if (uphours) {
              sprintf (uptime, "Uptime: %2d:%02d ", uphours, upminutes);
            } else {
              sprintf (uptime, "Uptime: %d min ", upminutes);
            }
            break;
        }
    }
  }
  
  kr_disconnect (&kr_client);
  
  return uptime;
  
}
