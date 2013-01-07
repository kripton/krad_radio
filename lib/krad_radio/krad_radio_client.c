#include "krad_radio_client_internal.h"
#include "krad_radio_client.h"


#include "krad_radio_common.h"
#include "krad_compositor_common.h"
#include "krad_transponder_common.h"
#include "krad_mixer_common.h"

#include "krad_compositor_client.h"
#include "krad_transponder_client.h"
#include "krad_mixer_client.h"


kr_client_t *kr_connect (char *sysname) {

  kr_client_t *kr_client;
  
  kr_client = calloc (1, sizeof(kr_client_t));
  kr_client->krad_ipc_client = krad_ipc_connect (sysname);
  if (kr_client->krad_ipc_client != NULL) {
    kr_client->krad_ebml = kr_client->krad_ipc_client->krad_ebml;
    return kr_client;
  } else {
    free (kr_client);
  }
  
  return NULL;
}

void kr_disconnect (kr_client_t **kr_client) {
  if ((kr_client != NULL) && (*kr_client != NULL)) {
    krad_ipc_disconnect ((*kr_client)->krad_ipc_client);
    free (*kr_client);
    *kr_client = NULL;
  }
}

krad_ebml_t *kr_client_get_ebml (kr_client_t *kr_client) {
  return kr_client->krad_ipc_client->krad_ebml;
}

int kr_client_local (kr_client_t *client) {
  if (client->krad_ipc_client->tcp_port == 0) {
    return 1;
  }
  return 0;
}

void kr_broadcast_subscribe (kr_client_t *kr_client, uint32_t broadcast_id) {
  krad_ipc_broadcast_subscribe (kr_client->krad_ipc_client, broadcast_id);
}

void kr_shm_destroy (kr_shm_t *kr_shm) {

  if (kr_shm != NULL) {
    if (kr_shm->buffer != NULL) {
      munmap (kr_shm->buffer, kr_shm->size);
      kr_shm->buffer = NULL;
    }
    if (kr_shm->fd != 0) {
      close (kr_shm->fd);
    }
    free(kr_shm);
  }
}

kr_shm_t *kr_shm_create (kr_client_t *client) {

  char filename[] = "/tmp/krad-shm-XXXXXX";
  kr_shm_t *kr_shm;

  kr_shm = calloc (1, sizeof(kr_shm_t));

  if (kr_shm == NULL) {
    return NULL;
  }

  kr_shm->size = 960 * 540 * 4 * 2;

  kr_shm->fd = mkstemp (filename);
  if (kr_shm->fd < 0) {
    fprintf(stderr, "open %s failed: %m\n", filename);
    kr_shm_destroy (kr_shm);
    return NULL;
  }

  if (ftruncate (kr_shm->fd, kr_shm->size) < 0) {
    fprintf (stderr, "ftruncate failed: %m\n");
    kr_shm_destroy (kr_shm);
    return NULL;
  }

  kr_shm->buffer = mmap (NULL, kr_shm->size, PROT_READ | PROT_WRITE, MAP_SHARED, kr_shm->fd, 0);
  unlink (filename);

  if (kr_shm->buffer == MAP_FAILED) {
    fprintf (stderr, "mmap failed: %m\n");
    kr_shm_destroy (kr_shm);
    return NULL;
  }

  return kr_shm;

}

int kr_send_fd (kr_client_t *client, int fd) {
  return krad_ipc_client_send_fd (client->krad_ipc_client, fd);
}

int kr_poll (kr_client_t *kr_client, uint32_t timeout_ms) {

  krad_ipc_client_t *client;
  
  client = kr_client->krad_ipc_client;

  fd_set set;
  struct timeval tv;
  
  if (client->tcp_port) {
    tv.tv_sec = 1;
  } else {
      tv.tv_sec = 0;
  }
  tv.tv_usec = timeout_ms * 1000;
  
  FD_ZERO (&set);
  FD_SET (client->sd, &set);  

  return select (client->sd+1, &set, NULL, NULL, &tv);
}

void kr_response_type_print (kr_response_t *kr_response) {

  switch ( kr_response->unit ) {
    case KR_RADIO:
      printf ("Response Type: Krad Radio\n");
      break;
    case KR_MIXER:
      printf ("Response Type: Krad Mixer\n");
      break;
    case KR_COMPOSITOR:
      printf ("Response Type: Krad Compositor\n");
      break;
    case KR_TRANSPONDER:
      printf ("Response Type: Krad Transponder\n");
      break;
  }
}

kr_unit_t kr_response_type (kr_response_t *kr_response) {
  return kr_response->unit;
}

void kr_response_size_print (kr_response_t *kr_response) {
  printf ("Response Size: %zu byte payload.\n", kr_response->size);
}

