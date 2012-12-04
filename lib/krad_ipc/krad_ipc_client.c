#include "krad_ipc_client.h"

static void krad_radio_watchdog (char *config_file);
static char *krad_radio_run_cmd (char *cmd);

kr_client_t *kr_connect (char *sysname) {
	
	krad_ipc_client_t *client = calloc (1, sizeof (krad_ipc_client_t));
	
	if (client == NULL) {
		failfast ("Krad IPC Client mem alloc fail");
		return NULL;
	}
	
	if ((client->buffer = calloc (1, KRAD_IPC_BUFFER_SIZE)) == NULL) {
		failfast ("Krad IPC Client buffer alloc fail");
		return NULL;
	}
	
	krad_system_init ();
	
	uname(&client->unixname);

	if (krad_valid_host_and_port (sysname)) {
	
		krad_get_host_and_port (sysname, client->host, &client->tcp_port);
	
	} else {
	
		strncpy (client->sysname, sysname, sizeof (client->sysname));

		if (strncmp(client->unixname.sysname, "Linux", 5) == 0) {
			client->on_linux = 1;
			client->ipc_path_pos = sprintf(client->ipc_path, "@krad_radio_%s_ipc", sysname);
		} else {
			client->ipc_path_pos = sprintf(client->ipc_path, "%s/krad_radio_%s_ipc", "/tmp", sysname);
		}
	
		if (!client->on_linux) {
			if(stat(client->ipc_path, &client->info) != 0) {
				kr_disconnect(client);
				failfast ("Krad IPC Client: IPC PATH Failure\n");
				return NULL;
			}
		}
		
	}
	
	if (krad_ipc_client_init (client) == 0) {
		printke ("Krad IPC Client: Failed to init!");
		kr_disconnect (client);
		return NULL;
	}


	return client;
}


int krad_ipc_client_init (krad_ipc_client_t *client) {

	struct sockaddr_in serveraddr;
	struct hostent *hostp;
	int sent;

	if (client->tcp_port != 0) {

		printkd ("Krad IPC Client: Connecting to remote %s:%d", client->host, client->tcp_port);

		if ((client->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			failfast ("Krad IPC Client: Socket Error");
		}

		memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons (client->tcp_port);
	
		if ((serveraddr.sin_addr.s_addr = inet_addr(client->host)) == (unsigned long)INADDR_NONE) {
			// get host address 
			hostp = gethostbyname(client->host);
			if (hostp == (struct hostent *)NULL) {
				printke ("Krad IPC Client: Remote Host Error");
				close (client->sd);
			}
			memcpy (&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
		}

		// connect() to server. 
		if ((sent = connect(client->sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
			printke ("Krad IPC Client: Remote Connect Error");
		}

	} else {

		client->sd = socket (AF_UNIX, SOCK_STREAM, 0);
		if (client->sd == -1) {
			failfast ("Krad IPC Client: socket fail");
			return 0;
		}

		client->saddr.sun_family = AF_UNIX;
		snprintf (client->saddr.sun_path, sizeof(client->saddr.sun_path), "%s", client->ipc_path);

		if (client->on_linux) {
			client->saddr.sun_path[0] = '\0';
		}


		if (connect (client->sd, (struct sockaddr *) &client->saddr, sizeof (client->saddr)) == -1) {
			close (client->sd);
			client->sd = 0;
			printke ("Krad IPC Client: Can't connect to socket %s", client->ipc_path);
			return 0;
		}

		client->flags = fcntl (client->sd, F_GETFL, 0);

		if (client->flags == -1) {
			close (client->sd);
			client->sd = 0;
			printke ("Krad IPC Client: socket get flag fail");
			return 0;
		}
	
	}
	
	
/*
	client->flags |= O_NONBLOCK;

	client->flags = fcntl (client->sd, F_SETFL, client->flags);
	if (client->flags == -1) {
		close (client->sd);
		client->sd = 0;
		printke ("Krad IPC Client: socket set flag fail\n");
		return 0;
	}
*/

	client->krad_ebml = krad_ebml_open_active_socket (client->sd, KRAD_EBML_IO_READWRITE);

	krad_ebml_header_advanced (client->krad_ebml, KRAD_IPC_CLIENT_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
	krad_ebml_write_sync (client->krad_ebml);
	
	krad_ebml_read_ebml_header (client->krad_ebml, client->krad_ebml->header);
	krad_ebml_check_ebml_header (client->krad_ebml->header);
	//krad_ebml_print_ebml_header (client->krad_ebml->header);
	
	if (krad_ebml_check_doctype_header (client->krad_ebml->header, KRAD_IPC_SERVER_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION)) {
		//printf("Matched %s Version: %d Read Version: %d\n", KRAD_IPC_SERVER_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
	} else {
		printke ("Did Not Match %s Version: %d Read Version: %d", KRAD_IPC_SERVER_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
	}	
	
	return client->sd;
}

static char *krad_radio_run_cmd (char *cmd) {

  FILE *fp;
  int pos;
  char buf[256];
  static char buffer[8192];

  pos = 0;

  if (cmd == NULL) {
    return NULL;
  }

  fp = popen(cmd, "r");
  if (fp == NULL) {
    return NULL;
  }
  
  buffer[0] = '\0';

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    pos += snprintf (buffer + pos, pos - sizeof(buffer), "%s", buf);
    if (pos + sizeof(buf) >= sizeof(buffer)) {
      buffer[pos] = '\0';
      break;
    }
  }

  pclose (fp);
  
  return buffer;

}

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

char *krad_radio_threads (char *sysname) {

  int pid;
  char cmd[64];
  
  pid = 0;

  if ((sysname != NULL) && (krad_valid_sysname(sysname))) {
    pid = krad_radio_pid (sysname);
  }

  if (pid > 0) {
    snprintf (cmd, sizeof(cmd), "ps -AL|grep %d|grep kr_", pid);
  } else {
    snprintf (cmd, sizeof(cmd), "ps -AL|grep kr_");
  }

  return krad_radio_run_cmd (cmd);

}

int krad_radio_running (char *sysname) {
  if ((krad_radio_pid (sysname)) > 0) {
    return 1;
  }
  
  return 0; 
}

int krad_radio_pid (char *sysname) {

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
	
	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);	

	execlp ("krad_radio_daemon", "krad_radio_daemon", sysname, (char *)NULL);

}

void krad_radio_watchdog_run_program_with_options (char *filename, char *const options[]) {

	pid_t pid;

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

	execv (filename, options);

}


void krad_radio_watchdog_run_jack_dummy_44100 () {

	char *const jack_options[] = {"/usr/bin/jackd", "--silent", "-ddummy", "-r44100", NULL};

	krad_radio_watchdog_run_program_with_options ("/usr/bin/jackd", jack_options);
}

void krad_radio_watchdog_run_xmms2d_with_ipc_path (char *ipc_path) {

	char ipc_option[256];
	
	sprintf(ipc_option, "--ipc-socket=%s", ipc_path);

	char *xmms2_options[] = {"/usr/local/bin/xmms2d", "--output=jack", "--quiet", ipc_option, NULL};

	krad_radio_watchdog_run_program_with_options ("/usr/local/bin/xmms2d", xmms2_options);
}

void krad_radio_watchdog_run_program (char *filename) {

	pid_t pid;

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

	execl (filename, filename, (char *)NULL);

}

void krad_radio_watchdog_launch (char *config_file) {

	pid_t sid, pid;

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

	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
	//close (STDIN_FILENO);
	//close (STDOUT_FILENO);
	//close (STDERR_FILENO);

	umask(0);
 
	sid = setsid();
	
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	krad_radio_watchdog (config_file);

}

void krad_radio_watchdog_check_daemon (char *sysname, char *launch_script) {
	
	krad_ipc_client_t *client;
	client = NULL;

	if (fork() == 0) {
		if (fork() == 0) {
			client = NULL;
			client = kr_connect (sysname);
	
			if (client != NULL) {
				kr_disconnect (client);
				client = NULL;
			} else {
				krad_radio_destroy (sysname);
				if (launch_script != NULL) {
					krad_radio_watchdog_run_program (launch_script);
				} else {
					krad_radio_launch (sysname);
				}
			}
			exit (EXIT_SUCCESS);
		}
		exit (EXIT_SUCCESS);
	}
	wait (NULL);

	usleep (4000000);

}

void krad_radio_watchdog_read_config (krad_radio_watchdog_t *krad_radio_watchdog, char *config_file) {

	int ret;
	char station[80];
	char script[768];
	FILE *config;

	config = NULL;

	config = fopen (config_file, "r");

	if (config == NULL) {
		exit (1);
	} else {

		while (1) {
			ret = fscanf (config, "%s %s", station, script);
			if (ret < 1) {
				break;
			}
			krad_radio_watchdog->stations[krad_radio_watchdog->count] = strdup (station);
			if (ret == 2) {
				krad_radio_watchdog->launch_scripts[krad_radio_watchdog->count] = strdup (script);
			}

			krad_radio_watchdog->count++;
		}
		fclose (config);
	}
}

void krad_radio_watchdog (char *config_file) {

	/* Emma the Dog */

	int d;

	krad_radio_watchdog_t *krad_radio_watchdog;

	krad_radio_watchdog = calloc (1, sizeof(krad_radio_watchdog_t));

	krad_radio_watchdog_read_config (krad_radio_watchdog, config_file);

	//krad_radio_watchdog_run_jack_dummy_44100 ();
	//usleep(500000);
	//krad_radio_watchdog_run_xmms2d_with_ipc_path ("/tmp/xmms-ipc-music1");

	while (1) {
		for (d = 0; d < krad_radio_watchdog->count; d++) {
			krad_radio_watchdog_check_daemon (krad_radio_watchdog->stations[d],
											  krad_radio_watchdog->launch_scripts[d]);
			usleep (200000);
		}
		usleep (2000000);
	}

	free (krad_radio_watchdog);

}


void krad_ipc_broadcast_subscribe (krad_ipc_client_t *client, uint32_t broadcast_id) {

	uint64_t radio_command;

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_BROADCAST_SUBSCRIBE, broadcast_id);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_get_portgroups (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t get_portgroups;
	//uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS, &get_portgroups);	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");	

	//krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, get_portgroups);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_get_tags (krad_ipc_client_t *client, char *item) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t get_tags;
	//uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_LIST_TAGS, &get_tags);	

	if (item == NULL) {
		item = "station";
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");	

	//krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, get_tags);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_get_tag (krad_ipc_client_t *client, char *item, char *tag_name) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t get_tag;
	uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_TAG, &get_tag);	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	if (item == NULL) {
		item = "station";
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");	

	krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, get_tag);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_set_tag (krad_ipc_client_t *client, char *item, char *tag_name, char *tag_value) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t set_tag;
	uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_TAG, &set_tag);	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	if (item == NULL) {
		item = "station";
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, tag_value);	

	krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, set_tag);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_webon (krad_ipc_client_t *client, int http_port, int websocket_port,
					char *headcode, char *header, char *footer) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t webon;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE, &webon);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_HTTP_PORT, http_port);	
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_WEBSOCKET_PORT, websocket_port);	

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_HEADCODE, headcode);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_HEADER, header);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_FOOTER, footer);
	
	krad_ebml_finish_element (client->krad_ebml, webon);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_weboff (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t weboff;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_DISABLE, &weboff);

	krad_ebml_finish_element (client->krad_ebml, weboff);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_enable_remote (krad_ipc_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t enable_remote;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_ENABLE, &enable_remote);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_remote);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_disable_remote (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t disable_remote;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE, &disable_remote);

	krad_ebml_finish_element (client->krad_ebml, disable_remote);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_enable_linker_listen (krad_ipc_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t enable_linker;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINKER_CMD_LISTEN_ENABLE, &enable_linker);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_linker);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_disable_linker_listen (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t disable_linker;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINKER_CMD_LISTEN_DISABLE, &disable_linker);

	krad_ebml_finish_element (client->krad_ebml, disable_linker);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_portgroup_xmms2_cmd (krad_ipc_client_t *client, char *portgroupname, char *xmms2_cmd) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t bind;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_PORTGROUP_XMMS2_CMD, &bind);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_XMMS2_CMD, xmms2_cmd);
	krad_ebml_finish_element (client->krad_ebml, bind);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_bind_portgroup_xmms2 (krad_ipc_client_t *client, char *portgroupname, char *ipc_path) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t bind;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_BIND_PORTGROUP_XMMS2, &bind);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_XMMS2_IPC_PATH, ipc_path);
	krad_ebml_finish_element (client->krad_ebml, bind);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_unbind_portgroup_xmms2 (krad_ipc_client_t *client, char *portgroupname) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t unbind;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UNBIND_PORTGROUP_XMMS2, &unbind);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_finish_element (client->krad_ebml, unbind);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_set_mixer_sample_rate (krad_ipc_client_t *client, int sample_rate) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t set_sample_rate;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_SET_SAMPLE_RATE, &set_sample_rate);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_SAMPLE_RATE, sample_rate);

	krad_ebml_finish_element (client->krad_ebml, set_sample_rate);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_get_mixer_sample_rate (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t get_sample_rate;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_GET_SAMPLE_RATE, &get_sample_rate);
	krad_ebml_finish_element (client->krad_ebml, get_sample_rate);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_enable_linker_transmitter (krad_ipc_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t enable_transmitter;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINKER_CMD_TRANSMITTER_ENABLE, &enable_transmitter);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_transmitter);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_disable_linker_transmitter (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t disable_transmitter;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINKER_CMD_TRANSMITTER_DISABLE, &disable_transmitter);

	krad_ebml_finish_element (client->krad_ebml, disable_transmitter);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_enable_osc (krad_ipc_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t enable_osc;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_OSC_ENABLE, &enable_osc);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_UDP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_osc);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_disable_osc (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t disable_osc;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_OSC_DISABLE, &disable_osc);

	krad_ebml_finish_element (client->krad_ebml, disable_osc);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_plug_portgroup (krad_ipc_client_t *client, char *name, char *remote_name) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t plug;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_PLUG_PORTGROUP, &plug);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, remote_name);
	krad_ebml_finish_element (client->krad_ebml, plug);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_unplug_portgroup (krad_ipc_client_t *client, char *name, char *remote_name) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t unplug;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UNPLUG_PORTGROUP, &unplug);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, remote_name);
	krad_ebml_finish_element (client->krad_ebml, unplug);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_create_portgroup (krad_ipc_client_t *client, char *name, char *direction, int channels) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t create;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_CREATE_PORTGROUP, &create);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION, direction);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS, channels);
	krad_ebml_finish_element (client->krad_ebml, create);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_mixer_push_tone (krad_ipc_client_t *client, char *tone) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t push;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_PUSH_TONE, &push);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_TONE_NAME, tone);
	
	krad_ebml_finish_element (client->krad_ebml, push);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_radio_set_dir (krad_ipc_client_t *client, char *dir) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t setdir;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_DIR, &setdir);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_DIR, dir);
	
	krad_ebml_finish_element (client->krad_ebml, setdir);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_update_portgroup_map_channel (krad_ipc_client_t *client, char *portgroupname, int in_channel, int out_channel) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t update;
	uint64_t map;


	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP, &update);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_MAP_CHANNEL, &map);
	krad_ebml_finish_element (client->krad_ebml, map);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL, in_channel);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL, out_channel);
	
	
	
	krad_ebml_finish_element (client->krad_ebml, update);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_update_portgroup_mixmap_channel (krad_ipc_client_t *client, char *portgroupname, int in_channel, int out_channel) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t update;
	uint64_t map;


	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP, &update);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_MIXMAP_CHANNEL, &map);
	krad_ebml_finish_element (client->krad_ebml, map);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL, in_channel);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL, out_channel);
	
	
	
	krad_ebml_finish_element (client->krad_ebml, update);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_mixer_update_portgroup (krad_ipc_client_t *client, char *portgroupname, uint64_t update_command, char *string) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t update;



	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP, &update);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_write_string (client->krad_ebml, update_command, string);
	
	
	krad_ebml_finish_element (client->krad_ebml, update);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_remove_portgroup (krad_ipc_client_t *client, char *name) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t destroy;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_DESTROY_PORTGROUP, &destroy);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);

	krad_ebml_finish_element (client->krad_ebml, destroy);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_set_handler_callback (krad_ipc_client_t *client, int handler (krad_ipc_client_t *, void *), void *ptr) {

	client->handler = handler;
	client->ptr = ptr;

}

