#include "krad_ipc_server.h"

static krad_ipc_server_t *krad_ipc_server_init (char *appname, char *sysname);
static void *krad_ipc_server_run_thread (void *arg);
static int krad_ipc_server_tcp_socket_create_and_bind (char *interface, int port);
static void krad_ipc_disconnect_client (krad_ipc_server_client_t *client);
static void krad_ipc_server_update_pollfds (krad_ipc_server_t *krad_ipc_server);
static krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server, int sd);

static krad_ipc_broadcaster_t *krad_ipc_server_broadcaster_create ( krad_ipc_server_t *ipc_server );
static int krad_ipc_server_broadcaster_destroy ( krad_ipc_broadcaster_t **broadcaster );

static krad_ipc_server_t *krad_ipc_server_init (char *appname, char *sysname) {

  krad_ipc_server_t *krad_ipc_server = calloc (1, sizeof (krad_ipc_server_t));
  int i;

  if (krad_ipc_server == NULL) {
    return NULL;
  }
  
  if (krad_control_init (&krad_ipc_server->krad_control)) {
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
    printke ("Krad IPC Server: Socket failed.\n");
    krad_ipc_server_destroy (krad_ipc_server);
    return NULL;
  }

  krad_ipc_server->saddr.sun_family = AF_UNIX;

  if (krad_ipc_server->on_linux) {
    snprintf (krad_ipc_server->saddr.sun_path, sizeof (krad_ipc_server->saddr.sun_path), "@%s_%s_ipc", appname, sysname);  
    krad_ipc_server->saddr.sun_path[0] = '\0';
    if (connect (krad_ipc_server->sd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) != -1) {
      /* active socket already exists! */
      printke ("Krad IPC Server: Krad IPC path in use. (linux abstract)\n");
      krad_ipc_server_destroy (krad_ipc_server);
      return NULL;
    }
  } else {
    snprintf (krad_ipc_server->saddr.sun_path, sizeof (krad_ipc_server->saddr.sun_path), "%s/%s_%s_ipc", "/tmp", appname, sysname);  
    if (access (krad_ipc_server->saddr.sun_path, F_OK) == 0) {
      if (connect (krad_ipc_server->sd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) != -1) {
        /* active socket already exists! */
        printke ("Krad IPC Server: Krad IPC path in use.\n");
        krad_ipc_server_destroy (krad_ipc_server);
        return NULL;
      }
      /* remove stale socket */
      unlink (krad_ipc_server->saddr.sun_path);
    }
  }
  
  if (bind (krad_ipc_server->sd, (struct sockaddr *) &krad_ipc_server->saddr, sizeof (krad_ipc_server->saddr)) == -1) {
    printke ("Krad IPC Server: Can't bind.\n");
    krad_ipc_server_destroy (krad_ipc_server);
    return NULL;
  }

  listen (krad_ipc_server->sd, SOMAXCONN);

  krad_ipc_server->flags = fcntl (krad_ipc_server->sd, F_GETFL, 0);

  if (krad_ipc_server->flags == -1) {
    krad_ipc_server_destroy (krad_ipc_server);
    return NULL;
  }
/*
  krad_ipc_server->flags |= O_NONBLOCK;

  krad_ipc_server->flags = fcntl (krad_ipc_server->sd, F_SETFL, krad_ipc_server->flags);
  if (krad_ipc_server->flags == -1) {
    krad_ipc_server_destroy (krad_ipc_server);
    return NULL;
  }
*/  
  return krad_ipc_server;

}

static int krad_ipc_server_tcp_socket_create_and_bind (char *interface, int port) {

  char port_string[6];
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s;
  int sfd = 0;
  
  printk ("Krad IPC Server: interface: %s port %d", interface, port);

  snprintf (port_string, 6, "%d", port);

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  s = getaddrinfo (interface, port_string, &hints, &result);
  if (s != 0) {
    printke ("getaddrinfo: %s\n", gai_strerror (s));
    return -1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    
    sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    
    if (sfd == -1) {
      continue;
    }

    s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
    
    if (s == 0) {
      /* We managed to bind successfully! */
      break;
    }

    close (sfd);
  }

  if (rp == NULL) {
    printke ("Could not bind %d\n", port);
    return -1;
  }

  freeaddrinfo (result);

  return sfd;
}