void kr_response_print (kr_response_t *kr_response) {
  kr_response_type_print (kr_response);
  kr_response_size_print (kr_response);
}

uint32_t kr_response_size (kr_response_t *kr_response) {
  return kr_response->size;
}

void kr_response_free (kr_response_t *kr_response) {
  if (kr_response->buffer != NULL) {
    free (kr_response->buffer);
  }
  free (kr_response);
}

kr_response_t *kr_response_alloc () {
  return calloc (1, sizeof(kr_response_t));
}

void kr_client_response_get (kr_client_t *kr_client, kr_response_t **kr_response) {

  krad_ipc_client_t *client;
  kr_response_t *response;

  uint32_t ebml_id;
  uint64_t ebml_data_size;

  ebml_id = 0;
  ebml_data_size = 0;

  client = kr_client->krad_ipc_client;
  response = kr_response_alloc ();
  *kr_response = response;

  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

  switch ( ebml_id ) {
    case EBML_ID_KRAD_RADIO_MSG:
      response->unit = KR_RADIO;
      break;
    case EBML_ID_KRAD_MIXER_MSG:
      response->unit = KR_MIXER;
      break;
    case EBML_ID_KRAD_COMPOSITOR_MSG:
      response->unit = KR_COMPOSITOR;
      break;
    case EBML_ID_KRAD_TRANSPONDER_MSG:
      response->unit = KR_TRANSPONDER;
      break;
  }

  if (ebml_data_size > 0) {
    response->size = ebml_data_size;
    response->buffer = malloc (response->size);
    krad_ebml_read (client->krad_ebml, response->buffer, ebml_data_size);
  }
}

void kr_client_response_wait (kr_client_t *kr_client, kr_response_t **kr_response) {
  kr_poll (kr_client, 250);
  kr_client_response_get (kr_client, kr_response);
}

void kr_client_response_wait_print (kr_client_t *kr_client) {

  kr_response_t *kr_response;

  kr_client_response_wait (kr_client, &kr_response);
  kr_response_print (kr_response);
  
  kr_response_free (kr_response);
  
}

void kr_set_dir (kr_client_t *client, char *dir) {

  uint64_t command;
  uint64_t setdir;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_DIR, &setdir);

  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_DIR, dir);
  
  krad_ebml_finish_element (client->krad_ebml, setdir);
  krad_ebml_finish_element (client->krad_ebml, command);
    
  krad_ebml_write_sync (client->krad_ebml);
}


void kr_uptime (kr_client_t *client) {

  uint64_t command;
  uint64_t uptime_command;
  command = 0;
  uptime_command = 0;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_UPTIME, &uptime_command);
  krad_ebml_finish_element (client->krad_ebml, uptime_command);

  krad_ebml_finish_element (client->krad_ebml, command);
    
  krad_ebml_write_sync (client->krad_ebml);

}

void kr_system_info (kr_client_t *client) {

  uint64_t command;
  uint64_t info_command;
  command = 0;
  info_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_INFO, &info_command);
  krad_ebml_finish_element (client->krad_ebml, info_command);

  krad_ebml_finish_element (client->krad_ebml, command);
    
  krad_ebml_write_sync (client->krad_ebml);  
  
}


void kr_logname (kr_client_t *client) {

  uint64_t command;
  uint64_t log_command;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_LOGNAME, &log_command);
  krad_ebml_finish_element (client->krad_ebml, log_command);
  krad_ebml_finish_element (client->krad_ebml, command);
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_system_cpu_usage (kr_client_t *client) {

  uint64_t command;
  uint64_t cpu_command;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_CPU_USAGE, &cpu_command);
  krad_ebml_finish_element (client->krad_ebml, cpu_command);
  krad_ebml_finish_element (client->krad_ebml, command);
  krad_ebml_write_sync (client->krad_ebml);
}

/* Remote Control */

void kr_remote_enable (kr_client_t *client, int port) {

  uint64_t radio_command;
  uint64_t enable_remote;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_ENABLE, &enable_remote);  

  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

  krad_ebml_finish_element (client->krad_ebml, enable_remote);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_remote_disable (kr_client_t *client) {

  uint64_t radio_command;
  uint64_t disable_remote;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE, &disable_remote);

  krad_ebml_finish_element (client->krad_ebml, disable_remote);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_web_enable (kr_client_t *client, int http_port, int websocket_port,
          char *headcode, char *header, char *footer) {

  uint64_t radio_command;
  uint64_t webon;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE, &webon);  

  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_HTTP_PORT, http_port);  
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_WEBSOCKET_PORT, websocket_port);  

  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_HEADCODE, headcode);
  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_HEADER, header);
  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_FOOTER, footer);
  
  krad_ebml_finish_element (client->krad_ebml, webon);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_web_disable (kr_client_t *client) {

  uint64_t radio_command;
  uint64_t weboff;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_DISABLE, &weboff);

  krad_ebml_finish_element (client->krad_ebml, weboff);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_osc_enable (kr_client_t *client, int port) {

  uint64_t radio_command;
  uint64_t enable_osc;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_OSC_ENABLE, &enable_osc);  

  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_UDP_PORT, port);

  krad_ebml_finish_element (client->krad_ebml, enable_osc);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_osc_disable (kr_client_t *client) {

  uint64_t radio_command;
  uint64_t disable_osc;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_OSC_DISABLE, &disable_osc);

  krad_ebml_finish_element (client->krad_ebml, disable_osc);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}


