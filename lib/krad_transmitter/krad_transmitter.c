#include "krad_transmitter.h"

int krad_transmitter_transmission_transmit_header (krad_transmission_t *krad_transmission,
											krad_transmission_receiver_t *krad_transmission_receiver) {

  int wrote;

  wrote = 0;

  while (krad_transmission_receiver->wrote_http_header == 0) {
	  wrote = write (krad_transmission_receiver->fd,
				   krad_transmission->http_header + krad_transmission_receiver->http_header_position,
				   krad_transmission->http_header_len - krad_transmission_receiver->http_header_position);

    if (wrote > 0) {
      krad_transmission_receiver->http_header_position += wrote;
	    if (krad_transmission_receiver->http_header_position == krad_transmission->http_header_len) {
		    krad_transmission_receiver->wrote_http_header = 1;
	    } else {
        return 0;
      }
    } else {
		  if (wrote == -1) {
			  if (errno != EAGAIN) {
				  printke ("Krad Transmitter: transmission error writing to socket");
				  krad_transmitter_receiver_destroy (krad_transmission_receiver);
			  }
      }
      return 0;
    }
  }

  while (krad_transmission_receiver->wrote_header == 0) {
	  wrote = write (krad_transmission_receiver->fd,
				   krad_transmission->header + krad_transmission_receiver->header_position,
				   krad_transmission->header_len - krad_transmission_receiver->header_position);

    if (wrote > 0) {
      krad_transmission_receiver->header_position += wrote;
	    if (krad_transmission_receiver->header_position == krad_transmission->header_len) {
		    krad_transmission_receiver->wrote_header = 1;
	    } else {
        return 0;
      }
    } else {
		  if (wrote == -1) {
			  if (errno != EAGAIN) {
				  printke ("Krad Transmitter: transmission error writing to socket");
				  krad_transmitter_receiver_destroy (krad_transmission_receiver);
			  }
      }
      return 0;
    }
  }

  krad_transmission_receiver->position = krad_transmission->sync_point;
  return 1;

}

int krad_transmitter_transmission_transmit (krad_transmission_t *krad_transmission,
											krad_transmission_receiver_t *krad_transmission_receiver) {

	int ret;
	uint64_t buf_avail;
	uint32_t vec_pos;
	uint32_t vec_avail;
	krad_ringbuffer_data_t *vec;

	vec_avail = 0;
	vec_pos = 0;
	ret = 0;
	buf_avail = 0;
	
	if ((krad_transmission == NULL) || (krad_transmission_receiver == NULL)) {
		failfast ("Krad Transmitter: this should not be");
	}
	
	while (1) {

    if ((krad_transmission_receiver->wrote_http_header != 1) || (krad_transmission_receiver->wrote_header != 1)) {
      ret = krad_transmitter_transmission_transmit_header (krad_transmission, krad_transmission_receiver);
      if (ret == 1) {
        continue;
      } else {
        return 0;
      }
    }
	
		if (krad_transmission_receiver->position == krad_transmission->position) {
			if (krad_transmission_receiver->ready == 0) {
				krad_transmission_add_ready (krad_transmission, krad_transmission_receiver);	
			}
			break;
		}

		if (krad_transmission_receiver->position < krad_transmission->horizon) {
			printke ("Krad Transmitter: client fell so far behind oh no");
			krad_transmitter_receiver_destroy (krad_transmission_receiver);
			break;
		}

		buf_avail = krad_transmission->position - krad_transmission_receiver->position;

		if (buf_avail > krad_transmission->tx_vec[1].len) {
			vec = &krad_transmission->tx_vec[0];
			vec_pos = krad_transmission->tx_vec[0].len - (buf_avail - krad_transmission->tx_vec[1].len);
			vec_avail = vec->len - vec_pos;

			ret = write (krad_transmission_receiver->fd,
						 vec->buf + vec_pos,
						 vec_avail);
		} else {
			vec = &krad_transmission->tx_vec[1];
			vec_pos = krad_transmission->tx_vec[1].len - buf_avail;
			vec_avail = vec->len - vec_pos;
			
			ret = write (krad_transmission_receiver->fd,
						 vec->buf + vec_pos,
						 vec_avail);
		}

		if (ret == -1) {
			if (errno != EAGAIN) {
				printke ("Krad Transmitter: transmission error writing to socket");
				krad_transmitter_receiver_destroy (krad_transmission_receiver);
				break;
			}
			if (krad_transmission_receiver->ready == 1) {
				krad_transmission_remove_ready (krad_transmission, krad_transmission_receiver);
			}
			break;
		}

		if (ret == 0) {
			printk ("Krad Transmitter: transmission transmit wrote to client 0 bytes");
			krad_transmitter_receiver_destroy (krad_transmission_receiver);
			break;
		}

		if (ret > 0) {
			krad_transmission_receiver->position += ret;
		}

		if (ret < vec_avail) {
			if (krad_transmission_receiver->ready == 1) {		
				krad_transmission_remove_ready (krad_transmission, krad_transmission_receiver);
			}
			break;
		}
	}

	return 0;
}