int krad_ipc_server_recvfd (krad_ipc_server_client_t *client) {

  int n;
  int fd;
  int ret;
  struct pollfd sockets[1];
  char buf[1];
  struct iovec iov;
  struct msghdr msg;
  struct cmsghdr *cmsg;
  char cms[CMSG_SPACE(sizeof(int))];

  sockets[0].fd = client->sd;
  sockets[0].events = POLLIN;

  ret = poll (sockets, 1, KRAD_IPC_SERVER_TIMEOUT_MS);

  if (ret > 0) {

    iov.iov_base = buf;
    iov.iov_len = 1;

    memset(&msg, 0, sizeof msg);
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = (caddr_t)cms;
    msg.msg_controllen = sizeof cms;

    if((n=recvmsg(client->sd, &msg, 0)) < 0)
        return -1;
    if(n == 0){
        printke("krad_ipc_server_recvfd: unexpected EOF");
        return 0;
    }
    cmsg = CMSG_FIRSTHDR(&msg);
    memmove(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
  } else {
    return 0;
  }
}

void krad_ipc_server_disable_remote (krad_ipc_server_t *krad_ipc_server, char *interface, int port) {

  //FIXME needs to loop thru clients and disconnect remote ones .. optionally?

  int r;
  int d;
  
  d = 0;
  
  if ((interface == NULL) || (strlen(interface) == 0)) {
    interface = "All";
  }

  for (r = 0; r < MAX_REMOTES; r++) {
    if ((krad_ipc_server->tcp_sd[r] != 0) && ((port == 0) || (krad_ipc_server->tcp_port[r] == port))) {
      close (krad_ipc_server->tcp_sd[r]);
      krad_ipc_server->tcp_port[r] = 0;
      krad_ipc_server->tcp_sd[r] = 0;
      free (krad_ipc_server->tcp_interface[r]);
      d++;
    }
  }

  if (d > 0) {
    printk ("Disable remote on interface %s port %d", interface, port);
    krad_ipc_server_update_pollfds (krad_ipc_server);
  }
}

int krad_ipc_server_enable_remote (krad_ipc_server_t *krad_ipc_server, char *interface, int port) {

  int r;
  int sd;
  
  sd = 0;
  
  //FIXME handle when an adapter such as eth0 is specified or when interface is null
  // bind to all ips on that port
  
  if ((interface == NULL) || (strlen(interface) == 0)) {
    interface = NULL;
    printk ("Krad IPC Server: interface not specified, we should probably bind to all ips on this port");
  } else {
    if (krad_system_is_adapter (interface)) {
      printk ("Krad IPC Server: its an adapter, we should probably bind to all ips of this adapter");
      return -1;
    }
  }

  for (r = 0; r < MAX_REMOTES; r++) {
    if ((krad_ipc_server->tcp_sd[r] != 0) && (krad_ipc_server->tcp_port[r] == port)) {
      return 0;
    }
  }  
  
  for (r = 0; r < MAX_REMOTES; r++) {
    if ((krad_ipc_server->tcp_sd[r] == 0) && (krad_ipc_server->tcp_port[r] == 0)) {
    
      sd = krad_ipc_server_tcp_socket_create_and_bind (interface, port);
      
      if (sd < 0) {
        return 0;
      }
      
      krad_ipc_server->tcp_port[r] = port;
      krad_ipc_server->tcp_sd[r] = sd;

      if (krad_ipc_server->tcp_sd[r] != 0) {
        listen (krad_ipc_server->tcp_sd[r], SOMAXCONN);
        krad_ipc_server_update_pollfds (krad_ipc_server);
        if (interface == NULL) {
          krad_ipc_server->tcp_interface[r] = strdup ("");
        } else {
          krad_ipc_server->tcp_interface[r] = strdup (interface);
        }
        printk ("Enable remote on interface %s port %d", interface, port);
        return 1;
      } else {
        krad_ipc_server->tcp_port[r] = 0;
        return 0;
      }
    }
  }

  return 0;

}

static krad_ipc_server_client_t *krad_ipc_server_accept_client (krad_ipc_server_t *krad_ipc_server, int sd) {

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
      //printk ("Krad IPC Server: Overloaded with clients!\n");
      return NULL;
    }
  }

  sin_len = sizeof (sin);
  client->sd = accept (sd, (struct sockaddr *)&sin, &sin_len);

  if (client->sd >= 0) {

    flags = fcntl (client->sd, F_GETFL, 0);

    if (flags == -1) {
      close (client->sd);
      return NULL;
    }
/*
    flags |= O_NONBLOCK;

    flags = fcntl (client->sd, F_SETFL, flags);
    if (flags == -1) {
      close (client->sd);
      return NULL;
    }
*/
    client->active = 1;
    client->krad_ebml = krad_ebml_open_buffer (KRAD_EBML_IO_READONLY);
    krad_ipc_server_update_pollfds (client->krad_ipc_server);
    //printk ("Krad IPC Server: Client accepted!");  
    return client;
  }

  return NULL;
}

