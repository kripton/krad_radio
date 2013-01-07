#include "krad_receiver.h"

void krad_transponder_listen_promote_client (krad_transponder_listen_client_t *client) {

	krad_transponder_t *krad_transponder;
	krad_link_t *krad_link;	
	int k;

	krad_transponder = client->krad_transponder;

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

void *krad_transponder_listen_client_thread (void *arg) {


	krad_transponder_listen_client_t *client = (krad_transponder_listen_client_t *)arg;
	
	int ret;
	int wot;
	char *string;
	char byte;

	krad_system_set_thread_name ("kr_lsn_client");

	while (1) {
		ret = read (client->sd, client->in_buffer + client->in_buffer_pos, 1);		

		if (ret == 0 || ret == -1) {
			printk ("done with transponder listen client");
			krad_transponder_listen_destroy_client (client);
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
							krad_transponder_listen_destroy_client (client);							
						}
					
					} else {
						printk ("client no good! .. %s", client->in_buffer);
						krad_transponder_listen_destroy_client (client);
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
                printke ("krad transponder: unexpected write return value %d in transponder_listen_client_thread", wot);
              }							
							
							krad_transponder_listen_promote_client (client);
						}
					}
				}
			}
			
			
			if (client->in_buffer_pos > 1000) {
				printk ("client no good! .. %s", client->in_buffer);
				krad_transponder_listen_destroy_client (client);
			}
		}
	}

	
	printk ("Krad HTTP Request: %s\n", client->in_buffer);
	

	krad_transponder_listen_destroy_client (client);

	return NULL;	
	
}


void krad_transponder_listen_create_client (krad_transponder_t *krad_transponder, int sd) {

	krad_transponder_listen_client_t *client = calloc(1, sizeof(krad_transponder_listen_client_t));

	client->krad_transponder = krad_transponder;
	
	client->sd = sd;
	
	pthread_create (&client->client_thread, NULL, krad_transponder_listen_client_thread, (void *)client);
	pthread_detach (client->client_thread);	

}

void krad_transponder_listen_destroy_client (krad_transponder_listen_client_t *krad_transponder_listen_client) {

	close (krad_transponder_listen_client->sd);
		
	free (krad_transponder_listen_client);
	
	pthread_exit(0);	

}

void *krad_transponder_listening_thread (void *arg) {

	krad_transponder_t *krad_transponder = (krad_transponder_t *)arg;

	int ret;
	int addr_size;
	int client_fd;
	struct sockaddr_in remote_address;
	struct pollfd sockets[1];

	krad_system_set_thread_name ("kr_listener");
	
	printk ("Krad transponder: Listening thread starting\n");
	
	addr_size = 0;
	ret = 0;
	memset (&remote_address, 0, sizeof(remote_address));	

	addr_size = sizeof (remote_address);
	
	while (krad_transponder->stop_listening == 0) {

		sockets[0].fd = krad_transponder->sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, 250);	

		if (ret < 0) {
			printke ("Krad transponder: Failed on poll\n");
			krad_transponder->stop_listening = 1;
			break;
		}
	
		if (ret > 0) {
		
			if ((client_fd = accept(krad_transponder->sd, (struct sockaddr *)&remote_address, (socklen_t *)&addr_size)) < 0) {
				close (krad_transponder->sd);
				failfast ("Krad transponder: socket error on accept mayb a signal or such\n");
			}

			krad_transponder_listen_create_client (krad_transponder, client_fd);

		}
	}
	
	close (krad_transponder->sd);
	krad_transponder->port = 0;
	krad_transponder->listening = 0;	

	printk ("Krad transponder: Listening thread exiting\n");

	return NULL;
}

void krad_transponder_stop_listening (krad_transponder_t *krad_transponder) {

	if (krad_transponder->listening == 1) {
		krad_transponder->stop_listening = 1;
		pthread_join (krad_transponder->listening_thread, NULL);
		krad_transponder->stop_listening = 0;
	}
}