void *krad_transmitter_transmission_thread (void *arg) {

	krad_transmission_t *krad_transmission = (krad_transmission_t *)arg;
	
	krad_system_set_thread_name ("kr_tx_txmtr");	

	int e;
	int ret;
	int wait_time;
	uint64_t last_position;
	uint32_t new_bytes_avail;
	uint32_t space_avail;
	uint32_t drop_count;
	krad_transmission_receiver_t *krad_transmission_receiver;

	e = 0;
	ret = 0;
	wait_time = 8;	
	last_position = 0;
	new_bytes_avail = 0;
	space_avail = 0;
	drop_count = 0;
	krad_transmission_receiver = NULL;
	
	printk ("Krad Transmitter: transmission thread starting for %s", krad_transmission->sysname);

	while (krad_transmission->active == 1) {

		if (krad_transmission->new_data == 1) {

			if (krad_transmission->new_sync_point == 1) {
				krad_transmission->sync_point = krad_transmission->position;
				krad_transmission->new_sync_point = 0;
			}

			space_avail = krad_ringbuffer_write_space (krad_transmission->transmission_ringbuffer);
			new_bytes_avail = krad_ringbuffer_read_space (krad_transmission->ringbuffer);

			if (space_avail < new_bytes_avail) {
				drop_count = new_bytes_avail - space_avail;			
				krad_ringbuffer_read_advance (krad_transmission->transmission_ringbuffer, drop_count);
			}

			krad_ringbuffer_read (krad_transmission->ringbuffer,
						  (char *)krad_transmission->transmission_buffer,
						          new_bytes_avail);

			krad_ringbuffer_write (krad_transmission->transmission_ringbuffer,
			               (char *)krad_transmission->transmission_buffer,
			                       new_bytes_avail);

			krad_transmission->position += new_bytes_avail;
			if (krad_transmission->position > DEFAULT_RING_SIZE) {
				krad_transmission->horizon = krad_transmission->position - DEFAULT_RING_SIZE;
			}

			krad_ringbuffer_get_read_vector (krad_transmission->transmission_ringbuffer,
											 &krad_transmission->tx_vec[0]);
			krad_transmission->new_data = 0;
		}

		ret = epoll_wait (krad_transmission->connections_efd, krad_transmission->transmission_events,
		                  KRAD_TRANSMITTER_MAXEVENTS, wait_time);

		if (ret < 0) {
			if ((ret < 0) && (errno == EINTR)) {
				continue;
			}		
			printke ("Krad Transmitter: Failed on epoll wait %s", strerror(errno));
			krad_transmission->active = 2;
			break;
		}
	
		if (ret > 0) {
			for (e = 0; e < ret; e++) {

				krad_transmission_receiver =
				(krad_transmission_receiver_t *)krad_transmission->transmission_events[e].data.ptr;			
	
				if (krad_transmission_receiver->noob == 1) {
					krad_transmission_receiver->noob = 0;
					krad_transmission->receivers++;
				}

				if ((krad_transmission->transmission_events[e].events & EPOLLERR) ||
					(krad_transmission->transmission_events[e].events & EPOLLHUP) ||
					(krad_transmission->transmission_events[e].events & EPOLLRDHUP)) {
					
					if (krad_transmission->transmission_events[e].events & EPOLLHUP) {
						printke ("Krad Transmitter: transmitter transmission receiver connection hangup");
					}
					if (krad_transmission->transmission_events[e].events & EPOLLERR) {
						printke ("Krad Transmitter: transmitter transmission receiver connection error");
					}
					if (krad_transmission->transmission_events[e].events & EPOLLRDHUP) {
						printk ("Krad Transmitter: client disconnected %d", krad_transmission_receiver->fd);
					}
					krad_transmitter_receiver_destroy (krad_transmission_receiver);
					continue;
				}
				
				if (krad_transmission->transmission_events[e].events & EPOLLOUT) {
  				krad_transmitter_transmission_transmit (krad_transmission, krad_transmission_receiver);
				}
			}
		}

		if ((last_position != krad_transmission->position) && (krad_transmission->ready_receiver_count > 0)) {
			last_position = krad_transmission->position;
			krad_transmission_receiver = krad_transmission->ready_receivers;
			while (krad_transmission_receiver != NULL) {
				krad_transmitter_transmission_transmit (krad_transmission, krad_transmission_receiver);
				krad_transmission_receiver = krad_transmission_receiver->next;
			}
		}
	}

	krad_transmission->active = 3;

	printk ("Krad Transmitter: transmission thread exiting for %s", krad_transmission->sysname);
	return NULL;

}


