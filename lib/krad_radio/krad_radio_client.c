#include "krad_radio_client_internal.h"
#include "krad_radio_client.h"


#include "krad_radio_common.h"
#include "krad_compositor_common.h"
#include "krad_transponder_common.h"
#include "krad_mixer_common.h"

#include "krad_compositor_client.h"
#include "krad_transponder_client.h"
#include "krad_mixer_client.h"

static int kr_remote_port_valid (int port);

kr_client_t *kr_client_create (char *client_name) {

  kr_client_t *client;
  int len;
  
  if (client_name == NULL) {
    return NULL;
  }
  len = strlen (client_name);
  if ((len == 0) || (len > 255)) {
    return NULL;
  }

  client = calloc (1, sizeof(kr_client_t));
  client->name = strdup (client_name);

  return client;
}

int kr_connect_remote (kr_client_t *client, char *host, int port) {
  
  char url[532];
  int len;
  if ((client == NULL) || (host == NULL) || 
      ((port < 1) || (port > 65535))) {
    return 0;
  }
  len = strlen (host);
  if ((len == 0) || (len > 512)) {
    return 0;
  }

  snprintf (url, sizeof(url), "%s:%d", host, port);

  return kr_connect (client, url);
}

int kr_connect (kr_client_t *client, char *sysname) {

  if (client == NULL) {
    return 0;
  }
  if (kr_connected (client)) {
    kr_disconnect (client);
  }
  client->krad_ipc_client = krad_ipc_connect (sysname);
  if (client->krad_ipc_client != NULL) {
    client->krad_ebml = client->krad_ipc_client->krad_ebml;
    return 1;
  }

  return 0;
}

int kr_connected (kr_client_t *client) {
  if (client->krad_ipc_client != NULL) {
    return 1; 
  }
  return 0;
}

int kr_disconnect (kr_client_t *client) {
  if (client != NULL) {
    if (kr_connected (client)) {
      krad_ipc_disconnect (client->krad_ipc_client);
      client->krad_ipc_client = NULL;
      return 1;
    }
    return -2;
  }
  return -1;
}

int kr_client_destroy (kr_client_t **client) {
  if (*client != NULL) {
    if (kr_connected (*client)) {
      kr_disconnect (*client);
    }
    free (*client);
    *client = NULL;
    return 1;
  }
  return -1;
}

krad_ebml_t *kr_client_get_ebml (kr_client_t *kr_client) {
  return kr_client->krad_ipc_client->krad_ebml;
}

int kr_client_local (kr_client_t *client) {
  if (client != NULL) {
    if (kr_connected (client)) {
      if (client->krad_ipc_client->tcp_port == 0) {
        return 1;
      }
      return 0;
    }
  }
  return -1;
}

