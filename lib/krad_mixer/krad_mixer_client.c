#include "krad_radio_client.h"
#include "krad_radio_client_internal.h"
#include "krad_mixer_common.h"

typedef struct kr_audioport_St kr_audioport_t;

struct kr_audioport_St {

	int samplerate;
	kr_shm_t *kr_shm;
	kr_client_t *client;
	int sd;
	
	krad_mixer_portgroup_direction_t direction;
	
	int (*callback)(uint32_t, void *);
	void *pointer;
	
	int active;
	
	pthread_t process_thread;	
	
};



void kr_mixer_portgroups_list (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t get_portgroups;
	//uint64_t tag;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_LIST_PORTGROUPS, &get_portgroups);	
	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG, &tag);	

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_NAME, tag_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_RADIO_TAG_VALUE, "");	

	//krad_ebml_finish_element (client->krad_ebml, tag);
	krad_ebml_finish_element (client->krad_ebml, get_portgroups);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void kr_audioport_destroy_cmd (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t destroy_audioport;
	
	compositor_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_LOCAL_AUDIOPORT_DESTROY, &destroy_audioport);

	//krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_LINK_LINK_NUMBER, number);

	krad_ebml_finish_element (client->krad_ebml, destroy_audioport);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_audioport_create_cmd (kr_client_t *client, krad_mixer_portgroup_direction_t direction) {

	//uint64_t ipc_command;
	uint64_t compositor_command;
	uint64_t create_audioport;
	
	compositor_command = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &compositor_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_LOCAL_AUDIOPORT_CREATE, &create_audioport);

	if (direction == OUTPUT) {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION, "output");
	} else {
		krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION, "input");
	}
	
	krad_ebml_finish_element (client->krad_ebml, create_audioport);
	krad_ebml_finish_element (client->krad_ebml, compositor_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


float *kr_audioport_get_buffer (kr_audioport_t *kr_audioport, int channel) {
	return (float *)kr_audioport->kr_shm->buffer + (channel * 1600);
}


void kr_audioport_set_callback (kr_audioport_t *kr_audioport, int callback (uint32_t, void *), void *pointer) {

	kr_audioport->callback = callback;
	kr_audioport->pointer = pointer;

}

void *kr_audioport_process_thread (void *arg) {

	kr_audioport_t *kr_audioport = (kr_audioport_t *)arg;
  int ret;
	char buf[1];
	
	krad_system_set_thread_name ("krc_audioport");

	while (kr_audioport->active == 1) {
	
		// wait for socket to have a byte
		ret = read (kr_audioport->sd, buf, 1);
    if (ret != 1) {
      printke ("krad mixer client: unexpected read return value %d in kr_audioport_process_thread", ret);
    }
		//kr_audioport->callback (kr_audioport->kr_shm->buffer, kr_audioport->pointer);
		kr_audioport->callback (1600, kr_audioport->pointer);

		// write a byte to socket
		ret = write (kr_audioport->sd, buf, 1);
    if (ret != 1) {
      printke ("krad mixer client: unexpected write return value %d in kr_audioport_process_thread", ret);
    }

	}


	return NULL;

}

void kr_audioport_activate (kr_audioport_t *kr_audioport) {
	if ((kr_audioport->active == 0) && (kr_audioport->callback != NULL)) {
		pthread_create (&kr_audioport->process_thread, NULL, kr_audioport_process_thread, (void *)kr_audioport);
		kr_audioport->active = 1;
	}
}

void kr_audioport_deactivate (kr_audioport_t *kr_audioport) {

	if (kr_audioport->active == 1) {
		kr_audioport->active = 2;
		pthread_join (kr_audioport->process_thread, NULL);
		kr_audioport->active = 0;
	}
}

kr_audioport_t *kr_audioport_create (kr_client_t *client, krad_mixer_portgroup_direction_t direction) {

	kr_audioport_t *kr_audioport;
	int sockets[2];

	if (!kr_client_local (client)) {
		// Local clients only
		return NULL;
	}

	kr_audioport = calloc (1, sizeof(kr_audioport_t));

	if (kr_audioport == NULL) {
		return NULL;
	}

	kr_audioport->client = client;
	kr_audioport->direction = direction;

	kr_audioport->kr_shm = kr_shm_create (kr_audioport->client);

	//sprintf (kr_audioport->kr_shm->buffer, "waa hoo its yaytime");

	if (kr_audioport->kr_shm == NULL) {
		free (kr_audioport);
		return NULL;
	}

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
		kr_shm_destroy (kr_audioport->kr_shm);
		free (kr_audioport);
		return NULL;
    }

	kr_audioport->sd = sockets[0];
	
	printf ("sockets %d and %d\n", sockets[0], sockets[1]);
	
	kr_audioport_create_cmd (kr_audioport->client, kr_audioport->direction);
	//FIXME use a return message from daemon to indicate ready to receive fds
	usleep (33000);
	kr_send_fd (kr_audioport->client, kr_audioport->kr_shm->fd);
	usleep (33000);
	kr_send_fd (kr_audioport->client, sockets[1]);
	usleep (33000);
	return kr_audioport;

}