void kr_mixer_add_effect (krad_ipc_client_t *client, char *portgroup_name, char *effect_name) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t add_effect;
	
	mixer_command = 0;
	add_effect = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_ADD_EFFECT, &add_effect);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_EFFECT_NAME, effect_name);

	krad_ebml_finish_element (client->krad_ebml, add_effect);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_remove_effect (krad_ipc_client_t *client, char *portgroup_name, int effect_num) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t remove_effect;
	
	mixer_command = 0;
	remove_effect = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_REMOVE_EFFECT, &remove_effect);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_EFFECT_NUM, effect_num);

	krad_ebml_finish_element (client->krad_ebml, remove_effect);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_set_effect_control (krad_ipc_client_t *client, char *portgroup_name, int effect_num, 
                                  char *control_name, int subunit, float control_value) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t set_control;
	
	mixer_command = 0;
	set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_SET_EFFECT_CONTROL, &set_control);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_EFFECT_NUM, effect_num);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_SUBUNIT, subunit);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, set_control);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_set_control (krad_ipc_client_t *client, char *portgroup_name, char *control_name, float control_value) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t set_control;
	
	mixer_command = 0;
	set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_SET_CONTROL, &set_control);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, set_control);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_background (krad_ipc_client_t *client, char *filename) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t background;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_BACKGROUND, &background);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FILENAME, filename);

	krad_ebml_finish_element (client->krad_ebml, background);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);
	
}