void krad_transmission_add_ready (krad_transmission_t *krad_transmission, krad_transmission_receiver_t *krad_transmission_receiver) {

	if (krad_transmission_receiver->ready == 1) {
		printke ("Krad Transmitter: receiver was already ready!");
		return;
	}

	krad_transmission_receiver->ready = 1;
	
	if (krad_transmission->ready_receivers == NULL) {
		krad_transmission_receiver->prev = NULL;
		krad_transmission->ready_receivers = krad_transmission_receiver;
		krad_transmission_receiver->next = NULL;		
	} else {
		krad_transmission_receiver->next = krad_transmission->ready_receivers;
		krad_transmission->ready_receivers->prev = krad_transmission_receiver;
		krad_transmission->ready_receivers = krad_transmission_receiver;
		krad_transmission_receiver->prev = NULL;		
	}

	krad_transmission->ready_receiver_count++;
}


void krad_transmission_remove_ready (krad_transmission_t *krad_transmission, krad_transmission_receiver_t *krad_transmission_receiver) {

	if (krad_transmission_receiver->ready != 1) {
		printke ("Krad Transmitter: receiver was not ready!");	
		return;
	}

	krad_transmission_receiver->ready = 0;

	while (1) {
		if ((krad_transmission_receiver->prev == NULL) && (krad_transmission_receiver->next == NULL)) {
			krad_transmission->ready_receivers = NULL;
			break;
		}
		if ((krad_transmission_receiver->prev == NULL) && (krad_transmission_receiver->next != NULL)) {
			krad_transmission->ready_receivers = krad_transmission_receiver->next;
			krad_transmission_receiver->next->prev = NULL;
			break;
		}
		if ((krad_transmission_receiver->prev != NULL) && (krad_transmission_receiver->next == NULL)) {
			krad_transmission_receiver->prev->next = NULL;
			break;
		}
		if ((krad_transmission_receiver->prev != NULL) && (krad_transmission_receiver->next != NULL)) {
			krad_transmission_receiver->prev->next = krad_transmission_receiver->next;
			krad_transmission_receiver->next->prev = krad_transmission_receiver->prev;
			break;
		}
	}

	krad_transmission_receiver->prev = NULL;
	krad_transmission_receiver->next = NULL;

	krad_transmission->ready_receiver_count--;
}

