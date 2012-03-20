#include "krad_ipc_server.h"


krad_ipc_server_t *krad_ipc_server_create (char *callsign_or_ipc_path_or_port) {

	krad_ipc_server_t *krad_ipc_server = calloc (1, sizeof (krad_ipc_server_t));
	int i;

	if (krad_ipc_server == NULL) {
		return NULL;
	}
	
	krad_ipc_server->shutdown = KRAD_IPC_STARTING;
	
	if ((krad_ipc_server->clients = calloc (KRAD_IPC_SERVER_MAX_CLIENTS, sizeof (krad_ipc_server_client_t))) == NULL) {
		krad_ipc_server_destroy (krad_ipc_server);
	}
	
	for(i = 0; i < KRAD_IPC_SERVER_MAX_CLIENTS; i++) {
		krad_ipc_server->clients[i].krad_ipc_server = krad_ipc_server;
	}
	
	uname (&krad_ipc_server->unixname);
	if (strncmp(krad_ipc_server->unixname.sysname, "Linux", 5) == 0) {
		krad_ipc_server->on_linux = 1;
	}
		
	krad_ipc_server->sd = socket (AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

	if (krad_ipc_server->sd == -1) {
		printf ("Krad IPC Server: Socket failed.\n");
		krad_ipc_server_destroy (krad_ipc_server);
		return NULL;
	}

	krad_ipc_server->saddr.sun_family = AF_UNIX;

	if (krad_ipc_server->on_linux) {
		snprintf (krad_ipc_server->saddr.sun_path, sizeof (krad_ipc_server->saddr.sun_path), "@%s_ipc", callsign_or_ipc_path_or_port);	
		krad_ipc_server->saddr.sun_path[0] = '\0';
		if (connect (krad_ipc_server->sd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) != -1) {
			/* active socket already exists! */
			printf("Krad IPC Server: Krad IPC path in use. (linux abstract)\n");
			krad_ipc_server_destroy (krad_ipc_server);
			return NULL;
		}
	} else {
		snprintf (krad_ipc_server->saddr.sun_path, sizeof (krad_ipc_server->saddr.sun_path), "%s/%s_ipc", getenv ("HOME"), callsign_or_ipc_path_or_port);	
		if (access (krad_ipc_server->saddr.sun_path, F_OK) == 0) {
			if (connect (krad_ipc_server->sd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) != -1) {
				/* active socket already exists! */
				printf("Krad IPC Server: Krad IPC path in use.\n");
				krad_ipc_server_destroy (krad_ipc_server);
				return NULL;
			}
			/* remove stale socket */
			unlink (krad_ipc_server->saddr.sun_path);
		}
	}
	
	if (bind (krad_ipc_server->sd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) == -1) {
		printf("Krad IPC Server: Can't bind.\n");
		krad_ipc_server_destroy (krad_ipc_server);
		return NULL;
	}

	listen (krad_ipc_server->sd, 5);

	krad_ipc_server->flags = fcntl (krad_ipc_server->sd, F_GETFL, 0);

	if (krad_ipc_server->flags == -1) {
		krad_ipc_server_destroy (krad_ipc_server);
		return NULL;
	}

	krad_ipc_server->flags |= O_NONBLOCK;

	krad_ipc_server->flags = fcntl (krad_ipc_server->sd, F_SETFL, krad_ipc_server->flags);
	if (krad_ipc_server->flags == -1) {
		krad_ipc_server_destroy (krad_ipc_server);
		return NULL;
	}
	
	return krad_ipc_server;

}



krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server) {

	krad_ipc_server_client_t *client = NULL;
	
	int i;
	struct sockaddr_un sin;
	socklen_t sin_len;
	int flags;
		
	while (client == NULL) {
		for(i = 0; i < KRAD_IPC_SERVER_MAX_CLIENTS; i++) {
			if (krad_ipc_server->clients[i].active == 0) {
				client = &krad_ipc_server->clients[i];
				break;
			}
		}
		if (client == NULL) {
			//printf("Krad IPC Server: Overloaded with clients!\n");
			return NULL;
		}
	}

	sin_len = sizeof (sin);
	client->sd = accept (krad_ipc_server->sd, (struct sockaddr *)&sin, &sin_len);

	if (client->sd >= 0) {

		flags = fcntl (client->sd, F_GETFL, 0);

		if (flags == -1) {
			close (client->sd);
			return NULL;
		}

		flags |= O_NONBLOCK;

		flags = fcntl (client->sd, F_SETFL, flags);
		if (flags == -1) {
			close (client->sd);
			return NULL;
		}

		client->active = 1;
		krad_ipc_server_update_pollfds (client->krad_ipc_server);
		printf("Krad IPC Server: Client accepted!\n");	
		return client;
	}

	return NULL;
}

