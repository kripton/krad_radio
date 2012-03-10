#include "krad_ipc_server.h"


int shutdown_krad_ipc_server;


krad_ipc_server_t *krad_ipc_server_setup(const char *daemon, char *station) {

	shutdown_krad_ipc_server = 0;

	krad_ipc_server_t *krad_ipc_server = calloc (1, sizeof (krad_ipc_server_t));

	if (krad_ipc_server == NULL) {
		printf("Krad IPC Server: Krad IPC server mem alloc fail");
		exit (1);
	} 
	
	if ((krad_ipc_server->client = calloc (CLIENT_SLOTS, sizeof (krad_ipc_server_client_t))) == NULL) {
		printf("Krad IPC Server: clients mem alloc fail");
		exit (1);
	} else {
		printf("Krad IPC Server: calloc'ed %d * %zu\n", CLIENT_SLOTS, sizeof (krad_ipc_server_client_t) );
	}
	
	
	uname(&krad_ipc_server->unixname);
	
	if (strncmp(krad_ipc_server->unixname.sysname, "Linux", 5) == 0) {
		krad_ipc_server->linux_abstract_sockets = 1;
		krad_ipc_server->ipc_path_pos = sprintf(krad_ipc_server->ipc_path, "@%s_ipc", daemon);
	} else {
		krad_ipc_server->ipc_path_pos = sprintf(krad_ipc_server->ipc_path, "%s/krad/run/%s_ipc", getenv ("HOME"), daemon);
	}
	
	if (station != NULL) {
		sprintf(krad_ipc_server->ipc_path + krad_ipc_server->ipc_path_pos, "_%s", station);
	}
	
	krad_ipc_server->fd = socket (AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

	if (krad_ipc_server->fd == -1) {
		printf("Krad IPC Server: Socket fail\n");
		krad_ipc_server_free(krad_ipc_server);
		return NULL;
	}

	krad_ipc_server->saddr.sun_family = AF_UNIX;

	snprintf (krad_ipc_server->saddr.sun_path, sizeof (krad_ipc_server->saddr.sun_path), "%s", krad_ipc_server->ipc_path);

	if (krad_ipc_server->linux_abstract_sockets) {
		krad_ipc_server->saddr.sun_path[0] = '\0';
		if (connect (krad_ipc_server->fd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) != -1) {
			/* active socket already exists! */
			close (krad_ipc_server->fd);
			printf("Krad IPC Server: Krad IPC path in use.. (wtf?) (linux abstract)\n");
			krad_ipc_server_free(krad_ipc_server);
			return NULL;
		}
	} else {
		if (access (krad_ipc_server->saddr.sun_path, F_OK) == 0) {
			if (connect (krad_ipc_server->fd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) != -1) {
				/* active socket already exists! */
				close (krad_ipc_server->fd);
				printf("Krad IPC Server: Krad IPC path in use.. (wtf?)\n");
				krad_ipc_server_free(krad_ipc_server);
				return NULL;
			}
			/* remove stale socket */
			unlink (krad_ipc_server->saddr.sun_path);
		}
	}
	
	if (bind (krad_ipc_server->fd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) == -1) {
		close (krad_ipc_server->fd);
		krad_ipc_server_free(krad_ipc_server);
		return NULL;
	}

	listen (krad_ipc_server->fd, 5);

	krad_ipc_server->flags = fcntl (krad_ipc_server->fd, F_GETFL, 0);

	if (krad_ipc_server->flags == -1) {
		close (krad_ipc_server->fd);
		krad_ipc_server_free(krad_ipc_server);
		return NULL;
	}

	krad_ipc_server->flags |= O_NONBLOCK;

	krad_ipc_server->flags = fcntl (krad_ipc_server->fd, F_SETFL, krad_ipc_server->flags);
	if (krad_ipc_server->flags == -1) {
		close (krad_ipc_server->fd);
		krad_ipc_server_free(krad_ipc_server);
		return NULL;
	}

	pthread_rwlock_init(&krad_ipc_server->send_lock, NULL);
	
	return krad_ipc_server;

}



krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server) {

	krad_ipc_server_client_t *client = NULL;
	
	int i;
	
	while (client == NULL) {
		for(i = 0; i < CLIENT_SLOTS; i++) {
			if (krad_ipc_server->client[i].active == 0) {
				client = &krad_ipc_server->client[i];
				break;
			}
		}
		if (client == NULL) {
			printf("---***Krad IPC Server****---: High load! dood! Had to cycle this shit...\n");
		}
	}
	
	client->krad_ipc_server = krad_ipc_server;

	struct sockaddr_un sin;
	socklen_t sin_len;

	sin_len = sizeof (sin);
	client->fd = accept (krad_ipc_server->fd, (struct sockaddr *)&sin, &sin_len);
	//client->fd = accept4 (krad_ipc_server->fd, (struct sockaddr *)&sin, &sin_len, SOCK_CLOEXEC);
	if (client->fd >= 0) {
		int flags;

		flags = fcntl (client->fd, F_GETFL, 0);

		if (flags == -1) {
			close (client->fd);
			return 0;
		}
		// the clients do block?!?!
		flags |= O_NONBLOCK;

		flags = fcntl (client->fd, F_SETFL, flags);
		if (flags == -1) {
			close (client->fd);
			return 0;
		}
		
		flags = fcntl (client->fd, F_GETFD, 0);

		if (flags == -1) {
			close (client->fd);
			return 0;
		}

		//flags |= SOCK_CLOEXEC;
		flags |= FD_CLOEXEC;

		flags = fcntl (client->fd, F_SETFD, flags);
		if (flags == -1) {
			close (client->fd);
			return 0;
		}
		
		if (krad_ipc_server->new_client_callback != NULL) {
			client->client_pointer = krad_ipc_server->new_client_callback(krad_ipc_server->client_callback_pointer);
		}
		krad_ipc_server->clients++;
		client->active = 1;
		return client;
	}

	return 0;
}


