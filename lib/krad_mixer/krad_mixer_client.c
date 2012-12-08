#include "krad_mixer_client.h"


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

	krad_system_set_thread_name ("krc_audioport");

	char buf[1];

	while (kr_audioport->active == 1) {
	
		// wait for socket to have a byte
		read (kr_audioport->sd, buf, 1);
	
		//kr_audioport->callback (kr_audioport->kr_shm->buffer, kr_audioport->pointer);
		kr_audioport->callback (1600, kr_audioport->pointer);

		// write a byte to socket
		write (kr_audioport->sd, buf, 1);


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

	if (client->tcp_port != 0) {
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
	kr_client_sendfd (kr_audioport->client, kr_audioport->kr_shm->fd);
	usleep (33000);
	kr_client_sendfd (kr_audioport->client, sockets[1]);
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

void kr_mixer_set_mixer_sample_rate (kr_client_t *client, int sample_rate) {

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

int kr_mixer_read_portgroup ( kr_client_t *client, char *portname, float *volume, char *crossfade_name, float *crossfade, int *has_xmms2) {

	uint32_t ebml_id;
	uint64_t ebml_data_size;
	int bytes_read;
	
	char string[1024];
	float floaty;
	
	int8_t channels;
	
	bytes_read = 0;

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
	
	bytes_read += ebml_data_size + 9;
	
	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP) {
		//printk ("hrm wtf");
	} else {
		//printk ("tag size %"PRIu64"", ebml_data_size);
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_NAME) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, portname, ebml_data_size);
	
	//printk ("Input name: %s", portname);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CHANNELS) {
		//printk ("hrm wtf3");
	} else {
		//printk ("tag value size %"PRIu64"", ebml_data_size);
	}

	channels = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	printk  ("Channels: %d", channels);

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_TYPE) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);
	
	//printk ("Type: %s", string);	
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_VOLUME) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}

	floaty = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	*volume = floaty;
	
	//printk ("Volume: %f", floaty);
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_MIXBUS) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, string, ebml_data_size);	
	
	//printk ("Bus: %s", string);

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);	

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE_NAME) {
		//printk ("hrm wtf2");
	} else {
		//printk ("tag name size %"PRIu64"", ebml_data_size);
	}

	krad_ebml_read_string (client->krad_ebml, crossfade_name, ebml_data_size);	
	
	
	if (strlen(crossfade_name)) {
		//printk ("Crossfade With: %s", crossfade_name);	
	}

	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_CROSSFADE) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}

	floaty = krad_ebml_read_float (client->krad_ebml, ebml_data_size);
	
	if (strlen(crossfade_name)) {
		*crossfade = floaty;
		//printk ("Crossfade: %f", floaty);
	} else {
		*crossfade = 0.0f;
	}
	
	krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);

	if (ebml_id != EBML_ID_KRAD_MIXER_PORTGROUP_XMMS2) {
		//printk ("hrm wtf3");
	} else {
		//printk ("VOLUME value size %"PRIu64"", ebml_data_size);
	}
	
	*has_xmms2 = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
	
	return bytes_read;
	
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

void kr_mixer_set_control (kr_client_t *client, char *portgroup_name, char *control_name, float control_value) {

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

	krad_ebml_finish_element (client->krad_ebml, set_control);
	krad_ebml_finish_element (client->krad_ebml, mixer_command);
	//krad_ebml_finish_element (client->krad_ebml, ipc_command);
		
	krad_ebml_write_sync (client->krad_ebml);

}