static void krad_ipc_disconnect_client (krad_ipc_server_client_t *client) {

  close (client->sd);
  
  if (client->krad_ebml != NULL) {
    krad_ebml_destroy (client->krad_ebml);
    client->krad_ebml = NULL;
  }
  
  if (client->krad_ebml2 != NULL) {
    krad_ebml_destroy (client->krad_ebml2);
    client->krad_ebml2 = NULL;
  }
  client->input_buffer_pos = 0;
  client->output_buffer_pos = 0;
  client->broadcasts = 0;
  client->confirmed = 0;
  memset (client->input_buffer, 0, sizeof(client->input_buffer));
  memset (client->output_buffer, 0, sizeof(client->output_buffer));
  client->active = 0;

  krad_ipc_server_update_pollfds (client->krad_ipc_server);
  //printk ("Krad IPC Server: Client Disconnected");

}

static void krad_ipc_server_update_pollfds (krad_ipc_server_t *krad_ipc_server) {

  int r;
  int b;
  int c;
  int s;

  s = 0;

  krad_ipc_server->sockets[s].fd = krad_controller_get_client_fd (&krad_ipc_server->krad_control);
  krad_ipc_server->sockets[s].events = POLLIN;

  s++;
  
  krad_ipc_server->sockets[s].fd = krad_ipc_server->sd;
  krad_ipc_server->sockets[s].events = POLLIN;

  s++;
  
  for (r = 0; r < MAX_REMOTES; r++) {
    if (krad_ipc_server->tcp_sd[r] != 0) {
      krad_ipc_server->sockets[s].fd = krad_ipc_server->tcp_sd[r];
      krad_ipc_server->sockets[s].events = POLLIN;
      s++;
    }
  }
  
  for (b = 0; b < MAX_BROADCASTERS + MAX_REMOTES + 2; b++) {
    krad_ipc_server->sockets_broadcasters[b] = NULL;
  }
  
  for (b = 0; b < MAX_BROADCASTERS; b++) {
    if (krad_ipc_server->broadcasters[b] != NULL) {
      krad_ipc_server->sockets[s].fd = krad_ipc_server->broadcasters[b]->sockets[1];
      krad_ipc_server->sockets[s].events |= POLLIN;
      krad_ipc_server->sockets_broadcasters[s] = krad_ipc_server->broadcasters[b];
      s++;
    }
  }

  for (c = 0; c < KRAD_IPC_SERVER_MAX_CLIENTS; c++) {
    if (krad_ipc_server->clients[c].active == 1) {
      krad_ipc_server->sockets[s].fd = krad_ipc_server->clients[c].sd;
      krad_ipc_server->sockets[s].events |= POLLIN;
      krad_ipc_server->sockets_clients[s] = &krad_ipc_server->clients[c];
      s++;
    }
  }
  
  krad_ipc_server->socket_count = s;

  //printk ("Krad IPC Server: sockets rejiggerd!\n");  

}

int krad_ipc_server_read_command (krad_ipc_server_t *krad_ipc_server, uint32_t *ebml_id_ptr, uint64_t *ebml_data_size_ptr) {
  return krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, ebml_id_ptr, ebml_data_size_ptr);
}

uint64_t krad_ipc_server_read_number (krad_ipc_server_t *krad_ipc_server, uint64_t data_size) {
  return krad_ebml_read_number (krad_ipc_server->current_client->krad_ebml, data_size);
}