void *krad_ipc_server_client_loop(void *arg) {

	krad_ipc_server_client_t *client = (krad_ipc_server_client_t *)arg;

	printf("Krad IPC Server: Starting per-client-connection loop!\n");


	const int timeout_msecs = 5500;
		
	client->fds[0].fd = client->fd;
	//fds[0].events = POLLOUT | POLLIN;
	client->fds[0].events = POLLIN;


	while (1) {

		client->ret = poll(client->fds, 1, timeout_msecs);


		if (client->ret > 0) {

				if (client->fds[0].revents & POLLIN) {
				       		
					
				client->bytes += recv(client->fds[0].fd, client->recbuffer + client->bytes, ((sizeof client->recbuffer) - client->bytes), 0);
				printf("Krad IPC Server: got %d bytes\n", client->bytes);
					
					while (strchr(client->recbuffer, '|') != NULL) {
										
						//printf("Krad IPC Server: found delmitmer\n");
										
						client->msglen = strcspn(client->recbuffer, "|");
						strncpy(client->buffer, client->recbuffer, client->msglen);
						if (client->msglen == (client->bytes - 1)) {
							//printf("Krad IPC Server: one msg buf, nuked\n");
							client->bytes = 0;
							client->recbuffer[client->bytes] = '\0';
						} else {
							//printf("Krad IPC Server: multi .. memmoved\n");
							memmove(client->recbuffer, client->recbuffer + client->msglen + 1, client->bytes - (client->msglen + 1));
							client->bytes = client->bytes - (client->msglen + 1);
						}

					
						client->buffer[client->msglen] = '\0';
					
						if (client->krad_ipc_server->handler_callback != NULL) {
							client->broadcast = client->krad_ipc_server->handler_callback(client->buffer, client->client_pointer);
						}
						
						// we should be adding 1 to this for the null value
						// but we add it in in the client libs, so whatever
						client->rbytes = (strlen(client->buffer));
						printf("Krad IPC Server: broadcast was %d bytes returned is %d\n", client->broadcast, client->rbytes);
						// returned bytes
						if (client->rbytes > 0) {

							// 0 = no broadcast, 1 = broadcast to everyone but me, 2 = broadcast to everyone including back to me
							if (client->broadcast) {
								
								if (client->broadcast == 1) {
									krad_ipc_server_client_broadcast_skip(client->krad_ipc_server, client->buffer, client->rbytes, 1, client);
								} else {
									krad_ipc_server_client_broadcast(client->krad_ipc_server, client->buffer, client->rbytes, 1);
								}
								client->broadcast = 0;
							} else {
								printf("Krad IPC Server: sending %s\n", client->buffer);
								pthread_rwlock_rdlock (&client->krad_ipc_server->send_lock);								
								send (client->fd, client->buffer, client->rbytes, 0);
								pthread_rwlock_unlock (&client->krad_ipc_server->send_lock);
							}
						}
					}
					
				}
    
				if (client->fds[0].revents & POLLOUT) {
					//printf("I could write\n");
					//sleep(1);
				}

				if (client->fds[0].revents & POLLHUP) {
					printf("Krad IPC Server: Client hung up!\n");

					if (client->krad_ipc_server->close_client_callback != NULL) {
						client->krad_ipc_server->close_client_callback(client->client_pointer);
					}
					close(client->fd);
					client->bytes = 0;
					client->active = 2;
					if (client->broadcast_level > 0) {
						client->krad_ipc_server->broadcast_clients_level[client->broadcast_level]--;
						client->krad_ipc_server->broadcast_clients--;
					}
					client->broadcast_level = 0;
					client->krad_ipc_server->clients--;
					memset(client->buffer, 0, sizeof(client->buffer));
					memset(client->recbuffer, 0, sizeof(client->recbuffer));
					client->active = 0;
					return NULL;
				}
		}
	}
	
}

