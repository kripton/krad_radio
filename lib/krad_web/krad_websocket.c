#include "krad_websocket.h"

static void json_to_krad_api (kr_ws_client_t *kr_ws_client, char *value, int len);
static int krad_api_handler (kr_ws_client_t *kr_ws_client);
static void rep_to_json (kr_ws_client_t *kr_ws_client, kr_rep_t *rep);
static void *krad_websocket_server_run (void *arg);
static void add_poll_fd (int fd, short events, int fd_is, kr_ws_client_t *kr_ws_client, void *bspointer);
static void del_poll_fd (int fd);

static int callback_http (struct libwebsocket_context *this,
                          struct libwebsocket *wsi,
                          enum libwebsocket_callback_reasons reason, void *user,
                          void *in, size_t len);

static int callback_kr_client (struct libwebsocket_context *this,
                               struct libwebsocket *wsi,
                               enum libwebsocket_callback_reasons reason, void *user,
                               void *in, size_t len);

struct libwebsocket_protocols protocols[] = {
  /* first protocol must always be HTTP handler */

  {
    "http-only",    /* name */
    callback_http,    /* callback */
    0      /* per_session_data_size */
  },
  {
    "krad-ws-api",
    callback_kr_client,
    sizeof(kr_ws_client_t),
  },
  {
    NULL, NULL, 0    /* End of list */
  }
};

/* blame libwebsockets */
krad_websocket_t *krad_websocket_glob;


/* interpret JSON to speak Krad API */

static void json_to_krad_api (kr_ws_client_t *kr_ws_client, char *value, int len) {
  
  float floatval;
  cJSON *cmd;
  cJSON *part;
  cJSON *part2;
  cJSON *part3;
  
  part = NULL;
  part2 = NULL;
  
  cmd = cJSON_Parse (value);
  
  if (!cmd) {
    printke ("Krad WebSocket: JSON Error before: [%s]", cJSON_GetErrorPtr());
  } else {

    part = cJSON_GetObjectItem (cmd, "com");
    
    if ((part != NULL) && (strcmp(part->valuestring, "kradmixer") == 0)) {
      part = cJSON_GetObjectItem (cmd, "cmd");    
      if ((part != NULL) && (strcmp(part->valuestring, "update_portgroup") == 0)) {
        part = cJSON_GetObjectItem (cmd, "portgroup_name");
        part2 = cJSON_GetObjectItem (cmd, "control_name");
        part3 = cJSON_GetObjectItem (cmd, "value");
        if ((part != NULL) && (part2 != NULL) && (part3 != NULL)) {
          if (strcmp(part2->valuestring, "xmms2") == 0) {
            kr_mixer_portgroup_xmms2_cmd (kr_ws_client->kr_client, part->valuestring, part3->valuestring);
          } else {
            floatval = part3->valuedouble;
            kr_mixer_set_control (kr_ws_client->kr_client, part->valuestring, part2->valuestring, floatval, 0);
          }
        }
      }
      
      if ((part != NULL) && (strcmp(part->valuestring, "push_dtmf") == 0)) {
        part = cJSON_GetObjectItem (cmd, "dtmf");
        if (part != NULL) {
          kr_mixer_push_tone (kr_ws_client->kr_client, part->valuestring);
        }
      }
    }
    
    if ((part != NULL) && (strcmp(part->valuestring, "kradcompositor") == 0)) {
      part = cJSON_GetObjectItem (cmd, "cmd");    
      if ((part != NULL) && (strcmp(part->valuestring, "jsnap") == 0)) {
        kr_compositor_snapshot_jpeg (kr_ws_client->kr_client);
      }  
      if ((part != NULL) && (strcmp(part->valuestring, "snap") == 0)) {
        kr_compositor_snapshot (kr_ws_client->kr_client);
      }
      if ((part != NULL) && (strcmp(part->valuestring, "setsprite") == 0)) {
      
        part2 = cJSON_GetObjectItem (cmd, "x");
        part3 = cJSON_GetObjectItem (cmd, "y");
      
        kr_compositor_set_sprite (kr_ws_client->kr_client, 0, part2->valueint, part3->valueint,  0, 4,
                                  1.0f, 1.0f, 0.0f);
      }
    }
  
    if ((part != NULL) && (strcmp(part->valuestring, "kradradio") == 0)) {
      part = cJSON_GetObjectItem (cmd, "cmd");    
      if ((part != NULL) && (strcmp(part->valuestring, "stag") == 0)) {
        part2 = cJSON_GetObjectItem (cmd, "tag_name");
        part3 = cJSON_GetObjectItem (cmd, "tag_value");
        if ((part != NULL) && (part2 != NULL) && (part3 != NULL)) {      
          kr_set_tag (kr_ws_client->kr_client, NULL, part2->valuestring, part3->valuestring);
          //printk("aye got %s %s", part2->valuestring, part3->valuestring);
        }
      }  
    }
    cJSON_Delete (cmd);
  }
}

