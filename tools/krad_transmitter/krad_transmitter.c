#include "krad_transmitter.h"

void set_socket_nonblocking (int sd) {

	int ret;
	int flags;
	
	flags = 0;
	ret = 0;

	flags = fcntl (sd, F_GETFL, 0);
	if (flags == -1) {
		failfast ("Krad Transmitter: fcntl on incoming connections sd F_GETFL");
	}

	flags |= O_NONBLOCK;
	
	ret = fcntl (sd, F_SETFL, flags);
	if (ret == -1) {
		failfast ("Krad Transmitter: fcntl on incoming connections sd F_SETFL");
	}
}

krad_transmission_receiver_t *krad_transmitter_receiver_create (krad_transmitter_t *krad_transmitter, int fd) {

	int r;

	r = 0;

	while (r < TOTAL_RECEIVERS) {
		if (krad_transmitter->krad_transmission_receivers[r].active == 0) {
			krad_transmitter->krad_transmission_receivers[r].active = 1;
			krad_transmitter->krad_transmission_receivers[r].bufpos = 0;
			krad_transmitter->krad_transmission_receivers[r].fd = fd;
			krad_transmitter->krad_transmission_receivers[r].event.data.ptr = &krad_transmitter->krad_transmission_receivers[r];
			krad_transmitter->krad_transmission_receivers[r].event.events = EPOLLIN | EPOLLET;
			return &krad_transmitter->krad_transmission_receivers[r];
		}
	}

	return NULL;

}

void krad_transmitter_receiver_destroy (krad_transmission_receiver_t *krad_transmission_receiver) {

	krad_transmission_receiver->active = 2;
	if (krad_transmission_receiver->fd != 0) {
		close (krad_transmission_receiver->fd);
		krad_transmission_receiver->fd = 0;
	}
	krad_transmission_receiver->bufpos = 0;
	memset (krad_transmission_receiver->buffer, 0, sizeof(krad_transmission_receiver->buffer));
	krad_transmission_receiver->active = 0;

}


void krad_transmitter_handle_incoming_connection (krad_transmitter_t *krad_transmitter, krad_transmission_receiver_t *krad_transmission_receiver) {

	printk ("Krad Transmitter: incoming_connection buffer at %d bytes", krad_transmission_receiver->bufpos);

}