void kr_audioport_destroy (kr_audioport_t *kr_audioport) {

	if (kr_audioport->active == 1) {
		kr_audioport_deactivate (kr_audioport);
	}

	kr_audioport_destroy_cmd (kr_audioport->client);

	if (kr_audioport != NULL) {
		if (kr_audioport->sd != 0) {
			close (kr_audioport->sd);
			kr_audioport->sd = 0;
		}
		if (kr_audioport->kr_shm != NULL) {
			kr_shm_destroy (kr_audioport->kr_shm);
			kr_audioport->kr_shm = NULL;
		}
		free(kr_audioport);
	}
}




int kr_mixer_read_control ( kr_client_t *client, char **portgroup_name, char **control_name, float *value ) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
		//printf("hrm wtf1\n");
	}
	krad_ebml_read_string (client->krad_ebml, *portgroup_name, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_NAME) {
		//printf("hrm wtf2\n");
	}
	krad_ebml_read_string (client->krad_ebml, *control_name, ebml_data_size);
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_CONTROL_VALUE) {
		//printf("hrm wtf3\n");
	}
	
	*value = krad_ebml_read_float (client->krad_ebml, ebml_data_size);

	//printf("krad_ipc_client_read_mixer_control %s %s %f\n", *portgroup_name, *control_name, *value );
		
	return 0;		
						
}


void kr_mixer_portgroup_xmms2_cmd (kr_client_t *client, char *portgroupname, char *xmms2_cmd) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t bind;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_PORTGROUP_XMMS2_CMD, &bind);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_XMMS2_CMD, xmms2_cmd);
	krad_ebml_finish_element (client->krad_ebml, bind);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_bind_portgroup_xmms2 (kr_client_t *client, char *portgroupname, char *ipc_path) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t bind;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_BIND_PORTGROUP_XMMS2, &bind);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_XMMS2_IPC_PATH, ipc_path);
	krad_ebml_finish_element (client->krad_ebml, bind);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_unbind_portgroup_xmms2 (kr_client_t *client, char *portgroupname) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t unbind;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UNBIND_PORTGROUP_XMMS2, &unbind);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_finish_element (client->krad_ebml, unbind);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_set_sample_rate (kr_client_t *client, int sample_rate) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t set_sample_rate;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_SET_SAMPLE_RATE, &set_sample_rate);	

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_SAMPLE_RATE, sample_rate);

	krad_ebml_finish_element (client->krad_ebml, set_sample_rate);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_sample_rate (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t get_sample_rate;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_GET_SAMPLE_RATE, &get_sample_rate);
	krad_ebml_finish_element (client->krad_ebml, get_sample_rate);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_plug_portgroup (kr_client_t *client, char *name, char *remote_name) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t plug;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_PLUG_PORTGROUP, &plug);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, remote_name);
	krad_ebml_finish_element (client->krad_ebml, plug);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_unplug_portgroup (kr_client_t *client, char *name, char *remote_name) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t unplug;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UNPLUG_PORTGROUP, &unplug);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, remote_name);
	krad_ebml_finish_element (client->krad_ebml, unplug);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_create_portgroup (kr_client_t *client, char *name, char *direction, int channels) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t create;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_CREATE_PORTGROUP, &create);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_DIRECTION, direction);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS, channels);
	krad_ebml_finish_element (client->krad_ebml, create);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void kr_mixer_push_tone (kr_client_t *client, char *tone) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t push;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_PUSH_TONE, &push);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_TONE_NAME, tone);
	
	krad_ebml_finish_element (client->krad_ebml, push);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_update_portgroup_map_channel (kr_client_t *client, char *portgroupname, int in_channel, int out_channel) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t update;
	uint64_t map;


	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP, &update);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_MAP_CHANNEL, &map);
	krad_ebml_finish_element (client->krad_ebml, map);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL, in_channel);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL, out_channel);
	
	
	
	krad_ebml_finish_element (client->krad_ebml, update);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_update_portgroup_mixmap_channel (kr_client_t *client, char *portgroupname, int in_channel, int out_channel) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t update;
	uint64_t map;


	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP, &update);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_MIXMAP_CHANNEL, &map);
	krad_ebml_finish_element (client->krad_ebml, map);

	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL, in_channel);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_CHANNEL, out_channel);
	
	
	
	krad_ebml_finish_element (client->krad_ebml, update);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}