int kr_client_get_fd (kr_client_t *client) {
  if (client != NULL) {
    if (kr_connected (client)) {
      return client->krad_ipc_client->sd;
    }
  }
  return -1;
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

void kr_response_free_string (char **string) {
  free (*string);
}

char *kr_response_alloc_string (int length) {
  return calloc (1, length + 16);
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

int kr_radio_uptime_to_string (uint64_t uptime, char **string) {

  int days;
  int hours;
  int minutes;
  int seconds;
  
  int pos;
  
  pos = 0;

  *string = kr_response_alloc_string (64);

  pos += sprintf (*string + pos, "up");

  days = uptime / (60*60*24);
  minutes = uptime / 60;
  hours = (minutes / 60) % 24;
  minutes %= 60;
  seconds = uptime % 60;

  if (days) {
    pos += sprintf (*string + pos, " %d day%s,", days, (days != 1) ? "s" : "");
  }
  if (hours) {
    pos += sprintf (*string + pos, " %d:%02d", hours, minutes);
  } else {
    if (minutes) {
      pos += sprintf (*string + pos, " %02d min", minutes);
    } else {
      pos += sprintf (*string + pos, " %02d seconds", seconds);
    }
  }
  return pos;
}

int kr_response_read_into_string (unsigned char *ebml_frag, uint64_t ebml_data_size, char *string) {

  int pos;
  pos = 0;

  if (ebml_data_size > 0) {
    pos += krad_ebml_read_string_from_frag (ebml_frag, ebml_data_size, string);
    string[pos++] = '\0';
  }
  return pos;
}

int kr_response_get_string (unsigned char *ebml_frag, uint64_t ebml_data_size, char **string) {

  int pos;
  pos = 0;

  if (ebml_data_size > 0) {
    *string = kr_response_alloc_string (ebml_data_size + 1);
    pos += krad_ebml_read_string_from_frag (ebml_frag + pos, ebml_data_size, *string);
    (*string)[pos] = '\0';
  }
  return pos;
}

int kr_radio_response_get_string_from_tag (unsigned char *ebml_frag, uint64_t item_size, char **string) {

  int item_pos;
  int string_pos;
  int ret;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	
	ret = 0;
  item_pos = 0;
  string_pos = 0;

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  string_pos += sprintf (*string + string_pos, "Tag on ");
  ret = krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, *string + string_pos);
  item_pos += ret;
  string_pos += ret;
  
  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  string_pos += sprintf (*string + string_pos, ": ");
  ret = krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, *string + string_pos);
  item_pos += ret;
  string_pos += ret;

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  string_pos += sprintf (*string + string_pos, " - ");
  ret = krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, *string + string_pos);
  item_pos += ret;
  string_pos += ret;

  string_pos += sprintf (*string + string_pos, "\n");

  return string_pos;

}

int kr_radio_response_get_string_from_remote (unsigned char *ebml_frag, uint64_t item_size, char **string) {

  int item_pos;
  int string_pos;
  int ret;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int number;
	
	ret = 0;
  item_pos = 0;
  string_pos = 0;

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  string_pos += sprintf (*string + string_pos, "Interface: ");
  ret = krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, *string + string_pos);
  item_pos += ret;
  string_pos += ret;
  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  number = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  string_pos += sprintf (*string + string_pos, " Port: %d\n", number);

  return string_pos;

}

item_callback_t kr_response_get_item_to_string_converter (uint32_t list_type) {

  switch ( list_type ) {
    case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
    case EBML_ID_KRAD_MIXER_PORTGROUP:
      return kr_mixer_response_get_string_from_portgroup;
    case EBML_ID_KRAD_RADIO_TAG_LIST:
    case EBML_ID_KRAD_RADIO_TAG:
      return kr_radio_response_get_string_from_tag;
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS_LIST:
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS:
      return kr_radio_response_get_string_from_remote;
    default:
      return NULL;
  }
}

int kr_response_get_string_from_list (uint32_t list_type, unsigned char *ebml_frag, uint64_t list_size, char **string) {

  int list_pos;
  int string_pos;
  int ret;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	char *string_loc_pos;
	item_callback_t item_callback;
	
	ret = 0;
  list_pos = 0;
  string_pos = 0;
  item_callback = NULL;
  
  string_loc_pos = *string;
  
  item_callback = kr_response_get_item_to_string_converter (list_type);
  
  if (item_callback == NULL) {
    printke ("Unable to find item to string conversion");
    return 0;
  }
  
  string_pos += sprintf (string_loc_pos + string_pos, "List: (%"PRIu64" bytes):\n", list_size);  
  string_loc_pos = string_loc_pos + string_pos;
  while (list_pos != list_size) {
    ret = krad_ebml_read_element_from_frag (ebml_frag + list_pos, &ebml_id, &ebml_data_size);
    string_loc_pos += item_callback (ebml_frag + list_pos + ret, ebml_data_size, &string_loc_pos);    
    list_pos += ebml_data_size + ret;
  }

  return list_pos;
}