int krad_transmitter_transmission_set_header (krad_transmission_t *krad_transmission, unsigned char *buffer, int length) {

	int header_temp_len;
	unsigned char *header_temp;
	unsigned char *temp;
	
	temp = NULL;

	header_temp_len = length;
	header_temp = calloc (1, header_temp_len);
	memcpy (header_temp, buffer, header_temp_len);

	if (krad_transmission->header != NULL) {
		temp = krad_transmission->header;
	}
	
	// FIXME this isn't atomic
	krad_transmission->header_len = 0;
	krad_transmission->header = header_temp;
	krad_transmission->header_len = header_temp_len;
	
	if (temp != NULL) {
		free (temp);
	}

	printk ("Krad Transmitter: transmission %s set header at %d bytes long",
			krad_transmission->sysname,
			krad_transmission->header_len);

	return length;
}

void krad_transmitter_transmission_add_header (krad_transmission_t *krad_transmission, unsigned char *buffer, int length) {

	int header_temp_len;
	unsigned char *header_temp;
	unsigned char *temp;
	
	temp = NULL;

	header_temp_len = length;
	
	if (krad_transmission->header_len != 0) {
		header_temp_len += krad_transmission->header_len;
	}
	
	header_temp = calloc (1, header_temp_len);
	
	if (krad_transmission->header_len != 0) {
		memcpy (header_temp, krad_transmission->header, krad_transmission->header_len);
		memcpy (header_temp + krad_transmission->header_len, buffer, length);
	} else {
		memcpy (header_temp, buffer, header_temp_len);
	}

	if (krad_transmission->header != NULL) {
		temp = krad_transmission->header;
	}
	
	// FIXME this isn't atomic
	krad_transmission->header_len = 0;
	krad_transmission->header = header_temp;
	krad_transmission->header_len = header_temp_len;
	
	if (temp != NULL) {
		free (temp);
	}

	printk ("Krad Transmitter: transmission %s added header of %d bytes total header size is %d",
			krad_transmission->sysname,
			length,
			krad_transmission->header_len);

}


int krad_transmitter_transmission_add_data (krad_transmission_t *krad_transmission, unsigned char *buffer, int length) {
	return krad_transmitter_transmission_add_data_opt (krad_transmission, buffer, length, 0);
}

int krad_transmitter_transmission_add_data_sync (krad_transmission_t *krad_transmission, unsigned char *buffer, int length) {
	return krad_transmitter_transmission_add_data_opt (krad_transmission, buffer, length, 1);
}

int krad_transmitter_transmission_add_data_opt (krad_transmission_t *krad_transmission, unsigned char *buffer, int length, int sync) {

	int ret;
	ret = 0;

	while (krad_transmission->new_data == 1) {
		usleep(5000);
	}

	if ((krad_transmission->ready == 0) && (krad_transmission->header_len > 0) && (length > 0)) {
		ret = krad_ringbuffer_write (krad_transmission->ringbuffer, (char *)buffer, length);
		krad_transmission->ready = 1;
		krad_transmission->new_sync_point = 1;
		krad_transmission->new_data = 1;

		printk ("Krad Transmitter: transmission %s added now ready!",
				krad_transmission->sysname);
		return ret;
	} else {
		ret = krad_ringbuffer_write (krad_transmission->ringbuffer, (char *)buffer, length);
		krad_transmission->new_sync_point = sync;
		krad_transmission->new_data = 1;
		return ret;
	}
}