void krad_ipc_compositor_bug (krad_ipc_client_t *client, int x, int y, char *filename) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t bug;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_BUG, &bug);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, y);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FILENAME, filename);

	krad_ebml_finish_element (client->krad_ebml, bug);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_compositor_add_text (krad_ipc_client_t *client, char *text, int x, int y, int tickrate, 
									float scale, float opacity, float rotation, int red, int green, int blue, char *font) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t textcmd;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_ADD_TEXT, &textcmd);

	krad_ebml_write_string (client->krad_ebml,  EBML_ID_KRAD_COMPOSITOR_TEXT, text);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, y);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_TICKRATE, tickrate);
	
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_SCALE, scale);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_OPACITY, opacity);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_ROTATION, rotation);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_RED, red);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_GREEN, green);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_BLUE, blue);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FONT, font);	

	krad_ebml_finish_element (client->krad_ebml, textcmd);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_set_text (krad_ipc_client_t *client, int num, int x, int y, int tickrate, 
									float scale, float opacity, float rotation, int red, int green, int blue) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t text;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_TEXT, &text);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_NUMBER, num);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, y);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_TICKRATE, tickrate);
	
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_SCALE, scale);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_OPACITY, opacity);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_ROTATION, rotation);
	
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_RED, red);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_GREEN, green);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_BLUE, blue);
	
	krad_ebml_finish_element (client->krad_ebml, text);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_remove_text (krad_ipc_client_t *client, int num) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t text;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_REMOVE_TEXT, &text);


	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_NUMBER, num);

	krad_ebml_finish_element (client->krad_ebml, text);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_list_texts (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t text;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LIST_TEXTS, &text);



	krad_ebml_finish_element (client->krad_ebml, text);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_add_sprite (krad_ipc_client_t *client, char *filename, int x, int y, int tickrate, 
									float scale, float opacity, float rotation) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t sprite;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_ADD_SPRITE, &sprite);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FILENAME, filename);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, y);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_TICKRATE, tickrate);
	
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_SCALE, scale);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_OPACITY, opacity);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_ROTATION, rotation);

	krad_ebml_finish_element (client->krad_ebml, sprite);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_set_sprite (krad_ipc_client_t *client, int num, int x, int y, int tickrate, 
									float scale, float opacity, float rotation) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t sprite;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_SPRITE, &sprite);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_NUMBER, num);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, y);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_TICKRATE, tickrate);
	
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_SCALE, scale);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_OPACITY, opacity);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_ROTATION, rotation);
	
	krad_ebml_finish_element (client->krad_ebml, sprite);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_remove_sprite (krad_ipc_client_t *client, int num) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t sprite;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_REMOVE_SPRITE, &sprite);


	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_NUMBER, num);

	krad_ebml_finish_element (client->krad_ebml, sprite);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_list_sprites (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t sprite;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LIST_SPRITES, &sprite);



	krad_ebml_finish_element (client->krad_ebml, sprite);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_vu (krad_ipc_client_t *client, int on_off) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t vu;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_VU_MODE, &vu);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_VU_ON, on_off);

	krad_ebml_finish_element (client->krad_ebml, vu);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_close_display (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t display;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_CLOSE_DISPLAY, &display);

	krad_ebml_finish_element (client->krad_ebml, display);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_open_display (krad_ipc_client_t *client, int width, int height) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t display;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_OPEN_DISPLAY, &display);

	krad_ebml_finish_element (client->krad_ebml, display);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_hex (krad_ipc_client_t *client, int x, int y, int size) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t hexdemo;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_HEX_DEMO, &hexdemo);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, y);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SIZE, size);

	krad_ebml_finish_element (client->krad_ebml, hexdemo);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_set_frame_rate (krad_ipc_client_t *client, int numerator, int denominator) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t set_frame_rate;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_FRAME_RATE, &set_frame_rate);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FPS_NUMERATOR, numerator);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FPS_DENOMINATOR, denominator);

	krad_ebml_finish_element (client->krad_ebml, set_frame_rate);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_set_resolution (krad_ipc_client_t *client, int width, int height) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t set_resolution;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_RESOLUTION, &set_resolution);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_WIDTH, width);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_HEIGHT, height);

	krad_ebml_finish_element (client->krad_ebml, set_resolution);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_radio_uptime (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t uptime_command;
	command = 0;
	uptime_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_UPTIME, &uptime_command);
	krad_ebml_finish_element (client->krad_ebml, uptime_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_radio_get_system_info (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t info_command;
	command = 0;
	info_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_INFO, &info_command);
	krad_ebml_finish_element (client->krad_ebml, info_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);	
	
}

void krad_ipc_compositor_snapshot (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t snap_command;
	command = 0;
	snap_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SNAPSHOT, &snap_command);
	krad_ebml_finish_element (client->krad_ebml, snap_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);	
	
}

void krad_ipc_compositor_snapshot_jpeg (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t snap_command;
	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SNAPSHOT_JPEG, &snap_command);
	krad_ebml_finish_element (client->krad_ebml, snap_command);
	krad_ebml_finish_element (client->krad_ebml, command);
	krad_ebml_write_sync (client->krad_ebml);
}

void krad_ipc_radio_get_logname (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t log_command;
	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_LOGNAME, &log_command);
	krad_ebml_finish_element (client->krad_ebml, log_command);
	krad_ebml_finish_element (client->krad_ebml, command);
	krad_ebml_write_sync (client->krad_ebml);
}

void krad_ipc_radio_get_system_cpu_usage (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t cpu_command;
	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_CPU_USAGE, &cpu_command);
	krad_ebml_finish_element (client->krad_ebml, cpu_command);
	krad_ebml_finish_element (client->krad_ebml, command);
	krad_ebml_write_sync (client->krad_ebml);
}

void krad_ipc_radio_get_last_snap_name (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t snap_command;
	
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_GET_LAST_SNAPSHOT_NAME, &snap_command);
	krad_ebml_finish_element (client->krad_ebml, snap_command);
	krad_ebml_finish_element (client->krad_ebml, command);
	krad_ebml_write_sync (client->krad_ebml);
}

void krad_ipc_compositor_info (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t info_command;
	command = 0;
	info_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_INFO, &info_command);
	krad_ebml_finish_element (client->krad_ebml, info_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);	
	
}


void krad_ipc_compositor_get_frame_rate (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t info_command;
	command = 0;
	info_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_GET_FRAME_RATE, &info_command);
	krad_ebml_finish_element (client->krad_ebml, info_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);	
	
}

void krad_ipc_compositor_get_frame_size (krad_ipc_client_t *client) {

	uint64_t command;
	uint64_t info_command;
	command = 0;
	info_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_GET_FRAME_SIZE, &info_command);
	krad_ebml_finish_element (client->krad_ebml, info_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);
	
}

void krad_ipc_create_capture_link (krad_ipc_client_t *client, krad_link_video_source_t video_source, char *device,
								   int width, int height, int fps_numerator, int fps_denominator,
								   krad_link_av_mode_t av_mode, char *audio_input, char *codec) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (CAPTURE));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE, krad_link_video_source_to_string (video_source));

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_CAPTURE_DEVICE, device);
	
	if (video_source == DECKLINK) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_CAPTURE_DECKLINK_AUDIO_INPUT, audio_input);
	}
	
	if (video_source == V4L2) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_CAPTURE_CODEC, codec);
	}	
	
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH, width);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT, height);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR, fps_numerator);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR, fps_denominator);

	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_receive_link (krad_ipc_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (RECEIVE));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "udp");
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PORT, port);
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_remote_playback_link (krad_ipc_client_t *client, char *host, int port, char *mount) {



	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (PLAYBACK));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "tcp");
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_HOST, host);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PORT, port);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_MOUNT, mount);
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);


}

void krad_ipc_create_playback_link (krad_ipc_client_t *client, char *path) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (PLAYBACK));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "filesystem");
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FILENAME, path);
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_transmit_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode,
									char *host, int port, char *mount, char *password, char *codecs,
									int video_width, int video_height, int video_bitrate, char *audio_bitrate) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	int passthru;	
	
	krad_codec_t audio_codec;
	krad_codec_t video_codec;
	
	int audio_bitrate_int;
	float vorbis_quality;
	
	audio_codec = VORBIS;
	video_codec = VP8;
	passthru = 0;

	if (codecs != NULL) {
		audio_codec = krad_string_to_audio_codec (codecs);
		video_codec = krad_string_to_video_codec (codecs);
		if (strstr(codecs, "pass") != NULL) {
			passthru = 1;
		}
	}
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (TRANSMIT));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AV_MODE, krad_link_av_mode_to_string (av_mode));

	if ((av_mode == VIDEO_ONLY) || (av_mode == AUDIO_AND_VIDEO)) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (video_codec));
		
		if (video_codec == VP8) {
			if (video_bitrate == 0) {
				video_bitrate = 92 * 8;
			}
		}
		
		if (video_codec == THEORA) {
			if ((video_width % 16) || (video_height % 16)) {
				video_width = 0;
				video_height = 0;
			}
			if (video_bitrate == 0) {
				video_bitrate = 31;
			}
		}
		
		if ((video_codec == MJPEG) || (video_codec == H264)) {
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_USE_PASSTHRU_CODEC, passthru);
		}
		
		krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH, video_width);
		krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT, video_height);
		
		if ((video_codec == VP8) || (video_codec == H264)) {
			if (video_bitrate < 100) {
				video_bitrate = 100;
			}
			
			if (video_bitrate > 10000) {
				video_bitrate = 10000;
			}
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, video_bitrate);	
		}
		
		if (video_codec == THEORA) {
		
			if (video_bitrate < 0) {
				video_bitrate = 0;
			}
			
			if (video_bitrate > 63) {
				video_bitrate = 63;
			}
		
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY, video_bitrate);	
		}	
		
	}

	if ((av_mode == AUDIO_ONLY) || (av_mode == AUDIO_AND_VIDEO)) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (audio_codec));
		if (audio_codec == FLAC) {
			if (atoi(audio_bitrate) == 24) {
				krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, 24);
			} else {
				krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, 16);
			}
		}
		if (audio_codec == VORBIS) {
			vorbis_quality = atof(audio_bitrate);
			if (vorbis_quality == 0.0) {
				vorbis_quality = 0.4;
			}
			if (vorbis_quality > 0.8) {
				vorbis_quality = 0.8;
			} 
			if (vorbis_quality < 0.2) {
				vorbis_quality = 0.2;
			}
		
			krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY, vorbis_quality);
		}
		if (audio_codec == OPUS) {
			audio_bitrate_int = atoi(audio_bitrate);
			if (audio_bitrate_int == 0) {
				audio_bitrate_int = 132000;
			} 
			if (audio_bitrate_int < 5000) {
				audio_bitrate_int = 5000;
			}
			if (audio_bitrate_int > 320000) {
				audio_bitrate_int = 320000;
			}
			
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, audio_bitrate_int);
		}
	}
	
	if (strcmp(password, "udp") == 0) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "udp");
	} else {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE, "tcp");
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_HOST, host);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PORT, port);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_MOUNT, mount);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_PASSWORD, password);	
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_create_record_link (krad_ipc_client_t *client, krad_link_av_mode_t av_mode, char *filename, char *codecs,
									int video_width, int video_height, int video_bitrate, char *audio_bitrate) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t create_link;
	uint64_t link;
	int passthru;
	
	krad_codec_t audio_codec;
	krad_codec_t video_codec;
	
	int audio_bitrate_int;
	float vorbis_quality;
	
	audio_codec = VORBIS;
	video_codec = VP8;
	passthru = 0;
		
	if (codecs != NULL) {
		audio_codec = krad_string_to_audio_codec (codecs);
		video_codec = krad_string_to_video_codec (codecs);
		if (strstr(codecs, "pass") != NULL) {
			passthru = 1;
		}
	}
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_CREATE_LINK, &create_link);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_LINK, &link);	
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPERATION_MODE, krad_link_operation_mode_to_string (RECORD));
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AV_MODE, krad_link_av_mode_to_string (av_mode));

	if ((av_mode == VIDEO_ONLY) || (av_mode == AUDIO_AND_VIDEO)) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC, krad_codec_to_string (video_codec));
		
		if (video_codec == VP8) {
			if (video_bitrate == 0) {
				video_bitrate = 140 * 8;
			}
		}
		
		if (video_codec == THEORA) {
			if (video_bitrate == 0) {
				video_bitrate = 41;
			}
		}
		
		if ((video_codec == MJPEG) || (video_codec == H264)) {
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_USE_PASSTHRU_CODEC, passthru);
		}

		krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH, video_width);
		krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT, video_height);
		
		if ((video_codec == VP8) || (video_codec == H264)) {
			if (video_bitrate < 100) {
				video_bitrate = 100;
			}
			
			if (video_bitrate > 10000) {
				video_bitrate = 10000;
			}
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VP8_BITRATE, video_bitrate);	
		}
		
		if (video_codec == THEORA) {
		
			if (video_bitrate < 0) {
				video_bitrate = 0;
			}
			
			if (video_bitrate > 63) {
				video_bitrate = 63;
			}
		
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY, video_bitrate);	
		}	
		
	}

	if ((av_mode == AUDIO_ONLY) || (av_mode == AUDIO_AND_VIDEO)) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC, krad_codec_to_string (audio_codec));
		if (audio_codec == FLAC) {
			if (atoi(audio_bitrate) == 24) {
				krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, 24);
			} else {
				krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH, 16);
			}
		}
		if (audio_codec == VORBIS) {
			vorbis_quality = atof(audio_bitrate);
			if (vorbis_quality == 0.0) {
				vorbis_quality = 0.4;
			}
			if (vorbis_quality > 0.8) {
				vorbis_quality = 0.8;
			} 
			if (vorbis_quality < 0.2) {
				vorbis_quality = 0.2;
			}
		
			krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY, vorbis_quality);
		}
		if (audio_codec == OPUS) {
			audio_bitrate_int = atoi(audio_bitrate);
			if (audio_bitrate_int == 0) {
				audio_bitrate_int = 132000;
			} 
			if (audio_bitrate_int < 5000) {
				audio_bitrate_int = 5000;
			}
			if (audio_bitrate_int > 320000) {
				audio_bitrate_int = 320000;
			}
			
			krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE, audio_bitrate_int);
		}
	}

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_FILENAME, filename);
	
	krad_ebml_finish_element (client->krad_ebml, link);

	krad_ebml_finish_element (client->krad_ebml, create_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_list_v4l2 (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t list_v4l2;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_LIST_V4L2, &list_v4l2);

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	//krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, list_v4l2);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_mixer_jack_running (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t jack_running;
	
	mixer_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_JACK_RUNNING, &jack_running);

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	//krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, jack_running);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);


}