/* callbacks from ipc handler to add JSON to websocket message */

void krad_websocket_set_tag (kr_ws_client_t *kr_ws_client, char *tag_item, char *tag_name, char *tag_value) {

  cJSON *msg;
  
  cJSON_AddItemToArray(kr_ws_client->msgs, msg = cJSON_CreateObject());
  
  cJSON_AddStringToObject (msg, "com", "kradradio");
  cJSON_AddStringToObject (msg, "info", "tag");
  cJSON_AddStringToObject (msg, "tag_item", tag_item);
  cJSON_AddStringToObject (msg, "tag_name", tag_name);
  cJSON_AddStringToObject (msg, "tag_value", tag_value);

}

void krad_websocket_set_cpu_usage (kr_ws_client_t *kr_ws_client, int usage) {

  cJSON *msg;
  
  cJSON_AddItemToArray (kr_ws_client->msgs, msg = cJSON_CreateObject());
  
  cJSON_AddStringToObject (msg, "com", "kradradio");
  cJSON_AddStringToObject (msg, "info", "cpu");
  cJSON_AddNumberToObject (msg, "system_cpu_usage", usage);

}

void krad_websocket_update_portgroup ( kr_ws_client_t *kr_ws_client, char *portname, float floatval, char *crossfade_name, float crossfade_val ) {

  cJSON *msg;
  
  cJSON_AddItemToArray (kr_ws_client->msgs, msg = cJSON_CreateObject());
  
  cJSON_AddStringToObject (msg, "com", "kradmixer");
  
  cJSON_AddStringToObject (msg, "cmd", "update_portgroup");
  cJSON_AddStringToObject (msg, "portgroup_name", portname);
  cJSON_AddNumberToObject (msg, "volume", floatval);
  
  cJSON_AddStringToObject (msg, "crossfade_name", crossfade_name);
  cJSON_AddNumberToObject (msg, "crossfade", crossfade_val);
  
  //kr_tags (kr_ws_client->kr_client, portname);
}

void krad_websocket_add_portgroup ( kr_ws_client_t *kr_ws_client, kr_mixer_portgroup_t *portgroup) {

  cJSON *msg;

  cJSON_AddItemToArray(kr_ws_client->msgs, msg = cJSON_CreateObject());
  
  cJSON_AddStringToObject (msg, "com", "kradmixer");
  
  cJSON_AddStringToObject (msg, "cmd", "add_portgroup");
  cJSON_AddStringToObject (msg, "portgroup_name", portgroup->sysname);
  cJSON_AddNumberToObject (msg, "volume", portgroup->volume[0]);
  
  if (portgroup->crossfade_group[0] != '\0') {
    cJSON_AddStringToObject (msg, "crossfade_name", portgroup->crossfade_group);
    cJSON_AddNumberToObject (msg, "crossfade", portgroup->fade);
  } else {
    cJSON_AddStringToObject (msg, "crossfade_name", "");
    cJSON_AddNumberToObject (msg, "crossfade", 0);
  }
  
  cJSON_AddNumberToObject (msg, "xmms2", portgroup->has_xmms2);  
  
  //kr_tags (kr_ws_client->kr_client, portgroup->sysname);
}