krad_transmission_t *krad_transmitter_transmission_create (krad_transmitter_t *krad_transmitter, char *name, char *content_type) {

	int t;

	t = 0;
	for (t = 0; t < DEFAULT_MAX_TRANSMISSIONS; t++) {
		if (krad_transmitter->krad_transmissions[t].active == 0) {
			krad_transmitter->krad_transmissions[t].active = 2;
			krad_transmitter->krad_transmissions[t].ready_receivers = NULL;	
			krad_transmitter->krad_transmissions[t].ready_receiver_count = 0;
			krad_transmitter->krad_transmissions[t].ready = 0;
			krad_transmitter->krad_transmissions[t].position = 0;
			krad_transmitter->krad_transmissions[t].sync_point = -1;
			strcpy (krad_transmitter->krad_transmissions[t].sysname, name);
			strcpy (krad_transmitter->krad_transmissions[t].content_type, content_type);
			
			krad_transmitter->krad_transmissions[t].http_header_len = sprintf (krad_transmitter->krad_transmissions[t].http_header,
																			   "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nServer: %s\r\n"
																				"Cache-Control: no-cache\r\n\r\n",
																			   krad_transmitter->krad_transmissions[t].content_type,
																			   KRAD_TRANSMITTER_SERVER
																			   );

			printk ("Krad Transmitter: http header for %s is %d bytes: %s",
					krad_transmitter->krad_transmissions[t].sysname,
					krad_transmitter->krad_transmissions[t].http_header_len,
					krad_transmitter->krad_transmissions[t].http_header);
			
			krad_transmitter->krad_transmissions[t].connections_efd = epoll_create1 (0);
			krad_transmitter->krad_transmissions[t].transmission_events = calloc (KRAD_TRANSMITTER_MAXEVENTS, sizeof (struct epoll_event));
			krad_transmitter->krad_transmissions[t].ringbuffer = krad_ringbuffer_create (DEFAULT_RING_SIZE);

			if (krad_transmitter->krad_transmissions[t].ringbuffer == NULL) {
				failfast ("Krad Transmitter: Out of memory creating new transmission");
			}
			
			krad_transmitter->krad_transmissions[t].transmission_ringbuffer = krad_ringbuffer_create (DEFAULT_RING_SIZE);

			if (krad_transmitter->krad_transmissions[t].transmission_ringbuffer == NULL) {
				failfast ("Krad Transmitter: Out of memory creating new transmission");
			}

			krad_transmitter->krad_transmissions[t].transmission_buffer = calloc (1, DEFAULT_RING_SIZE);

			if (krad_transmitter->krad_transmissions[t].transmission_buffer == NULL) {
				failfast ("Krad Transmitter: Out of memory creating new transmission");
			}

			pthread_create (&krad_transmitter->krad_transmissions[t].transmission_thread, NULL, krad_transmitter_transmission_thread, (void *)&krad_transmitter->krad_transmissions[t]);

			krad_transmitter->krad_transmissions[t].active = 1;			
			return &krad_transmitter->krad_transmissions[t];
		}
	}

	return NULL;

}


void krad_transmitter_transmission_destroy (krad_transmission_t *krad_transmission) {

	int r;

	r = 0;
	
	krad_transmission->active = 2;			

	pthread_join (krad_transmission->transmission_thread, NULL);	

	krad_transmission->active = 4;

	for (r = 0; r < TOTAL_RECEIVERS; r++) {
		if ((krad_transmission->krad_transmitter->krad_transmission_receivers[r].active == 1) && (krad_transmission->krad_transmitter->krad_transmission_receivers[r].krad_transmission == krad_transmission)) {
			krad_transmitter_receiver_destroy (&krad_transmission->krad_transmitter->krad_transmission_receivers[r]);
		}
	}

	memset (krad_transmission->sysname, '\0', sizeof(krad_transmission->sysname));
	memset (krad_transmission->content_type, '\0', sizeof(krad_transmission->content_type));
	memset (krad_transmission->http_header, '\0', sizeof(krad_transmission->http_header));
	
	free (krad_transmission->header);

	krad_transmission->http_header_len = 0;
	krad_transmission->header_len = 0;
	krad_transmission->ready_receivers = NULL;	
	krad_transmission->ready_receiver_count = 0;
	krad_transmission->sync_point = -1;
	krad_transmission->ready = 0;
	krad_transmission->position = 0;
	krad_ringbuffer_free (krad_transmission->ringbuffer);	
	krad_ringbuffer_free (krad_transmission->transmission_ringbuffer);
	free (krad_transmission->transmission_buffer);
	close (krad_transmission->connections_efd);
	free (krad_transmission->transmission_events);
	krad_transmission->active = 0;
	
}