void krad_ipc_list_decklink (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t list_decklink;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_LIST_DECKLINK, &list_decklink);

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	//krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, list_decklink);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_list_links (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t list_links;
	
	linker_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_LIST_LINKS, &list_links);

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	//krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, list_links);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_list_ports (krad_ipc_client_t *client) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t list_ports;
	
	compositor_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LIST_PORTS, &list_ports);

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	//krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, list_ports);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_destroy_link (krad_ipc_client_t *client, int number) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t destroy_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_DESTROY_LINK, &destroy_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_finish_element (client->krad_ebml, destroy_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_update_link_adv_num (krad_ipc_client_t *client, int number, uint32_t ebml_id, int newval) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t update_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_UPDATE_LINK, &update_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_write_int32 (client->krad_ebml, ebml_id, newval);

	krad_ebml_finish_element (client->krad_ebml, update_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_update_link_adv (krad_ipc_client_t *client, int number, uint32_t ebml_id, char *newval) {

	//uint64_t ipc_command;
	uint64_t linker_command;
	uint64_t update_link;
	
	linker_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD, &linker_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_LINK_CMD_UPDATE_LINK, &update_link);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_write_string (client->krad_ebml, ebml_id, newval);

	krad_ebml_finish_element (client->krad_ebml, update_link);
	krad_ebml_finish_element (client->krad_ebml, linker_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void krad_ipc_compositor_set_port_mode (krad_ipc_client_t *client, int number, uint32_t x, uint32_t y,
										uint32_t width, uint32_t height, uint32_t crop_x, uint32_t crop_y,
										uint32_t crop_width, uint32_t crop_height, float opacity, float rotation) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t update_port;
	
	compositor_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_UPDATE_PORT, &update_port);

	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_NUMBER, number);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_X, x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_Y, y);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_WIDTH, width);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_HEIGHT, height);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_CROP_X, crop_x);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_CROP_Y, crop_y);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_CROP_WIDTH, crop_width);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_CROP_HEIGHT, crop_height);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_OPACITY, opacity);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_ROTATION, rotation);

	krad_ebml_finish_element (client->krad_ebml, update_port);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void krad_ipc_send (krad_ipc_client_t *client, char *cmd) {


	int len;
	fd_set set;
	
	strcat(cmd, "|");
	len = strlen(cmd);

	FD_ZERO (&set);
	FD_SET (client->sd, &set);

	select (client->sd+1, NULL, &set, NULL, NULL);
	send (client->sd, cmd, len, 0);

}
/*
int krad_ipc_cmd2 (krad_ipc_client_t *client, int value) {

	fd_set set;
	
	uint32_t ebml_id;
	uint64_t ebml_data_size;	
	
	uint64_t number;		
	
//	FD_ZERO (&set);
//	FD_SET (client->sd, &set);
	
//	select (client->sd+1, NULL, &set, NULL, NULL);
	
	uint64_t ipc_command;
	uint64_t radio_command;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_CONTROL, value);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

	return 0;
}
*/

void krad_ipc_client_handle (krad_ipc_client_t *client) {
	client->handler ( client, client->ptr );
}

int krad_ipc_client_poll (krad_ipc_client_t *client) {

	int ret;
	struct pollfd sockets[1];
	//uint32_t ebml_id;
	//uint64_t ebml_data_size;	

	sockets[0].fd = client->sd;
	sockets[0].events = POLLIN;

	while ((ret = poll(sockets, 1, 0)) > 0) {
		//krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
		//*value = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
		//printf("Received number %d\n", *value);
		

		client->handler ( client, client->ptr );
	}

	return 0;
}

int krad_ipc_cmd (krad_ipc_client_t *client, char *cmd) {
/*
	fd_set set;
	
	uint32_t ebml_id;
	uint64_t ebml_data_size;	
	
	uint64_t number;		
	
	FD_ZERO (&set);
	FD_SET (client->sd, &set);
	
	select (client->sd+1, NULL, &set, NULL, NULL);
	
	uint64_t ipc_command;
	uint64_t radio_command;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_CONTROL, 82);

	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);


	//usleep(200000);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_write_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_CONTROL);
	krad_ebml_write_data_size (client->krad_ebml, 0);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

	printf("sent\n");

	FD_ZERO (&set);
	FD_SET (client->sd, &set);

	select (client->sd+1, &set, NULL, NULL, NULL);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	printf("Received number %zu\n", number);
	
*/	
	
	return 0;
}


int krad_ipc_client_read_mixer_control ( krad_ipc_client_t *client, char **portgroup_name, char **control_name, float *value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
		//printf("hrm wtf1\n");
	}
	krad_ebml_read_string (client->krad_ebml, *portgroup_name, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_NAME) {
		//printf("hrm wtf2\n");
	}
	krad_ebml_read_string (client->krad_ebml, *control_name, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
		//printf("hrm wtf3\n");
	}
	
	*value = krad_ebml_read_float (client->krad_ebml, ebml_data_size);

	//printf("krad_ipc_client_read_mixer_control %s %s %f\n", *portgroup_name, *control_name, *value );
		
	return 0;		
						
}						

