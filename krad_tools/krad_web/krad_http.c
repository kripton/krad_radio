#include "krad_http.h"

krad_http_client_t *krad_http_create_client(krad_http_t *krad_http) {

	krad_http_client_t *client = calloc(1, sizeof(krad_http_client_t));

	client->krad_http = krad_http;
	
	return client;

}

void krad_http_destroy_client(krad_http_t *krad_http, krad_http_client_t *client) {

	//printf("Destroy client!!\n");
	
	close (client->sd);
		
	if (client->file_fd) {
		close (client->file_fd);
	}
		
	free (client);
	
	pthread_exit(0);	

}

void krad_http_write_headers (krad_http_client_t *client, char *content_type) {

	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "HTTP/1.1 200 OK\r\n");
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "Status: 200 OK\r\n");
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "Connection: close\r\n");
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "Content-Type: %s; charset=utf-8\r\n", content_type);
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "\r\n");

	client->wrote = write (client->sd, client->out_buffer, strlen(client->out_buffer));

}

void krad_http_404 (krad_http_client_t *client) {

	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "HTTP/1.1 404 Not Found\r\n");
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "Status: 404 Not Found\r\n");
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "Connection: close\r\n");
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "Content-Type: text/html; charset=utf-8\r\n");
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "\r\n");
	client->out_buffer_pos += sprintf(client->out_buffer + client->out_buffer_pos, "404 Not Found");
	
	client->wrote = write (client->sd, client->out_buffer, strlen(client->out_buffer));
	
	krad_http_destroy_client(client->krad_http, client);
	
}

void *krad_http_client_thread (void *arg) {


	krad_http_client_t *client = (krad_http_client_t *)arg;
	
	//printf("\nNew Web Client\n");
	
	/*
	struct pollfd sockets[1];	
	int flags;
	
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
	
	sockets[0].fd = client->sd;
	sockets[0].events = POLLIN;
	
	client->ret = poll (sockets, 1, 250);
	*/

	client->ret = read (client->sd, client->in_buffer + client->in_buffer_pos, BUFSIZE);		
	
	if (client->ret == 0 || client->ret == -1) {
		printf("failed to read browser request\n");
		krad_http_destroy_client(client->krad_http, client);
	}

	
	printf("\nsdfdsf: %s\n", client->in_buffer);
	
	while (client->in_buffer_pos <= 256) {
	
		if (strncmp(client->in_buffer, "GET /", 5) == 0) {
	
			client->ret = strcspn (client->in_buffer + client->in_buffer_pos + 5, "\r ?");
			memcpy (client->get, client->in_buffer + client->in_buffer_pos + 5, client->ret);
			client->get[client->ret] = '\0';
		
			//printf("Wanted: %s\n", client->get);
		
			if (strncmp("favicon.ico", client->get, 11) == 0) {
		
				printf("favicon web client done\n");

				krad_http_destroy_client(client->krad_http, client);

			}
		
			if (strncmp("krad_radio.js", client->get, 13) == 0) {
		

				sprintf(client->filename, "%s/kode/krad_ebml_experimental/krad_apps/krad_radio.js", client->krad_http->homedir);

				if ((client->file_fd = open (client->filename, O_RDONLY)) == -1) {
					printf("failed to open js api file");
				}

				krad_http_write_headers (client, "text/javascript");

				while (	(client->ret = read(client->file_fd, client->out_buffer, BUFSIZE)) > 0 ) {
					client->wrote = write(client->sd, client->out_buffer, client->ret);
				}
		
				//printf("js web client done\n");

				krad_http_destroy_client (client->krad_http, client);


			}
		
			if ((strlen(client->get) == 0) || (strncmp("krad_radio.html", client->get, 15) == 0)) {

				sprintf(client->filename, "%s/kode/krad_ebml_experimental/krad_apps/krad_radio.html", client->krad_http->homedir);

				if ((client->file_fd = open (client->filename, O_RDONLY)) == -1) {
					printf("failed to open html file");
				}

				krad_http_write_headers (client, "text/html");

				while (	(client->ret = read(client->file_fd, client->out_buffer, BUFSIZE)) > 0 ) {
					client->wrote = write(client->sd, client->out_buffer, client->ret);
				}
		
				//printf("html web client done\n");

				krad_http_destroy_client (client->krad_http, client);

			}

			krad_http_404 (client);
		
		}
		
		client->in_buffer_pos += strcspn(client->in_buffer + client->in_buffer_pos, "\r") + 2;
		printf("spin %d\n", client->in_buffer_pos);
		if (strncmp(client->in_buffer + client->in_buffer_pos, "\r\n", 2) == 0) {
			client->in_buffer_pos = client->in_buffer_pos + 2;
			//printf("Headers Length: %d\n", client->in_buffer_pos );
			break;
		}		
		
	}

	printf ("web client fail\n");

	krad_http_destroy_client (client->krad_http, client);
	
	return NULL;
	
}


void krad_http_server_destroy (krad_http_t *krad_http) {

	printf ("krad_http Shutting Down\n");

	if (krad_http != NULL) {
	
		krad_http->shutdown = 1;

		usleep (25000);
	
		pthread_cancel (krad_http->server_thread);

		close (krad_http->listenfd);

		free (krad_http);

	}

}

void *krad_http_server_run (void *arg) {

	krad_http_t *krad_http = (krad_http_t *)arg;

	krad_http_client_t *newclient;
	socklen_t length;
	static struct sockaddr_in cli_addr;

	while (!krad_http->shutdown) {
	
	
		newclient = krad_http_create_client(krad_http);
	
		length = sizeof(cli_addr);
		
		if ((krad_http->socketfd = accept(krad_http->listenfd, (struct sockaddr *)&cli_addr, &length)) < 0) {
			close(krad_http->listenfd);
			printf("krad_http socket error on accept mayb a signal or such\n");
			exit(1);
		}

		newclient->sd = krad_http->socketfd;

		pthread_create (&newclient->client_thread, NULL, krad_http_client_thread, (void *)newclient);
		pthread_detach (newclient->client_thread);

	}
	
	return NULL;
	
}

krad_http_t *krad_http_server_create (int port) {
	
	krad_http_t *krad_http = calloc(1, sizeof(krad_http_t));

	static struct sockaddr_in serv_addr;
	int on = 1;

	krad_http->port = port;
	
	if (krad_http->port < 0 || krad_http->port > 60000) {
		printf("krad_http port number error\n");
		exit(1);
	}
	
	printf("Krad web Starting Up on port %d\n", krad_http->port);

	krad_http->homedir = getenv ("HOME");
 	
	/* setup the network socket */
	if ((krad_http->listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0) {
		printf("krad_http system call socket error\n");
		exit(1);
	}
	
	if ((setsockopt (krad_http->listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0) {
		printf("kradweb system call setsockopt error");
		close(krad_http->listenfd);
		exit (1);
	}	
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(krad_http->port);
	
	if (bind(krad_http->listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
		printf ("krad_http system call bind error\n");
		close (krad_http->listenfd);
		exit (1);
	}

	if( listen(krad_http->listenfd, 5) <0) {
		printf("krad_http system call listen error\n");
		close (krad_http->listenfd);
		exit(1);
	}

	pthread_create (&krad_http->server_thread, NULL, krad_http_server_run, (void *)krad_http);
	pthread_detach (krad_http->server_thread);

	return krad_http;

}