int kr_radio_response_to_string (kr_response_t *kr_response, char **string) {

  int pos;
	uint32_t ebml_id;
	uint64_t ebml_data_size;

  pos = 0;
  
  pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);

  switch ( ebml_id ) {
    case EBML_ID_KRAD_RADIO_TAG_LIST:
      *string = kr_response_alloc_string (ebml_data_size * 4);
      return kr_response_get_string_from_list (ebml_id, kr_response->buffer + pos, ebml_data_size, string);
      break;
    case EBML_ID_KRAD_RADIO_TAG:
      *string = kr_response_alloc_string (ebml_data_size * 4);
      return kr_radio_response_get_string_from_tag (kr_response->buffer + pos, ebml_data_size, string);
      break;
    case EBML_ID_KRAD_RADIO_UPTIME:
      return kr_radio_uptime_to_string (krad_ebml_read_number_from_frag (kr_response->buffer + pos, ebml_data_size), string);
      break;
    case EBML_ID_KRAD_RADIO_SYSTEM_INFO:
      return kr_response_get_string (kr_response->buffer + pos, ebml_data_size, string);
      break;
    case EBML_ID_KRAD_RADIO_LOGNAME:
      return kr_response_get_string (kr_response->buffer + pos, ebml_data_size, string);
      break;
    case EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE:
      *string = kr_response_alloc_string (ebml_data_size + 1);
      return sprintf (*string, "%"PRIu64"%%",
                      krad_ebml_read_number_from_frag (kr_response->buffer + pos, ebml_data_size));
      break;
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS_LIST:
      *string = kr_response_alloc_string (ebml_data_size * 4);
      return kr_response_get_string_from_list (ebml_id, kr_response->buffer + pos, ebml_data_size, string);
      break;
      
  }

  return 0;  
}

int kr_radio_response_to_int (kr_response_t *kr_response, int *number) {

  int pos;
	uint32_t ebml_id;
	uint64_t ebml_data_size;

  pos = 0;
  
  pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);

  switch ( ebml_id ) {
    case EBML_ID_KRAD_RADIO_UPTIME:
      *number = krad_ebml_read_number_from_frag (kr_response->buffer + pos, ebml_data_size);
      return 1;
      break;
    case EBML_ID_KRAD_RADIO_SYSTEM_CPU_USAGE:
      *number = krad_ebml_read_number_from_frag (kr_response->buffer + pos, ebml_data_size);
      return 1;
      break;
  }

  *number = 0;
  return 0;  
}

int kr_response_to_int (kr_response_t *kr_response, int *number) {

  switch ( kr_response->unit ) {
    case KR_RADIO:
      return kr_radio_response_to_int (kr_response, number);
      break;
    case KR_MIXER:
      //
      break;
    case KR_COMPOSITOR:
      //
      break;
    case KR_TRANSPONDER:
      //
      break;
  }
  return 0;
}

int kr_response_to_string (kr_response_t *kr_response, char **string) {

  switch ( kr_response->unit ) {
    case KR_RADIO:
      return kr_radio_response_to_string (kr_response, string);
      break;
    case KR_MIXER:
      return kr_mixer_response_to_string (kr_response, string);
      break;
    case KR_COMPOSITOR:
      //
      break;
    case KR_TRANSPONDER:
      //
      break;
  }
  return 0;
}


int kr_response_is_list (kr_response_t *kr_response) {

  int pos;
	uint32_t ebml_id;
	uint64_t ebml_data_size;

  pos = 0;
  
  pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);

  switch ( ebml_id ) {
    case EBML_ID_KRAD_RADIO_TAG_LIST:
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS_LIST:
    case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
      return 1;
  }

  return 0;  
}


int kr_response_list_length (kr_response_t *kr_response) {

  int list_pos;
  uint64_t list_size;
  int list_items;
  int ret;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	unsigned char *buffer;
	
	ret = 0;
  list_pos = 0;
  list_items = 0;
  
  if (!kr_response_is_list (kr_response)) {
    return 0;
  }
  
  buffer = kr_response->buffer + krad_ebml_read_element_from_frag (kr_response->buffer + list_pos, &ebml_id, &list_size);
  
  while (list_pos != list_size) {
    ret = krad_ebml_read_element_from_frag (buffer + list_pos, &ebml_id, &ebml_data_size);
    list_pos += ebml_data_size + ret;
    list_items++;
  }

  return list_items;

}

