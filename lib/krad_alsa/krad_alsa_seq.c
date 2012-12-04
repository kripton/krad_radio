#include "krad_alsa_seq.h"


#ifdef KLUDGE
void krad_alsa_seq_set_kr_client (krad_alsa_seq_t *krad_alsa_seq, kr_client_t *kr_client) {
		krad_alsa_seq->kr_client = kr_client;
}
#endif

void *krad_alsa_seq_running_thread (void *arg) {

	krad_alsa_seq_t *krad_alsa_seq = (krad_alsa_seq_t *)arg;

	int ret;
	int sock_count;
	struct pollfd sockets[16];
	snd_seq_event_t *ev;
	unsigned int c;
	float value;

	usleep (100000);

	printk ("Krad alsa_seq running thread starting\n");

	ret = 0;

	sock_count = snd_seq_poll_descriptors_count (krad_alsa_seq->seq_handle, POLLIN);
	snd_seq_poll_descriptors (krad_alsa_seq->seq_handle, sockets, sock_count, POLLIN);

	//printk ("sock_count %d\n", sock_count);

	while (krad_alsa_seq->stop == 0) {

		//sockets[0].fd = krad_alsa_seq->sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, 250);	

		if (ret < 0) {
			printke ("Krad alsa_seq Failed on poll\n");
			krad_alsa_seq->stop = 1;
			break;
		}

		if (ret > 0) {

			while ((snd_seq_event_input (krad_alsa_seq->seq_handle, &ev)) && (krad_alsa_seq->stop == 0)) {

				if (ev != NULL) {

					switch (ev->type) {

						case SND_SEQ_EVENT_CONTROLLER:

							c = ev->data.control.channel;
							printk ("Krad ALSA Seq Control event Channel %2d: %2d %5d\n",
									c, ev->data.control.param, ev->data.control.value);

								#ifdef KLUDGE
									// 10 cross, 13, 14
									if ((ev->data.control.param == 10) || (ev->data.control.param == 17)) {
										value = ((ev->data.control.value / 127.0) * 200.0) - 100.0;
										krad_ipc_set_control (krad_alsa_seq->kr_client, "Music1", 
															  "crossfade", value);
									}
									if ((ev->data.control.param == 12) || (ev->data.control.param == 13)) {
										value = (ev->data.control.value / 127.0) * 100.0;
										krad_ipc_set_control (krad_alsa_seq->kr_client, "Music1", 
															  "volume", value);
									}
									if (ev->data.control.param == 14) {
										value = (ev->data.control.value / 127.0) * 100.0;
										krad_ipc_set_control (krad_alsa_seq->kr_client, "Music2", 
															  "volume", value);
									}
								#endif

							break;

            case SND_SEQ_EVENT_PITCHBEND:

							c = ev->data.control.channel;

							printk ("Pitchbender event on Channel %2d: %5d   \n",
									c, ev->data.control.value);

              break;

            case SND_SEQ_EVENT_CHANPRESS:

							c = ev->data.control.channel;

							printk ("touch event on Channel %2d: %5d   \n",
									c, ev->data.control.value);

							break;

            case SND_SEQ_EVENT_NOTEON:

							c = ev->data.note.channel;

                printk ("Note On event on Channel %2d: %5d %5d      \n",
                        c, ev->data.note.note,ev->data.note.velocity);
							break;

            case SND_SEQ_EVENT_NOTEOFF:
							c = ev->data.note.channel;
							printk ("Note Off event on Channel %2d: %5d      \n",
									c, ev->data.note.note);
							break;
            }
          }
			}
		}
	}

	ret = snd_seq_delete_port (krad_alsa_seq->seq_handle, krad_alsa_seq->port_id);
	if (ret < 0) {
		printke ("Krad ALSA Seq Error snd_seq_delete_port.\n");
	}	
	ret = snd_seq_close (krad_alsa_seq->seq_handle);
	if (ret < 0) {
		printke ("Krad ALSA Seq Error snd_seq_close.\n");
	}

	krad_alsa_seq->running = 0;	

	printk ("Krad alsa_seq running thread exiting\n");

	return NULL;
}

void krad_alsa_seq_stop (krad_alsa_seq_t *krad_alsa_seq) {

	if (krad_alsa_seq->running == 1) {
		krad_alsa_seq->stop = 1;
		pthread_join (krad_alsa_seq->running_thread, NULL);
		krad_alsa_seq->stop = 0;
	}
}


int krad_alsa_seq_run (krad_alsa_seq_t *krad_alsa_seq, char *name) {

	int err;

	if (krad_alsa_seq->running == 1) {
		krad_alsa_seq_stop (krad_alsa_seq);
	}

	strcpy (krad_alsa_seq->name, name);
	krad_alsa_seq->running = 1;

	err = snd_seq_open (&krad_alsa_seq->seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
	if (err < 0) {
		printke ("Krad ALSA Seq Error snd_seq_open.\n");
		krad_alsa_seq->running = 0;
		return 1;
	}

	snd_seq_set_client_name (krad_alsa_seq->seq_handle, krad_alsa_seq->name);

	krad_alsa_seq->port_id = snd_seq_create_simple_port (krad_alsa_seq->seq_handle, "KInput",
							 SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
							 SND_SEQ_PORT_TYPE_APPLICATION);

	if (krad_alsa_seq->port_id < 0) {
		printke ("Krad ALSA Seq Error creating sequencer port.\n");
		krad_alsa_seq->running = 0;
		return 1;		
	}

	pthread_create (&krad_alsa_seq->running_thread, NULL, krad_alsa_seq_running_thread, (void *)krad_alsa_seq);

	return 0;
}

void krad_alsa_seq_destroy (krad_alsa_seq_t *krad_alsa_seq) {

	if (krad_alsa_seq->running == 1) {
		krad_alsa_seq_stop (krad_alsa_seq);
	}

	free (krad_alsa_seq->buffer);

	free (krad_alsa_seq);

}

krad_alsa_seq_t *krad_alsa_seq_create () {

	krad_alsa_seq_t *krad_alsa_seq = calloc (1, sizeof(krad_alsa_seq_t));

	krad_alsa_seq->buffer = calloc (1, 4000);


	return krad_alsa_seq;

}