void krad_ipc_server_read_tag ( krad_ipc_server_t *krad_ipc_server, char **tag_item, char **tag_name, char **tag_value ) {

  uint32_t ebml_id;
  uint64_t ebml_data_size;

  krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);
  
  if (ebml_id != EBML_ID_KRAD_RADIO_TAG) {
    printke ("hrm wtf\n");
  } else {
    //printf("tag size %zu\n", ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
    printke ("hrm wtf2\n");
  } else {
    //printf("tag name size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, *tag_item, ebml_data_size);  
  
  krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
    printke ("hrm wtf2\n");
  } else {
    //printf("tag name size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, *tag_name, ebml_data_size);
  
  krad_ebml_read_element (krad_ipc_server->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
    printke ("hrm wtf3\n");
  } else {
    //printf("tag value size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (krad_ipc_server->current_client->krad_ebml, *tag_value, ebml_data_size);
}

void krad_ipc_server_response_start ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t *response) {
  krad_ebml_start_element (krad_ipc_server->current_client->krad_ebml2, ebml_id, response);
}

void krad_ipc_server_response_finish ( krad_ipc_server_t *krad_ipc_server, uint64_t response) {
  krad_ebml_finish_element (krad_ipc_server->current_client->krad_ebml2, response);
}

void krad_ipc_server_response_list_start ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, uint64_t *list) {
  krad_ebml_start_element (krad_ipc_server->current_client->krad_ebml2, ebml_id, list);
}

void krad_ipc_server_response_add_tag ( krad_ipc_server_t *krad_ipc_server, char *tag_item, char *tag_name, char *tag_value) {

  uint64_t tag;

  krad_ebml_start_element (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_RADIO_TAG, &tag);  
  krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_RADIO_TAG_ITEM, tag_item);
  krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
  krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_RADIO_TAG_VALUE, tag_value);  
  //krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, EBML_ID_KRAD_RADIO_TAG_SOURCE, "");
  krad_ebml_finish_element (krad_ipc_server->current_client->krad_ebml2, tag);

}

void krad_ipc_server_response_list_finish ( krad_ipc_server_t *krad_ipc_server, uint64_t list) {
  krad_ebml_finish_element (krad_ipc_server->current_client->krad_ebml2, list);
}

void krad_ipc_server_respond_number ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, int32_t number) {
  krad_ebml_write_int32 (krad_ipc_server->current_client->krad_ebml2, ebml_id, number);
}

void krad_ipc_server_respond_string ( krad_ipc_server_t *krad_ipc_server, uint32_t ebml_id, char *string) {
  krad_ebml_write_string (krad_ipc_server->current_client->krad_ebml2, ebml_id, string);
}

static krad_ipc_broadcaster_t *krad_ipc_server_broadcaster_create ( krad_ipc_server_t *ipc_server ) {

  krad_ipc_broadcaster_t *broadcaster;
  
  if (ipc_server == NULL) {
    return NULL;
  }
  
  broadcaster = calloc (1, sizeof(krad_ipc_broadcaster_t));
  broadcaster->ipc_server = ipc_server;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, broadcaster->sockets)) {
    printke ("Krad IPC Server: can't socketpair errno: %d", errno);
    free (broadcaster);
    return NULL;
  }

  broadcaster->msg_ring = krad_ringbuffer_create ( 200 * sizeof(krad_broadcast_msg_t *) );

  return broadcaster;
}

static int krad_ipc_server_broadcaster_destroy ( krad_ipc_broadcaster_t **broadcaster ) {
  if (*broadcaster != NULL) {

    close ((*broadcaster)->sockets[0]);
    close ((*broadcaster)->sockets[1]);

    if ((*broadcaster)->msg_ring != NULL) {
      krad_ringbuffer_free ( (*broadcaster)->msg_ring );
      (*broadcaster)->msg_ring = NULL;
    }
    free (*broadcaster);
    *broadcaster = NULL;
    return 1;
  }
  return -1;
}

int krad_ipc_server_broadcaster_unregister ( krad_ipc_broadcaster_t **broadcaster ) {
  return 1;
}