void krad_websocket_remove_portgroup ( kr_ws_client_t *kr_ws_client, kr_mixer_portgroup_t *portgroup ) {

  cJSON *msg;
  
  cJSON_AddItemToArray(kr_ws_client->msgs, msg = cJSON_CreateObject());
  
  cJSON_AddStringToObject (msg, "com", "kradmixer");
  
  cJSON_AddStringToObject (msg, "cmd", "remove_portgroup");
  cJSON_AddStringToObject (msg, "portgroup_name", portgroup->sysname);
}

void krad_websocket_set_portgroup_control ( kr_ws_client_t *kr_ws_client, kr_address_t *address, float value) {

  cJSON *msg;  
  
  cJSON_AddItemToArray(kr_ws_client->msgs, msg = cJSON_CreateObject());
  
  cJSON_AddStringToObject (msg, "com", "kradmixer");
  
  cJSON_AddStringToObject (msg, "cmd", "control_portgroup");
  cJSON_AddStringToObject (msg, "portgroup_name", address->id.name);
  cJSON_AddStringToObject (msg, "control_name", portgroup_control_to_string(address->control.portgroup_control));
  cJSON_AddNumberToObject (msg, "value", value);
}

void krad_websocket_set_mixer ( kr_ws_client_t *kr_ws_client, kr_mixer_t *mixer) {

  cJSON *msg;
  
  cJSON_AddItemToArray(kr_ws_client->msgs, msg = cJSON_CreateObject());
  
  cJSON_AddStringToObject (msg, "com", "kradmixer");
  
  cJSON_AddStringToObject (msg, "cmd", "set_mixer_params");
  cJSON_AddNumberToObject (msg, "sample_rate", mixer->sample_rate);
}


void krad_websocket_set_frame_rate ( kr_ws_client_t *kr_ws_client, int numerator, int denominator) {

  cJSON *msg;
  
  cJSON_AddItemToArray(kr_ws_client->msgs, msg = cJSON_CreateObject());
  
  cJSON_AddStringToObject (msg, "com", "kradcompositor");
  
  cJSON_AddStringToObject (msg, "cmd", "set_frame_rate");
  cJSON_AddNumberToObject (msg, "numerator", numerator);
  cJSON_AddNumberToObject (msg, "denominator", denominator);
}

void krad_websocket_set_frame_size ( kr_ws_client_t *kr_ws_client, int width, int height) {

  cJSON *msg;
  
  cJSON_AddItemToArray(kr_ws_client->msgs, msg = cJSON_CreateObject());

  cJSON_AddStringToObject (msg, "com", "kradcompositor");
  
  cJSON_AddStringToObject (msg, "cmd", "set_frame_size");
  cJSON_AddNumberToObject (msg, "width", width);
  cJSON_AddNumberToObject (msg, "height", height);
}

/* Krad API Handler */

static void rep_to_json (kr_ws_client_t *kr_ws_client, kr_rep_t *rep) {
  switch ( rep->type ) {
    case KR_MIXER:
      krad_websocket_set_mixer (kr_ws_client, rep->rep_ptr.mixer);
      return;
    case KR_PORTGROUP:
      krad_websocket_add_portgroup (kr_ws_client, rep->rep_ptr.portgroup);
      return;
  }
}

static int krad_api_handler (kr_ws_client_t *kr_ws_client) {

  kr_client_t *client;
  kr_response_t *response;
  kr_address_t *address;
  kr_rep_t *rep;
  int integer;
  float real;
  
  integer = 0;
  real = 0.0f;
  response = NULL;
  rep = NULL;

  client = kr_ws_client->kr_client;

  kr_client_response_get (client, &response);

  if (response != NULL) {

    kr_response_address (response, &address);
    
    if ((kr_response_get_event (response) == EBML_ID_KRAD_SUBUNIT_CONTROL) &&
        (address->path.unit == KR_MIXER) && (address->path.subunit.mixer_subunit == KR_PORTGROUP)) {
        if (kr_response_to_float (response, &real)) {
          krad_websocket_set_portgroup_control (kr_ws_client, address, real);
        }
    }
    
    if (kr_response_to_int (response, &integer)) {
      //printk ("Response Int: %d\n", integer);
    }
    
    if (kr_response_to_float (response, &real)) {
      //printk ("Response Float: %f\n", real);
    }

    if (kr_response_to_rep (response, &rep)) {
      rep_to_json (kr_ws_client, rep);
      kr_rep_free (&rep);
    }
    
    kr_response_free (&response);
  } else {
    //printk ("Krad WebSocket: Krad API Handler.. I should have got a response :/");
  }
  
  return 0;
}