void krad_ipc_disconnect_client (krad_ipc_server_client_t *client) {

	close (client->sd);
	client->input_buffer_pos = 0;
	client->output_buffer_pos = 0;
	memset (client->input_buffer, 0, sizeof(client->input_buffer));
	memset (client->output_buffer, 0, sizeof(client->output_buffer));
	client->active = 0;
	krad_ipc_server_update_pollfds (client->krad_ipc_server);
	printf("Krad IPC Server: Client hung up!\n");
	
}

/*
void krad_ipc_server_client_broadcast (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level) {

	krad_ipc_server_client_broadcast_skip(krad_ipc_server, data, size, broadcast_level, NULL);

}

void krad_ipc_server_client_broadcast_skip (krad_ipc_server_t *krad_ipc_server, char *data, int size, int broadcast_level, krad_ipc_server_client_t *client) {

	pthread_rwlock_wrlock (&krad_ipc_server->send_lock);								
	//printf("Krad IPC Server: broadcasting start\n");

	for(krad_ipc_server->c = 0; krad_ipc_server->c < KRAD_IPC_SERVER_MAX_CLIENTS; krad_ipc_server->c++) {
		if ((krad_ipc_server->client[krad_ipc_server->c].active == 1) && (krad_ipc_server->client[krad_ipc_server->c].broadcast_level >= broadcast_level) && (&krad_ipc_server->client[krad_ipc_server->c] != client)) {
			send (krad_ipc_server->client[krad_ipc_server->c].sd, data, size, 0);
		}
	}
	
	//printf("Krad IPC Server: broadcasting end\n");
	pthread_rwlock_unlock (&krad_ipc_server->send_lock);

	//printf("Krad IPC Server: broadcasting: %s\n", data);

}


void krad_ipc_server_set_client_broadcasts(krad_ipc_server_t *krad_ipc_server, void *client_pointer, int broadcast_level) {

	// locking here is just becasue we are cheaply using that same c counter
	pthread_rwlock_wrlock (&krad_ipc_server->send_lock);								

	for (krad_ipc_server->c = 0; krad_ipc_server->c < KRAD_IPC_SERVER_MAX_CLIENTS; krad_ipc_server->c++) {
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

	send (kclient->sd, data, strlen(data), 0);

}
*/

void krad_ipc_server_update_pollfds (krad_ipc_server_t *krad_ipc_server) {

	int c;
	int s;

	s = 0;

	krad_ipc_server->sockets[s].fd = krad_ipc_server->sd;
	krad_ipc_server->sockets[s].events = POLLIN;

	s++;

	for (c = 0; c < KRAD_IPC_SERVER_MAX_CLIENTS; c++) {
		if (krad_ipc_server->clients[c].active == 1) {
			krad_ipc_server->sockets[s].fd = krad_ipc_server->clients[c].sd;
			krad_ipc_server->sockets[s].events |= POLLIN;			
			krad_ipc_server->sockets_clients[s] = &krad_ipc_server->clients[c];
			s++;
		}
	}
	
	krad_ipc_server->socket_count = s;

	printf("Krad IPC Server: sockets rejiggerd!\n");	

}


