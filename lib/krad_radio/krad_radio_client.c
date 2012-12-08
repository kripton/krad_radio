#include "krad_radio_client.h"
#include "krad_radio_client_internal.h"

#include "krad_mixer_common.h"
#include "krad_transponder_common.h"
#include "krad_compositor_client.h"



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

void kr_set_dir (kr_client_t *client, char *dir) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t setdir;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_SET_DIR, &setdir);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_DIR, dir);
	
	krad_ebml_finish_element (client->krad_ebml, setdir);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void kr_uptime (kr_client_t *client) {

	uint64_t command;
	uint64_t uptime_command;
	command = 0;
	uptime_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_UPTIME, &uptime_command);
	krad_ebml_finish_element (client->krad_ebml, uptime_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_system_info (kr_client_t *client) {

	uint64_t command;
	uint64_t info_command;
	command = 0;
	info_command = 0;
	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &command);

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_GET_SYSTEM_INFO, &info_command);
	krad_ebml_finish_element (client->krad_ebml, info_command);

	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);	
	
}


void kr_get_logname (kr_client_t *client) {

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

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t enable_remote;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_ENABLE, &enable_remote);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_TCP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_remote);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_remote_disable (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t disable_remote;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_REMOTE_DISABLE, &disable_remote);

	krad_ebml_finish_element (client->krad_ebml, disable_remote);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_web_enable (kr_client_t *client, int http_port, int websocket_port,
					char *headcode, char *header, char *footer) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t webon;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_ENABLE, &webon);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_HTTP_PORT, http_port);	
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_WEBSOCKET_PORT, websocket_port);	

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_HEADCODE, headcode);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_HEADER, header);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_WEB_FOOTER, footer);
	
	krad_ebml_finish_element (client->krad_ebml, webon);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_web_disable (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t weboff;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_WEB_DISABLE, &weboff);

	krad_ebml_finish_element (client->krad_ebml, weboff);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_osc_enable (kr_client_t *client, int port) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t enable_osc;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_OSC_ENABLE, &enable_osc);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_RADIO_UDP_PORT, port);

	krad_ebml_finish_element (client->krad_ebml, enable_osc);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_osc_disable (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t disable_osc;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD, &radio_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_CMD_OSC_DISABLE, &disable_osc);

	krad_ebml_finish_element (client->krad_ebml, disable_osc);
	krad_ebml_finish_element (client->krad_ebml, radio_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


/* Tagging */

void krad_ipc_client_read_tag_inner ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
/*
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	if (ebml_id != EBML_ID_KRAD_RADIO_TAG) {
		printf("hrm wtf\n");
	} else {
		//printf("tag size %zu\n", ebml_data_size);
	}
*/

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

void kr_get_tags (kr_client_t *client, char *item) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t get_tags;
	//uint64_t tag;

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

void kr_get_tag (kr_client_t *client, char *item, char *tag_name) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t get_tag;
	uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
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
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void kr_set_tag (kr_client_t *client, char *item, char *tag_name, char *tag_value) {

	//uint64_t ipc_command;
	uint64_t radio_command;
	uint64_t set_tag;
	uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
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
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}