krad_transmission_receiver_t *krad_transmitter_receiver_create (krad_transmitter_t *krad_transmitter, int fd) {

	int r;

	r = 0;

	for (r = 0; r < TOTAL_RECEIVERS; r++) {
		if (krad_transmitter->krad_transmission_receivers[r].active == 0) {
			krad_transmitter->krad_transmission_receivers[r].active = 1;
			krad_transmitter->krad_transmission_receivers[r].position = 0;
			krad_transmitter->krad_transmission_receivers[r].http_header_position = 0;
			krad_transmitter->krad_transmission_receivers[r].header_position = 0;
			krad_transmitter->krad_transmission_receivers[r].fd = fd;
			krad_transmitter->krad_transmission_receivers[r].event.data.ptr = &krad_transmitter->krad_transmission_receivers[r];
			krad_transmitter->krad_transmission_receivers[r].event.events = EPOLLIN | EPOLLET;
			return &krad_transmitter->krad_transmission_receivers[r];
		}
	}

	return NULL;
}

void krad_transmitter_receiver_destroy (krad_transmission_receiver_t *krad_transmission_receiver) {

	if (krad_transmission_receiver->ready == 1) {
		krad_transmission_remove_ready (krad_transmission_receiver->krad_transmission, krad_transmission_receiver);
	}
	
	if (krad_transmission_receiver->krad_transmission != NULL) {
		if (krad_transmission_receiver->noob == 0) {
			krad_transmission_receiver->krad_transmission->receivers--;
		}
		krad_transmission_receiver->krad_transmission = NULL;
	}

	krad_transmission_receiver->active = 2;
	if (krad_transmission_receiver->fd != 0) {
		close (krad_transmission_receiver->fd);
		krad_transmission_receiver->fd = 0;
	}
	krad_transmission_receiver->position = 0;
	krad_transmission_receiver->wrote_http_header = 0;
	krad_transmission_receiver->wrote_header = 0;
	krad_transmission_receiver->http_header_position = 0;
	krad_transmission_receiver->header_position = 0;
	memset (krad_transmission_receiver->buffer, 0, sizeof(krad_transmission_receiver->buffer));
	krad_transmission_receiver->active = 0;

}

void krad_transmitter_receiver_attach (krad_transmission_receiver_t *krad_transmission_receiver, char *request) {
	
	int t;
	int eret;

	eret = 0;
	t = 0;
			
	printk ("Krad Transmitter: request was for %s", request);
	pthread_rwlock_rdlock (&krad_transmission_receiver->krad_transmitter->krad_transmissions_rwlock);
	
	for (t = 0; t < DEFAULT_MAX_TRANSMISSIONS; t++) {
		if ((krad_transmission_receiver->krad_transmitter->krad_transmissions[t].active == 1) && (krad_transmission_receiver->krad_transmitter->krad_transmissions[t].ready == 1)) {
			if (strcmp (krad_transmission_receiver->krad_transmitter->krad_transmissions[t].sysname, request) == 0) {
				printk ("Krad Transmitter: request was for %s WAS FOUND!", request);	
				krad_transmission_receiver->krad_transmission = &krad_transmission_receiver->krad_transmitter->krad_transmissions[t];
				krad_transmission_receiver->noob = 1;
				eret = epoll_ctl (krad_transmission_receiver->krad_transmitter->incoming_connections_efd, EPOLL_CTL_DEL, krad_transmission_receiver->fd, NULL);
				if (eret != 0) {
					failfast ("Krad Transmitter: incoming transmitter connection epoll error eret is %d errno is %i", eret, errno);
				}
				krad_transmission_receiver->event.events = EPOLLRDHUP | EPOLLOUT | EPOLLET;
				eret = epoll_ctl (krad_transmission_receiver->krad_transmission->connections_efd, EPOLL_CTL_ADD, krad_transmission_receiver->fd, &krad_transmission_receiver->event);
				if (eret != 0) {
					failfast ("Krad Transmitter: incoming transmitter connection epoll error eret is %d errno is %i", eret, errno);
				}
				
				pthread_rwlock_unlock (&krad_transmission_receiver->krad_transmitter->krad_transmissions_rwlock);
				return;
			}
		}
	}
	
	printk ("Krad Transmitter: request was for %s NOT FOUND", request);
	pthread_rwlock_unlock (&krad_transmission_receiver->krad_transmitter->krad_transmissions_rwlock);
	krad_transmitter_receiver_destroy (krad_transmission_receiver);
}