void *krad_ipc_server_run (void *arg) {

	krad_ipc_server_t *krad_ipc_server = (krad_ipc_server_t *)arg;
	krad_ipc_server_client_t *client;
	int ret, s;
	
	krad_ipc_server->shutdown = KRAD_IPC_RUNNING;
	
	krad_ipc_server_update_pollfds (krad_ipc_server);

	while (!krad_ipc_server->shutdown) {

		ret = poll(krad_ipc_server->sockets, krad_ipc_server->socket_count, KRAD_IPC_SERVER_TIMEOUT_MS);

		if (ret > 0) {
		
			if (krad_ipc_server->shutdown) {
				break;
			}
	
			if (krad_ipc_server->sockets[0].revents & POLLIN) {
				krad_ipc_server_accept_client (krad_ipc_server);
			}
	
			for (s = 1; s <= ret; s++) {

				if (krad_ipc_server->sockets[s].revents) {
				
					client = krad_ipc_server->sockets_clients[s];
				
					if (krad_ipc_server->sockets[s].revents & POLLIN) {
	
						client->input_buffer_pos += recv(krad_ipc_server->sockets[s].fd, client->input_buffer + client->input_buffer_pos, (sizeof (client->input_buffer) - client->input_buffer_pos), 0);
					
						// big enough to read element id and data size
						if (client->input_buffer_pos > 0) {
							sprintf(client->output_buffer, "hi you sent me %d bytes", client->input_buffer_pos);
							//send (client->sd, client->output_buffer, strlen (client->output_buffer) + 1, 0);
							send (krad_ipc_server->sockets[s].fd, client->output_buffer, strlen (client->output_buffer) + 1, 0);
							client->input_buffer_pos = 0;
						}

						//printf("Krad IPC Server: got %d bytes\n", client->bytes);
	/*	
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

									send (client->sd, client->buffer, client->rbytes, 0);

								}
							}
						}
	*/	
					}

					if (krad_ipc_server->sockets[s].revents & POLLOUT) {
						//printf("I could write\n");
					}

					if ((krad_ipc_server->sockets[s].revents & POLLHUP) || (krad_ipc_server->sockets[s].revents & POLLERR)) {
						krad_ipc_disconnect_client (client);
					}
				}
			}
		}
	}

	krad_ipc_server->shutdown = KRAD_IPC_SHUTINGDOWN;

	return NULL;

}


void krad_ipc_server_destroy (krad_ipc_server_t *krad_ipc_server) {

	int c;
	int patience;
	
	patience = KRAD_IPC_SERVER_TIMEOUT_US * 3;
	
	if (krad_ipc_server->shutdown == KRAD_IPC_RUNNING) {
		krad_ipc_server->shutdown = KRAD_IPC_DO_SHUTDOWN;
	
		while ((krad_ipc_server->shutdown != KRAD_IPC_SHUTINGDOWN) && (patience > 0)) {
			usleep (KRAD_IPC_SERVER_TIMEOUT_US / 4);
			patience -= KRAD_IPC_SERVER_TIMEOUT_US / 4;
		}
	}

	if (krad_ipc_server->sd != 0) {
		close (krad_ipc_server->sd);
		if (!krad_ipc_server->on_linux) {
			unlink (krad_ipc_server->saddr.sun_path);
		}
	}

	for (c = 0; c < KRAD_IPC_SERVER_MAX_CLIENTS; c++) {
		if (krad_ipc_server->clients[c].active == 1) {
			krad_ipc_disconnect_client (&krad_ipc_server->clients[c]);
		}
	}
	
	
	free (krad_ipc_server->clients);
	free (krad_ipc_server);
	
}


krad_ipc_server_t *krad_ipc_server (char *callsign_or_ipc_path_or_port, int handler (void *, void *, int *, void *), void *ptr) {


	krad_ipc_server_t *krad_ipc_server;
	
	krad_ipc_server = krad_ipc_server_create (callsign_or_ipc_path_or_port);

	if (krad_ipc_server == NULL) {
		return NULL;
	}

	pthread_create (&krad_ipc_server->server_thread, NULL, krad_ipc_server_run, (void *)krad_ipc_server);
	pthread_detach (krad_ipc_server->server_thread);

	return krad_ipc_server;	

}