int krad_ipc_client_read_tag ( krad_ipc_client_t *client, char **tag_item, char **tag_name, char **tag_value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	bytes_read += ebml_data_size + 9;
	
	if (ebml_id != EBML_ID_KRAD_RADIO_TAG) {
		//printf("hrm wtf\n");
	} else {
		//printf("tag size %zu\n", ebml_data_size);
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
		//printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_item, ebml_data_size);	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
		//printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_name, ebml_data_size);
	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
		//printf("hrm wtf3\n");
	} else {
		//printf("tag value size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_value, ebml_data_size);
	
	return bytes_read;
	
}


int krad_link_rep_to_string (krad_link_rep_t *krad_link, char *text) {

	int pos;
	
	pos = 0;
	
	pos += sprintf (text + pos, "%s %s", 
					krad_link_operation_mode_to_string (krad_link->operation_mode),
					krad_link_av_mode_to_string (krad_link->av_mode));

	if ((krad_link->operation_mode == RECORD) || (krad_link->operation_mode == TRANSMIT)) {

		if ((krad_link->operation_mode == TRANSMIT) && (krad_link->transport_mode == UDP)) {
			pos += sprintf (text + pos, " %s", krad_link_transport_mode_to_string (krad_link->transport_mode));
		}

		if (krad_link->operation_mode == TRANSMIT) {
			pos += sprintf (text + pos, " to %s:%d%s", krad_link->host, krad_link->port, krad_link->mount);
		}

		if (krad_link->operation_mode == RECORD) {
			pos += sprintf (text + pos, " to %s", krad_link->filename);
		}
				
		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

			pos += sprintf (text + pos, " Video -");

			pos += sprintf (text + pos, " %dx%d %d/%d %d",
							krad_link->width, krad_link->height,
							krad_link->fps_numerator, krad_link->fps_denominator,
							krad_link->color_depth);

			pos += sprintf (text + pos, " Codec: %s", krad_codec_to_string (krad_link->video_codec));

			if (krad_link->video_codec == THEORA) {
				pos += sprintf (text + pos, " Quality: %d", krad_link->theora_quality); 
			}	
	
			if (krad_link->video_codec == VP8) {
				pos += sprintf (text + pos, " Bitrate: %d Min Quantizer: %d Max Quantizer: %d Deadline: %d",
								krad_link->vp8_bitrate, krad_link->vp8_min_quantizer,
								krad_link->vp8_max_quantizer, krad_link->vp8_deadline);
			}
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			pos += sprintf (text + pos, " Audio -");
			pos += sprintf (text + pos, " Sample Rate: %d", krad_link->audio_sample_rate);
			pos += sprintf (text + pos, " Channels: %d", krad_link->audio_channels);
			pos += sprintf (text + pos, " Codec: %s", krad_codec_to_string (krad_link->audio_codec));
						
			if (krad_link->audio_codec == FLAC) {
				pos += sprintf (text + pos, " Bit Depth: %d", krad_link->flac_bit_depth);
			}		

			if (krad_link->audio_codec == VORBIS) {
				pos += sprintf (text + pos, " Quality: %.1f", krad_link->vorbis_quality); 
			}		

			if (krad_link->audio_codec == OPUS) {
				pos += sprintf (text + pos, " Complexity: %d Bitrate: %d Frame Size: %d Signal: %s Bandwidth: %s", krad_link->opus_complexity,
								krad_link->opus_bitrate, krad_link->opus_frame_size, krad_opus_signal_to_string (krad_link->opus_signal),
								krad_opus_bandwidth_to_string (krad_link->opus_bandwidth));

			}
		}
	}

	if (krad_link->operation_mode == RECEIVE) {
		if ((krad_link->transport_mode == UDP) || (krad_link->transport_mode == TCP)) {
			pos += sprintf (text + pos, " Port %d", krad_link->port);
		}
	}
	
	if (krad_link->operation_mode == PLAYBACK) {
		if (krad_link->transport_mode == FILESYSTEM) {
			pos += sprintf (text + pos, " File %s", krad_link->filename);
		}

		if (krad_link->transport_mode == TCP) {
			pos += sprintf (text + pos, " %s:%d%s",
							krad_link->host, krad_link->port, krad_link->mount);
		}
	}
	
	if (krad_link->operation_mode == CAPTURE) {
		pos += sprintf (text + pos, " from %s", krad_link_video_source_to_string (krad_link->video_source));
		pos += sprintf (text + pos, " with device %s", krad_link->video_device);		

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

			pos += sprintf (text + pos, " at ");

			//pos += sprintf (text + pos, " %dx%d %d/%d %d",
			//				krad_link->width, krad_link->height,
			//				krad_link->fps_numerator, krad_link->fps_denominator,
			//				krad_link->color_depth);

			pos += sprintf (text + pos, " %dx%d %d/%d",
							krad_link->width, krad_link->height,
							krad_link->fps_numerator, krad_link->fps_denominator);

		}

	}

	return pos;
}

int krad_ipc_client_read_port ( krad_ipc_client_t *client, char *text) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	int source_width;
	
	char string[1024];
	memset (string, '\0', 1024);
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_COMPOSITOR_PORT) {
		//printk ("hrm wtf1\n");
	} else {
		bytes_read += ebml_data_size + 11;
	}
	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_COMPOSITOR_PORT_SOURCE_WIDTH) {
		//printk ("hrm wtf2\n");
	} else {
		//printk ("tag name size %zu\n", ebml_data_size);
	}

	source_width = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	
	sprintf (text, "Source Width: %d", source_width);
	
	
	return bytes_read;

}

void krad_ipc_client_read_compositor_subunit_controls (krad_ipc_client_t *client, kr_compositor_subunit_controls_t *controls) {
	
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->x = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->y = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->z = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->width = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->height = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->xscale = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->yscale = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->rotation = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	controls->opacity = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
}
int krad_ipc_client_read_sprite ( krad_ipc_client_t *client, char *text, krad_sprite_rep_t **krad_sprite_rep) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	int sprite_number;
	
	krad_sprite_rep_t *krad_sprite;
	
	char string[1024];
	memset (string, '\0', 1024);
	
	krad_sprite = calloc (1, sizeof (krad_sprite_rep_t));
	krad_sprite->controls = calloc (1, sizeof (kr_compositor_subunit_controls_t));
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_COMPOSITOR_SPRITE_LIST) {
		//printk ("hrm wtf1\n");
	} else {
		bytes_read += ebml_data_size + 10;
	}
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	sprite_number = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	krad_ebml_read_string( client->krad_ebml, krad_sprite->filename, ebml_data_size);
	
	krad_ipc_client_read_compositor_subunit_controls (client, krad_sprite->controls);
	
	sprintf (text, "Id: %d  Filename: \"%s\"  X: %d  Y: %d  Z: %d  Xscale: %f Yscale: %f  Rotation: %f  Opacity: %f", 
	         sprite_number, krad_sprite->filename,
	         krad_sprite->controls->x, krad_sprite->controls->y, krad_sprite->controls->z,
	         krad_sprite->controls->xscale, krad_sprite->controls->yscale,
	         krad_sprite->controls->rotation, krad_sprite->controls->opacity);
         
		
	return bytes_read;
}

int krad_ipc_client_read_text ( krad_ipc_client_t *client, char *text, krad_text_rep_t **krad_text_rep) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	int text_number;
	
	krad_text_rep_t *krad_text;
	
	char string[1024];
	memset (string, '\0', 1024);
	
	krad_text = calloc (1, sizeof (krad_text_rep_t));
	krad_text->controls = calloc (1, sizeof (kr_compositor_subunit_controls_t));
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_COMPOSITOR_TEXT) {
		//printk ("hrm wtf1\n");
	} else {
		bytes_read += ebml_data_size + 9;
	}
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	text_number = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	krad_ebml_read_string( client->krad_ebml, krad_text->text, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	krad_ebml_read_string( client->krad_ebml, krad_text->font, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_COMPOSITOR_RED) {
		//printk ("hrm wtf2\n");
	} else {
		//printk ("tag name size %zu\n", ebml_data_size);
	}
		
	krad_text->red = 
	        krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_COMPOSITOR_GREEN) {
		//printk ("hrm wtf2\n");
	} else {
		//printk ("tag name size %zu\n", ebml_data_size);
	}
		
	krad_text->green = 
	        krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_COMPOSITOR_BLUE) {
		//printk ("hrm wtf2\n");
	} else {
		//printk ("tag name size %zu\n", ebml_data_size);
	}
		
	krad_text->blue = 
	        krad_ebml_read_float (client->krad_ebml, ebml_data_size);

	krad_ipc_client_read_compositor_subunit_controls (client, krad_text->controls);

	sprintf (text, "Id: %d  Text:\"%s\"  Font: \"%s\"  Red: %d  Green: %d  Blue: %d  X: %d  Y: %d  Z: %d  Xscale: %f Yscale: %f  Rotation: %f  Opacity: %f", 
	         text_number, krad_text->text, krad_text->font,
	         (int) (1000*krad_text->red), (int) (1000*krad_text->green), (int) (1000*krad_text->blue),
	         krad_text->controls->x, krad_text->controls->y, krad_text->controls->z,
	         krad_text->controls->xscale, krad_text->controls->yscale,
	         krad_text->controls->rotation, krad_text->controls->opacity);
		
	return bytes_read;
}
int krad_ipc_client_read_frame_size ( krad_ipc_client_t *client, char *text, krad_compositor_rep_t **krad_compositor_rep) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	int width, height;
	
	char string[1024];
	memset (string, '\0', 1024);
	
	bytes_read = 6;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	width = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
	bytes_read += ebml_data_size;
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	height = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
	bytes_read += ebml_data_size;

	sprintf (text, "Width: %d  Height: %d", 
	         width, height);
		
	return bytes_read;
}

int krad_ipc_client_read_frame_rate ( krad_ipc_client_t *client, char *text, krad_compositor_rep_t **krad_compositor_rep) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	int numerator, denominator;
	
	char string[1024];
	memset (string, '\0', 1024);
	
	bytes_read = 6;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	numerator = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
	bytes_read += ebml_data_size;
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	denominator = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
	bytes_read += ebml_data_size;

	sprintf (text, "Numerator: %d  Denominator: %d - %f", 
	         numerator, denominator, (float) numerator / (float) denominator );
		
	return bytes_read;
}

