#include "krad_receiver.h"

void krad_receiver_promote_client (krad_receiver_client_t *client) {

	krad_transponder_t *krad_transponder;
	krad_link_t *krad_link;	
	int k;

	krad_transponder = client->krad_receiver->krad_transponder;

	pthread_mutex_lock (&krad_transponder->change_lock);


	for (k = 0; k < KRAD_TRANSPONDER_MAX_LINKS; k++) {
		if (krad_transponder->krad_link[k] == NULL) {

			krad_transponder->krad_link[k] = krad_link_prepare (k);
			krad_link = krad_transponder->krad_link[k];
			krad_link->krad_radio = krad_transponder->krad_radio;
			krad_link->krad_transponder = krad_transponder;
			
			krad_tags_set_set_tag_callback (krad_link->krad_tags, krad_transponder->krad_radio->remote.krad_ipc, 
											(void (*)(void *, char *, char *, char *, int))krad_ipc_server_broadcast_tag);
	
			
			sprintf (krad_link->sysname, "link%d", k);

			krad_link->sd = client->sd;
			strcpy (krad_link->mount, client->mount);
			strcpy (krad_link->content_type, client->content_type);
			strcpy (krad_link->host, "ListenSD");
			krad_link->port = client->sd;
			krad_link->operation_mode = PLAYBACK;
			krad_link->transport_mode = TCP;
			//FIXME default
			krad_link->av_mode = AUDIO_AND_VIDEO;
			
			krad_link_start (krad_link);

			break;
		}
	}

	pthread_mutex_unlock (&krad_transponder->change_lock);
	
	free (client);
	
	pthread_exit(0);	

}

void *krad_receiver_client_thread (void *arg) {


	krad_receiver_client_t *client = (krad_receiver_client_t *)arg;
	
	int ret;
	int wot;
	char *string;
	char byte;

	krad_system_set_thread_name ("kr_lsn_client");

	while (1) {
		ret = read (client->sd, client->in_buffer + client->in_buffer_pos, 1);		

		if (ret == 0 || ret == -1) {
			printk ("done with receiver listen client");
			krad_receiver_destroy_client (client);
		} else {
	
			client->in_buffer_pos += ret;
			
			byte = client->in_buffer[client->in_buffer_pos - 1];
			
			if ((byte == '\n') || (byte == '\r')) {
			
				if (client->got_mount == 0) {
					if (client->in_buffer_pos > 8) {
					
						if (strncmp(client->in_buffer, "SOURCE /", 8) == 0) {
							ret = strcspn (client->in_buffer + 8, "\n\r ");
							memcpy (client->mount, client->in_buffer + 8, ret);
							client->mount[ret] = '\0';
						
							printk ("Got a mount! its %s", client->mount);
						
							client->got_mount = 1;
						} else {
							printk ("client no good! %s", client->in_buffer);
							krad_receiver_destroy_client (client);							
						}
					
					} else {
						printk ("client no good! .. %s", client->in_buffer);
						krad_receiver_destroy_client (client);
					}
				} else {
					printk ("client buffer: %s", client->in_buffer);
					
					
					if (client->got_content_type == 0) {
						if (((string = strstr(client->in_buffer, "Content-Type:")) != NULL) ||
						    ((string = strstr(client->in_buffer, "content-type:")) != NULL) ||
							((string = strstr(client->in_buffer, "Content-type:")) != NULL)) {
							ret = strcspn(string + 14, "\n\r ");
							memcpy(client->content_type, string + 14, ret);
							client->content_type[ret] = '\0';
							client->got_content_type = 1;
							printk ("Got a content_type! its %s", client->content_type);							
						}
					} else {
					
						if (memcmp ("\r\n\r\n", &client->in_buffer[client->in_buffer_pos - 4], 4) == 0) {
						//if ((string = strstr(client->in_buffer, "\r\n\r\n")) != NULL) {
							printk ("got to the end of the http headers!");
							
							char *goodresp = "HTTP/1.0 200 OK\r\n\r\n";
							
							wot = write (client->sd, goodresp, strlen(goodresp));
              if (wot != strlen(goodresp)) {
                printke ("Krad Receiver: unexpected write return value %d in receiver_client_thread", wot);
              }							
							
							krad_receiver_promote_client (client);
						}
					}
				}
			}
			
			
			if (client->in_buffer_pos > 1000) {
				printk ("client no good! .. %s", client->in_buffer);
				krad_receiver_destroy_client (client);
			}
		}
	}

	
	printk ("Krad HTTP Request: %s\n", client->in_buffer);
	

	krad_receiver_destroy_client (client);

	return NULL;	
	
}