int krad_transponder_listen (krad_transponder_t *krad_transponder, int port) {

	if (krad_transponder->listening == 1) {
		krad_transponder_stop_listening (krad_transponder);
	}

	krad_transponder->port = port;
	krad_transponder->listening = 1;
	
	krad_transponder->local_address.sin_family = AF_INET;
	krad_transponder->local_address.sin_port = htons (krad_transponder->port);
	krad_transponder->local_address.sin_addr.s_addr = htonl (INADDR_ANY);
	
	if ((krad_transponder->sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printke ("Krad transponder: system call socket error\n");
		krad_transponder->listening = 0;
		krad_transponder->port = 0;		
		return 1;
	}

	if (bind (krad_transponder->sd, (struct sockaddr *)&krad_transponder->local_address, sizeof(krad_transponder->local_address)) == -1) {
		printke ("Krad transponder: bind error for tcp port %d\n", krad_transponder->port);
		close (krad_transponder->sd);
		krad_transponder->listening = 0;
		krad_transponder->port = 0;
		return 1;
	}
	
	if (listen (krad_transponder->sd, SOMAXCONN) <0) {
		printke ("Krad transponder: system call listen error\n");
		close (krad_transponder->sd);
		return 1;
	}	
	
	pthread_create (&krad_transponder->listening_thread, NULL, krad_transponder_listening_thread, (void *)krad_transponder);
	
	return 0;
}

/*

void *udp_input_thread(void *arg) {

	krad_system_set_thread_name ("kr_udp_in");

	krad_link_t *krad_link = (krad_link_t *)arg;

	printk ("UDP Input thread starting");

	int sd;
	int ret;
	int rsize;
	unsigned char *buffer;
	unsigned char *packet_buffer;
	struct sockaddr_in local_address;
	struct sockaddr_in remote_address;
	struct pollfd sockets[1];	
	int nocodec;
	int opus_codec;
	int packets;
	
	packets = 0;
	rsize = sizeof(remote_address);
	opus_codec = OPUS;
	nocodec = NOCODEC;
	buffer = calloc (1, 2000);
	packet_buffer = calloc (1, 500000);
	sd = socket (AF_INET, SOCK_DGRAM, 0);

	krad_link->krad_rebuilder = krad_rebuilder_create ();

	memset((char *) &local_address, 0, sizeof(local_address));
	local_address.sin_family = AF_INET;
	local_address.sin_port = htons (krad_link->port);
	local_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1 ) {
		failfast ("UDP Input bind error");
	}
	
	//kludge to get header
	krad_opus_t *opus_temp;
	unsigned char opus_header[256];
	int opus_header_size;
	
	opus_temp = krad_opus_encoder_create (2, krad_link->krad_radio->krad_mixer->sample_rate, 110000, 
										 OPUS_APPLICATION_AUDIO);
										 
	opus_header_size = opus_temp->header_data_size;
	memcpy (opus_header, opus_temp->header_data, opus_header_size);
	krad_opus_encoder_destroy(opus_temp);
	
	printk ("placing opus header size is %d", opus_header_size);
	
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&nocodec, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_codec, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_header_size, 4);
	krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)opus_header, opus_header_size);

	while (!krad_link->destroy) {
	
		sockets[0].fd = sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, 250);	
	
		if (ret < 0) {
			printk ("Krad Link UDP Poll Failure");
			krad_link->destroy = 1;
			continue;
		}
	
		if (ret > 0) {
		
			ret = recvfrom (sd, buffer, 2000, 0, (struct sockaddr *)&remote_address, (socklen_t *)&rsize);
		
			if (ret == -1) {
				printk ("Krad Link UDP Recv Failure");
				krad_link->destroy = 1;
				continue;
			}
		
			//printk ("Received packet from %s:%d", 
			//		inet_ntoa(remote_address.sin_addr), ntohs(remote_address.sin_port));


			krad_rebuilder_write (krad_link->krad_rebuilder, buffer, ret);

			ret = krad_rebuilder_read_packet (krad_link->krad_rebuilder, packet_buffer, 1);
		
			if (ret != 0) {
				//printk ("read a packet with %d bytes", ret);

				if ((krad_link->av_mode == AUDIO_ONLY) || (krad_link->av_mode == AUDIO_AND_VIDEO)) {
			
					while ((krad_ringbuffer_write_space(krad_link->encoded_audio_ringbuffer) < ret + 4 + 4) && (!krad_link->destroy)) {
						usleep(10000);
					}
				
					if (packets > 0) {
						krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&opus_codec, 4);
					}
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)&ret, 4);
					krad_ringbuffer_write(krad_link->encoded_audio_ringbuffer, (char *)packet_buffer, ret);
					packets++;

				}

			}
		}
	}

	krad_rebuilder_destroy (krad_link->krad_rebuilder);
	close (sd);
	free (buffer);
	free (packet_buffer);
	printk ("UDP Input thread exiting");
	
	return NULL;

}
*/