int kr_response_to_item (kr_response_t *kr_response, unsigned char *ebml_frag, uint32_t ebml_id, uint64_t item_size, kr_item_t **kr_item) {

  switch ( ebml_id ) {
    case EBML_ID_KRAD_MIXER_CONTROL:
      kr_response->kr_item.type = EBML_ID_KRAD_MIXER_CONTROL;
      kr_response->kr_item.buffer = ebml_frag;
      kr_response->kr_item.size = item_size;
      *kr_item = &kr_response->kr_item;
      return 1;
    case EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED:
      kr_response->kr_item.type = EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED;
      kr_response->kr_item.buffer = ebml_frag;
      kr_response->kr_item.size = item_size;
      *kr_item = &kr_response->kr_item;
      return 1;
    case EBML_ID_KRAD_RADIO_TAG:
      kr_response->kr_item.type = EBML_ID_KRAD_RADIO_TAG;
      kr_response->kr_item.buffer = ebml_frag;
      kr_response->kr_item.size = item_size;
      *kr_item = &kr_response->kr_item;
      return 1;
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS:
      kr_response->kr_item.type = EBML_ID_KRAD_RADIO_REMOTE_STATUS;
      kr_response->kr_item.buffer = ebml_frag;
      kr_response->kr_item.size = item_size;
      *kr_item = &kr_response->kr_item;
      return 1;
    case EBML_ID_KRAD_MIXER_PORTGROUP:
      kr_response->kr_item.type = EBML_ID_KRAD_MIXER_PORTGROUP;
      kr_response->kr_item.buffer = ebml_frag;
      kr_response->kr_item.size = item_size;
      *kr_item = &kr_response->kr_item;
      return 1;
  }

  return 0;

}

const char *kr_item_get_type_string (kr_item_t *item) {

  switch ( item->type ) {
    case EBML_ID_KRAD_RADIO_TAG:
      return "Tag";
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS:
      return "Remote Status";
    case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
      return "Mixer Portgroup";
    case EBML_ID_KRAD_MIXER_CONTROL:
      return "Mixer Portgroup Control";
  }
  
  return "";
}

int kr_ebml_to_remote_status_rep (unsigned char *ebml_frag, kr_remote_t *remote) {
    
  int item_pos;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	
  item_pos = 0;

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, remote->interface);

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  remote->port = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

  return item_pos;

}

int kr_ebml_to_tag_rep (unsigned char *ebml_frag, kr_tag_t *tag) {
    
  int item_pos;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	
  item_pos = 0;

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, tag->unit);

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, tag->name);

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
  item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, tag->value);

  return item_pos;

}

kr_rep_t *kr_item_to_rep (kr_item_t *kr_item) {

  kr_rep_t *kr_rep;

  if (kr_item == NULL) {
    return NULL;
  }

  kr_rep = calloc (1, sizeof(kr_rep_t));
  kr_rep->buffer = malloc (4096);
  kr_rep->type = kr_item->type;

  switch ( kr_rep->type ) {
    case EBML_ID_KRAD_MIXER_CONTROL:
      kr_rep->rep_ptr.mixer_portgroup_control = (kr_mixer_portgroup_control_rep_t *)kr_rep->buffer;
      kr_ebml_to_mixer_portgroup_control_rep (kr_item->buffer, &kr_rep->rep_ptr.mixer_portgroup_control);
      break;
    case EBML_ID_KRAD_MIXER_PORTGROUP:
      kr_rep->rep_ptr.mixer_portgroup = (kr_mixer_portgroup_t *)kr_rep->buffer;
      kr_ebml_to_mixer_portgroup_rep (kr_item->buffer, &kr_rep->rep_ptr.mixer_portgroup);
      break;
    case EBML_ID_KRAD_RADIO_REMOTE_STATUS:
      kr_rep->rep_ptr.remote = (kr_remote_t *)kr_rep->buffer;
      kr_ebml_to_remote_status_rep (kr_item->buffer, kr_rep->rep_ptr.remote);
      break;
    case EBML_ID_KRAD_RADIO_TAG:
      kr_rep->rep_ptr.tag = (kr_tag_t *)kr_rep->buffer;
      kr_ebml_to_tag_rep (kr_item->buffer, kr_rep->rep_ptr.tag);
      break;
  }

  return kr_rep;

}