void kr_mixer_update_portgroup (kr_client_t *client, char *portgroupname, uint64_t update_command, char *string) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t update;



	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_UPDATE_PORTGROUP, &update);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);
	krad_ebml_write_string (client->krad_ebml, update_command, string);
	
	
	krad_ebml_finish_element (client->krad_ebml, update);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_portgroup_info (kr_client_t *client, char *portgroupname) {

	uint64_t command;
	uint64_t info;

	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_PORTGROUP_INFO, &info);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroupname);

	krad_ebml_finish_element (client->krad_ebml, info);
	krad_ebml_finish_element (client->krad_ebml, command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_remove_portgroup (kr_client_t *client, char *name) {

	//uint64_t ipc_command;
	uint64_t command;
	uint64_t destroy;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_DESTROY_PORTGROUP, &destroy);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, name);

	krad_ebml_finish_element (client->krad_ebml, destroy);
	krad_ebml_finish_element (client->krad_ebml, command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_jack_running (kr_client_t *client) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t jack_running;
	
	mixer_command = 0;
	//set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_JACK_RUNNING, &jack_running);

	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	//krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	//krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, jack_running);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);


}


void kr_mixer_add_effect (kr_client_t *client, char *portgroup_name, char *effect_name) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t add_effect;
	
	mixer_command = 0;
	add_effect = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_ADD_EFFECT, &add_effect);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_EFFECT_NAME, effect_name);

	krad_ebml_finish_element (client->krad_ebml, add_effect);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_remove_effect (kr_client_t *client, char *portgroup_name, int effect_num) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t remove_effect;
	
	mixer_command = 0;
	remove_effect = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_REMOVE_EFFECT, &remove_effect);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_EFFECT_NUM, effect_num);

	krad_ebml_finish_element (client->krad_ebml, remove_effect);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_set_effect_control (kr_client_t *client, char *portgroup_name, int effect_num, 
                                  char *control_name, int subunit, float control_value) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t set_control;
	
	mixer_command = 0;
	set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_SET_EFFECT_CONTROL, &set_control);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_EFFECT_NUM, effect_num);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_SUBUNIT, subunit);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);

	krad_ebml_finish_element (client->krad_ebml, set_control);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

void kr_mixer_set_control (kr_client_t *client, char *portgroup_name, char *control_name, float control_value, int duration) {

	//uint64_t ipc_command;
	uint64_t mixer_command;
	uint64_t set_control;
	
	mixer_command = 0;
	set_control = 0;

	//krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_IPC_CMD, &ipc_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD, &mixer_command);
	krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_MIXER_CMD_SET_CONTROL, &set_control);

	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_PORTGROUP_NAME, portgroup_name);
	krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_NAME, control_name);
	krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_VALUE, control_value);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_MIXER_CONTROL_DURATION, duration);

	krad_ebml_finish_element (client->krad_ebml, set_control);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}

kr_mixer_portgroup_control_rep_t *kr_ebml_to_mixer_portgroup_control_rep (unsigned char *ebml_frag,
                                                            kr_mixer_portgroup_control_rep_t **kr_mixer_portgroup_control_repn) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
  int item_pos;
  kr_mixer_portgroup_control_rep_t *kr_mixer_portgroup_control_rep;

  item_pos = 0;
  *kr_mixer_portgroup_control_repn = kr_mixer_portgroup_control_rep_create();

  kr_mixer_portgroup_control_rep = *kr_mixer_portgroup_control_repn;
  
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
	item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, kr_mixer_portgroup_control_rep->name);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
	item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, kr_mixer_portgroup_control_rep->control);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_mixer_portgroup_control_rep->value = krad_ebml_read_float_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	return kr_mixer_portgroup_control_rep;

}