int krad_ipc_client_read_link ( krad_ipc_client_t *client, char *text, krad_link_rep_t **krad_link_rep) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	krad_link_rep_t *krad_link;
	
	char string[1024];
	memset (string, '\0', 1024);
	
	krad_link = calloc (1, sizeof (krad_link_rep_t));
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_LINK_LINK) {
		//printk ("hrm wtf1\n");
	} else {
		bytes_read += ebml_data_size + 9;
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	krad_link->link_num = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_LINK_LINK_AV_MODE) {
		//printk ("hrm wtf2\n");
	} else {
		//printk ("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);

	if (strcmp(string, "audio only") == 0) {
		krad_link->av_mode = AUDIO_ONLY;
	}
	
	if (strcmp(string, "video only") == 0) {
		krad_link->av_mode = VIDEO_ONLY;
	}
	
	if (strcmp(string, "audio and video") == 0) {
		krad_link->av_mode = AUDIO_AND_VIDEO;
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_LINK_LINK_OPERATION_MODE) {
		//printk ("hrm wtf3\n");
	} else {
		//printk ("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);

	krad_link->operation_mode = krad_link_string_to_operation_mode (string);
	
	if (krad_link->operation_mode == RECEIVE) {
	
		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			//printk ("hrm wtf4\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		if ((krad_link->transport_mode == UDP) || (krad_link->transport_mode == TCP)) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
				//printk ("hrm wtf5\n");
			} else {
				//printk ("tag value size %zu\n", ebml_data_size);
			}

			krad_link->port = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
		}
	
	}
	
	if (krad_link->operation_mode == PLAYBACK) {
	
		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

		if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
			//printk ("hrm wtf4\n");
		} else {
			//printk ("tag name size %zu\n", ebml_data_size);
		}

		krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
		krad_link->transport_mode = krad_link_string_to_transport_mode (string);
	
		if (krad_link->transport_mode == FILESYSTEM) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_FILENAME) {
				//printk ("hrm wtf5\n");
			} else {
				//printk ("tag value size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->filename, ebml_data_size);
		}
	
		if (krad_link->transport_mode == TCP) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
				//printk ("hrm wtf4\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->host, ebml_data_size);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
				//printk ("hrm wtf5\n");
			} else {
				//printk ("tag value size %zu\n", ebml_data_size);
			}

			krad_link->port = krad_ebml_read_number (client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
				//printk ("hrm wtf6\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->mount, ebml_data_size);
		
		}	
	
	
	}
	
	if (krad_link->operation_mode == CAPTURE) {
	
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_SOURCE) {
				//printk ("hrm wtf2v\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
			krad_link->video_source = krad_link_string_to_video_source (string);
			
	}

	if ((krad_link->operation_mode == TRANSMIT) || (krad_link->operation_mode == RECORD)) {
	
	
		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
	
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_VIDEO_CODEC) {
				//printk ("hrm wtf2v\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
			krad_link->video_codec = krad_string_to_codec (string);
		
		}

		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
	
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_AUDIO_CODEC) {
				//printk ("hrm wtf2a\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
		
			krad_link->audio_codec = krad_string_to_codec (string);
		}
	
		if (krad_link->operation_mode == TRANSMIT) {

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_TRANSPORT_MODE) {
				//printk ("hrm wtf4\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
			
			krad_link->transport_mode = krad_link_string_to_transport_mode (string);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_HOST) {
				//printk ("hrm wtf4\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->host, ebml_data_size);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_PORT) {
				//printk ("hrm wtf5\n");
			} else {
				//printk ("tag value size %zu\n", ebml_data_size);
			}

			krad_link->port = krad_ebml_read_number (client->krad_ebml, ebml_data_size);

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_MOUNT) {
				//printk ("hrm wtf6\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->mount, ebml_data_size);
		}
		
		if (krad_link->operation_mode == RECORD) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

			if (ebml_id != EBML_ID_KRAD_LINK_LINK_FILENAME) {
				//printk ("hrm wtf4\n");
			} else {
				//printk ("tag name size %zu\n", ebml_data_size);
			}

			krad_ebml_read_string (client->krad_ebml, krad_link->filename, ebml_data_size);			
		
		}


		if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
		
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_AUDIO_CHANNELS) {
				krad_link->audio_channels = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_AUDIO_SAMPLE_RATE) {
				krad_link->audio_sample_rate = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}					

		}

		if ((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VIDEO_WIDTH) {
				krad_link->width = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VIDEO_HEIGHT) {
				krad_link->height = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_FPS_NUMERATOR) {
				krad_link->fps_numerator = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_FPS_DENOMINATOR) {
				krad_link->fps_denominator = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}									

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VIDEO_COLOR_DEPTH) {
				krad_link->color_depth = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}			
		}		

		if (((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->audio_codec == OPUS)) {

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_SIGNAL) {
				krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
				krad_link->opus_signal = krad_opus_string_to_signal (string);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BANDWIDTH) {
				krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
				krad_link->opus_bandwidth = krad_opus_string_to_bandwidth (string);					
			}

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_BITRATE) {
				krad_link->opus_bitrate = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_COMPLEXITY) {
				krad_link->opus_complexity = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_OPUS_FRAME_SIZE) {
				krad_link->opus_frame_size = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}	

			//EBML_ID_KRAD_LINK_LINK_OGG_MAX_PACKETS_PER_PAGE, atoi(argv[5]));
		}

		if (((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->audio_codec == VORBIS)) {
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VORBIS_QUALITY) {
				krad_link->vorbis_quality = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
			}
		}

		if (((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->audio_codec == FLAC)) {
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_FLAC_BIT_DEPTH) {
				krad_link->flac_bit_depth = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}			
		}

		if (((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->video_codec == THEORA)) {
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_THEORA_QUALITY) {
				krad_link->theora_quality = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
		}

		if (((krad_link->av_mode == VIDEO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) && (krad_link->video_codec == VP8)) {

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_BITRATE) {
				krad_link->vp8_bitrate = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_MIN_QUANTIZER) {
				krad_link->vp8_min_quantizer = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
						
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_MAX_QUANTIZER) {
				krad_link->vp8_max_quantizer = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}

			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			if (ebml_id == EBML_ID_KRAD_LINK_LINK_VP8_DEADLINE) {
				krad_link->vp8_deadline = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
			}
		}
	}
	
	if (text != NULL) {
		krad_link_rep_to_string ( krad_link, text );
	}
	
	if (krad_link_rep != NULL) {
		*krad_link_rep = krad_link;
	} else {
		free (krad_link);
	}
	
	return bytes_read;

}


int krad_ipc_client_read_portgroup ( krad_ipc_client_t *client, char *portname, float *volume, char *crossfade_name, float *crossfade, int *has_xmms2) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	char string[1024];
	float floaty;
	
	int8_t channels;
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	bytes_read += ebml_data_size + 9;
	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP) {
		//printk ("hrm wtf");
	} else {
		//printk ("tag size %"PRIu64"", ebml_data_size);
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, portname, ebml_data_size);
	
	//printk ("Input name: %s", portname);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS) {
		//printk ("hrm wtf3");
	} else {
		//printk ("tag value size %"PRIu64"", ebml_data_size);
	}

	channels = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	printk  ("Channels: %d", channels);

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_TYPE) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
	
	//printk ("Type: %s", string);	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}

	floaty = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	*volume = floaty;
	
	//printk ("Volume: %f", floaty);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);	
	
	//printk ("Bus: %s", string);

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, crossfade_name, ebml_data_size);	
	
	
	if (strlen(crossfade_name)) {
		//printk ("Crossfade With: %s", crossfade_name);	
	}

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}

	floaty = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	if (strlen(crossfade_name)) {
		*crossfade = floaty;
		//printk ("Crossfade: %f", floaty);
	} else {
		*crossfade = 0.0f;
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_XMMS2) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}
	
	*has_xmms2 = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	return bytes_read;
	
}

void krad_ipc_client_read_tag_inner ( krad_ipc_client_t *client, char **tag_item, char **tag_name, char **tag_value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
/*
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_RADIO_TAG) {
		printf("hrm wtf\n");
	} else {
		//printf("tag size %zu\n", ebml_data_size);
	}
*/

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
		//printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_item, ebml_data_size);

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
		//printf("hrm wtf2\n");
	} else {
		//printf("tag name size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_name, ebml_data_size);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
		//printf("hrm wtf3\n");
	} else {
		//printf("tag value size %zu\n", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, *tag_value, ebml_data_size);
	
}

