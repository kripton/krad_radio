#include "krad_system.h"

int krad_system_initialized;
krad_system_t krad_system;

int krad_control_init (krad_control_t *krad_control) {
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, krad_control->sockets)) {
    printke ("Krad System: can't socketpair errno: %d", errno);
    krad_control->sockets[0] = 0;
    krad_control->sockets[1] = 0;
    return -1;
  }
  return 0;
}

int krad_controller_get_client_fd (krad_control_t *krad_control) {
  if ((krad_control->sockets[0] != 0) && (krad_control->sockets[1] != 0)) {
    return krad_control->sockets[1];
  }
  return -1;
}

int krad_controller_get_controller_fd (krad_control_t *krad_control) {
  if ((krad_control->sockets[0] != 0) && (krad_control->sockets[1] != 0)) {
    return krad_control->sockets[1];
  }
  return -1;
}

int krad_controller_client_close (krad_control_t *krad_control) {
  if ((krad_control->sockets[0] != 0) && (krad_control->sockets[1] != 0)) {
    close (krad_control->sockets[1]);
    krad_control->sockets[1] = 0;
    return 0;
  }
  return -1;
}


int krad_controller_client_wait (krad_control_t *krad_control, int timeout) {

  struct pollfd pollfds[1];
  
  pollfds[0].fd = krad_control->sockets[1];
  pollfds[0].events = POLLIN;
  if (poll (pollfds, 1, timeout) != 0) {
    return 1;
  }
    
  return 0;
}

int krad_controller_shutdown (krad_control_t *krad_control, pthread_t *thread, int timeout) {

  struct pollfd pollfds[1];
  int ret;
  
  timeout = (timeout / 2) + 2;

  pollfds[0].fd = krad_control->sockets[0];
  pollfds[0].events = POLLOUT;
  if (poll (pollfds, 1, timeout) == 1) {
    ret = write (krad_control->sockets[0], "0", 1);
    if (ret == 1) {
      pollfds[0].fd = krad_control->sockets[0];
      pollfds[0].events = POLLIN;
      if (poll (pollfds, 1, timeout) == 1) {
        pthread_join (*thread, NULL);
        close (krad_control->sockets[0]);
        krad_control->sockets[0] = 1;
        return 1;
      }
    }
  }

  return 0;
}

void krad_controller_destroy (krad_control_t *krad_control, pthread_t *thread) {

  if (krad_control->sockets[1] != 0) {
    close (krad_control->sockets[1]);
    krad_control->sockets[1] = 0;
  }
  pthread_cancel (*thread);
  pthread_join (*thread, NULL);
  if (krad_control->sockets[1] != 0) {
    close (krad_control->sockets[1]);
    krad_control->sockets[1] = 0;
  }
}

int krad_system_get_cpu_usage () {
  return krad_system.system_cpu_usage;
}

void krad_system_set_monitor_cpu_interval (int ms) {
  krad_system.kcm.interval = ms;
}

void krad_system_monitor_cpu_on () {

  if (krad_system.kcm.on == 0) {
    if (krad_control_init (&krad_system.kcm.control)) {
      printke ("Krad System: can't socketpair errno: %d", errno);
      return;
    }
    krad_system.kcm.on = 1;
    pthread_create (&krad_system.kcm.monitor_thread, NULL, krad_system_monitor_cpu_thread, NULL);
  }
}

