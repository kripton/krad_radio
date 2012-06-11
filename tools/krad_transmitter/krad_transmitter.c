#include "krad_transmitter.h"



void *krad_transmitter_listening_thread (void *arg) {

	krad_transmitter_t *krad_transmitter = (krad_transmitter_t *)arg;

	int ret;
	int addr_size;
	int client_fd;
	struct sockaddr_in remote_address;
	
	printk ("Krad Transmitter: Listening thread starting");
	
	addr_size = 0;
	ret = 0;
	memset (&remote_address, 0, sizeof(remote_address));	

	addr_size = sizeof (remote_address);
	
	while (krad_transmitter->stop_listening == 0) {

		ret = 0;
		usleep (5000);

		if (ret < 0) {
			printke ("Krad Transmitter: Failed on poll");
			krad_transmitter->stop_listening = 1;
			break;
		}
	
		if (ret > 0) {
		
			if ((client_fd = accept(krad_transmitter->incoming_connections_sd, (struct sockaddr *)&remote_address, (socklen_t *)&addr_size)) < 0) {
				close (krad_transmitter->incoming_connections_sd);
				failfast ("Krad Transmitter: socket error on accept mayb a signal or such");
			}

			//krad_linker_listen_create_client (krad_linker, client_fd);

		}
	}

	close (krad_transmitter->incoming_connections_efd);
	close (krad_transmitter->incoming_connections_sd);
	free (krad_transmitter->incoming_connection_events);
	
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
	int flags;
	
	flags = 0;
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

	flags = fcntl (krad_transmitter->incoming_connections_sd, F_GETFL, 0);
	if (flags == -1) {
		printke ("Krad Transmitter: fcntl on incoming connections sd F_GETFL");
		return 1;
	}

	flags |= O_NONBLOCK;
	ret = fcntl (krad_transmitter->incoming_connections_sd, F_SETFL, flags);
	if (ret == -1) {
		printke ("Krad Transmitter: fcntl on incoming connections sd F_SETFL");
		return 1;
	}

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

	krad_transmitter->incoming_connection_events = calloc (MAXEVENTS, sizeof (struct epoll_event));

	if (krad_transmitter->incoming_connection_events == NULL) {
		failfast ("Krad Transmitter: Out of memory!");
	}

	printk ("Krad Transmitter: Listening on port %d", krad_transmitter->port);
	
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