void krad_ipc_print_response (krad_ipc_client_t *client) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	fd_set set;
	char tag_name_actual[256];
	char tag_item_actual[256];
	char tag_value_actual[1024];

	char string[1024];	
	krad_link_rep_t *krad_link;
	krad_text_rep_t *krad_text;
	krad_sprite_rep_t *krad_sprite;
	krad_compositor_rep_t *krad_compositor;
	
	tag_item_actual[0] = '\0';	
	tag_name_actual[0] = '\0';
	tag_value_actual[0] = '\0';
	
	string[0] = '\0';	
	
	char *tag_item = tag_item_actual;
	char *tag_name = tag_name_actual;
	char *tag_value = tag_value_actual;	
	
	char crossfadename_actual[1024];	
	char *crossfadename = crossfadename_actual;
	float crossfade;	
	
	int bytes_read;
	int list_size;
	int list_count;	
	
	int has_xmms2;
	
	has_xmms2 = 0;
	list_size = 0;
	bytes_read = 0;
	
	float floatval;
	int i;
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
	
	floatval = 0;
	i = 0;
	ebml_id = 0;
	ebml_data_size = 0;
	
	FD_ZERO (&set);
	FD_SET (client->sd, &set);	

	ret = select (client->sd+1, &set, NULL, NULL, &tv);
	
	if (ret > 0) {

		krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
		switch ( ebml_id ) {
	
			case EBML_ID_KRAD_RADIO_MSG:
				//printf("Received KRAD_RADIO_MSG %zu bytes of data.\n", ebml_data_size);		
		
				krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
				switch ( ebml_id ) {
	
					case EBML_ID_KRAD_RADIO_TAG_LIST:
						//printf("Received Tag list %"PRIu64" bytes of data.\n", ebml_data_size);
						list_size = ebml_data_size;
						while ((list_size) && ((bytes_read += krad_ipc_client_read_tag ( client, &tag_item, &tag_name, &tag_value )) <= list_size)) {
							printf ("%s: %s - %s\n", tag_item, tag_name, tag_value);
							if (bytes_read == list_size) {
								break;
							}
						}
						break;
					case EBML_ID_KRAD_RADIO_TAG:
						krad_ipc_client_read_tag_inner ( client, &tag_item, &tag_name, &tag_value );
						printf ("%s: %s - %s\n", tag_item, tag_name, tag_value);
						break;

					case EBML_ID_KRAD_RADIO_UPTIME:
						number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
						printf("up ");
						updays = number / (60*60*24);
						if (updays) {
							printf ("%d day%s, ", updays, (updays != 1) ? "s" : "");
						}
						upminutes = number / 60;
						uphours = (upminutes / 60) % 24;
						upminutes %= 60;
						if (uphours) {
							printf ("%2d:%02d ", uphours, upminutes);
						} else {
							printf ("%d min ", upminutes);
						}
						printf ("\n");
					
						break;
					case EBML_ID_KRAD_RADIO_SYSTEM_INFO:
						krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
						if (string[0] != '\0') {
							printf ("%s\n", string);
						}
						break;
					case EBML_ID_KRAD_RADIO_LOGNAME:
						krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
						if (string[0] != '\0') {
							printf ("%s\n", string);
						}
						break;
					case EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE:
						number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
						printf ("%"PRIu64"%%\n", number);
						break;
						
				}
		
		
				break;
		case EBML_ID_KRAD_MIXER_MSG:
			//printf("Received KRAD_MIXER_MSG %"PRIu64" bytes of data.\n", ebml_data_size);
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			switch ( ebml_id ) {
			case EBML_ID_KRAD_MIXER_CONTROL:
				printk ("Received mixer control list %"PRIu64" bytes of data.\n", ebml_data_size);
				break;	
				
			case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
				//printf("Received PORTGROUP list %zu bytes of data.\n", ebml_data_size);
				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nAudio Inputs: \n");
				}
				while ((list_size) && ((bytes_read += krad_ipc_client_read_portgroup ( client, tag_name, &floatval, crossfadename, &crossfade, &has_xmms2 )) <= list_size)) {
					printf ("  %0.2f%%  %s", floatval, tag_name);
					if (strlen(crossfadename)) {
						printf (" < %0.2f > %s", crossfade, crossfadename);
					}
					if (has_xmms2) {
						if (!(strlen(crossfadename))) {
							printf ("\t\t");
						}							
						printf ("\t\t[XMMS2]");
					}
					printf ("\n");
					if (bytes_read == list_size) {
						break;
					}
				}	
				break;
				
			case EBML_ID_KRAD_MIXER_PORTGROUP:
				//krad_ipc_client_read_portgroup_inner ( client, &tag_name, &tag_value );
				printf("PORTGROUP %"PRIu64" bytes  \n", ebml_data_size );
				break;
					
			case EBML_ID_KRAD_MIXER_SAMPLE_RATE:
				//krad_ipc_client_read_portgroup_inner ( client, &tag_name, &tag_value );
				number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
				printf ("Krad Mixer Sample Rate: %"PRIu64"\n", number );
				break;
					
			case EBML_ID_KRAD_MIXER_JACK_RUNNING:
				//krad_ipc_client_read_portgroup_inner ( client, &tag_name, &tag_value );
				number = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
				if (number > 0) {
					printf ("Yes\n");
				} else {
					printf ("Jack Server not running\n");
				}
				break;
			}
			break;
			
		case EBML_ID_KRAD_COMPOSITOR_MSG:
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			switch ( ebml_id ) {
			
			case EBML_ID_KRAD_COMPOSITOR_INFO:
				krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
				if (string[0] != '\0') {
					printf ("%s\n", string);
				}
				break;
			case EBML_ID_KRAD_COMPOSITOR_FRAME_SIZE:
				list_size = ebml_data_size;
				
				i = 0;
				while ((list_size) && ((bytes_read +=  krad_ipc_client_read_frame_size ( 
				                            client, tag_value, &krad_compositor)) <= list_size)) {
					printf ("  %s\n", tag_value);
					i++;
					if (bytes_read == list_size) {
						break;
					} else {
						//printf ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;
			case EBML_ID_KRAD_COMPOSITOR_FRAME_RATE:
				list_size = ebml_data_size;
				
				i = 0;
				while ((list_size) && ((bytes_read += krad_ipc_client_read_frame_rate ( client, tag_value, &krad_compositor)) <= list_size)) {
					printf ("  %s\n", tag_value);
					i++;
					if (bytes_read == list_size) {
						break;
					} else {
						//printf ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;
			case EBML_ID_KRAD_COMPOSITOR_LAST_SNAPSHOT_NAME:
				krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
				if (string[0] != '\0') {
					printf ("%s\n", string);
				}
				break;
			case EBML_ID_KRAD_COMPOSITOR_PORT_LIST:
				//printk ("Received LINK control list %"PRIu64" bytes of data.\n", ebml_data_size);
				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nVideo Ports:\n");
				}
				i = 0;
				while ((list_size) && ((bytes_read += krad_ipc_client_read_port ( client, tag_value)) <= list_size)) {
					printf ("  %d: %s\n", i, tag_value);
					i++;
					if (bytes_read == list_size) {
						break;
					} else {
						//printk ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;
			
			case EBML_ID_KRAD_COMPOSITOR_TEXT_LIST:
				//printf ("Received TEXT list %"PRIu64" bytes of data, %d bytes read\n", ebml_data_size, bytes_read);

				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nTexts:\n");
				}
				i = 0;
				while ((list_size) && ((bytes_read += krad_ipc_client_read_text ( client, tag_value, &krad_text)) <= list_size)) {
					printf ("  %s\n", tag_value);
					i++;
					if (bytes_read == list_size) {
						break;
					} else {
						//printf ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;							
			
				
			case EBML_ID_KRAD_COMPOSITOR_SPRITE_LIST:
				//printf ("Received SPRITE list %"PRIu64" bytes of data, %d bytes read\n", ebml_data_size, bytes_read);

				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nSprites:\n");
				}
				i = 0;
				while ((list_size) && ((bytes_read += krad_ipc_client_read_sprite ( client, tag_value, &krad_sprite)) <= list_size)) {
					printf ("  %s\n", tag_value);
					i++;
					if (bytes_read == list_size) {
						break;
					} else {
						//printf ("%d: %d\n", list_size, bytes_read);
					}
				}	
				break;							
			}
			break;
		case EBML_ID_KRAD_LINK_MSG:
			
			krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
			
			switch ( ebml_id ) {
			
			case EBML_ID_KRAD_LINK_DECKLINK_LIST:
				
				krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
				list_count = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
				for (i = 0; i < list_count; i++) {
					krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);						
					krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
					printf ("%d: %s\n", i, string);
				}	
				break;
				
				
			case EBML_ID_KRAD_LINK_V4L2_LIST:
				
				krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
				list_count = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
				for (i = 0; i < list_count; i++) {
					krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);						
					krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
					printf ("%s\n", string);
				}	
				break;				
				
			case EBML_ID_KRAD_LINK_LINK_LIST:
				//printf("Received LINK control list %"PRIu64" bytes of data.\n", ebml_data_size);
				
				list_size = ebml_data_size;
				if (list_size) {
					printf ("\nLinks:\n");
				}
				i = 0;
				while ((list_size) && ((bytes_read += krad_ipc_client_read_link ( client, tag_value, &krad_link)) <= list_size)) {
					printf ("  Id: %d  %s\n", krad_link->link_num, tag_value);
					free (krad_link);
					i++;
					if (bytes_read == list_size) {
						break;
					}
				}	
				break;
			
			default:
				printf("Received KRAD_LINK_MSG %"PRIu64" bytes of data.\n", ebml_data_size);
				break;
			}
			
			break;
		}
	}
}

void kr_audioport_destroy_cmd (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t destroy_audioport;
	
	compositor_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_LOCAL_AUDIOPORT_DESTROY, &destroy_audioport);

	//krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_finish_element (client->krad_ebml, destroy_audioport);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_audioport_create_cmd (kr_client_t *client, krad_mixer_portgroup_direction_t direction) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t create_audioport;
	
	compositor_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_LOCAL_AUDIOPORT_CREATE, &create_audioport);

	if (direction == OUTPUT) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION, "output");
	} else {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION, "input");
	}
	
	krad_ebml_finish_element (client->krad_ebml, create_audioport);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_videoport_destroy_cmd (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t destroy_videoport;
	
	compositor_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LOCAL_VIDEOPORT_DESTROY, &destroy_videoport);

	//krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_finish_element (client->krad_ebml, destroy_videoport);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_videoport_create_cmd (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t create_videoport;
	
	compositor_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LOCAL_VIDEOPORT_CREATE, &create_videoport);

	//krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_finish_element (client->krad_ebml, create_videoport);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

int krad_ipc_client_sendfd (kr_client_t *client, int fd) {
	char buf[1];
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	int n;
	char cms[CMSG_SPACE(sizeof(int))];

	buf[0] = 0;
	iov.iov_base = buf;
	iov.iov_len = 1;

	memset(&msg, 0, sizeof msg);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (caddr_t)cms;
	msg.msg_controllen = CMSG_LEN(sizeof(int));

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	memmove(CMSG_DATA(cmsg), &fd, sizeof(int));

	if((n=sendmsg(client->sd, &msg, 0)) != iov.iov_len)
		    return 0;
	return 1;
}

float *kr_audioport_get_buffer (kr_audioport_t *kr_audioport, int channel) {
	return (float *)kr_audioport->kr_shm->buffer + (channel * 1600);
}


void kr_audioport_set_callback (kr_audioport_t *kr_audioport, int callback (uint32_t, void *), void *pointer) {

	kr_audioport->callback = callback;
	kr_audioport->pointer = pointer;

}