/****  Poll Functions  ****/

static void add_poll_fd (int fd, short events, int fd_is, kr_ws_client_t *kr_ws_client, void *bspointer) {

  krad_websocket_t *krad_websocket = krad_websocket_glob;
  krad_websocket->fdof[krad_websocket->count_pollfds] = fd_is;
  if (fd_is == KRAD_IPC) {
    krad_websocket->sessions[krad_websocket->count_pollfds] = kr_ws_client;
  }
  krad_websocket->pollfds[krad_websocket->count_pollfds].fd = fd;
  krad_websocket->pollfds[krad_websocket->count_pollfds].events = events;
  krad_websocket->pollfds[krad_websocket->count_pollfds++].revents = 0;
}

static void del_poll_fd (int fd) {

  krad_websocket_t *krad_websocket = krad_websocket_glob;
  int n;
  krad_websocket->count_pollfds--;

  for (n = 0; n < krad_websocket->count_pollfds; n++) {
    if (krad_websocket->pollfds[n].fd == fd) {
      while (n < krad_websocket->count_pollfds) {
        krad_websocket->pollfds[n] = krad_websocket->pollfds[n + 1];
        krad_websocket->sessions[n] = krad_websocket->sessions[n + 1];
        krad_websocket->fdof[n] = krad_websocket->fdof[n + 1];
        n++;
      }
    }
  }
}


/* WebSocket Functions */

static int callback_http (struct libwebsocket_context *this,
                          struct libwebsocket *wsi,
                          enum libwebsocket_callback_reasons reason,
                          void *user, void *in, size_t len) {

  int n;
  krad_websocket_t *krad_websocket;

  n = 0;
  krad_websocket = krad_websocket_glob;

  switch (reason) {
    case LWS_CALLBACK_ADD_POLL_FD:
      krad_websocket->fdof[krad_websocket->count_pollfds] = MYSTERY;
      krad_websocket->pollfds[krad_websocket->count_pollfds].fd = (int)(long)user;
      krad_websocket->pollfds[krad_websocket->count_pollfds].events = (int)len;
      krad_websocket->pollfds[krad_websocket->count_pollfds++].revents = 0;
      break;
    case LWS_CALLBACK_DEL_POLL_FD:
      n = (int)(long)user;
      del_poll_fd (n);
      break;
    case LWS_CALLBACK_SET_MODE_POLL_FD:
      for (n = 0; n < krad_websocket->count_pollfds; n++) {
        if (krad_websocket->pollfds[n].fd == (int)(long)user) {
          krad_websocket->pollfds[n].events |= (int)(long)len;
        }
      }
      break;
    case LWS_CALLBACK_CLEAR_MODE_POLL_FD:
      for (n = 0; n < krad_websocket->count_pollfds; n++) {
        if (krad_websocket->pollfds[n].fd == (int)(long)user) {
          krad_websocket->pollfds[n].events &= ~(int)(long)len;
        }
      }
      break;
    default:
      break;
  }
  return 0;
}