void krad_ipc_server_client_broadcast (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level) {

	krad_ipc_server_client_broadcast_skip(krad_ipc_server, data, size, broadcast_level, NULL);

}

void krad_ipc_server_client_broadcast_skip (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level, krad_ipc_server_client_t *client) {

	pthread_rwlock_wrlock (&krad_ipc_server->send_lock);								
	//printf("Krad IPC Server: broadcasting start\n");

	for(krad_ipc_server->c = 0; krad_ipc_server->c < CLIENT_SLOTS; krad_ipc_server->c++) {
		if ((krad_ipc_server->client[krad_ipc_server->c].active == 1) && (krad_ipc_server->client[krad_ipc_server->c].broadcast_level >= broadcast_level) && (&krad_ipc_server->client[krad_ipc_server->c] != client)) {
			send (krad_ipc_server->client[krad_ipc_server->c].fd, data, size, 0);
		}
	}
	
	//printf("Krad IPC Server: broadcasting end\n");
	pthread_rwlock_unlock (&krad_ipc_server->send_lock);

	//printf("Krad IPC Server: broadcasting: %s\n", data);

}


void krad_ipc_server_set_client_broadcasts(krad_ipc_server_t *krad_ipc_server, void *client_pointer, int broadcast_level) {

	// locking here is just becasue we are cheaply using that same c counter
	pthread_rwlock_wrlock (&krad_ipc_server->send_lock);								

	for(krad_ipc_server->c = 0; krad_ipc_server->c < CLIENT_SLOTS; krad_ipc_server->c++) {
		if ((krad_ipc_server->client[krad_ipc_server->c].active == 1) && (krad_ipc_server->client[krad_ipc_server->c].client_pointer == client_pointer)) {

			//reset old broadcast level
			if (krad_ipc_server->client[krad_ipc_server->c].broadcast_level > 0) {
				krad_ipc_server->broadcast_clients_level[krad_ipc_server->client[krad_ipc_server->c].broadcast_level]--;
				krad_ipc_server->broadcast_clients--;
			}

			// set new
			krad_ipc_server->client[krad_ipc_server->c].broadcast_level = broadcast_level;
		}
	}
	krad_ipc_server->broadcast_clients++;
	krad_ipc_server->broadcast_clients_level[broadcast_level]++;
	
	pthread_rwlock_unlock (&krad_ipc_server->send_lock);								

}

/*
void krad_ipc_server_client_send (void *client, char *data) {

	krad_ipc_client_t *kclient = (krad_ipc_client_t *)client;

	send (kclient->fd, data, strlen(data), 0);

}
*/
int krad_ipc_server_run(krad_ipc_server_t *krad_ipc_server) {

	krad_ipc_server_client_t *client;
	fd_set set;

	while (1) {

		client = NULL;
		FD_ZERO (&set);
		FD_SET (krad_ipc_server->fd, &set);

		printf("Waiting for clients..\n");
		pselect (krad_ipc_server->fd+1, &set, NULL, NULL, NULL, &krad_ipc_server->orig_mask);

		if (shutdown_krad_ipc_server) {
			krad_ipc_server_shutdown(krad_ipc_server);			
			return 0;
		}


		client = krad_ipc_server_accept_client (krad_ipc_server);
		if (client == 0)
			return 0;

		printf("Client connected...\n");
		pthread_create(&client->serve_client_thread, NULL, krad_ipc_server_client_loop, (void *)client);
		pthread_detach(client->serve_client_thread);
	}

}

void randomsleep(times) {

	struct timespec start;
	int count;
	
	for ( count = 0; count < times; count++ ) {
		clock_gettime( CLOCK_REALTIME, &start);
		int number = start.tv_nsec;
		srand (number);
		number = (rand () % 255033 );
		printf("Randomly sleeping for %d usecs\n", number);
		usleep (number);
	}
}