krad_ipc_broadcaster_t *krad_ipc_server_broadcaster_register ( krad_ipc_server_t *ipc_server ) {

  int b;

  if (ipc_server == NULL) {
    return NULL;
  }
  if (ipc_server->broadcasters_count == MAX_BROADCASTERS) {
    return NULL;
  }

  for (b = 0; b < MAX_BROADCASTERS; b++) {
    if (ipc_server->broadcasters[b] == NULL) {
      ipc_server->broadcasters[b] = krad_ipc_server_broadcaster_create ( ipc_server );
      return ipc_server->broadcasters[b];
    }
  }

  return NULL;
}

int krad_broadcast_msg_destroy (krad_broadcast_msg_t **broadcast_msg) {
  
  if (*broadcast_msg != NULL) {
    if ((*broadcast_msg)->buffer != NULL) {
      free ( (*broadcast_msg)->buffer );
    }
    free (*broadcast_msg);
    *broadcast_msg = NULL;
    return 1;
  }
  return -1;
}

krad_broadcast_msg_t *krad_broadcast_msg_create (unsigned char *buffer, uint32_t size) {
  
  krad_broadcast_msg_t *broadcast_msg;
  
  if (size < 1) {
    return NULL;
  }
  
  broadcast_msg = calloc (1, sizeof(krad_broadcast_msg_t));

  broadcast_msg->size = size;  
  broadcast_msg->buffer = malloc (broadcast_msg->size);
  memcpy (broadcast_msg->buffer, buffer, broadcast_msg->size);
  
  return broadcast_msg;
}



int krad_ipc_server_broadcaster_broadcast ( krad_ipc_broadcaster_t *broadcaster, krad_broadcast_msg_t **broadcast_msg ) {

  struct pollfd pollfds[1];
  int ret;
  int timeout;
  
  timeout = 2;

  ret = krad_ringbuffer_write ( broadcaster->msg_ring, (char *)broadcast_msg, sizeof(krad_broadcast_msg_t *) );
  if (ret != sizeof(krad_broadcast_msg_t *)) {
    printke ("Krad IPC Server: invalid broadcast msg write len %d... broadcast ringbuffer full", ret);
    return -1;
  }


  pollfds[0].fd = broadcaster->sockets[0];
  pollfds[0].events = POLLOUT;
  if (poll (pollfds, 1, timeout) == 1) {
    ret = write (broadcaster->sockets[0], "0", 1);
    if (ret == 1) {
      return 1;
    } else {
      printke ("Krad IPC Server: some error broadcasting: %d", ret);
    }
  }

  return 0;

}

void krad_ipc_server_broadcaster_register_broadcast ( krad_ipc_broadcaster_t *broadcaster, uint32_t broadcast_ebml_id ) {

  broadcaster->ipc_server->broadcasts[broadcaster->ipc_server->broadcasts_count] = broadcast_ebml_id;
  broadcaster->ipc_server->broadcasts_count++;

}

void krad_ipc_server_add_client_to_broadcast ( krad_ipc_server_t *krad_ipc_server, uint32_t broadcast_ebml_id ) {

  if (broadcast_ebml_id == EBML_ID_KRAD_RADIO_GLOBAL_BROADCAST) {
    printk("client subscribing to global broadcasts");
  }

  krad_ipc_server->current_client->broadcasts = 1;
}

int handle_broadcast (krad_ipc_broadcaster_t *broadcaster) {
  
  int b;
  int c;
  int ret;
  char buf[1];
  krad_broadcast_msg_t *broadcast_msg;
  
  b = 0;

  ret = read (broadcaster->sockets[1], buf, 1);

  if (ret != 1) {
    printke ("Error handling broadcast");
    return -1;
  }

  ret = krad_ringbuffer_read ( broadcaster->msg_ring, (char *)&broadcast_msg, sizeof(krad_broadcast_msg_t *) );
  if (ret != sizeof(krad_broadcast_msg_t *)) {
    printke ("Krad IPC Server: invalid broadcast msg read len %d", ret);
    return -1;
  }

  for (c = 0; c < KRAD_IPC_SERVER_MAX_CLIENTS; c++) {
    if (broadcaster->ipc_server->clients[c].broadcasts == 1) {
      krad_ebml_write (broadcaster->ipc_server->clients[c].krad_ebml2, broadcast_msg->buffer, broadcast_msg->size);
      krad_ebml_write_sync (broadcaster->ipc_server->clients[c].krad_ebml2);
      b++;
    }
  }
  
  //printk ("Krad IPC Server: Broadcasted to %d", b);  
  
  krad_broadcast_msg_destroy (&broadcast_msg);
  
  return b;
}