int kr_rep_free (kr_rep_t **kr_rep) {
  if (*kr_rep != NULL) {
    if ((*kr_rep)->buffer != NULL) {
      free ((*kr_rep)->buffer);
    }
    free ((*kr_rep));
    return 0;
  }
  return -1;
}

int kr_item_to_string (kr_item_t *kr_item, char **string) {

  *string = kr_response_alloc_string (kr_item->size * 4 + 1024); 

  int string_pos;
	char *string_loc_pos;
	item_callback_t item_callback;
	
  string_pos = 0;
  item_callback = NULL;
  
  string_loc_pos = *string;
  
  item_callback = kr_response_get_item_to_string_converter (kr_item->type);
  
  if (item_callback == NULL) {
    printke ("Unable to find item to string conversion");
    return 0;
  }
  
  string_pos += sprintf (string_loc_pos + string_pos, "Item (%d bytes):\n", kr_item->size);  
  string_loc_pos = string_loc_pos + string_pos;

  string_pos += item_callback (kr_item->buffer, kr_item->size, &string_loc_pos);    


  return string_pos;


}

int kr_item_read_into_string (kr_item_t *kr_item, char *string) {

  int string_pos;
	char *string_loc_pos;
	item_callback_t item_callback;
	
  string_pos = 0;
  item_callback = NULL;
  
  string_loc_pos = string;
  
  item_callback = kr_response_get_item_to_string_converter (kr_item->type);
  
  if (item_callback == NULL) {
    printke ("Unable to find item to string conversion");
    return 0;
  }
  
  string_pos += sprintf (string_loc_pos + string_pos, "Item (%d bytes):\n", kr_item->size);  
  string_loc_pos = string_loc_pos + string_pos;

  string_pos += item_callback (kr_item->buffer, kr_item->size, &string_loc_pos);    


  return string_pos;


}

int kr_response_list_get_item (kr_response_t *kr_response, int item_num, kr_item_t **item) {

  int list_pos;
  uint64_t list_size;
  int i;
  int ret;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	unsigned char *buffer;
	
	ret = 0;
  list_pos = 0;
  i = 0;
  
  if (!kr_response_is_list (kr_response)) {
    return 0;
  }
  
  if ((item_num < 0) || (item_num > (kr_response_list_length (kr_response) - 1))) {
    return 0;
  }
  
  buffer = kr_response->buffer + krad_ebml_read_element_from_frag (kr_response->buffer + list_pos, &ebml_id, &list_size);
  
  while (list_pos != list_size) {
    ret = krad_ebml_read_element_from_frag (buffer + list_pos, &ebml_id, &ebml_data_size);
    if (item_num == i) {
      return kr_response_to_item (kr_response, buffer + list_pos + ret, ebml_id, ebml_data_size, item);
    }
    list_pos += ebml_data_size + ret;
    i++;
  }

  return 0;

}

int kr_response_get_item (kr_response_t *kr_response, kr_item_t **item) {

  int pos;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
	
  pos = 0;
  
  pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);

  if (ebml_id == EBML_ID_KRAD_MIXER_CONTROL) {
    return kr_response_to_item (kr_response, kr_response->buffer + pos, ebml_id, ebml_data_size, item);
  }
  
  if (ebml_id == EBML_ID_KRAD_MIXER_PORTGROUP_CREATED) {
    pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);
    return kr_response_to_item (kr_response, kr_response->buffer + pos, ebml_id, ebml_data_size, item);
  }

  if (ebml_id == EBML_ID_KRAD_MIXER_PORTGROUP_UPDATED) {
    pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);
    return kr_response_to_item (kr_response, kr_response->buffer + pos, ebml_id, ebml_data_size, item);
  }

  return 0;
}