void krad_transmitter_handle_incoming_connection (krad_transmitter_t *krad_transmitter,
												  krad_transmission_receiver_t *krad_transmission_receiver) {

	int b;
	int r;
	char request[256];

	b = 0;
	r = 0;
	
	printk ("Krad Transmitter: incoming_connection buffer at %d bytes", krad_transmission_receiver->position);

	for (b = 0; b < krad_transmission_receiver->position; b++) {

		if ((krad_transmission_receiver->buffer[b] == '\n') || (krad_transmission_receiver->buffer[b] == '\r')) {
			printk ("Krad Transmitter: incoming_connection got req line %d long", b);
			
			krad_transmission_receiver->buffer[b] = '\0';
			
			if ((b < 6) || (memcmp(krad_transmission_receiver->buffer, "GET /", 5) != 0)) {
				krad_transmitter_receiver_destroy (krad_transmission_receiver);
				return;
			}
			
			for (r = 0; r < b; r++) {

				request[r] = krad_transmission_receiver->buffer[r + 5];
				
				if ((request[r] == ' ') || (request[r] == '\n') || (request[r] == '\r')) {
					request[r] = '\0';
					break;
				}
			}
			request[r] = '\0';
			krad_transmission_receiver->position = 0;
			krad_transmitter_receiver_attach (krad_transmission_receiver, request);
			
			break;
		}
	}
}