static void *krad_ipc_server_run_thread (void *arg) {

  krad_ipc_server_t *krad_ipc_server = (krad_ipc_server_t *)arg;
  krad_ipc_server_client_t *client;
  int ret, s, r;
  
  krad_system_set_thread_name ("kr_ipc_server");
  
  krad_ipc_server->shutdown = KRAD_IPC_RUNNING;
  
  krad_ipc_server_update_pollfds (krad_ipc_server);

  while (!krad_ipc_server->shutdown) {

    ret = poll (krad_ipc_server->sockets, krad_ipc_server->socket_count, KRAD_IPC_SERVER_TIMEOUT_MS);

    if (ret > 0) {
    
      if (krad_ipc_server->shutdown) {
        break;
      }
  
      s = 0;

      if (krad_ipc_server->sockets[s].revents) {
        break;
      }
      
      s++;
  
      if (krad_ipc_server->sockets[s].revents & POLLIN) {
        krad_ipc_server_accept_client (krad_ipc_server, krad_ipc_server->sd);
        ret--;
      }
      
      s++;
      
      for (r = 0; r < MAX_REMOTES; r++) {
        if (krad_ipc_server->tcp_sd[r] != 0) {
          if ((krad_ipc_server->tcp_sd[r] != 0) && (krad_ipc_server->sockets[s].revents & POLLIN)) {
            krad_ipc_server_accept_client (krad_ipc_server, krad_ipc_server->tcp_sd[r]);
            ret--;
          }
          s++;
        }
      }
  
      for (; ret > 0; s++) {

        if (krad_ipc_server->sockets[s].revents) {
          
          ret--;
          
          
          if (krad_ipc_server->sockets_broadcasters[s] != NULL) {
            handle_broadcast ( krad_ipc_server->sockets_broadcasters[s] );
          } else {

            client = krad_ipc_server->sockets_clients[s];
          
            if (krad_ipc_server->sockets[s].revents & POLLIN) {
    
              client->input_buffer_pos += recv(krad_ipc_server->sockets[s].fd, client->input_buffer + client->input_buffer_pos, (sizeof (client->input_buffer) - client->input_buffer_pos), 0);
            
              //printk ("Krad IPC Server: Got %d bytes\n", client->input_buffer_pos);
            
              if (client->input_buffer_pos == 0) {
                //printk ("Krad IPC Server: Client EOF\n");
                krad_ipc_disconnect_client (client);
                continue;
              }

              if (client->input_buffer_pos == -1) {
                printke ("Krad IPC Server: Client Socket Error");
                krad_ipc_disconnect_client (client);
                continue;
              }
            
              // big enough to read element id and data size
              if ((client->input_buffer_pos > 7) && (client->confirmed == 0)) {
                krad_ebml_io_buffer_push(&client->krad_ebml->io_adapter, client->input_buffer, client->input_buffer_pos);
                client->input_buffer_pos = 0;
                
                krad_ebml_read_ebml_header (client->krad_ebml, client->krad_ebml->header);
                krad_ebml_check_ebml_header (client->krad_ebml->header);
                //krad_ebml_print_ebml_header (client->krad_ebml->header);
                
                if (krad_ebml_check_doctype_header (client->krad_ebml->header, KRAD_IPC_CLIENT_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION)) {
                  //printk ("Matched %s Version: %d Read Version: %d\n", KRAD_IPC_CLIENT_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
                  client->confirmed = 1;
                } else {
                  printke ("Did Not Match %s Version: %d Read Version: %d\n", KRAD_IPC_CLIENT_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
                  krad_ipc_disconnect_client (client);
                  continue;
                }
                
                
                client->krad_ebml2 = krad_ebml_open_active_socket (client->sd, KRAD_EBML_IO_READWRITE);

                krad_ebml_header_advanced (client->krad_ebml2, KRAD_IPC_SERVER_DOCTYPE, KRAD_IPC_DOCTYPE_VERSION, KRAD_IPC_DOCTYPE_READ_VERSION);
                krad_ebml_write_sync (client->krad_ebml2);
                
              } else {
                if ((client->input_buffer_pos > 3) && (client->confirmed == 1)) {
                  krad_ebml_io_buffer_push(&client->krad_ebml->io_adapter, client->input_buffer, client->input_buffer_pos);
                  client->input_buffer_pos = 0;
                }
              
                while (krad_ebml_io_buffer_read_space (&client->krad_ebml->io_adapter)) {
                  client->krad_ipc_server->current_client = client; /* single thread has a few perks */
                  client->krad_ipc_server->handler (client->output_buffer, &client->command_response_len, client->krad_ipc_server->pointer);
                  //printk ("Krad IPC Server: CMD Response %d %d bytes\n", resp, client->command_response_len);
                  krad_ebml_write_sync (krad_ipc_server->current_client->krad_ebml2);
                }
              }
            }
            
            if (krad_ipc_server->sockets[s].revents & POLLOUT) {
              //printk ("I could write\n");
            }

            if (krad_ipc_server->sockets[s].revents & POLLHUP) {
              //printk ("Krad IPC Server: POLLHUP\n");
              krad_ipc_disconnect_client (client);
              continue;
            }

            if (krad_ipc_server->sockets[s].revents & POLLERR) {
              printke ("Krad IPC Server: POLLERR\n");
              krad_ipc_disconnect_client (client);
              continue;
            }

            if (krad_ipc_server->sockets[s].revents & POLLNVAL) {
              printke ("Krad IPC Server: POLLNVAL\n");
              krad_ipc_disconnect_client (client);
              continue;
            }
          }
        }
      }
    }
  }

  krad_ipc_server->shutdown = KRAD_IPC_SHUTINGDOWN;

  krad_controller_client_close (&krad_ipc_server->krad_control);

  return NULL;

}

void krad_ipc_server_disable (krad_ipc_server_t *krad_ipc_server) {
  
  printk ("Krad IPC Server: Disable Started");  

  if (!krad_controller_shutdown (&krad_ipc_server->krad_control, &krad_ipc_server->server_thread, 30)) {
    krad_controller_destroy (&krad_ipc_server->krad_control, &krad_ipc_server->server_thread);
  }

  krad_ipc_server_disable_remote (krad_ipc_server, "", 0);

  if (krad_ipc_server->sd != 0) {
    close (krad_ipc_server->sd);
    if (!krad_ipc_server->on_linux) {
      unlink (krad_ipc_server->saddr.sun_path);
    }
  }
  
  printk ("Krad IPC Server: Disable Complete");    

}

void krad_ipc_server_destroy (krad_ipc_server_t *ipc_server) {

  int i;

  printk ("Krad IPC Server: Destroy Started");

  if (ipc_server->shutdown != KRAD_IPC_SHUTINGDOWN) {
    krad_ipc_server_disable (ipc_server);
  }
  
  for (i = 0; i < KRAD_IPC_SERVER_MAX_CLIENTS; i++) {
    if (ipc_server->clients[i].active == 1) {
      krad_ipc_disconnect_client (&ipc_server->clients[i]);
    }
  }
  
  for (i = 0; i < MAX_BROADCASTERS; i++) {
    if (ipc_server->broadcasters[i] != NULL) {
      krad_ipc_server_broadcaster_destroy ( &ipc_server->broadcasters[i] );
    }
  }  
  
  free (ipc_server->clients);
  free (ipc_server);
  
  printk ("Krad IPC Server: Destroy Completed");
  
}

void krad_ipc_server_run (krad_ipc_server_t *krad_ipc_server) {
  pthread_create (&krad_ipc_server->server_thread, NULL, krad_ipc_server_run_thread, (void *)krad_ipc_server);
}

krad_ipc_server_t *krad_ipc_server_create (char *appname, char *sysname, int handler (void *, int *, void *), void *pointer) {


  krad_ipc_server_t *krad_ipc_server;
  
  krad_ipc_server = krad_ipc_server_init (appname, sysname);

  if (krad_ipc_server == NULL) {
    return NULL;
  }
  
  krad_ipc_server->handler = handler;
  krad_ipc_server->pointer = pointer;

  return krad_ipc_server;  

}