void *kr_audioport_process_thread (void *arg) {

	kr_audioport_t *kr_audioport = (kr_audioport_t *)arg;

	krad_system_set_thread_name ("krc_audioport");

	char buf[1];

	while (kr_audioport->active == 1) {
	
		// wait for socket to have a byte
		read (kr_audioport->sd, buf, 1);
	
		//kr_audioport->callback (kr_audioport->kr_shm->buffer, kr_audioport->pointer);
		kr_audioport->callback (1600, kr_audioport->pointer);

		// write a byte to socket
		write (kr_audioport->sd, buf, 1);


	}


	return NULL;

}

void kr_audioport_activate (kr_audioport_t *kr_audioport) {
	if ((kr_audioport->active == 0) && (kr_audioport->callback != NULL)) {
		pthread_create (&kr_audioport->process_thread, NULL, kr_audioport_process_thread, (void *)kr_audioport);
		kr_audioport->active = 1;
	}
}

void kr_audioport_deactivate (kr_audioport_t *kr_audioport) {

	if (kr_audioport->active == 1) {
		kr_audioport->active = 2;
		pthread_join (kr_audioport->process_thread, NULL);
		kr_audioport->active = 0;
	}
}

kr_audioport_t *kr_audioport_create (kr_client_t *client, krad_mixer_portgroup_direction_t direction) {

	kr_audioport_t *kr_audioport;
	int sockets[2];

	if (client->tcp_port != 0) {
		// Local clients only
		return NULL;
	}

	kr_audioport = calloc (1, sizeof(kr_audioport_t));

	if (kr_audioport == NULL) {
		return NULL;
	}

	kr_audioport->client = client;
	kr_audioport->direction = direction;

	kr_audioport->kr_shm = kr_shm_create (kr_audioport->client);

	//sprintf (kr_audioport->kr_shm->buffer, "waa hoo its yaytime");

	if (kr_audioport->kr_shm == NULL) {
		free (kr_audioport);
		return NULL;
	}

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
		kr_shm_destroy (kr_audioport->kr_shm);
		free (kr_audioport);
		return NULL;
    }

	kr_audioport->sd = sockets[0];
	
	printf ("sockets %d and %d\n", sockets[0], sockets[1]);
	
	kr_audioport_create_cmd (kr_audioport->client, kr_audioport->direction);
	//FIXME use a return message from daemon to indicate ready to receive fds
	usleep (33000);
	krad_ipc_client_sendfd (kr_audioport->client, kr_audioport->kr_shm->fd);
	usleep (33000);
	krad_ipc_client_sendfd (kr_audioport->client, sockets[1]);
	usleep (33000);
	return kr_audioport;

}

void kr_audioport_destroy (kr_audioport_t *kr_audioport) {

	if (kr_audioport->active == 1) {
		kr_audioport_deactivate (kr_audioport);
	}

	kr_audioport_destroy_cmd (kr_audioport->client);

	if (kr_audioport != NULL) {
		if (kr_audioport->sd != 0) {
			close (kr_audioport->sd);
			kr_audioport->sd = 0;
		}
		if (kr_audioport->kr_shm != NULL) {
			kr_shm_destroy (kr_audioport->kr_shm);
			kr_audioport->kr_shm = NULL;
		}
		free(kr_audioport);
	}
}

void kr_videoport_set_callback (kr_videoport_t *kr_videoport, int callback (void *, void *), void *pointer) {

	kr_videoport->callback = callback;
	kr_videoport->pointer = pointer;

}

void *kr_videoport_process_thread (void *arg) {

	kr_videoport_t *kr_videoport = (kr_videoport_t *)arg;

	krad_system_set_thread_name ("krc_videoport");

	char buf[1];

	while (kr_videoport->active == 1) {
	
		// wait for socket to have a byte
		read (kr_videoport->sd, buf, 1);
	
		kr_videoport->callback (kr_videoport->kr_shm->buffer, kr_videoport->pointer);


		// write a byte to socket
		write (kr_videoport->sd, buf, 1);


	}


	return NULL;

}

void kr_videoport_activate (kr_videoport_t *kr_videoport) {
	if ((kr_videoport->active == 0) && (kr_videoport->callback != NULL)) {
		pthread_create (&kr_videoport->process_thread, NULL, kr_videoport_process_thread, (void *)kr_videoport);
		kr_videoport->active = 1;
	}
}

void kr_videoport_deactivate (kr_videoport_t *kr_videoport) {

	if (kr_videoport->active == 1) {
		kr_videoport->active = 2;
		pthread_join (kr_videoport->process_thread, NULL);
		kr_videoport->active = 0;
	}
}

kr_videoport_t *kr_videoport_create (kr_client_t *client) {

	kr_videoport_t *kr_videoport;
	int sockets[2];

	if (client->tcp_port != 0) {
		// Local clients only
		return NULL;
	}

	kr_videoport = calloc (1, sizeof(kr_videoport_t));

	if (kr_videoport == NULL) {
		return NULL;
	}

	kr_videoport->client = client;

	kr_videoport->kr_shm = kr_shm_create (kr_videoport->client);

	sprintf (kr_videoport->kr_shm->buffer, "waa hoo its yaytime");

	if (kr_videoport->kr_shm == NULL) {
		free (kr_videoport);
		return NULL;
	}

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
		kr_shm_destroy (kr_videoport->kr_shm);
		free (kr_videoport);
		return NULL;
    }

	kr_videoport->sd = sockets[0];
	
	printf ("sockets %d and %d\n", sockets[0], sockets[1]);
	
	kr_videoport_create_cmd (kr_videoport->client);
	//FIXME use a return message from daemon to indicate ready to receive fds
	usleep (33000);
	krad_ipc_client_sendfd (kr_videoport->client, kr_videoport->kr_shm->fd);
	usleep (33000);
	krad_ipc_client_sendfd (kr_videoport->client, sockets[1]);
	usleep (33000);
	return kr_videoport;

}

void kr_videoport_destroy (kr_videoport_t *kr_videoport) {

	if (kr_videoport->active == 1) {
		kr_videoport_deactivate (kr_videoport);
	}

	kr_videoport_destroy_cmd (kr_videoport->client);

	if (kr_videoport != NULL) {
		if (kr_videoport->sd != 0) {
			close (kr_videoport->sd);
			kr_videoport->sd = 0;
		}
		if (kr_videoport->kr_shm != NULL) {
			kr_shm_destroy (kr_videoport->kr_shm);
			kr_videoport->kr_shm = NULL;
		}
		free(kr_videoport);
	}
}

void kr_shm_destroy (kr_shm_t *kr_shm) {

	if (kr_shm != NULL) {
		if (kr_shm->buffer != NULL) {
			munmap (kr_shm->buffer, kr_shm->size);
			kr_shm->buffer = NULL;
		}
		if (kr_shm->fd != 0) {
			close (kr_shm->fd);
		}
		free(kr_shm);
	}
}

kr_shm_t *kr_shm_create (krad_ipc_client_t *client) {

	char filename[] = "/tmp/krad-shm-XXXXXX";
	kr_shm_t *kr_shm;

	kr_shm = calloc (1, sizeof(kr_shm_t));

	if (kr_shm == NULL) {
		return NULL;
	}

	kr_shm->size = 960 * 540 * 4 * 2;

	kr_shm->fd = mkstemp (filename);
	if (kr_shm->fd < 0) {
		fprintf(stderr, "open %s failed: %m\n", filename);
		kr_shm_destroy (kr_shm);
		return NULL;
	}

	if (ftruncate (kr_shm->fd, kr_shm->size) < 0) {
		fprintf (stderr, "ftruncate failed: %m\n");
		kr_shm_destroy (kr_shm);
		return NULL;
	}

	kr_shm->buffer = mmap (NULL, kr_shm->size, PROT_READ | PROT_WRITE, MAP_SHARED, kr_shm->fd, 0);
	unlink (filename);

	if (kr_shm->buffer == MAP_FAILED) {
		fprintf (stderr, "mmap failed: %m\n");
		kr_shm_destroy (kr_shm);
		return NULL;
	}

	return kr_shm;

}

int krad_ipc_wait (krad_ipc_client_t *client, char *buffer, int size) {

	//int len;
	int bytes;
	fd_set set;

	//strcat(cmd, "|");
	//len = strlen(cmd);
	
	//FD_ZERO (&set);
	//FD_SET (client->sd, &set);
	
	//select (client->sd+1, NULL, &set, NULL, NULL);
	//send (client->sd, cmd, len, 0);

	FD_ZERO (&set);
	FD_SET (client->sd, &set);

	select (client->sd+1, &set, NULL, NULL, NULL);
	bytes = recv(client->sd, buffer, size, 0);
	buffer[bytes] = '\0';
	//printf("Received %d bytes of data: '%s' \n", bytes, client->buffer);
	return bytes;
}

int krad_ipc_recv (krad_ipc_client_t *client, char *buffer, int size) {

	//int len;
	int bytes;
	//fd_set set;

	//strcat(cmd, "|");
	//len = strlen(cmd);
	
	//FD_ZERO (&set);
	//FD_SET (client->sd, &set);
	
	//select (client->sd+1, NULL, &set, NULL, NULL);
	//send (client->sd, cmd, len, 0);

	//FD_ZERO (&set);
	//FD_SET (client->sd, &set);

	//select (client->sd+1, &set, NULL, NULL, NULL);
	//bytes = recv(client->sd, client->buffer, KRAD_IPC_BUFFER_SIZE, 0);

	bytes = recv(client->sd, buffer, size, 0);
	buffer[bytes] = '\0';
	//printf("Received %d bytes of data: '%s' \n", bytes, client->buffer);
	return bytes;
}

void kr_disconnect(krad_ipc_client_t *client) {

	if (client != NULL) {
		if (client->buffer != NULL) {
			free (client->buffer);
		}
		if (client->sd != 0) {
			close (client->sd);
		}
		if (client->krad_ebml != NULL) {
			krad_ebml_destroy (client->krad_ebml);
		}
		free(client);
	}
}

