#include "krad_xpdr.h"

int kxpdr_shutdown;

int set_socket_mode (int sfd) {

	int flags, s;

	flags = fcntl (sfd, F_GETFL, 0);
	if (flags == -1)
	{
	  fprintf (stderr, "fcntl");
	  return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl (sfd, F_SETFL, flags);
	if (s == -1)
	{
	  fprintf (stderr, "fcntl");
	  return -1;
	}

	return 0;
}

int transponder_socket_create (char *port) {

	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s;
	int sfd = 0;

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE;     /* All interfaces */

	s = getaddrinfo (NULL, port, &hints, &result);
	if (s != 0)
	{
	  fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
	  return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
	  sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	  if (sfd == -1)
		continue;

	  s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
	  if (s == 0)
		{
		  /* We managed to bind successfully! */
		  break;
		}

	  close (sfd);
	}
	
	int on = 1;
	
	//if ((setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0)
	//{
	//  fprintf (stderr, "failed to set SO_REUSEADDR on port %s\n", port);
	//  abort();
	//}

	if (rp == NULL)
	{
	  fprintf (stderr, "Could not bind %s\n", port);
	  return -1;
	}

	freeaddrinfo (result);

	return sfd;
}

void receiver_activate (kxpdr_receiver_t *kxpdr_receiver, int sd) {

	kxpdr_receiver->sd = sd;

	kxpdr_receiver->http_header = calloc(1, 4096);
	kxpdr_receiver->stream_header = calloc(1, 128000);
	
	strcpy(kxpdr_receiver->name, "");
	kxpdr_receiver->transmitters = calloc(TRANSMITTERS_PER_RECEIVER, sizeof(kxpdr_transmitter_t));
	kxpdr_receiver->ringbuffer = krad_ringbuffer_create(RING_SIZE);

	kxpdr_receiver->receiver_events = calloc (MAXEVENTS, sizeof kxpdr_receiver->receiver_events);
	
	sprintf((char *)kxpdr_receiver->http_header, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", kxpdr_receiver->content_type);

    if ((strcmp(kxpdr_receiver->content_type, "application/x-ogg") == 0) || (strcmp(kxpdr_receiver->content_type, "application/ogg") == 0) || (strcmp(kxpdr_receiver->content_type, "audio/ogg") == 0) ||
        (strcmp(kxpdr_receiver->content_type, "video/ogg") == 0)) {
		kxpdr_receiver->ogg_stream = calloc(1, sizeof(ogg_stream_t));
		ogg_sync_init (&kxpdr_receiver->ogg_stream->os);
		//ogg_stream_init(&kxpdr_receiver->ogg_stream->oz);
		//printf("ogg stream detected, page processing enabled\n");
	} else {
		kxpdr_receiver->ogg_stream = NULL;
	}
	
    if ((strcmp(kxpdr_receiver->content_type, "video/x-matroska") == 0) || (strcmp(kxpdr_receiver->content_type, "audio/x-matroska") == 0) || (strcmp(kxpdr_receiver->content_type, "video/webm") == 0) || (strcmp(kxpdr_receiver->content_type, "audio/webm") == 0) ||
        (strcmp(kxpdr_receiver->content_type, "video/x-matroska-3d") == 0) || (strcmp(kxpdr_receiver->content_type, "audio/x-krad-opus") == 0)) {
		kxpdr_receiver->ebml_stream = calloc(1, sizeof(ebml_stream_t));
	} else {
		kxpdr_receiver->ebml_stream = NULL;
	}

	kxpdr_receiver->http_header_size = strlen((char *)kxpdr_receiver->http_header);
	kxpdr_receiver->http_header_position = strlen((char *)kxpdr_receiver->http_header);

	printf("Recvr http header size is %d\n", kxpdr_receiver->http_header_size);
	
	
	// process first bytes (kludgy yes)
	
	int b, y, f;
	
	f = 0;
	
	for (b = 0; b <= kxpdr_receiver->first_bytes_len - 4; b++) {
		
		y = memcmp ( kxpdr_receiver->first_bytes + b, "\r\n\r\n", 4 );
	
		if (y == 0) {
			f = 1;
			printf("found end of source client http header!\n");
			memmove(kxpdr_receiver->first_bytes, kxpdr_receiver->first_bytes + (b + 4), kxpdr_receiver->first_bytes_len - (b + 4));
			kxpdr_receiver->first_bytes_len -= (b + 4);
			
			printf("got %d first bytes remaining after source client http header\n", kxpdr_receiver->first_bytes_len);
		}
	}
	
	if (f == 0) {
		printf("did not get to end of http buffer in first bytes from source client, ruh oh (fix me)\n");
	}
	
	kxpdr_receiver->active = 2;
	pthread_create(&kxpdr_receiver->receiver_thread, NULL, receiver_thread, (void *)kxpdr_receiver);
}

void transmitter_activate (kxpdr_receiver_t *kxpdr_receiver) {
	pthread_create(&kxpdr_receiver->transmitter_thread, NULL, transmitter_thread, (void *)kxpdr_receiver);
}

void close_receiver (kxpdr_receiver_t *kxpdr_receiver) {

	kxpdr_receiver->active = 3;
	pthread_join (kxpdr_receiver->transmitter_thread, NULL);
	shutdown(kxpdr_receiver->sd, SHUT_RDWR);
	close(kxpdr_receiver->sd);
	usleep(100000);
	strcpy(kxpdr_receiver->mount, "");
	strcpy(kxpdr_receiver->content_type, "");
	free (kxpdr_receiver->transmitters);	
	free (kxpdr_receiver->receiver_events);
	if (kxpdr_receiver->http_header != NULL) {
		free(kxpdr_receiver->http_header);
		kxpdr_receiver->http_header = NULL;
	}
	if (kxpdr_receiver->stream_header != NULL) {
		free(kxpdr_receiver->stream_header);
		kxpdr_receiver->stream_header = NULL;
	}
	
	kxpdr_receiver->http_header_position = 0;
	kxpdr_receiver->http_header_size = 0;
	kxpdr_receiver->stream_header_position = 0;
	kxpdr_receiver->stream_header_size = 0;
	
	krad_ringbuffer_free (kxpdr_receiver->ringbuffer);
	kxpdr_receiver->ready_transmitters_head = NULL;
	kxpdr_receiver->ready_transmitters_tail = NULL;
	kxpdr_receiver->byte_position = 0;
	kxpdr_receiver->sync_byte_position = 0;
	kxpdr_receiver->ready = 0;
	close(kxpdr_receiver->receiver_efd);
	if (kxpdr_receiver->ogg_stream != NULL) {
		ogg_sync_clear (&kxpdr_receiver->ogg_stream->os);
		//ogg_stream_clear(&kxpdr_receiver->ogg_stream->oz);
		free(kxpdr_receiver->ogg_stream);
	}
	if (kxpdr_receiver->ebml_stream != NULL) {
		free(kxpdr_receiver->ebml_stream);
	}
	
	kxpdr_receiver->first_bytes_len = 0;
	memset(kxpdr_receiver->first_bytes, '\0', 512);
	
	kxpdr_receiver->active = 0;

}

void *receiver_thread (void *arg) {

	kxpdr_receiver_t *kxpdr_receiver = (kxpdr_receiver_t *)arg;

	int n;
	int i;
	int closed = 0;
	int s;
	int done = 0;
	int len = 0;
	//char buf[4096 * 16];
	char *buffer;
	
	printf("receiver thread activating!\n");
	
	//kxpdr_receiver->receiver_event.data.ptr = kxpdr_receiver
	kxpdr_receiver->receiver_event.data.fd = kxpdr_receiver->sd;

	kxpdr_receiver->receiver_efd = epoll_create1 (0);
	if (kxpdr_receiver->receiver_efd == -1) {
		fprintf (stderr, "receiver thread  epoll_create");
		abort ();
	}
	
	kxpdr_receiver->receiver_event.events = EPOLLIN | EPOLLET;
	
	s = epoll_ctl (kxpdr_receiver->receiver_efd, EPOLL_CTL_ADD, kxpdr_receiver->sd, &kxpdr_receiver->receiver_event);
	if (s == -1) {
		fprintf (stderr, "receiver thread  epoll_ctl");
		abort ();
	}
	
	while (!closed && !kxpdr_shutdown) {

		n = epoll_wait (kxpdr_receiver->receiver_efd, kxpdr_receiver->receiver_events, MAXEVENTS, 1000);
		
		for (i = 0; i < n; i++) {
		
			if ((kxpdr_receiver->receiver_events[i].events & EPOLLERR) || (kxpdr_receiver->receiver_events[i].events & EPOLLHUP) || (!(kxpdr_receiver->receiver_events[i].events & EPOLLIN))) {
				// An error has occured on this fd, or the socket is not
				// ready for reading (why were we notified then?) 
				fprintf (stderr, "receiver thread epoll error\n");
				close (kxpdr_receiver->receiver_events[i].data.fd);
				continue;

			} else {
		
				done = 0;
			
				while (1) {
				
	            	if (kxpdr_receiver->active == 2) {
						kxpdr_receiver->active = 1;
						transmitter_activate(kxpdr_receiver);
	            	}

					if (kxpdr_receiver->ogg_stream != NULL) {
					
						// kludge zone
						if (kxpdr_receiver->first_bytes_len) {
							buffer = ogg_sync_buffer (&kxpdr_receiver->ogg_stream->os, kxpdr_receiver->first_bytes_len );
						   	ogg_sync_wrote (&kxpdr_receiver->ogg_stream->os, kxpdr_receiver->first_bytes_len);
							kxpdr_receiver->first_bytes_len = 0;
						}
					
						// input bytes
						buffer = ogg_sync_buffer (&kxpdr_receiver->ogg_stream->os, 4096 );
						len = read ( kxpdr_receiver->receiver_events[i].data.fd, buffer, 4096 );
						//printf("cycle %d len\n", len);
						if (len > 0) {
						   	ogg_sync_wrote (&kxpdr_receiver->ogg_stream->os, len);					   	
						} else {
					   		ogg_sync_wrote (&kxpdr_receiver->ogg_stream->os, 0);					   	
						}
						
						if(len > 100) {						   	
					   	//get pages
						while (ogg_sync_pageout (&kxpdr_receiver->ogg_stream->os, &kxpdr_receiver->ogg_stream->op) > 0) {
							// this poorly named 'kludger' is simply because ogg vorbis uses two pages to start a stream
							// and ogg opus only needs one, should be renamed
							
							// essentially we are kludging that vorbis streams have a start page and aux page, and opus streams
							// have a single start page, this should be reworked to inspect the ogg pages maually 
							// to handle chaining and work without copying in and out of the ogg library
							
							if ((ogg_page_bos (&kxpdr_receiver->ogg_stream->op)) || (kxpdr_receiver->ogg_stream->kludger)) {
								if (kxpdr_receiver->ogg_stream->kludger) {
									//kxpdr_receiver->stream_header_size = 0;
									//kxpdr_receiver->stream_header_position = 0;
									kxpdr_receiver->ogg_stream->kludger = 0;
								} else {
									kxpdr_receiver->stream_header_size = 0;
									kxpdr_receiver->stream_header_position = 0;
									kxpdr_receiver->ogg_stream->kludger = 1;
								}
								memcpy(kxpdr_receiver->stream_header + kxpdr_receiver->stream_header_position, kxpdr_receiver->ogg_stream->op.header, kxpdr_receiver->ogg_stream->op.header_len);
								kxpdr_receiver->stream_header_position += kxpdr_receiver->ogg_stream->op.header_len;
								memcpy(kxpdr_receiver->stream_header + kxpdr_receiver->stream_header_position, kxpdr_receiver->ogg_stream->op.body, kxpdr_receiver->ogg_stream->op.body_len);
								kxpdr_receiver->stream_header_position += kxpdr_receiver->ogg_stream->op.body_len;
								kxpdr_receiver->stream_header_size = kxpdr_receiver->stream_header_position;
								// kradcast opus source kludge
								if (kxpdr_receiver->stream_header_size == 47) {
									kxpdr_receiver->ogg_stream->kludger = 0;
								}
								printf("bos detected stream header is %d bytes kludger is %d\n", kxpdr_receiver->stream_header_size, kxpdr_receiver->ogg_stream->kludger);
							} else {
								kxpdr_receiver->ogg_stream->page_header_position = 0;
								kxpdr_receiver->ogg_stream->page_body_position = 0;
								kxpdr_receiver->ogg_stream->page_size = kxpdr_receiver->ogg_stream->op.header_len + kxpdr_receiver->ogg_stream->op.body_len;
								while ((kxpdr_receiver->ogg_stream->page_body_position + kxpdr_receiver->ogg_stream->page_header_position) != kxpdr_receiver->ogg_stream->page_size) {
								
									krad_ringbuffer_get_write_vector(kxpdr_receiver->ringbuffer, &kxpdr_receiver->write_vector[0]);
									
									if (kxpdr_receiver->write_vector[0].len == 0) {
										usleep(10000);
										continue;
									}
	
									if (kxpdr_receiver->ogg_stream->page_header_position != kxpdr_receiver->ogg_stream->op.header_len) {
										if (kxpdr_receiver->write_vector[0].len < kxpdr_receiver->ogg_stream->op.header_len) {
											memcpy(kxpdr_receiver->write_vector[0].buf, kxpdr_receiver->ogg_stream->op.header + kxpdr_receiver->ogg_stream->page_header_position, kxpdr_receiver->write_vector[0].len);
											krad_ringbuffer_write_advance(kxpdr_receiver->ringbuffer, kxpdr_receiver->write_vector[0].len);
											kxpdr_receiver->ogg_stream->page_header_position += kxpdr_receiver->write_vector[0].len;
										} else {
											memcpy(kxpdr_receiver->write_vector[0].buf, kxpdr_receiver->ogg_stream->op.header + kxpdr_receiver->ogg_stream->page_header_position, kxpdr_receiver->ogg_stream->op.header_len - kxpdr_receiver->ogg_stream->page_header_position);
											krad_ringbuffer_write_advance(kxpdr_receiver->ringbuffer, kxpdr_receiver->ogg_stream->op.header_len);
											kxpdr_receiver->ogg_stream->page_header_position += kxpdr_receiver->ogg_stream->op.header_len - kxpdr_receiver->ogg_stream->page_header_position;
										}
									} else {
										if (kxpdr_receiver->write_vector[0].len < kxpdr_receiver->ogg_stream->op.body_len) {
											memcpy(kxpdr_receiver->write_vector[0].buf, kxpdr_receiver->ogg_stream->op.body + kxpdr_receiver->ogg_stream->page_body_position, kxpdr_receiver->write_vector[0].len);
											krad_ringbuffer_write_advance(kxpdr_receiver->ringbuffer, kxpdr_receiver->write_vector[0].len);
											kxpdr_receiver->ogg_stream->page_body_position += kxpdr_receiver->write_vector[0].len;
										} else {
											memcpy(kxpdr_receiver->write_vector[0].buf, kxpdr_receiver->ogg_stream->op.body + kxpdr_receiver->ogg_stream->page_body_position, kxpdr_receiver->ogg_stream->op.body_len - kxpdr_receiver->ogg_stream->page_body_position);
											krad_ringbuffer_write_advance(kxpdr_receiver->ringbuffer, kxpdr_receiver->ogg_stream->op.body_len);
											kxpdr_receiver->ogg_stream->page_body_position += kxpdr_receiver->ogg_stream->op.body_len - kxpdr_receiver->ogg_stream->page_body_position;
										}
									}
								}
								//printf("got page sized %d\n", kxpdr_receiver->ogg_stream->page_size);
								kxpdr_receiver->byte_position += kxpdr_receiver->ogg_stream->page_size;
								kxpdr_receiver->sync_byte_position = kxpdr_receiver->byte_position - kxpdr_receiver->ogg_stream->page_size;	
						   	}
						}
						}

					}

					if (kxpdr_receiver->ebml_stream != NULL) {

						// kludge zone
						if (kxpdr_receiver->first_bytes_len) {
							krad_ringbuffer_get_write_vector(kxpdr_receiver->ringbuffer, &kxpdr_receiver->write_vector[0]);
							memcpy ( kxpdr_receiver->write_vector[0].buf, kxpdr_receiver->first_bytes, kxpdr_receiver->first_bytes_len );
							memcpy(kxpdr_receiver->stream_header + kxpdr_receiver->stream_header_position, kxpdr_receiver->first_bytes, kxpdr_receiver->first_bytes_len);
							kxpdr_receiver->stream_header_position += kxpdr_receiver->first_bytes_len;					   	
							kxpdr_receiver->first_bytes_len = 0;
						}

						krad_ringbuffer_get_write_vector(kxpdr_receiver->ringbuffer, &kxpdr_receiver->write_vector[0]);
						len = read ( kxpdr_receiver->receiver_events[i].data.fd, kxpdr_receiver->write_vector[0].buf, kxpdr_receiver->write_vector[0].len );
						if (len > 0) {
						
							if (kxpdr_receiver->stream_header_size == 0) {
								memcpy(kxpdr_receiver->stream_header + kxpdr_receiver->stream_header_position, kxpdr_receiver->write_vector[0].buf, len);
								kxpdr_receiver->stream_header_position += len;
							}

							int newsync;
							int z;
							int y;
							y = 0;

							newsync = 0;
							
							for (z = 0; z < len - 4; z++) {
			
								y = memcmp ( kxpdr_receiver->write_vector[0].buf + z, "\x1F\x43\xB6\x75", 4 );
								if (y == 0) {

									printf("match!\n");
									newsync = kxpdr_receiver->byte_position + z;

									if (kxpdr_receiver->stream_header_size == 0) {
										kxpdr_receiver->stream_header_size = kxpdr_receiver->stream_header_position - len + z;
									}
								}
							}

							krad_ringbuffer_write_advance(kxpdr_receiver->ringbuffer, len);
							kxpdr_receiver->byte_position += len;
							if (newsync != 0) {
								kxpdr_receiver->sync_byte_position = newsync;
							}							
							
			            	if (len < kxpdr_receiver->write_vector[0].len) {
		    	        		//printf("read space exhaseted\n");
		    	        		break;
			            	}
						}
					}				
					
					if ((kxpdr_receiver->ogg_stream == NULL) && (kxpdr_receiver->ebml_stream == NULL)) {
					
						krad_ringbuffer_get_write_vector(kxpdr_receiver->ringbuffer, &kxpdr_receiver->write_vector[0]);
						len = read ( kxpdr_receiver->receiver_events[i].data.fd, kxpdr_receiver->write_vector[0].buf, kxpdr_receiver->write_vector[0].len );
						if (len > 0) {
							krad_ringbuffer_write_advance(kxpdr_receiver->ringbuffer, len);
							kxpdr_receiver->byte_position += len;
			            	if (len < kxpdr_receiver->write_vector[0].len) {
		    	        		//printf("read space exhaseted\n");
		    	        		break;
			            	}
						}	
					}

					if (len == -1) {
						if (errno != EAGAIN) {
							fprintf (stderr, "read");
							done = 1;
						}
						
						break;
						
					} else if (len == 0) { //EOF
						done = 1;
						break;
					}	    
				}

			}					 	
			
			if (done) {
				printf ("Closed connection on receiver %d\n", kxpdr_receiver->receiver_events[i].data.fd);
				close (kxpdr_receiver->receiver_events[i].data.fd);
				closed = 1;
			}
		}
	}

	close_receiver(kxpdr_receiver);
	printf("receiver thread dactivating!\n");

	return NULL;

}

void close_transmitter (kxpdr_transmitter_t *transmitter) {
		
	close (transmitter->sd);
	transmitter->http_header_position = 0;
	transmitter->stream_header_position = 0;
	transmitter->ring_position_offset = 0;
	transmitter->ring_bytes_avail = 0;
	transmitter->byte_position = 0;
	transmitter->ready = 0;
	transmitter->active = 0;

}

void kxpdr_receiver_add_ready (kxpdr_receiver_t *kxpdr_receiver, kxpdr_transmitter_t *transmitter) {

	transmitter->ready = 1;

	if (kxpdr_receiver->ready_transmitters_head == NULL) {
		kxpdr_receiver->ready_transmitters_head = transmitter;
		transmitter->prev = NULL;
	}
	
	if (kxpdr_receiver->ready_transmitters_tail == NULL) {
		kxpdr_receiver->ready_transmitters_tail = transmitter;
		transmitter->next = NULL;
	} else {
		kxpdr_receiver->ready_transmitters_tail->next = transmitter;
		transmitter->prev = kxpdr_receiver->ready_transmitters_tail;
		kxpdr_receiver->ready_transmitters_tail = transmitter;
		transmitter->next = NULL;
	}

	kxpdr_receiver->ready++;
	printf("added to ready list ready is %d\n", kxpdr_receiver->ready);
}

void kxpdr_receiver_remove_ready (kxpdr_receiver_t *kxpdr_receiver, kxpdr_transmitter_t *transmitter) {

	transmitter->ready = 0;

	if (transmitter->prev != NULL) {
		if (transmitter->next != NULL) {
			transmitter->prev->next = transmitter->next;
		}
		if (transmitter->next == NULL) {
			transmitter->prev->next = NULL;
		}
	}

	if (transmitter->next != NULL) {
		if (transmitter->prev != NULL) {
			transmitter->next->prev = transmitter->prev;
		}
		if (transmitter->prev == NULL) {
			transmitter->next->prev = NULL;
		}
	}

	if ((transmitter->next == NULL) && (transmitter->prev == NULL)) {
		kxpdr_receiver->ready_transmitters_head = NULL;
		kxpdr_receiver->ready_transmitters_tail = NULL;
	}

	if ((transmitter->next != NULL) && (transmitter->prev == NULL)) {
		kxpdr_receiver->ready_transmitters_head = transmitter->next;
	}
	
	if ((transmitter->next == NULL) && (transmitter->prev != NULL)) {
		kxpdr_receiver->ready_transmitters_tail = transmitter->prev;
	}

	transmitter->prev = NULL;
	transmitter->next = NULL;

	kxpdr_receiver->ready--;
	printf("removed from ready list ready is %d\n", kxpdr_receiver->ready);

}

int transmit (kxpdr_receiver_t *kxpdr_receiver, kxpdr_transmitter_t *transmitter) {

	int vec_avail_len;
	int offset;
	int v;

	int ret;
	int actual_ring_bytes_avail;
	
	while (1) {

		if (transmitter->http_header_position != kxpdr_receiver->http_header_size) {
	
			transmitter->http_header_position += write(transmitter->sd, kxpdr_receiver->http_header + transmitter->http_header_position, kxpdr_receiver->http_header_size - transmitter->http_header_position);
	
			printf("\nsending http header, now at position %d of %d\n", transmitter->http_header_position, kxpdr_receiver->http_header_size);
		
			continue;
	
		}
		
		if (transmitter->stream_header_position != kxpdr_receiver->stream_header_size) {
	
			transmitter->stream_header_position += write(transmitter->sd, kxpdr_receiver->stream_header + transmitter->stream_header_position, kxpdr_receiver->stream_header_size - transmitter->stream_header_position);
	
			printf("\nsending stream header, now at position %d of %d\n", transmitter->stream_header_position, kxpdr_receiver->stream_header_size);
		
			continue;
	
		}
	

		transmitter->ring_bytes_avail = kxpdr_receiver->byte_position - transmitter->byte_position;						

		printf("%zu bytes from ringbuffer byte pos is %lu rec byte pos is %lu\n", transmitter->ring_bytes_avail, kxpdr_receiver->byte_position, transmitter->byte_position);
		
		if (transmitter->ring_bytes_avail == 0) {
			// add to tail of readylist
			if (transmitter->ready == 0) {
				kxpdr_receiver_add_ready(kxpdr_receiver, transmitter);
			}
			return -2;
		}

		actual_ring_bytes_avail = krad_ringbuffer_read_space(kxpdr_receiver->ringbuffer);

		if (transmitter->ring_bytes_avail > actual_ring_bytes_avail) {

			printf("transmitter socket %d fell to far behind\n", transmitter->sd);
			close_transmitter(transmitter);
			return -2;

		}

		transmitter->ring_position_offset = actual_ring_bytes_avail - transmitter->ring_bytes_avail;

		krad_ringbuffer_get_read_vector(kxpdr_receiver->ringbuffer, kxpdr_receiver->read_vector);
	
		if (transmitter->ring_position_offset >= kxpdr_receiver->read_vector[0].len) {
			//we need to start in vector[1]
			offset = transmitter->ring_position_offset - kxpdr_receiver->read_vector[0].len;
			vec_avail_len = transmitter->ring_bytes_avail;
			v = 1;
		} else {
			v = 0;
			offset = transmitter->ring_position_offset;
			vec_avail_len = kxpdr_receiver->read_vector[0].len - transmitter->ring_position_offset;
		}

		//printf("going to write %d offset %d v %d\n", vec_avail_len, offset, v);

		ret = write(transmitter->sd, kxpdr_receiver->read_vector[v].buf + offset, vec_avail_len);

		//printf("\nret from write was %d\n", ret);
	
		if (ret > 0) {
			transmitter->byte_position += ret;
			if (vec_avail_len > ret) {
				//printf("write space exchasted %d was remaining\n", vec_avail_len - ret);
				transmitter->ready = 0;
				return 0;
			}
		}

		if (ret == -1) {
			if (errno != EAGAIN) {
				fprintf (stderr, "transmit errno %d", errno);
				close_transmitter(transmitter);
				return 1;
			}
			transmitter->ready = 0;
			return 0;
		} else if (ret == 0) { // EOF
			fprintf (stderr, "transmit eof");
			close_transmitter(transmitter);
			return 1;
		}
	}

	return ret;
	
}

void *transmitter_thread (void *arg) {

	kxpdr_receiver_t *kxpdr_receiver = (kxpdr_receiver_t *)arg;


	int n;
	int i;

	int done = 0;
	size_t len;
	int ret;
	char buf[4096];

	
	int last_byte_pos = 0;
	int wait_time = 1000;
	
	kxpdr_transmitter_t *transmitter;

	printf("transmitter thread activating!\n");
	
	kxpdr_receiver->transmitter_events = calloc (MAXEVENTS, sizeof kxpdr_receiver->transmitter_events);

	kxpdr_receiver->transmitter_efd = epoll_create1 (0);
	if (kxpdr_receiver->transmitter_efd == -1) {
		fprintf (stderr, "transmitter thread  epoll_create");
		abort ();
	}
	
		
	while (kxpdr_receiver->active == 1) {

		n = epoll_wait (kxpdr_receiver->transmitter_efd, kxpdr_receiver->transmitter_events, MAXEVENTS, wait_time);
		
		for (i = 0; i < n; i++) {

			transmitter = (kxpdr_transmitter_t *)kxpdr_receiver->transmitter_events[i].data.ptr;

			//printf("transmitter event %d %d\n", i, transmitter->sd);
			
			if ((kxpdr_receiver->transmitter_events[i].events & EPOLLERR) || (kxpdr_receiver->transmitter_events[i].events & EPOLLHUP)) {
				// An error has occured on this fd, or the socket is not
				// ready for reading (why were we notified then?)
				
				if (kxpdr_receiver->transmitter_events[i].events & EPOLLERR) {
					fprintf (stderr, "transmitter thread err on socket %d\n", transmitter->sd);
				}

				if (kxpdr_receiver->transmitter_events[i].events & EPOLLHUP) {
					fprintf (stderr, "transmitter thread hangup on socket %d\n", transmitter->sd);
				}

				if (transmitter->ready) {
					kxpdr_receiver_remove_ready(kxpdr_receiver, transmitter);
				}
				close_transmitter(transmitter);			
				continue;

			} else {
		
				done = 0;

				if (kxpdr_receiver->transmitter_events[i].events & EPOLLOUT) {						

					ret = transmit(kxpdr_receiver, transmitter);

				} else {

					while (1) {

						// this should never happen because we are not asking if it will

						len = read (transmitter->sd, buf, sizeof buf);

						if (len == -1) {
							if (errno != EAGAIN) {
								fprintf (stderr, "read");
								done = 1;
							}
							break;
						} else if (len == 0) { // EOF
							done = 1;
							break;
						}
						
						printf("read %zu bytes from a transmitter connection and did nothing with it\n", len);

						if (done) {
							printf ("Closed connection on descriptor %d\n", transmitter->sd);
							close_transmitter(transmitter);
						}
					}
				}
			}
		}
		
		int x;
		int readynum = kxpdr_receiver->ready;
		kxpdr_transmitter_t *temp_transmitter;
		
		if ((last_byte_pos != kxpdr_receiver->byte_position) && (kxpdr_receiver->ready)) {
			//printf("processing ready list\n");
			last_byte_pos = kxpdr_receiver->byte_position;

			temp_transmitter = kxpdr_receiver->ready_transmitters_head;
			for (x = 0; x < kxpdr_receiver->ready; x++) {
				transmit(kxpdr_receiver, temp_transmitter);
				temp_transmitter = temp_transmitter->next;
			}
			
			temp_transmitter = NULL;
			for (x = 0; x < readynum; x++) {
				if (temp_transmitter == NULL) {
					temp_transmitter = kxpdr_receiver->ready_transmitters_head;
				}
				
				if (temp_transmitter->ready == 0) {
					kxpdr_receiver_remove_ready(kxpdr_receiver, temp_transmitter);
					temp_transmitter = NULL;
				} else {
					temp_transmitter = temp_transmitter->next;
				}
			}
		}
		
		if (kxpdr_receiver->ready) {
			wait_time = 10;
		} else {
			wait_time = 1000;
		}
		
		// no events or done with events, here we move the read pointer
		
		int ring_bytes_avail;
		ring_bytes_avail = krad_ringbuffer_read_space(kxpdr_receiver->ringbuffer);
		if (ring_bytes_avail > (RING_SIZE / 2)) {
			printf("TRANSMITTER THREAD: ringba: %d advancing %d\n", ring_bytes_avail, ring_bytes_avail - (RING_SIZE / 2));
			krad_ringbuffer_read_advance(kxpdr_receiver->ringbuffer, ring_bytes_avail - (RING_SIZE / 2));
		
		}		
	}

	int x;
	for (x = 0; x < TRANSMITTERS_PER_RECEIVER; x++) {
		if (kxpdr_receiver->transmitters[x].active) {
			close_transmitter(&kxpdr_receiver->transmitters[x]);
		}
	}
	
	close(kxpdr_receiver->transmitter_efd);
	
	printf ("transmitter thread dactivating!\n");

	return NULL;

}


void *incoming_receiver_connections_thread (void *arg) {

	kxpdr_t *kxpdr = (kxpdr_t *)arg;

	printf("incoming receiver connections thread activated!\n");

	int i = 0;
	int s;
	int n;
	int done = 0;
	ssize_t len;
	char buf[512];
	char mount[256];
	char content_type[256];
	int r;
	int len1;
	int len2;
	int len3;
	int wrote;
      
	while (!kxpdr_shutdown) {

		n = epoll_wait (kxpdr->incoming_receiver_connections_efd, kxpdr->incoming_receiver_connection_events, MAXEVENTS, 1000);
		for (i = 0; i < n; i++) {
			if ((kxpdr->incoming_receiver_connection_events[i].events & EPOLLERR) ||
		        (kxpdr->incoming_receiver_connection_events[i].events & EPOLLHUP) ||
		      (!(kxpdr->incoming_receiver_connection_events[i].events & EPOLLIN)))
			{
				fprintf (stderr, "incoming_receiver_connections epoll error\n");
				close (kxpdr->incoming_receiver_connection_events[i].data.fd);
				continue;

			} else if (kxpdr->incoming_receiver_connections_sd == kxpdr->incoming_receiver_connection_events[i].data.fd) {

				while (1) {
			
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

					in_len = sizeof in_addr;
					infd = accept (kxpdr->incoming_receiver_connections_sd, &in_addr, &in_len);
					if (infd == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							// We have processed all incoming connections. 
							break;
						} else {
							fprintf (stderr, "accept");
							break;
						}
					}

					s = getnameinfo (&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV);
				
					if (s == 0) {
						printf("Accepted receiver connection on descriptor %d (host=%s, port=%s)\n", infd, hbuf, sbuf);
					}

					// Make the incoming socket non-blocking and add it to the
					//  list of fds to monitor.
				
					s = set_socket_mode (infd);
					if (s == -1) {
						abort ();
					}
					
					kxpdr->incoming_receiver_connection_event.data.fd = infd;
					kxpdr->incoming_receiver_connection_event.events = EPOLLIN | EPOLLET;
					s = epoll_ctl (kxpdr->incoming_receiver_connections_efd, EPOLL_CTL_ADD, kxpdr->incoming_receiver_connection_event.data.fd, &kxpdr->incoming_receiver_connection_event);
					if (s != 0) {
						printf("incoming receiver connection epoll error s is %d errno is %i %s\n", s, errno, strerror(errno));
					}
				}
		
				continue;
			
			} else {

				done = 0;

				//while (1) {

					// need to account for situation if not all buffer is avail and we need to read again to get it all
					len = read (kxpdr->incoming_receiver_connection_events[i].data.fd, buf, ((sizeof (buf)) - 1));

					if (len == -1) {
						if (errno != EAGAIN) {
							fprintf (stderr, "read");
							done = 1;
						}
						break;
					} else if (len == 0) { // EOF
						done = 1;
						break;
					}

					// Here is where we determine what receiver to hook this receiver up to

					if (len > 0) {
						done = 1;
						s = epoll_ctl (kxpdr->incoming_receiver_connections_efd, EPOLL_CTL_DEL, kxpdr->incoming_receiver_connection_events[i].data.fd, NULL);
						if (s != 0) {
							perror(NULL);
							printf("incoming receiver connection epoll error z is %d errno is %i %s\n", s, errno, strerror(errno));
						}
						
						buf[512] = '\0';
						char *ct;
						
						if (strncmp(buf, "SOURCE /", 8) == 0) {
							len1 = strcspn(buf + 8, "\n\r ");
							memcpy(mount, buf + 8, len1);
							mount[len1] = '\0';
			
							if ((ct = strstr(buf, "content-type:")) != NULL) {
								len1 = strcspn(ct + 14, "\n\r ");
								memcpy(content_type, ct + 14, len1);
								content_type[len1] = '\0';
							}
			
							printf("incoming receiver connection buf len is %zu\n", len);
							printf("SOURCE Client Wanted: --%s--\n", mount);
							printf("content-type: --%s--\n", content_type);
							for (r = 0; r < RECEIVER_COUNT; r++ ) {
								if (kxpdr->receivers[r].active == 0) {
		
									kxpdr->receivers[r].kxpdr = kxpdr;
									strcpy(kxpdr->receivers[r].mount, mount);
									strcpy(kxpdr->receivers[r].content_type, content_type);
									strncpy(kxpdr->receivers[r].first_bytes, buf, len);
									kxpdr->receivers[r].first_bytes_len = len;
									receiver_activate(&kxpdr->receivers[r], kxpdr->incoming_receiver_connection_events[i].data.fd);

									kxpdr->active++;
									done = 0;
									break;
								}
							}
							
							//need to account for max receivers reached
						}
					}
				//}

				if (done) {
					// might fail due to blocking but we dont care right now
					wrote = write (kxpdr->incoming_receiver_connection_events[i].data.fd, kxpdr->not_found, kxpdr->not_found_len);
					if (wrote != kxpdr->not_found_len) {
						printf("oops didn't write 404 header, wrote %d bytes header len is %d bytes\n", wrote, kxpdr->not_found_len);
					} else {
						printf("404 not found buddy\n");
					}
				
					printf ("Closed connection on descriptor %d\n", kxpdr->incoming_receiver_connection_events[i].data.fd);
					close (kxpdr->incoming_receiver_connection_events[i].data.fd);
				}
			}
		}
	}

	printf("incoming receiver connections thread deactivating!\n");

	return NULL;

}

void *incoming_transmitter_connections_thread (void *arg) {

	kxpdr_t *kxpdr = (kxpdr_t *)arg;

	printf("incoming transmitter connections thread activated!\n");

	int i = 0;
	int s;
	int n;
	int done = 0;
	ssize_t len;
	char buf[4096];
	char get[256];
	int r;
	int t;
	int len1;
	int len2;
	int len3;
	int wrote;
      
	while (!kxpdr_shutdown) {

		n = epoll_wait (kxpdr->incoming_transmitter_connections_efd, kxpdr->incoming_transmitter_connection_events, MAXEVENTS, 1000);
		for (i = 0; i < n; i++) {
			if ((kxpdr->incoming_transmitter_connection_events[i].events & EPOLLERR) ||
		        (kxpdr->incoming_transmitter_connection_events[i].events & EPOLLHUP) ||
		      (!(kxpdr->incoming_transmitter_connection_events[i].events & EPOLLIN)))
			{
				fprintf (stderr, "incoming_transmitter_connection epoll error\n");
				close (kxpdr->incoming_transmitter_connection_events[i].data.fd);
				continue;

			} else if (kxpdr->incoming_transmitter_connections_sd == kxpdr->incoming_transmitter_connection_events[i].data.fd) {

				while (1) {
			
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

					in_len = sizeof in_addr;
					infd = accept (kxpdr->incoming_transmitter_connections_sd, &in_addr, &in_len);
					if (infd == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							// We have processed all incoming connections. 
							break;
						} else {
							fprintf (stderr, "accept");
							break;
						}
					}

					s = getnameinfo (&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV);
				
					if (s == 0) {
						printf("Accepted transmitter connection on descriptor %d (host=%s, port=%s)\n", infd, hbuf, sbuf);
					}
				
					s = set_socket_mode (infd);
					if (s == -1) {
						abort ();
					}
					
					kxpdr->incoming_transmitter_connection_event.data.fd = infd;
					kxpdr->incoming_transmitter_connection_event.events = EPOLLIN | EPOLLET;
					s = epoll_ctl (kxpdr->incoming_transmitter_connections_efd, EPOLL_CTL_ADD, kxpdr->incoming_transmitter_connection_event.data.fd, &kxpdr->incoming_transmitter_connection_event);
					if (s != 0) {
						printf("incoming transmitter connection epoll error s is %d errno is %i\n", s, errno);
					}
				}
		
				continue;
			
			} else {
			
				done = 0;

				while (1) {

					len = read (kxpdr->incoming_transmitter_connection_events[i].data.fd, buf, sizeof buf);
					if (len == -1) {
						if (errno != EAGAIN) {
							fprintf (stderr, "read");
							done = 1;
						}
						break;
					} else if (len == 0) { // EOF
						done = 1;
						break;
					}

					// Here is where we determine what receiver to hook this transmitter up to

					if (len > 0) {
						done = 1;
						s = epoll_ctl (kxpdr->incoming_transmitter_connections_efd, EPOLL_CTL_DEL, kxpdr->incoming_transmitter_connection_events[i].data.fd, NULL);
						if (s != 0) {
							printf("incoming transmitter connection epoll error s is %d errno is %i\n", s, errno);
						}
						
						buf[255] = '\0';
	
						if (strncmp(buf, "GET /", 5) == 0) {
		
							len1 = strcspn(buf + 5, "\r");
							len2 = strcspn(buf + 5, " ");
							len3 = (len1) > (len2) ? (len2) : (len1);
							memcpy(get, buf + 5, len3);
							get[len3] = '\0';
			
							printf("Get Client Wanted: %s\n", get);
						
							for (r = 0; r < RECEIVER_COUNT; r++ ) {
								if (kxpdr->receivers[r].active == 1) {
									if (strncmp(get, kxpdr->receivers[r].mount, len3) == 0) {
										printf("Found that reciever\n");
										for (t = 0; t < TRANSMITTERS_PER_RECEIVER; t++) { 
							   				if (kxpdr->receivers[r].transmitters[t].active == 0) {
							   					if (kxpdr->receivers[r].sync_byte_position != 0) {
							   						kxpdr->receivers[r].transmitters[t].byte_position = kxpdr->receivers[r].sync_byte_position;
							   					} else {
							   						kxpdr->receivers[r].transmitters[t].byte_position = kxpdr->receivers[r].byte_position - BURST_SIZE;
							   					}
								   				kxpdr->receivers[r].transmitters[t].sd = kxpdr->incoming_transmitter_connection_events[i].data.fd;
									   			kxpdr->receivers[r].transmitters[t].event.data.ptr = &kxpdr->receivers[r].transmitters[t];
												kxpdr->receivers[r].transmitters[t].event.events = EPOLLOUT | EPOLLET;
												s = epoll_ctl (kxpdr->receivers[r].transmitter_efd, EPOLL_CTL_ADD, kxpdr->receivers[r].transmitters[t].sd, &kxpdr->receivers[r].transmitters[t].event);
												if (s != 0) {
													printf("incoming transmitter yy connection epoll error s is %d errno is %i\n", s, errno);
												} else {
													kxpdr->receivers[r].transmitters[t].active = 1;
												}
												done = 0;
												break;
											}
										}
										
										break;
									} else {
										printf("--%s-- != --%s--\n", get, kxpdr->receivers[r].mount);
									}
									
									
								}
							}
						}
					}
				}

				if (done) {
					// might fail due to blocking but we dont care right now
					wrote = write (kxpdr->incoming_transmitter_connection_events[i].data.fd, kxpdr->not_found, kxpdr->not_found_len);
					if (wrote != kxpdr->not_found_len) {
						printf("oops didn't write 404 header, wrote %d bytes header len is %d bytes\n", wrote, kxpdr->not_found_len);
					} else {
						printf("404 not found buddy\n");
					}
				
					printf ("Closed connection on descriptor %d\n", kxpdr->incoming_transmitter_connection_events[i].data.fd);
					close (kxpdr->incoming_transmitter_connection_events[i].data.fd);
				}
			}
		}
	}
	

	printf("incoming transmitter connections thread deactivating!\n");

	return NULL;

}

