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


/*

void upload(int fd) {
	int file_fd = 0, buflen = 0, wrote = 0, end = 0, ret = 0, bufpos = 0, wrote_total = 0, length = 0, status;
	
	static char buffer[8192];
 
 	char station[256];
 	char filename[512];
	char fullfilename[768];

	struct stat st;

	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	
	printf("\nNew Upload\n");
	
	ret = read(fd,buffer,BUFSIZE); 	 
	
	if (ret == 0 || ret == -1) {	 
		printf("failed to read browser request\n");
		exit(1);
	}
	
	buflen = BUFSIZE / 2;

	while(bufpos <= buflen) {
		if (strncmp(buffer + bufpos, "Content-Length: ", 15) == 0) {
			length = atoi(buffer + 15 + bufpos);
			printf("Upload Size: %dK\n", length / 1000);
		}

		if (strncmp(buffer + bufpos, "X-File-Name: ", 13) == 0) {
			end = strcspn(buffer + bufpos + 13, "\r");
			strncpy(filename, buffer + bufpos + 13, end);
			filename[end] = '\0';
			printf("Filename: %s\n", filename);
		}
		
		if (strncmp(buffer + bufpos, "X-Krad-Station: ", 16) == 0) {
			strncpy(station, buffer + bufpos + 16, strcspn(buffer + bufpos + 16, "\r"));
			printf("Station %s\n", station);
		}
		
		bufpos += strcspn(buffer + bufpos, "\r") + 2;
		//printf("spin %d - %d\n", bufpos, buflen);
		if (strncmp(buffer + bufpos, "\r\n", 2) == 0) {
			bufpos = bufpos + 2;
			printf("Headers Length: %d\n", bufpos );
			break;
		}		
	}


	// test dir existance
	sprintf(fullfilename, "%s/krad/uploads/%s", homedir, station);
	if(stat(fullfilename, &st) == 0) {
		printf("Directory Exists: %s\n", fullfilename );
	} else {
		status = mkdir(fullfilename, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (status == 0) {	
			printf("Directory Created: %s\n", fullfilename );
		} else {
			printf("Cant create directory: %s\n", fullfilename );
		}	
	}

	sprintf(fullfilename, "%s/krad/uploads/%s/%s", homedir, station, filename);

	if ((file_fd = open(fullfilename, O_WRONLY | O_CREAT, mode)) == -1) {
		printf("failed to open new file");
		exit(1);
	}


	wrote = write(file_fd, buffer + bufpos, ret - bufpos);
	wrote_total += wrote;
	//printf("wrote %d bytes total %d\n", wrote, wrote_total);

	while (wrote_total != length) {

		if (wrote_total > length) {	 
			printf("failed got to much data\n");
			close(file_fd);
			close(fd);
			exit(1);
		}

		ret = read(fd, buffer, BUFSIZE);
		
		if (ret == 0 || ret < 0) {	 
			printf("failed to read incomping socket\n");
			close(file_fd);
			close(fd);
			exit(1);
		}
		
		wrote = write(file_fd, buffer, ret);
		wrote_total += wrote;
	}

	close(file_fd);
	close(fd);

	printf("Upload Complete %s - %dKB / %dMB for station %s\n", fullfilename, wrote_total / 1000, (wrote_total / 1000 / 1000), station);

	tell_station_about_upload(station, fullfilename);


	exit(0);
}


*/

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
		

				sprintf(client->filename, "%s/kode/krad_radio/krad_apps/krad_radio.js", client->krad_http->homedir);

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

				sprintf(client->filename, "%s/kode/krad_radio/krad_apps/krad_radio.html", client->krad_http->homedir);

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