kr_unit_t kr_response_unit (kr_response_t *kr_response) {
  return kr_response->unit;
}

uint32_t kr_response_size (kr_response_t *kr_response) {
  return kr_response->size;
}

void kr_response_free (kr_response_t **kr_response) {
  if (*kr_response != NULL) {
    if ((*kr_response)->buffer != NULL) {
      free ((*kr_response)->buffer);
    }
    free ((*kr_response));
  }
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

void kr_client_response_wait_print (kr_client_t *client) {

  kr_response_t *response;
  char *string;
  int wait_time_ms;
  int length;

  string = NULL;
  response = NULL;  
  wait_time_ms = 250;

  if (kr_poll (client, wait_time_ms)) {
    kr_client_response_get (client, &response);
    if (response != NULL) {
      length = kr_response_to_string (response, &string);
      if (length > 0) {
        printf ("%s\n", string);
        kr_response_free_string (&string);
      }
      kr_response_free (&response);
    }
  } else {
    printf ("No response after waiting %dms\n", wait_time_ms);
  }
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

static int kr_remote_port_valid (int port) {

  if ((port >= 0) && (port <= 65535)) {
    return 1;
  }

  return 0;
}

void kr_remote_status (kr_client_t *client) {

  uint64_t command;
  uint64_t remote_status_command;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_REMOTE_STATUS, &remote_status_command);
  krad_ebml_finish_element (client->krad_ebml, remote_status_command);
  krad_ebml_finish_element (client->krad_ebml, command);
  krad_ebml_write_sync (client->krad_ebml);
}

int kr_remote_enable (kr_client_t *client, char *interface, int port) {

  uint64_t radio_command;
  uint64_t enable_remote;

  if (!kr_remote_port_valid (port)) {
    return -1;
  }

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_ENABLE, &enable_remote);  

  if ((interface != NULL) && (strlen(interface))) {
    krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_REMOTE_INTERFACE, interface);
  } else {
    krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_REMOTE_INTERFACE, "");
  }

  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

  krad_ebml_finish_element (client->krad_ebml, enable_remote);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
  
  return 1;
  
}

int kr_remote_disable (kr_client_t *client, char *interface, int port) {

  uint64_t radio_command;
  uint64_t disable_remote;

  if (!kr_remote_port_valid (port)) {
    return -1;
  }

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE, &disable_remote);

  if ((interface != NULL) && (strlen(interface))) {
    krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_REMOTE_INTERFACE, interface);
  } else {
    krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_REMOTE_INTERFACE, "");
  }
  
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

  krad_ebml_finish_element (client->krad_ebml, disable_remote);
  krad_ebml_finish_element (client->krad_ebml, radio_command);
    
  krad_ebml_write_sync (client->krad_ebml);
  
  return 1;
  
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

#define MAX_TOKENS 8

