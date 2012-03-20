#include "krad_ipc_server.h"


krad_ipc_server_t *krad_ipc_server_create (char *callsign_or_ipc_path_or_port) {

	krad_ipc_server_t *krad_ipc_server = calloc (1, sizeof (krad_ipc_server_t));

	if (krad_ipc_server == NULL) {
		return NULL;
	} 
	
	if ((krad_ipc_server->client = calloc (KRAD_IPC_MAX_CLIENTS, sizeof (krad_ipc_server_client_t))) == NULL) {
		krad_ipc_server_destroy (krad_ipc_server);
	}
	
	uname (&krad_ipc_server->unixname);
	
	if (strncmp(krad_ipc_server->unixname.sysname, "Linux", 5) == 0) {
		krad_ipc_server->linux_abstract_sockets = 1;
		sprintf(krad_ipc_server->ipc_path, "@%s_ipc", callsign_or_ipc_path_or_port);
	} else {
		sprintf(krad_ipc_server->ipc_path, "%s/%s_ipc", getenv ("HOME"), callsign_or_ipc_path_or_port);
	}
	
	krad_ipc_server->fd = socket (AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

	if (krad_ipc_server->fd == -1) {
		printf ("Krad IPC Server: Socket failed.\n");
		krad_ipc_server_destroy (krad_ipc_server);
		return NULL;
	}

	krad_ipc_server->saddr.sun_family = AF_UNIX;

	snprintf (krad_ipc_server->saddr.sun_path, sizeof (krad_ipc_server->saddr.sun_path), "%s", krad_ipc_server->ipc_path);

	if (krad_ipc_server->linux_abstract_sockets) {
		krad_ipc_server->saddr.sun_path[0] = '\0';
		if (connect (krad_ipc_server->fd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) != -1) {
			/* active socket already exists! */
			printf("Krad IPC Server: Krad IPC path in use. (linux abstract)\n");
			krad_ipc_server_destroy (krad_ipc_server);
			return NULL;
		}
	} else {
		if (access (krad_ipc_server->saddr.sun_path, F_OK) == 0) {
			if (connect (krad_ipc_server->fd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) != -1) {
				/* active socket already exists! */
				printf("Krad IPC Server: Krad IPC path in use.\n");
				krad_ipc_server_destroy (krad_ipc_server);
				return NULL;
			}
			/* remove stale socket */
			unlink (krad_ipc_server->saddr.sun_path);
		}
	}
	
	if (bind (krad_ipc_server->fd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) == -1) {
		printf("Krad IPC Server: Can't bind.\n");
		krad_ipc_server_destroy (krad_ipc_server);
		return NULL;
	}

	listen (krad_ipc_server->fd, 5);

	krad_ipc_server->flags = fcntl (krad_ipc_server->fd, F_GETFL, 0);

	if (krad_ipc_server->flags == -1) {
		krad_ipc_server_destroy (krad_ipc_server);
		return NULL;
	}

	/*
	krad_ipc_server->flags |= O_NONBLOCK;

	krad_ipc_server->flags = fcntl (krad_ipc_server->fd, F_SETFL, krad_ipc_server->flags);
	if (krad_ipc_server->flags == -1) {
		krad_ipc_server_destroy (krad_ipc_server);
		return NULL;
	}
	*/
	
	pthread_rwlock_init(&krad_ipc_server->send_lock, NULL);
	
	return krad_ipc_server;

}



krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server) {

	krad_ipc_server_client_t *client = NULL;
	
	int i;
	struct sockaddr_un sin;
	socklen_t sin_len;
	int flags;
		
	while (client == NULL) {
		for(i = 0; i < KRAD_IPC_MAX_CLIENTS; i++) {
			if (krad_ipc_server->client[i].active == 0) {
				client = &krad_ipc_server->client[i];
				break;
			}
		}
		if (client == NULL) {
			//printf("Krad IPC Server: Overloaded with clients!\n");
			usleep (25000);
		}
	}
	
	client->krad_ipc_server = krad_ipc_server;

	sin_len = sizeof (sin);
	client->fd = accept (krad_ipc_server->fd, (struct sockaddr *)&sin, &sin_len);
	//client->fd = accept4 (krad_ipc_server->fd, (struct sockaddr *)&sin, &sin_len, SOCK_CLOEXEC);
	if (client->fd >= 0) {

		flags = fcntl (client->fd, F_GETFL, 0);

		if (flags == -1) {
			close (client->fd);
			return 0;
		}
		/*
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
		*/
		
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
					
						if (client->krad_ipc_server->handler != NULL) {
							//client->broadcast = client->krad_ipc_server->handler(client->buffer, client->client_pointer);
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

	for(krad_ipc_server->c = 0; krad_ipc_server->c < KRAD_IPC_MAX_CLIENTS; krad_ipc_server->c++) {
		if ((krad_ipc_server->client[krad_ipc_server->c].active == 1) && (krad_ipc_server->client[krad_ipc_server->c].broadcast_level >= broadcast_level) && (&krad_ipc_server->client[krad_ipc_server->c] != client)) {
			send (krad_ipc_server->client[krad_ipc_server->c].fd, data, size, 0);
		}
	}
	
	//printf("Krad IPC Server: broadcasting end\n");
	pthread_rwlock_unlock (&krad_ipc_server->send_lock);

	//printf("Krad IPC Server: broadcasting: %s\n", data);

}

/*
void krad_ipc_server_set_client_broadcasts(krad_ipc_server_t *krad_ipc_server, void *client_pointer, int broadcast_level) {

	// locking here is just becasue we are cheaply using that same c counter
	pthread_rwlock_wrlock (&krad_ipc_server->send_lock);								

	for (krad_ipc_server->c = 0; krad_ipc_server->c < KRAD_IPC_MAX_CLIENTS; krad_ipc_server->c++) {
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


void krad_ipc_server_client_send (void *client, char *data) {

	krad_ipc_client_t *kclient = (krad_ipc_client_t *)client;

	send (kclient->fd, data, strlen(data), 0);

}
*/

void *krad_ipc_server_run (void *arg) {

	krad_ipc_server_t *krad_ipc_server = (krad_ipc_server_t *)arg;

	krad_ipc_server_client_t *client;
	fd_set set;

	while (1) {

		client = NULL;
		FD_ZERO (&set);
		FD_SET (krad_ipc_server->fd, &set);

		select (krad_ipc_server->fd+1, &set, NULL, NULL, NULL);

		client = krad_ipc_server_accept_client (krad_ipc_server);
		if (client == 0) {
			return NULL;
		}
		
		pthread_create (&client->serve_client_thread, NULL, krad_ipc_server_client_loop, (void *)client);
		pthread_detach (client->serve_client_thread);
	}

	return NULL;

}


void krad_ipc_server_destroy (krad_ipc_server_t *krad_ipc_server) {

	int c;

	for (c = 0; c < KRAD_IPC_MAX_CLIENTS; c++) {
		if (krad_ipc_server->client[c].active == 1) {
			close (krad_ipc_server->client[c].fd);
		}
	}

	if (krad_ipc_server->fd != 0) {
		close (krad_ipc_server->fd);
		if (!krad_ipc_server->linux_abstract_sockets) {
			unlink (krad_ipc_server->saddr.sun_path);
		}
	}
	
	free(krad_ipc_server);
	
}


krad_ipc_server_t *krad_ipc_server (char *callsign_or_ipc_path_or_port, int handler (void *, void *, int *, void *), void *ptr) {


	krad_ipc_server_t *krad_ipc_server;
	
	krad_ipc_server = krad_ipc_server_create (callsign_or_ipc_path_or_port);

	if (krad_ipc_server == NULL) {
		return NULL;
	}

	printf ("IPC Server worked...\n");

	pthread_create (&krad_ipc_server->server_thread, NULL, krad_ipc_server_run, (void *)krad_ipc_server);
	pthread_detach (krad_ipc_server->server_thread);

	return krad_ipc_server;	

}