static int callback_kr_client (struct libwebsocket_context *this,
                               struct libwebsocket *wsi, 
                               enum libwebsocket_callback_reasons reason, 
                               void *user, void *in, size_t len) {

  int ret;
  krad_websocket_t *krad_websocket = krad_websocket_glob;
  kr_ws_client_t *kr_ws_client = user;
  unsigned char *p = &krad_websocket->buffer[LWS_SEND_BUFFER_PRE_PADDING];
  
  switch (reason) {

    case LWS_CALLBACK_ESTABLISHED:

      kr_ws_client->context = this;
      kr_ws_client->wsi = wsi;
      kr_ws_client->krad_websocket = krad_websocket;

      kr_ws_client->kr_client = kr_client_create ("websocket client");
      
      if (kr_ws_client->kr_client == NULL) {
        break;
      }
      
      if (!kr_connect (kr_ws_client->kr_client, kr_ws_client->krad_websocket->sysname)) {
        kr_client_destroy (&kr_ws_client->kr_client);
        break;
      }

      kr_ws_client->kr_client_info = 0;
      kr_ws_client->hello_sent = 0;      
      kr_mixer_info (kr_ws_client->kr_client);
      //kr_compositor_info (kr_ws_client->kr_client);
      kr_mixer_portgroups_list (kr_ws_client->kr_client);
      //kr_transponder_decklink_list (kr_ws_client->kr_client);
      //kr_transponder_list (kr_ws_client->kr_client);
      //kr_tags (kr_ws_client->kr_client, NULL);
      kr_subscribe_all (kr_ws_client->kr_client);
      add_poll_fd (kr_client_get_fd (kr_ws_client->kr_client), POLLIN, KRAD_IPC, kr_ws_client, NULL);
      break;

    case LWS_CALLBACK_CLOSED:

      del_poll_fd (kr_client_get_fd (kr_ws_client->kr_client));
      kr_client_destroy (&kr_ws_client->kr_client);
      kr_ws_client->hello_sent = 0;
      kr_ws_client->context = NULL;
      kr_ws_client->wsi = NULL;
      break;

    case LWS_CALLBACK_BROADCAST:

      memcpy (p, in, len);
      libwebsocket_write (wsi, p, len, LWS_WRITE_TEXT);
      break;

    case LWS_CALLBACK_SERVER_WRITEABLE:

      if (kr_ws_client->kr_client_info == 1) {
        memcpy (p, kr_ws_client->msgstext, kr_ws_client->msgstextlen + 1);
        ret = libwebsocket_write (wsi, p, kr_ws_client->msgstextlen, LWS_WRITE_TEXT);
        if (ret < 0) {
          printke ("krad_ipc ERROR writing to socket");
          return 1;
        }
        kr_ws_client->kr_client_info = 0;
        kr_ws_client->msgstextlen = 0;
        free (kr_ws_client->msgstext);
      }
      break;

    case LWS_CALLBACK_RECEIVE:
      json_to_krad_api (kr_ws_client, in, len);
      break;
    default:
      break;
  }

  return 0;
}