void *krad_transmitter_listening_thread (void *arg) {

	krad_transmitter_t *krad_transmitter = (krad_transmitter_t *)arg;

	krad_transmission_receiver_t *krad_transmission_receiver;

	int e;
	int ret;
	int eret;
	int cret;
	int addr_size;
	int client_fd;
	struct sockaddr_in remote_address;
	
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXSERV];
	
	printk ("Krad Transmitter: Listening thread starting");
	
	addr_size = 0;
	e = 0;
	ret = 0;
	eret = 0;
	cret = 0;
	krad_transmission_receiver = NULL;
	memset (&remote_address, 0, sizeof(remote_address));	

	addr_size = sizeof (remote_address);
	
	while (krad_transmitter->stop_listening == 0) {

		ret = epoll_wait (krad_transmitter->incoming_connections_efd, krad_transmitter->incoming_connection_events, KRAD_TRANSMITTER_MAXEVENTS, 1000);

		if (ret < 0) {
			printke ("Krad Transmitter: Failed on epoll wait");
			krad_transmitter->stop_listening = 1;
			break;
		}
	
		if (ret > 0) {
		
			for (e = 0; e < ret; e++) {
	
				if ((krad_transmitter->incoming_connection_events[e].events & EPOLLERR) ||
				    (krad_transmitter->incoming_connection_events[e].events & EPOLLHUP))
				{

					if (krad_transmitter->incoming_connections_sd == krad_transmitter->incoming_connection_events[e].data.fd) {
						failfast ("Krad Transmitter: error on listen socket");
					} else {
					
						if (krad_transmitter->incoming_connection_events[e].events & EPOLLHUP) {
							printke ("Krad Transmitter: incoming transmitter connection hangup");
						}
						if (krad_transmitter->incoming_connection_events[e].events & EPOLLERR) {
							printke ("Krad Transmitter: incoming transmitter connection error");
						}
						krad_transmitter_receiver_destroy (krad_transmitter->incoming_connection_events[e].data.ptr);
						continue;
					}

				}
				
				if (krad_transmitter->incoming_connections_sd == krad_transmitter->incoming_connection_events[e].data.fd) {

					while (1) {

						client_fd = accept (krad_transmitter->incoming_connections_sd, (struct sockaddr *)&remote_address, (socklen_t *)&addr_size);
						if (client_fd == -1) {
							if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
								// We have processed all incoming connections. 
								break;
							} else {
								failfast ("Krad Transmitter: error on listen socket accept");
							}
						}
				
						if (getnameinfo ((struct sockaddr *)&remote_address, addr_size, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
							printk ("Krad Transmitter: Accepted transmitter connection on descriptor %d (host=%s, port=%s)", client_fd, hbuf, sbuf);
						} else {
							printke ("Krad Transmitter: Accepted transmitter connection on descriptor %d ... but could not getnameinfo()?", client_fd, hbuf, sbuf);
						}

						set_socket_nonblocking (client_fd);

						krad_transmission_receiver = krad_transmitter_receiver_create (krad_transmitter, client_fd);

						if (krad_transmission_receiver == NULL) {
							failfast ("Krad Transmitter: ran out of connections!");
						}

						eret = epoll_ctl (krad_transmitter->incoming_connections_efd, EPOLL_CTL_ADD, client_fd, &krad_transmission_receiver->event);
						if (eret != 0) {
							failfast ("Krad Transmitter: incoming transmitter connection epoll error eret is %d errno is %i", eret, errno);
						}
					}
		
					continue;
			
				}
				
				if (krad_transmitter->incoming_connection_events[e].events & EPOLLIN) {

					while (1) {
						krad_transmission_receiver = (krad_transmission_receiver_t *)krad_transmitter->incoming_connection_events[e].data.ptr;

						cret = read (krad_transmission_receiver->fd,
									 krad_transmission_receiver->buffer + krad_transmission_receiver->bufpos,
									 sizeof (krad_transmission_receiver->buffer) -  krad_transmission_receiver->bufpos);
						if (cret == -1) {
							if (errno != EAGAIN) {
								printke ("Krad Transmitter: error reading from a new incoming connection socket");
								krad_transmitter_receiver_destroy (krad_transmitter->incoming_connection_events[e].data.ptr);
							}
							break;
						}

						if (cret == 0) {
							printk ("Krad Transmitter: Client EOF Closed connection");
							krad_transmitter_receiver_destroy (krad_transmitter->incoming_connection_events[e].data.ptr);
							break;
						}

						if (cret > 0) {
							krad_transmission_receiver->bufpos += cret;
							krad_transmitter_handle_incoming_connection (krad_transmitter, krad_transmission_receiver);
						}
					}
				}
			}
		}

		
		if (ret == 0) {
			printk ("Krad Transmitter: Listening thread... nothing happened");
		}
		
	}

	close (krad_transmitter->incoming_connections_efd);
	close (krad_transmitter->incoming_connections_sd);
	free (krad_transmitter->incoming_connection_events);
	free (krad_transmitter->krad_transmission_receivers);
	
	krad_transmitter->port = 0;
	krad_transmitter->listening = 0;	

	printk ("Krad Transmitter: Listening thread exiting");
	
	return NULL;

}

void krad_transmitter_stop_listening (krad_transmitter_t *krad_transmitter) {

	if (krad_transmitter->listening == 1) {
		krad_transmitter->stop_listening = 1;
		pthread_join (krad_transmitter->listening_thread, NULL);
		krad_transmitter->stop_listening = 0;
	}

}