/* Tagging */

void kr_read_tag_inner ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value ) {

  uint32_t ebml_id;
  uint64_t ebml_data_size;

  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
    //printf("hrm wtf2\n");
  } else {
    //printf("tag name size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (client->krad_ebml, *tag_item, ebml_data_size);

  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
    //printf("hrm wtf2\n");
  } else {
    //printf("tag name size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (client->krad_ebml, *tag_name, ebml_data_size);
  
  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
    //printf("hrm wtf3\n");
  } else {
    //printf("tag value size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (client->krad_ebml, *tag_value, ebml_data_size);
}

int kr_read_tag ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value ) {

  uint32_t ebml_id;
  uint64_t ebml_data_size;
  int bytes_read;
  
  bytes_read = 0;

  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
  
  bytes_read += ebml_data_size + 9;
  
  if (ebml_id != EBML_ID_KRAD_RADIO_TAG) {
    //printf("hrm wtf\n");
  } else {
    //printf("tag size %zu\n", ebml_data_size);
  }
  
  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_ITEM) {
    //printf("hrm wtf2\n");
  } else {
    //printf("tag name size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (client->krad_ebml, *tag_item, ebml_data_size);  
  
  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_NAME) {
    //printf("hrm wtf2\n");
  } else {
    //printf("tag name size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (client->krad_ebml, *tag_name, ebml_data_size);
  
  
  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_RADIO_TAG_VALUE) {
    //printf("hrm wtf3\n");
  } else {
    //printf("tag value size %zu\n", ebml_data_size);
  }

  krad_ebml_read_string (client->krad_ebml, *tag_value, ebml_data_size);
  
  return bytes_read;
}

void kr_tags (kr_client_t *client, char *item) {

  uint64_t radio_command;
  uint64_t get_tags;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_LIST_TAGS, &get_tags);  

  if (item == NULL) {
    item = "station";
  }

  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);

  krad_ebml_finish_element (client->krad_ebml, get_tags);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_tag (kr_client_t *client, char *item, char *tag_name) {

  uint64_t radio_command;
  uint64_t get_tag;
  uint64_t tag;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_TAG, &get_tag);  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);  

  if (item == NULL) {
    item = "station";
  }

  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);
  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");  

  krad_ebml_finish_element (client->krad_ebml, tag);
  krad_ebml_finish_element (client->krad_ebml, get_tag);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}


void kr_set_tag (kr_client_t *client, char *item, char *tag_name, char *tag_value) {

  uint64_t radio_command;
  uint64_t set_tag;
  uint64_t tag;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_TAG, &set_tag);  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);  

  if (item == NULL) {
    item = "station";
  }

  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_ITEM, item);
  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, tag_value);  

  krad_ebml_finish_element (client->krad_ebml, tag);
  krad_ebml_finish_element (client->krad_ebml, set_tag);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}



int kr_subunit_control_set (kr_client_t *client,
                            kr_unit_t unit,
                            kr_subunit_t subunit,
                            kr_subunit_address_t address,
                            kr_subunit_control_t control,
                            kr_subunit_control_value_t value) {

  uint64_t command;
  uint64_t control_set;

  switch (unit) {
    case KR_MIXER:
      krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
      break;
    case KR_COMPOSITOR:
      krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);
      break;
    case KR_TRANSPONDER:
      krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_TRANSPONDER_CMD, &command);
      break;
    case KR_RADIO:
      break;
  }
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_CONTROL_SET, &control_set);
  
  switch (unit) {
    case KR_MIXER:
      // portgroup name
      // control vol/fade
      // value
      break;
    case KR_COMPOSITOR:
      // subunit type
      // subunit number
      // control type
      // value
      break;
    case KR_RADIO:
      break;
    case KR_TRANSPONDER:
      return -1;
      break;
  }
  
  krad_ebml_finish_element (client->krad_ebml, control_set);
  krad_ebml_finish_element (client->krad_ebml, command);
  krad_ebml_write_sync (client->krad_ebml);

  return 0;
}