krad_mixer_portgroup_rep_t *kr_ebml_to_mixer_portgroup_rep (unsigned char *ebml_frag,
                                                            krad_mixer_portgroup_rep_t **krad_mixer_portgroup_repn) {

  uint32_t ebml_id;
	uint64_t ebml_data_size;
  krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep;
  krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep_crossfade;
  int i;
  char string[256];
  char crossfade_name[128];
	float crossfade_value;
  int item_pos;

  item_pos = 0;

  *krad_mixer_portgroup_repn = krad_mixer_portgroup_rep_create();

  krad_mixer_portgroup_rep = *krad_mixer_portgroup_repn;
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
	item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, krad_mixer_portgroup_rep->sysname);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  krad_mixer_portgroup_rep->channels = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
	item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, string);

  if (strncmp (string, "Jack", 4) == 0) {
    krad_mixer_portgroup_rep->io_type = 0;
  } else {
    krad_mixer_portgroup_rep->io_type = 1;    
  }
	
  for (i = 0; i < krad_mixer_portgroup_rep->channels; i++) {
    item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
    if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
      //printk ("hrm wtf3");
    } 
    krad_mixer_portgroup_rep->volume[i] = krad_ebml_read_float_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  }
	
  for (i = 0; i < krad_mixer_portgroup_rep->channels; i++) {
    item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
    if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
      //printk ("hrm wtf3");
    } 
    krad_mixer_portgroup_rep->peak[i] = krad_ebml_read_float_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  }
  
  for (i = 0; i < krad_mixer_portgroup_rep->channels; i++) {
    item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
    if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
      //printk ("hrm wtf3");
    } 
    krad_mixer_portgroup_rep->rms[i] = krad_ebml_read_float_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  }
  
  item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS) {
		//printk ("hrm wtf2");
	} 
	item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, krad_mixer_portgroup_rep->mixbus_rep->sysname);	
	
	//printk ("Bus: %s", string);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, crossfade_name);	

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}

	crossfade_value = krad_ebml_read_float_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
	
	if (strlen(crossfade_name)) {
    krad_mixer_portgroup_rep_crossfade = krad_mixer_portgroup_rep_create ();
    strcpy (krad_mixer_portgroup_rep_crossfade->sysname, crossfade_name);
    
    krad_mixer_portgroup_rep->crossfade_group_rep = krad_mixer_crossfade_rep_create (krad_mixer_portgroup_rep,
                                                                                         krad_mixer_portgroup_rep_crossfade);
    krad_mixer_portgroup_rep->crossfade_group_rep->fade = crossfade_value;
	} 
	
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_XMMS2) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}
	
	krad_mixer_portgroup_rep->has_xmms2 = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
	
	return krad_mixer_portgroup_rep;
}

int kr_mixer_response_get_string_from_portgroup (unsigned char *ebml_frag, uint64_t item_size, char **string) {

	int pos;
  krad_mixer_portgroup_rep_t *krad_mixer_portgroup_rep;

  pos = 0;
  krad_mixer_portgroup_rep = NULL;
  
  kr_ebml_to_mixer_portgroup_rep (ebml_frag, &krad_mixer_portgroup_rep);

  pos += sprintf (*string + pos, "  Peak_L: %0.2f  Peak_R: %0.2f RMS_L: %0.2f  RMS_R: %0.2f  Vol: %0.2f%%  %s", 
            10.0 * log ((double) krad_mixer_portgroup_rep->peak[0]),  10.0 * log ((double) krad_mixer_portgroup_rep->peak[1]), 
           krad_mixer_portgroup_rep->rms[0], krad_mixer_portgroup_rep->rms[1],
           krad_mixer_portgroup_rep->volume[0], krad_mixer_portgroup_rep->sysname);
 
  if (krad_mixer_portgroup_rep->crossfade_group_rep != NULL) {
    pos += sprintf (*string + pos, "\t %s < %0.2f > %s",
                    krad_mixer_portgroup_rep->crossfade_group_rep->portgroup_rep[0]->sysname,
                    krad_mixer_portgroup_rep->crossfade_group_rep->fade,
                    krad_mixer_portgroup_rep->crossfade_group_rep->portgroup_rep[1]->sysname);
  }
  
  if (krad_mixer_portgroup_rep->has_xmms2 == 1) {
    pos += sprintf (*string + pos, "\t\t[XMMS2]");
  }

  pos += sprintf (*string + pos, "\n");

  krad_mixer_portgroup_rep_destroy (krad_mixer_portgroup_rep);
  
  return pos; 
}