void krad_system_monitor_cpu_off () {

  if (krad_system.kcm.on == 1) {
    krad_system.kcm.on = 2;
    if (krad_controller_shutdown (&krad_system.kcm.control, &krad_system.kcm.monitor_thread, 2)) {
      printk ("Krad System: CPU Monitor exited clean and quick");
    }
    if (krad_system.kcm.on != 0) {
      krad_controller_destroy (&krad_system.kcm.control, &krad_system.kcm.monitor_thread);
      krad_system.kcm.on = 0;
      if (krad_system.kcm.fd != 0) {
        close (krad_system.kcm.fd);
        krad_system.kcm.fd = 0;
      }
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
    dprintf (krad_system.log_fd, "Log %d started\n", krad_system.lognum);
  } else {
    failfast ("frak");
  }
  pthread_mutex_unlock (&krad_system.log_lock);
}

void krad_system_log_off () {

  pthread_mutex_lock (&krad_system.log_lock);
  if (krad_system.log_fd > 0) {
    dprintf (krad_system.log_fd, "Log %d ended\n", krad_system.lognum++);
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

  if (kcm->fd < 1) {
    kcm->on = 0;  
    return NULL;
  }

  while (1) {

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
      //    kcm->user, kcm->nice, kcm->system, kcm->idle, kcm->usage);

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

    if (krad_controller_client_wait (&kcm->control, kcm->interval)) {
      break;
    }
  }
  
  close (kcm->fd);
  kcm->fd = 0;
  krad_controller_client_close (&kcm->control);
  krad_system.kcm.on = 0;
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
}

uint64_t krad_system_daemon_uptime () {

  uint64_t now;

  now = time (NULL);
  krad_system.uptime = now - krad_system.krad_start_time;
  
  return krad_system.uptime;
}

void krad_system_daemon_wait () {

  int signal_caught;

  signal_caught = 0;

  while (1) {

    if (sigwait (&krad_system.signal_mask, &signal_caught) != 0) {
      failfast ("error on sigwait!");
    }
    switch (signal_caught) {
      case SIGHUP:
        printkd ("Got HANGUP Signal!");
        break;
      case SIGINT:
        printkd ("Got SIGINT Signal!");
      case SIGTERM:
        printkd ("Got SIGTERM Signal!");
      default:
        printk ("Krad Radio Shutting down!");
        return;
    }
  }
}


//FIXME these prints need locks or single/reader/writer buffers/handles
void failfast (char* format, ...) {

  va_list args;
  if (krad_system.log_fd > 0) {
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

  if (krad_system.log_fd > 0) {
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

  if (krad_system.log_fd > 0) {
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

  if (krad_system.log_fd > 0) {
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
    pthread_mutex_init (&krad_system.log_lock, NULL);
    krad_system_info_collect ();
      srand (time(NULL));
  }
}

void krad_system_daemonize () {

  pid_t pid, sid;
  FILE *refp;

  pid = fork();

  if (pid < 0) {
    exit (EXIT_FAILURE);
  }

  if (pid > 0) {
    exit (EXIT_SUCCESS);
  }

  refp = freopen("/dev/null", "r", stdin);
  if (refp == NULL) {
    exit(1);
  }
  refp = freopen("/dev/null", "w", stdout);
  if (refp == NULL) {
    exit(1);
  }
  refp = freopen("/dev/null", "w", stderr);
  if (refp == NULL) {
    exit(1);
  }

  umask(0);
 
  sid = setsid();
  
  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

  if ((chdir("/")) < 0) {
    exit(EXIT_FAILURE);
  }
  
  sigemptyset (&krad_system.signal_mask);
  sigaddset (&krad_system.signal_mask, SIGINT);
  sigaddset (&krad_system.signal_mask, SIGTERM);
  sigaddset (&krad_system.signal_mask, SIGHUP);
  if (pthread_sigmask (SIG_BLOCK, &krad_system.signal_mask, NULL) != 0) {
    failfast ("Could not set signal mask!");
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

int dir_exists (char *dir) {

  int err;
  struct stat s;

  err = stat (dir, &s);

  if (err == -1) {
    if(ENOENT == errno) {
      // does not exist
      return 0;
    } else {
      // another error
      return 0;
    }
  } else {
    if(S_ISDIR(s.st_mode)) {
      // it's a directory
      return 1;
    } else {
      // exists but is no dir
      return 0;
    }
  }

  return 0;
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
  
  if ((sysname == NULL) || (strlen(sysname) < KRAD_SYSNAME_MIN)) {
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
    
  *port = atoi (strrchr(string, ':') + 1);
  memset (host, '\0', 128);
  memcpy (host, string, strlen(string) - strlen(strrchr(string, ':')));
  printf ("Got host %s and port %d\n", host, *port);
}

//FIXME not actually any good

int krad_valid_host_and_port (char *string) {

  int port;
  char host[128];
      
  if (strchr(string, ':') != NULL) {
      
    port = atoi (strrchr(string, ':') + 1);
    memset (host, '\0', 128);
    memcpy (host, string, strlen(string) - strlen(strrchr(string, ':')));
    //if (((port >= 0) && (port <= 65535)) && (strlen(host) > 3)) {
    if (((port >= 0) && (port <= 65535)) && (1)) {
      printf ("Got host %s and port %d\n", host, port);
      return 1;
    } else {
      printke ("INVALID host %s port %d", host, port);
    }
  }
  
  return 0;
}

#ifdef IS_LINUX

int krad_system_is_adapter (char *adapter) {

  int sd;
  struct ifconf ifconf;
  struct ifreq ifreq[20];
  int interfaces;
  int i;

  // Create a socket or return an error.
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    failfast ("socket");
  }
  // Point ifconf's ifc_buf to our array of interface ifreqs.
  ifconf.ifc_buf = (char *) ifreq;
  
  // Set ifconf's ifc_len to the length of our array of interface ifreqs.
  ifconf.ifc_len = sizeof ifreq;

  //  Populate ifconf.ifc_buf (ifreq) with a list of interface names and addresses.
  if (ioctl(sd, SIOCGIFCONF, &ifconf) == -1) {
    failfast ("ioctl");
  }

  // Divide the length of the interface list by the size of each entry.
  // This gives us the number of interfaces on the system.
  interfaces = ifconf.ifc_len / sizeof(ifreq[0]);

  for (i = 0; i < interfaces; i++) {
    if (strncmp (adapter, ifreq[i].ifr_name, strlen(adapter)) == 0) {
      close (sd);
      return 1;      
    }
  }

  close (sd);
  return 0;
}
#endif