void *krad_transmitter_listening_thread (void *arg) {

	krad_transmitter_t *krad_transmitter = (krad_transmitter_t *)arg;

	krad_system_set_thread_name ("kr_tx_listen");

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
			if ((ret < 0) && (errno == EINTR)) {
				continue;
			}
			printke ("Krad Transmitter: Failed on epoll wait %s", strerror(errno));
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

						krad_system_set_socket_nonblocking (client_fd);

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
									 krad_transmission_receiver->buffer + krad_transmission_receiver->position,
									 sizeof (krad_transmission_receiver->buffer) -  krad_transmission_receiver->position);
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
							krad_transmission_receiver->position += cret;
							krad_transmitter_handle_incoming_connection (krad_transmitter, krad_transmission_receiver);
							break;
						}
					}
				}
			}
		}
		
		if (ret == 0) {
			//printk ("Krad Transmitter: Listening thread... nothing happened");
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
	int r;
	int on;
	
	on = 1;
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
		printke ("Krad Transmitter: system call socket error");
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;		
		return -1;
	}
	
	if ((setsockopt (krad_transmitter->incoming_connections_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0) {
		printke ("Krad Transmitter: Could not setsockopt SO_REUSEADDR");
	}

	if (bind (krad_transmitter->incoming_connections_sd, (struct sockaddr *)&krad_transmitter->local_address,
		sizeof(krad_transmitter->local_address)) == -1) {
		
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;
		close (krad_transmitter->incoming_connections_sd);		
		printke ("Krad Transmitter: bind error for tcp port %d", krad_transmitter->port);		
		return -1;
	}
	
	printk ("Krad Transmitter: Listening on port %d", krad_transmitter->port);	
	
	if (listen (krad_transmitter->incoming_connections_sd, SOMAXCONN) < 0) {
		printke ("Krad Transmitter: system call listen error");
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;
		close (krad_transmitter->incoming_connections_sd);
		return -1;
	}

	krad_system_set_socket_nonblocking (krad_transmitter->incoming_connections_sd);	

	krad_transmitter->incoming_connections_efd = epoll_create1 (0);
	
	if (krad_transmitter->incoming_connections_efd == -1) {
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;
		close (krad_transmitter->incoming_connections_sd);
		close (krad_transmitter->incoming_connections_efd);		
		printke ("Krad Transmitter: epoll_create");
		return -1;
	}

	krad_transmitter->event.data.fd = krad_transmitter->incoming_connections_sd;
	krad_transmitter->event.events = EPOLLIN | EPOLLET;
	
	ret = epoll_ctl (krad_transmitter->incoming_connections_efd, EPOLL_CTL_ADD,
					 krad_transmitter->incoming_connections_sd, &krad_transmitter->event);
	if (ret == -1) {
		printke ("Krad Transmitter: epoll_ctl");
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;
		close (krad_transmitter->incoming_connections_sd);
		close (krad_transmitter->incoming_connections_efd);
		return -1;
	}

	krad_transmitter->incoming_connection_events = calloc (KRAD_TRANSMITTER_MAXEVENTS, sizeof (struct epoll_event));

	if (krad_transmitter->incoming_connection_events == NULL) {
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;
		close (krad_transmitter->incoming_connections_sd);
		close (krad_transmitter->incoming_connections_efd);
		printke ("Krad Transmitter: Out of memory!");
		return -1;
	}

	krad_transmitter->krad_transmission_receivers = calloc (TOTAL_RECEIVERS, sizeof (krad_transmission_receiver_t));
	
	if (krad_transmitter->krad_transmission_receivers == NULL) {
		free (krad_transmitter->incoming_connection_events);
		krad_transmitter->listening = 0;
		krad_transmitter->port = 0;
		close (krad_transmitter->incoming_connections_sd);
		close (krad_transmitter->incoming_connections_efd);
		printke ("Krad Transmitter: Out of memory!");
		return -1;
	}
	
	for (r = 0; r < TOTAL_RECEIVERS; r++) {
		krad_transmitter->krad_transmission_receivers[r].krad_transmitter = krad_transmitter;
	}	
	
	pthread_create (&krad_transmitter->listening_thread, NULL, krad_transmitter_listening_thread, (void *)krad_transmitter);

	return 0;

}

krad_transmitter_t *krad_transmitter_create () {

	int t;
	krad_transmitter_t *krad_transmitter;

	t = 0;
	krad_transmitter = NULL;
	
	krad_transmitter = calloc (1, sizeof(krad_transmitter_t));

	if (krad_transmitter == NULL) {
		printke ("Krad Transmitter: Out of memory!");
		return NULL;		
	}
	
	krad_transmitter->krad_transmissions = calloc (DEFAULT_MAX_TRANSMISSIONS, sizeof(krad_transmission_t));

	if (krad_transmitter->krad_transmissions == NULL) {
		free (krad_transmitter);
		printke ("Krad Transmitter: Out of memory!");
		return NULL;
	}
	
	for (t = 0; t < DEFAULT_MAX_TRANSMISSIONS; t++) {
		krad_transmitter->krad_transmissions[t].krad_transmitter = krad_transmitter;
	}
	
	pthread_rwlock_init (&krad_transmitter->krad_transmissions_rwlock, NULL);

	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "HTTP/1.1 404 Not Found\r\n");
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "Status: 404 Not Found\r\n");
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "Connection: close\r\n");
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "Content-Type: text/html; charset=utf-8\r\n");
	krad_transmitter->not_found_len += sprintf (krad_transmitter->not_found + krad_transmitter->not_found_len, "\r\n404 Not Found");

	return krad_transmitter;

}

void krad_transmitter_destroy (krad_transmitter_t *krad_transmitter) {

	int t;
	
	t = 0;

	if (krad_transmitter->listening == 1) {
		krad_transmitter_stop_listening (krad_transmitter);
	}
	
	pthread_rwlock_wrlock (&krad_transmitter->krad_transmissions_rwlock);		
	
	for (t = 0; t < DEFAULT_MAX_TRANSMISSIONS; t++) {
		if (krad_transmitter->krad_transmissions[t].active == 1) {
			krad_transmitter_transmission_destroy (&krad_transmitter->krad_transmissions[t]);
		}
	}
	free (krad_transmitter->krad_transmission_receivers);
	free (krad_transmitter->krad_transmissions);
	
	pthread_rwlock_unlock (&krad_transmitter->krad_transmissions_rwlock);	
	pthread_rwlock_destroy (&krad_transmitter->krad_transmissions_rwlock);
	
	free (krad_transmitter);
	
	printk ("Krad Transmitter: Destroyed!");

}