void krad_ipc_server_shutdown(krad_ipc_server_t *krad_ipc_server) {

	if (krad_ipc_server->shutdown_callback != NULL) {
		randomsleep(2);
		printf("Running shutdown callback..\n");
		krad_ipc_server->shutdown_callback(krad_ipc_server->client_callback_pointer);
		printf("Shutdown callback complete\n");
	}

	for(krad_ipc_server->i = 0; krad_ipc_server->i < CLIENT_SLOTS; krad_ipc_server->i++) {
		if (krad_ipc_server->client[krad_ipc_server->i].active == 1) {
			close(krad_ipc_server->client[krad_ipc_server->i].fd);
		}
	}

	if (krad_ipc_server->fd != 0) {
		close(krad_ipc_server->fd);
		if(!krad_ipc_server->linux_abstract_sockets) {
			unlink (krad_ipc_server->saddr.sun_path);
		}
	}
	
	krad_ipc_server_free(krad_ipc_server);
	
	exit(0);
}

void krad_ipc_server_free(krad_ipc_server_t *krad_ipc_server) {
	free(krad_ipc_server);
	free(krad_ipc_server->client);
}	
	

void krad_ipc_server_want_shutdown() {

	printf("Krad IPC Server Got shutdown signal..\n");
	shutdown_krad_ipc_server = 1;
	
}


void krad_ipc_server_set_new_client_callback(krad_ipc_server_t *krad_ipc_server, void *new_client_callback(void *)) {

	krad_ipc_server->new_client_callback = new_client_callback;


}

void krad_ipc_server_set_close_client_callback(krad_ipc_server_t *krad_ipc_server, void close_client_callback(void *)) {

	krad_ipc_server->close_client_callback = close_client_callback;

}


void krad_ipc_server_set_shutdown_callback(krad_ipc_server_t *krad_ipc_server, void shutdown_callback(void *)) {

	krad_ipc_server->shutdown_callback = shutdown_callback;
	
}


void krad_ipc_server_set_client_callback_pointer(krad_ipc_server_t *krad_ipc_server, void *pointer) {

	krad_ipc_server->client_callback_pointer = pointer;

}


void krad_ipc_server_set_handler_callback(krad_ipc_server_t *krad_ipc_server, int handler_callback(char *, void *)) {

	krad_ipc_server->handler_callback = handler_callback;

}

 
void daemonize(char *daemon_name) {

		/* Our process ID and Session ID */
		pid_t pid, sid;

        printf("Daemonizing..\n");
 
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
 
        /* Change the file mode mask */
        umask(0);
 
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }
 
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }
 
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        //close(STDOUT_FILENO);
        //close(STDERR_FILENO);
        
        
		char err_log_file[98] = "";
		char log_file[98] = "";
		char *homedir = getenv ("HOME");
		
		char timestamp[64];
		
		time_t ltime;
    	ltime=time(NULL);
		sprintf(timestamp, "%s",asctime( localtime(&ltime) ) );
		
		int i;
		
		for (i = 0; i< strlen(timestamp); i++) {
			if (timestamp[i] == ' ') {
				timestamp[i] = '_';
			}
			if (timestamp[i] == ':') {
				timestamp[i] = '.';
			}
			if (timestamp[i] == '\n') {
				timestamp[i] = '\0';
			}
		
		}
		
		sprintf(err_log_file, "%s/krad/log/%s_%s_err.log", homedir, daemon_name, timestamp);
		sprintf(log_file, "%s/krad/log/%s_%s.log", homedir, daemon_name, timestamp);
		
		FILE *fp;
		
		fp = freopen( err_log_file, "w", stderr );
		if (fp == NULL) {
			printf("ruh oh error in freopen stderr\n");
		}
		fp = freopen( log_file, "w", stdout );
		if (fp == NULL) {
			printf("ruh oh error in freopen stdout\n");
		}
        
}

void krad_ipc_server_signals_setup(krad_ipc_server_t *krad_ipc_server) {

	memset (&krad_ipc_server->act, 0, sizeof(krad_ipc_server->act));
	krad_ipc_server->act.sa_handler = krad_ipc_server_want_shutdown;
	
	/* This server should shut down on SIGTERM. */
	if (sigaction(SIGTERM, &krad_ipc_server->act, 0)) {
		printf("err sigterm\n");
		exit(1);
	}
	/* This server should shut down on SIGINT. */
	if (sigaction(SIGINT, &krad_ipc_server->act, 0)) {
		printf("err sigint\n");
		exit(1);
	}
 
	sigemptyset (&krad_ipc_server->mask);
 	sigaddset (&krad_ipc_server->mask, SIGINT);
 	
	if (sigprocmask(SIG_BLOCK, &krad_ipc_server->mask, &krad_ipc_server->orig_mask) < 0) {
		printf("err sigprocmask\n");
		exit(1);
	}

}