kxpdr_t *kxpdr_create (char *incoming_receiver_connection_port, char *incoming_transmitter_connection_port) {

	kxpdr_t *kxpdr = calloc(1, sizeof(kxpdr_t));
	
	int s;
	
	if (kxpdr == NULL) {
		fprintf (stderr, "Out of memory!\n");
		abort ();
	} else {
		printf("activating kxpdr!\n");
	}
	
	printf("%s\n", KXPDR_VERSION);
	
	signal(SIGINT, kxpdr_initiate_shutdown);
	signal(SIGTERM, kxpdr_initiate_shutdown);
	
	kxpdr->not_found_len += sprintf(kxpdr->not_found + kxpdr->not_found_len, "HTTP/1.1 404 Not Found\r\n");
	kxpdr->not_found_len += sprintf(kxpdr->not_found + kxpdr->not_found_len, "Status: 404 Not Found\r\n");
	kxpdr->not_found_len += sprintf(kxpdr->not_found + kxpdr->not_found_len, "Connection: close\r\n");
	kxpdr->not_found_len += sprintf(kxpdr->not_found + kxpdr->not_found_len, "Content-Type: text/html; charset=utf-8\r\n");
	kxpdr->not_found_len += sprintf(kxpdr->not_found + kxpdr->not_found_len, "\r\n404 Not Found");
	
	kxpdr->receivers = calloc(RECEIVER_COUNT, sizeof(kxpdr_receiver_t));
	
	/* RECEIVERS */
	
	kxpdr->incoming_receiver_connections_sd = transponder_socket_create (incoming_receiver_connection_port);
	if (kxpdr->incoming_receiver_connections_sd == -1)
		abort ();

	s = set_socket_mode (kxpdr->incoming_receiver_connections_sd);
	if (s == -1)
		abort ();

	s = listen (kxpdr->incoming_receiver_connections_sd, SOMAXCONN);
	if (s == -1)
	{
	  fprintf (stderr, "listen");
	  abort ();
	}

	kxpdr->incoming_receiver_connections_efd = epoll_create1 (0);
	if (kxpdr->incoming_receiver_connections_efd == -1)
	{
	  fprintf (stderr, "epoll_create");
	  abort ();
	}

	kxpdr->incoming_receiver_connection_event.data.fd = kxpdr->incoming_receiver_connections_sd;
	kxpdr->incoming_receiver_connection_event.events = EPOLLIN | EPOLLET;
	s = epoll_ctl (kxpdr->incoming_receiver_connections_efd, EPOLL_CTL_ADD, kxpdr->incoming_receiver_connections_sd, &kxpdr->incoming_receiver_connection_event);
	if (s == -1)
	{
	  fprintf (stderr, "epoll_ctl");
	  abort ();
	}

	/* Buffer where events are returned */
	kxpdr->incoming_receiver_connection_events = calloc (MAXEVENTS, sizeof kxpdr->incoming_receiver_connection_event);

	printf("incoming receiver connections on port %s\n", incoming_receiver_connection_port);
	pthread_create(&kxpdr->incoming_receiver_connections_thread, NULL, incoming_receiver_connections_thread, (void *)kxpdr);

	/* TRANSMITTERS */

	kxpdr->incoming_transmitter_connections_sd = transponder_socket_create (incoming_transmitter_connection_port);
	if (kxpdr->incoming_transmitter_connections_sd == -1)
	abort ();

	s = set_socket_mode (kxpdr->incoming_transmitter_connections_sd);
	if (s == -1)
	abort ();

	s = listen (kxpdr->incoming_transmitter_connections_sd, SOMAXCONN);
	if (s == -1)
	{
	  fprintf (stderr, "listen");
	  abort ();
	}

	kxpdr->incoming_transmitter_connections_efd = epoll_create1 (0);
	if (kxpdr->incoming_transmitter_connections_efd == -1)
	{
	  fprintf (stderr, "epoll_create");
	  abort ();
	}

	kxpdr->incoming_transmitter_connection_event.data.fd = kxpdr->incoming_transmitter_connections_sd;
	kxpdr->incoming_transmitter_connection_event.events = EPOLLIN | EPOLLET;
	s = epoll_ctl (kxpdr->incoming_transmitter_connections_efd, EPOLL_CTL_ADD, kxpdr->incoming_transmitter_connections_sd, &kxpdr->incoming_transmitter_connection_event);
	if (s == -1)
	{
	  fprintf (stderr, "epoll_ctl");
	  abort ();
	}

	/* Buffer where events are returned */
	kxpdr->incoming_transmitter_connection_events = calloc (MAXEVENTS, sizeof kxpdr->incoming_transmitter_connection_event);

	printf("incoming transmitter connections on port %s\n", incoming_transmitter_connection_port);
	pthread_create(&kxpdr->incoming_transmitter_connections_thread, NULL, incoming_transmitter_connections_thread, (void *)kxpdr);


	printf("kxpdr activated!\n");

	return kxpdr;

}

void kxpdr_destroy (kxpdr_t *kxpdr) {

	pthread_join (kxpdr->incoming_transmitter_connections_thread, NULL);
	pthread_join (kxpdr->incoming_receiver_connections_thread, NULL);

	close (kxpdr->incoming_receiver_connections_efd);
	close (kxpdr->incoming_transmitter_connections_efd);

	close (kxpdr->incoming_receiver_connections_sd);
	close (kxpdr->incoming_transmitter_connections_sd);

	free (kxpdr->incoming_receiver_connection_events);
	free (kxpdr->incoming_transmitter_connection_events);
	
	free (kxpdr->receivers);
	free (kxpdr);
	
	printf("kxpdr destroyed!\n");

}

void kxpdr_initiate_shutdown () {

	if (kxpdr_shutdown == 0) {
		printf("kxpdr initiating shutdown sequence\n");
		kxpdr_shutdown = 1;
	} else {
		printf("kxpdr instantaneous self destruct activated\n");
		exit(1);
	}
}