int krad_transmitter_listen_on (krad_transmitter_t *krad_transmitter, int port) {

	int ret;
	int r;
	
	r = 0;
	ret = 0;
	
	if (krad_transmitter->listening == 1) {
		krad_transmitter_stop_listening (krad_transmitter);
	}

	krad_transmitter->port = port;
	krad_transmitter->listening = 1;
	
	krad_transmitter->local_address.sin_family = AF_INET;
	krad_transmitter->local_address.sin_port = htons (krad_transmitter->port);
	krad_transmitter->local_address.sin_addr.s_addr = htonl (INADDR_ANY);
	
	if ((krad_transmitter->incoming_connections_sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Krad Transmitter: system call socket error\n");
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;		
		return 1;
	}

	if (bind (krad_transmitter->incoming_connections_sd, (struct sockaddr *)&krad_transmitter->local_address,
		sizeof(krad_transmitter->local_address)) == -1) {
		
		printke ("Krad Transmitter: bind error for tcp port %d\n", krad_transmitter->port);
		close (krad_transmitter->incoming_connections_sd);
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;
		return 1;
	}
	
	if (listen (krad_transmitter->incoming_connections_sd, SOMAXCONN) < 0) {
		printke ("Krad Transmitter: system call listen error\n");
		close (krad_transmitter->incoming_connections_sd);
		return 1;
	}

	set_socket_nonblocking (krad_transmitter->incoming_connections_sd);	

	krad_transmitter->incoming_connections_efd = epoll_create1 (0);
	
	if (krad_transmitter->incoming_connections_efd == -1) {
		printke ("Krad Transmitter: epoll_create");
		return 1;
	}

	krad_transmitter->event.data.fd = krad_transmitter->incoming_connections_sd;
	krad_transmitter->event.events = EPOLLIN | EPOLLET;
	
	ret = epoll_ctl (krad_transmitter->incoming_connections_efd, EPOLL_CTL_ADD, krad_transmitter->incoming_connections_sd, &krad_transmitter->event);
	if (ret == -1) {
		printke ("Krad Transmitter: epoll_ctl");
		return 1;
	}

	krad_transmitter->incoming_connection_events = calloc (KRAD_TRANSMITTER_MAXEVENTS, sizeof (struct epoll_event));

	if (krad_transmitter->incoming_connection_events == NULL) {
		failfast ("Krad Transmitter: Out of memory!");
	}

	printk ("Krad Transmitter: Listening on port %d", krad_transmitter->port);
	
	
	krad_transmitter->krad_transmission_receivers = calloc (TOTAL_RECEIVERS, sizeof (krad_transmission_receiver_t));
	
	if (krad_transmitter->krad_transmission_receivers == NULL) {
		failfast ("Krad Transmitter: Out of memory!");
	}
	
	for (r = 0; r < TOTAL_RECEIVERS; r++) {
		krad_transmitter->krad_transmission_receivers[r].krad_transmitter = krad_transmitter;
	}	
	
	pthread_create (&krad_transmitter->listening_thread, NULL, krad_transmitter_listening_thread, (void *)krad_transmitter);

	return 0;

}

krad_transmitter_t *krad_transmitter_create () {

	krad_transmitter_t *krad_transmitter = calloc (1, sizeof(krad_transmitter_t));
		
	if (krad_transmitter == NULL) {
		failfast ("Krad Transmitter: Out of memory!");
	}
	
	krad_transmitter->krad_transmissions = calloc (DEFAULT_MAX_TRANSMISSIONS, sizeof(krad_transmission_t));
	
	if (krad_transmitter->krad_transmissions == NULL) {
		failfast ("Krad Transmitter: Out of memory!");
	}	
	
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "HTTP/1.1 404 Not Found\r\n");
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "Status: 404 Not Found\r\n");
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "Connection: close\r\n");
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "Content-Type: text/html; charset=utf-8\r\n");
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "\r\n404 Not Found");

	return krad_transmitter;

}

void krad_transmitter_destroy (krad_transmitter_t *krad_transmitter) {

	if (krad_transmitter->listening == 1) {
		krad_transmitter_stop_listening (krad_transmitter);
	}
	
	free (krad_transmitter->krad_transmissions);
	free (krad_transmitter);
	
	printk ("Krad Transmitter: Destroyed!");

}