static void *krad_websocket_server_run (void *arg) {

  krad_websocket_t *krad_websocket = (krad_websocket_t *)arg;
  int n;
  kr_ws_client_t *kr_ws_client;
  
  krad_system_set_thread_name ("kr_websocket");

  n = 0;
  kr_ws_client = NULL;
  krad_websocket->shutdown = KRAD_WEBSOCKET_RUNNING;

  while (!krad_websocket->shutdown) {

    n = poll (krad_websocket->pollfds, krad_websocket->count_pollfds, 500);
    
    if ((n < 0) || (krad_websocket->shutdown)) {
      break;
    }

    if (n > 0) {

      if (krad_websocket->pollfds[0].revents) {
        break;
      }

      for (n = 1; n < krad_websocket->count_pollfds; n++) {
        
        if (krad_websocket->pollfds[n].revents) {
        
          if (krad_websocket->pollfds[n].revents && (krad_websocket->fdof[n] == MYSTERY)) {
            libwebsocket_service_fd (krad_websocket->context, &krad_websocket->pollfds[n]);  
            continue;
          }
        
          if ((krad_websocket->pollfds[n].revents & POLLERR) || (krad_websocket->pollfds[n].revents & POLLHUP)) {
              
            if (krad_websocket->pollfds[n].revents & POLLERR) {
            }
          
            if (krad_websocket->pollfds[n].revents & POLLHUP) {
            }
            
            switch ( krad_websocket->fdof[n] ) {
              case KRAD_IPC:
              case KRAD_CONTROLLER:
              case MYSTERY:
                break;
            }
          
          } else {
          
            if (krad_websocket->pollfds[n].revents & POLLIN) {

              switch ( krad_websocket->fdof[n] ) {
                case KRAD_IPC:

                  kr_ws_client = krad_websocket->sessions[n];

                  kr_ws_client->msgs = cJSON_CreateArray();
                  cJSON *msg;

                  if (kr_ws_client->hello_sent == 0) {
                    cJSON_AddItemToArray (kr_ws_client->msgs, msg = cJSON_CreateObject());
                    cJSON_AddStringToObject (msg, "com", "kradradio");
                    cJSON_AddStringToObject (msg, "info", "sysname");
                    cJSON_AddStringToObject (msg, "infoval", krad_websocket->sysname);
                    kr_ws_client->hello_sent = 1;
                  }
                  
                  krad_api_handler (kr_ws_client);
                  
                  kr_ws_client->msgstext = cJSON_Print (kr_ws_client->msgs);
                  kr_ws_client->msgstextlen = strlen (kr_ws_client->msgstext);
                  cJSON_Delete (kr_ws_client->msgs);
                  
                  kr_ws_client->kr_client_info = 1;
                  libwebsocket_callback_on_writable (kr_ws_client->context, kr_ws_client->wsi);
                  break;
                case KRAD_CONTROLLER:
                case MYSTERY:
                  break;
              }
            }
            
            if (krad_websocket->pollfds[n].revents & POLLOUT) {
              switch ( krad_websocket->fdof[n] ) {
                case KRAD_IPC:
                case KRAD_CONTROLLER:
                case MYSTERY:
                  break;
              }
            }
          }
        }
      }
    }
  }
  
  krad_websocket->shutdown = KRAD_WEBSOCKET_SHUTINGDOWN;
  krad_controller_client_close (&krad_websocket->krad_control);
  
  return NULL;
}

void krad_websocket_server_destroy (krad_websocket_t *krad_websocket) {

  if (krad_websocket != NULL) {
    printk ("Krad Websocket shutdown started");  
    krad_websocket->shutdown = KRAD_WEBSOCKET_DO_SHUTDOWN;
    if (!krad_controller_shutdown (&krad_websocket->krad_control, &krad_websocket->server_thread, 30)) {
      krad_controller_destroy (&krad_websocket->krad_control, &krad_websocket->server_thread);
    }
    free (krad_websocket->buffer);
    libwebsocket_context_destroy (krad_websocket->context);
    free (krad_websocket);
    krad_websocket_glob = NULL;
    printk ("Krad Websocket shutdown complete");
  }
}

krad_websocket_t *krad_websocket_server_create (char *sysname, int port) {

  krad_websocket_t *krad_websocket = calloc (1, sizeof (krad_websocket_t));

  krad_websocket_glob = krad_websocket;
  krad_websocket->shutdown = KRAD_WEBSOCKET_STARTING;
  krad_websocket->port = port;
  strcpy (krad_websocket->sysname, sysname);

  if (krad_control_init (&krad_websocket->krad_control)) {
    free (krad_websocket);
    return NULL;
  }

  add_poll_fd (krad_controller_get_client_fd (&krad_websocket->krad_control), POLLIN, KRAD_CONTROLLER, NULL, NULL);

  krad_websocket->buffer = calloc(1, 32768 * 8);
  krad_websocket->context = libwebsocket_create_context (krad_websocket->port, NULL, protocols,
                                                         libwebsocket_internal_extensions, 
                                                         NULL, NULL, -1, -1, 0, NULL);
  if (krad_websocket->context == NULL) {
    printke ("libwebsocket init failed");
    krad_websocket_server_destroy (krad_websocket);
    return NULL;
  }
  
  pthread_create (&krad_websocket->server_thread, NULL, krad_websocket_server_run, (void *)krad_websocket);

  return krad_websocket;
}