int kr_string_to_unit_control_path_address (char *string, kr_unit_control_t *uc) {

  char *pch;
  char *save;
  char *tokens[MAX_TOKENS];
  int t;
  int i;
  
  kr_unit_t *unit;
  kr_subunit_t *subunit;
  kr_subunit_address_t *address;
  kr_subunit_control_t *control;
    
  unit = &uc->path.unit;
  subunit = &uc->path.subunit;
  address = &uc->address;    
  control = &uc->path.control;
  
  i = 0;
  t = 0;
  save = NULL;

  printf ("string was %s\n", string);

  pch = strtok_r (string, "/ ", &save);
  while ((pch != NULL) && (t < MAX_TOKENS)) {
    tokens[t] = pch;
    t++;
    printf ("%s ", pch);
    pch = strtok_r (NULL, "/ ", &save);
  }

  printf ("\n%d tokens\n", t);

  for (i = 0; i < t; i++) {
    printf ("%d: %s  ", i + 1, tokens[i]);
  }
  
  printf ("\n");
  
  if (t < 2) {
    printf ("Invalid Unit Control Address: Less than 2 parts\n");
    return -1;
  }
    
  // Determine Unit
  
  if (tokens[0][1] == '\0') {
    switch (tokens[0][0]) {
      case 'm':
        *unit = KR_MIXER;
        break;
      case 'c':
        *unit = KR_COMPOSITOR;
        break;
      case 't':
        *unit = KR_TRANSPONDER;
        break;
      case 'e':
        *unit = KR_MIXER;
        subunit->mixer_subunit = KR_EFFECT;
        break;
      case 's':
        *unit = KR_COMPOSITOR;
        subunit->compositor_subunit = KR_SPRITE;
        break;
      case 'v':
        *unit = KR_COMPOSITOR;
        subunit->compositor_subunit = KR_VIDEOPORT;
        break;
    }
  } else {
    if (tokens[0][2] == '\0') {
      if ((tokens[0][0] == 'c') && (tokens[0][1] == 's')) {
        *unit = KR_COMPOSITOR;
        subunit->compositor_subunit = KR_SPRITE;
      }
      if ((tokens[0][0] == 'v') && (tokens[0][1] == 'e')) {
        *unit = KR_COMPOSITOR;
        subunit->compositor_subunit = KR_VECTOR;
      }
      if ((tokens[0][0] == 't') && (tokens[0][1] == 't')) {
        *unit = KR_COMPOSITOR;
        subunit->compositor_subunit = KR_TEXT;
      }
    } else {
      if (tokens[0][3] == '\0') {
        if ((tokens[0][0] == 'v') && (tokens[0][1] == 'e') && (tokens[0][1] == 'c')) {
          *unit = KR_COMPOSITOR;
        }
        if ((tokens[0][0] == 'm') && (tokens[0][1] == 'i') && (tokens[0][1] == 'x')) {
          *unit = KR_MIXER;
        }
        if ((tokens[0][0] == 'c') && (tokens[0][1] == 'o') && (tokens[0][1] == 'm')) {
          *unit = KR_COMPOSITOR;
        }
      } else {
        if (tokens[0][4] == '\0') {
          if ((tokens[0][0] == 't') && (tokens[0][1] == 'e') &&
              (tokens[0][1] == 'x') && (tokens[0][1] == 't')) {
            *unit = KR_COMPOSITOR;
            subunit->compositor_subunit = KR_TEXT;
          }
          if ((tokens[0][0] == 'c') && (tokens[0][1] == 'o') &&
              (tokens[0][1] == 'm') && (tokens[0][1] == 'p')) {
            *unit = KR_COMPOSITOR;
          }
        } else {
          if (tokens[0][5] == '\0') {
            if ((tokens[0][0] == 'v') && (tokens[0][1] == 'e') &&
                (tokens[0][1] == 'c') && (tokens[0][1] == 'o') && (tokens[0][1] == 'r')) {
              *unit = KR_COMPOSITOR;
              subunit->compositor_subunit = KR_VECTOR;
            }
           if ((tokens[0][0] == 'm') && (tokens[0][1] == 'i') &&
                (tokens[0][1] == 'x') && (tokens[0][1] == 'e') && (tokens[0][1] == 'r')) {
              *unit = KR_MIXER;
            }
          } else {
            if (tokens[0][6] == '\0') {
              if ((tokens[0][0] == 's') && (tokens[0][1] == 'p') && (tokens[0][1] == 'r') &&
                  (tokens[0][1] == 'i') && (tokens[0][1] == 't') && (tokens[0][1] == 'e')) {
                *unit = KR_COMPOSITOR;
                subunit->compositor_subunit = KR_SPRITE;
              }
            } else {
              if ((strlen(tokens[0]) == 10) && (strncmp(tokens[0], "compositor", 10) == 0)) {
                *unit = KR_COMPOSITOR;
              } else {
                if ((strlen(tokens[0]) == 11) && (strncmp(tokens[0], "transponder", 11) == 0)) {
                  *unit = KR_TRANSPONDER;
                }
              }
            }
          }
        }
      }
    }
  }

  if (*unit == 0) {
    printf ("Unit could not be identified\n");
    return -1;
  }
  
  if (subunit->ptr == NULL) {
    switch (*unit) {
      case KR_MIXER:
        strncpy (address->name, tokens[1], sizeof (address->name));
        if (t == 2) {
          subunit->mixer_subunit = KR_PORTGROUP;
          control->portgroup_control = KR_VOLUME;
        }
        if (t > 3) {
          subunit->mixer_subunit = KR_EFFECT;
        } else {
          subunit->mixer_subunit = KR_PORTGROUP;
        }
        if (t == 3) {
          if ((tokens[2][0] == 'f') || (tokens[2][0] == 'c')) {
            control->portgroup_control = KR_CROSSFADE;
          } else {
            if ((tokens[2][0] == 'v')) {
              control->portgroup_control = KR_VOLUME;
            } else {
              printf ("Invalid Mixer Portgroup Control\n");
              return -1;
            }
          }
        }
        break;
      case KR_COMPOSITOR:
        printf ("Invalid COMPOSITOR Control\n");
        return -1;
        break;
      case KR_TRANSPONDER:
        printf ("Invalid TRANSPONDER Control\n");
        return -1;
        break;
      case KR_RADIO:
        printf ("Invalid RADIO Control\n");
        return -1;
        break;
    }
  }
  
  if (subunit->ptr == NULL) {
    printf ("Subunit could not be identified\n");
    return -1;
  }
 
  // Print it out
 
  switch (*unit) {
    case KR_RADIO:

      break;
    case KR_MIXER:
      switch (subunit->mixer_subunit) {
        case KR_PORTGROUP:
          printf ("Mixer Unit, Portgroup: %s ", address->name);
          if (control->portgroup_control == KR_VOLUME) {
            printf ("Volume\n");
          } else {
            printf ("Crossfade\n");
          }
          break;
        case KR_EFFECT:
          printf ("Mixer Unit, Effect\n");
          break;
      }
      break;
    case KR_COMPOSITOR:
      switch (subunit->compositor_subunit) {
        case KR_VIDEOPORT:
          printf ("Compositor Unit, Videoport\n");
          break;
        case KR_SPRITE:
          printf ("Compositor Unit, Sprite\n");
          break;
        case KR_TEXT:
          printf ("Compositor Unit, Text\n");
          break;
        case KR_VECTOR:
          printf ("Compositor Unit, Vector\n");
          break;
      }
      break;
    case KR_TRANSPONDER:
      switch (subunit->transponder_subunit) {
        case KR_TRANSMITTER:
          printf ("Transponder Unit, Transmitter\n");
          break;
        case KR_RECEIVER:
          printf ("Transponder Unit, Receiver\n");
          break;
        case KR_DEMUXER:
          printf ("Transponder Unit, Demuxer\n");
          break;
        case KR_MUXER:
          printf ("Transponder Unit, Muxer\n");
          break;
        case KR_ENCODER:
          printf ("Transponder Unit, Encoder\n");
          break;
        case KR_DECODER:
          printf ("Transponder Unit, Decoder\n");
          break;
      }
      break;
    }

  return 0;
}


int kr_unit_control_set (kr_client_t *client, kr_unit_control_t *uc) {

  uint64_t command;
  uint64_t control_set;

  switch (uc->path.unit) {
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
  
  switch (uc->path.unit) {
    case KR_MIXER:
      switch (uc->path.subunit.mixer_subunit) {
        case KR_PORTGROUP:
          if (uc->path.control.portgroup_control == KR_VOLUME) {
            kr_mixer_set_control (client, uc->address.name, "volume", uc->value.real, uc->duration);
          } else {
            kr_mixer_set_control (client, uc->address.name, "crossfade", uc->value.real, uc->duration);
          }

          break;

        case KR_EFFECT:

          break;
      }
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