void krad_receiver_create_client (krad_receiver_t *krad_receiver, int sd) {

	krad_receiver_client_t *client = calloc(1, sizeof(krad_receiver_client_t));

	client->krad_receiver = krad_receiver;
	
	client->sd = sd;
	
	pthread_create (&client->client_thread, NULL, krad_receiver_client_thread, (void *)client);
	pthread_detach (client->client_thread);	

}

void krad_receiver_destroy_client (krad_receiver_client_t *krad_receiver_client) {

	close (krad_receiver_client->sd);
		
	free (krad_receiver_client);
	
	pthread_exit(0);	

}

void *krad_receiver_thread (void *arg) {

	krad_receiver_t *krad_receiver = (krad_receiver_t *)arg;

	int ret;
	int addr_size;
	int client_fd;
	struct sockaddr_in remote_address;
	struct pollfd sockets[1];

	krad_system_set_thread_name ("kr_listener");
	
	printk ("Krad Receiver: Listening thread starting\n");
	
	addr_size = 0;
	ret = 0;
	memset (&remote_address, 0, sizeof(remote_address));	

	addr_size = sizeof (remote_address);
	
	while (krad_receiver->stop_listening == 0) {

		sockets[0].fd = krad_receiver->sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, 250);	

		if (ret < 0) {
			printke ("Krad Receiver: Failed on poll\n");
			krad_receiver->stop_listening = 1;
			break;
		}
	
		if (ret > 0) {
		
			if ((client_fd = accept(krad_receiver->sd, (struct sockaddr *)&remote_address, (socklen_t *)&addr_size)) < 0) {
				close (krad_receiver->sd);
				failfast ("Krad Receiver: socket error on accept mayb a signal or such\n");
			}

			krad_receiver_create_client (krad_receiver, client_fd);

		}
	}
	
	close (krad_receiver->sd);
	krad_receiver->port = 0;
	krad_receiver->listening = 0;	

	printk ("Krad Receiver: Listening thread exiting\n");

	return NULL;
}

void krad_receiver_stop_listening (krad_receiver_t *krad_receiver) {

	if (krad_receiver->listening == 1) {
		krad_receiver->stop_listening = 1;
		pthread_join (krad_receiver->listening_thread, NULL);
		krad_receiver->stop_listening = 0;
	}
}


int krad_receiver_listen_on (krad_receiver_t *krad_receiver, int port) {

	if (krad_receiver->listening == 1) {
		krad_receiver_stop_listening (krad_receiver);
	}

	krad_receiver->port = port;
	krad_receiver->listening = 1;
	
	krad_receiver->local_address.sin_family = AF_INET;
	krad_receiver->local_address.sin_port = htons (krad_receiver->port);
	krad_receiver->local_address.sin_addr.s_addr = htonl (INADDR_ANY);
	
	if ((krad_receiver->sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printke ("Krad Receiver: system call socket error\n");
		krad_receiver->listening = 0;
		krad_receiver->port = 0;		
		return 1;
	}

	if (bind (krad_receiver->sd, (struct sockaddr *)&krad_receiver->local_address, sizeof(krad_receiver->local_address)) == -1) {
		printke ("Krad receiver: bind error for tcp port %d\n", krad_receiver->port);
		close (krad_receiver->sd);
		krad_receiver->listening = 0;
		krad_receiver->port = 0;
		return 1;
	}
	
	if (listen (krad_receiver->sd, SOMAXCONN) <0) {
		printke ("Krad Receiver: system call listen error\n");
		close (krad_receiver->sd);
		return 1;
	}	
	
	pthread_create (&krad_receiver->listening_thread, NULL, krad_receiver_thread, (void *)krad_receiver);
	
	return 0;
}


void krad_receiver_destroy (krad_receiver_t *krad_receiver) {
  krad_receiver_stop_listening (krad_receiver);
  free (krad_receiver);
}

krad_receiver_t *krad_receiver_create (krad_transponder_t *krad_transponder) {

  krad_receiver_t *krad_receiver;

  krad_receiver = calloc (1, sizeof(krad_receiver_t));

  krad_receiver->krad_transponder = krad_transponder;

  return krad_receiver;
}