int kr_mixer_response_to_string (kr_response_t *kr_response, char **string) {

  int ret;
  int pos;
  int rpos;
	uint32_t ebml_id;
	uint64_t ebml_data_size;
  kr_item_t *item;

  item = NULL;
  ret = 0;
  pos = 0;
  rpos = 0;
  
  pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);

  switch ( ebml_id ) {
    case EBML_ID_KRAD_MIXER_CONTROL:
      *string = kr_response_alloc_string (ebml_data_size * 4 + 64);
      rpos += sprintf (*string + rpos, "Mixer Portgroup Control: ");
      pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);
      ret += kr_response_read_into_string (kr_response->buffer + pos, ebml_data_size, *string + rpos);
      pos += ret - 1;
      rpos += ret - 1;
      rpos += sprintf (*string + rpos, " ");
      pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);
      ret = kr_response_read_into_string (kr_response->buffer + pos, ebml_data_size, *string + rpos);
      pos += ret - 1;
      rpos += ret - 1;
      pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);
      rpos += sprintf (*string + rpos, " %0.2f%%", krad_ebml_read_float_from_frag_add (kr_response->buffer + pos, ebml_data_size, &pos));
      return rpos;
    case EBML_ID_KRAD_MIXER_PORTGROUP_LIST:
      //printf("Received KRAD_MIXER_PORTGROUP_LIST %"PRIu64" bytes of data.\n", ebml_data_size);
      *string = kr_response_alloc_string (ebml_data_size * 4);
      return kr_response_get_string_from_list (ebml_id, kr_response->buffer + pos, ebml_data_size, string);
//    case EBML_ID_KRAD_MIXER_PORTGROUP_DESTROYED:
//      *string = kr_response_alloc_string (ebml_data_size * 4 + 32);
//      rpos += sprintf (*string + rpos, "Mixer Portgroup Destroyed: ");
//      pos += krad_ebml_read_element_from_frag (kr_response->buffer + pos, &ebml_id, &ebml_data_size);
//      rpos += kr_response_read_into_string (kr_response->buffer + pos, ebml_data_size, *string + rpos);
//      return rpos;
    case EBML_ID_KRAD_MIXER_PORTGROUP_CREATED:
      *string = kr_response_alloc_string (ebml_data_size * 4 + 32);
      rpos += sprintf (*string + rpos, "Mixer Portgroup Created: ");
      if (kr_response_get_item (kr_response, &item)) {
        rpos += kr_item_read_into_string (item, *string + rpos);
      }
      return rpos;
    case EBML_ID_KRAD_MIXER_PORTGROUP:
      //printf("Received KRAD_MIXER_PORTGROUP %"PRIu64" bytes of data.\n", ebml_data_size);
      *string = kr_response_alloc_string (ebml_data_size * 4);
      return kr_mixer_response_get_string_from_portgroup (kr_response->buffer + pos, ebml_data_size, string);
    case EBML_ID_KRAD_MIXER_SAMPLE_RATE:
      //printf("Received KRAD_MIXER_SAMPLE_RATE %"PRIu64" bytes of data.\n", ebml_data_size);
      //printf("Received System Info %"PRIu64" bytes of data.\n", ebml_data_size);
      //pos += kr_response_print_string (kr_response->buffer + pos, ebml_data_size);
      return 0;
    case EBML_ID_KRAD_MIXER_JACK_RUNNING:
      //printf("Received KRAD_MIXER_JACK_RUNNING %"PRIu64" bytes of data.\n", ebml_data_size);
      //printf("Received Logname %"PRIu64" bytes of data.\n", ebml_data_size);
      //pos += kr_response_print_string (kr_response->buffer + pos, ebml_data_size);
      return 0;
  }
  
  return 0;
}

