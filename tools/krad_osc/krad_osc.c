#include "krad_osc.h"

void krad_osc_parse_message (krad_osc_t *krad_osc, unsigned char *message, int size) {

	int len;
	char *typetag;

	if (message[0] == '#') {
		printk ("Krad OSC message is a bundle %d bytes big\n", size);
	} else {
		printk ("Krad OSC message address pattern: %s\n", message);
		len = strlen ((char *)message) + 1;
		while ((len % 4) != 0) {
			len++;
		}
		typetag = (char *)message + len;
		printk ("Krad OSC message typetag: %s\n", typetag);
		
		
		
	}

}

void *krad_osc_listening_thread (void *arg) {

	krad_osc_t *krad_osc = (krad_osc_t *)arg;

	int ret;
	int addr_size;
	struct sockaddr_in remote_address;
	struct pollfd sockets[1];
	
	printk ("Krad OSC Listening thread starting\n");
	
	addr_size = 0;
	ret = 0;
	memset (&remote_address, 0, sizeof(remote_address));	

	addr_size = sizeof (remote_address);
	
	while (krad_osc->stop_listening == 0) {

		sockets[0].fd = krad_osc->sd;
		sockets[0].events = POLLIN;

		ret = poll (sockets, 1, 250);	

		if (ret < 0) {
			printke ("Krad OSC Failed on poll\n");
			krad_osc->stop_listening = 1;
			break;
		}
	
		if (ret > 0) {
			ret = recvfrom (krad_osc->sd, krad_osc->buffer, 4000, 0, 
							(struct sockaddr *)&remote_address, (socklen_t *)&addr_size);
		
			if (ret == -1) {
				printke ("Krad OSC Failed on recvfrom\n");
				krad_osc->stop_listening = 1;
				break;
			}
			
			if (ret > 0) {
				krad_osc_parse_message (krad_osc, krad_osc->buffer, ret);
			}
		}
	}
	
	close (krad_osc->sd);
	krad_osc->port = 0;
	krad_osc->listening = 0;	

	printk ("Krad OSC Listening thread exiting\n");

	return NULL;
}

void krad_osc_stop_listening (krad_osc_t *krad_osc) {

	if (krad_osc->listening == 1) {
		krad_osc->stop_listening = 1;
		pthread_join (krad_osc->listening_thread, NULL);
		krad_osc->stop_listening = 0;
	}
}


int krad_osc_listen (krad_osc_t *krad_osc, int port) {

	if (krad_osc->listening == 1) {
		krad_osc_stop_listening (krad_osc);
	}

	krad_osc->port = port;
	krad_osc->listening = 1;
	
	krad_osc->local_address.sin_family = AF_INET;
	krad_osc->local_address.sin_port = htons (krad_osc->port);
	krad_osc->local_address.sin_addr.s_addr = htonl (INADDR_ANY);
	
	if ((krad_osc->sd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
		printke ("Krad OSC system call socket error\n");
		krad_osc->listening = 0;
		krad_osc->port = 0;		
		return 1;
	}

	if (bind (krad_osc->sd, (struct sockaddr *)&krad_osc->local_address, sizeof(krad_osc->local_address)) == -1) {
		printke ("Krad OSC bind error for udp port %d\n", krad_osc->port);
		krad_osc->listening = 0;
		krad_osc->port = 0;
		return 1;
	}
	
	pthread_create (&krad_osc->listening_thread, NULL, krad_osc_listening_thread, (void *)krad_osc);
	
	return 0;
}


void krad_osc_destroy (krad_osc_t *krad_osc) {

	if (krad_osc->listening == 1) {
		krad_osc_stop_listening (krad_osc);
	}

	free (krad_osc->buffer);

	free (krad_osc);

}

krad_osc_t *krad_osc_create () {

	krad_osc_t *krad_osc = calloc (1, sizeof(krad_osc_t));

	krad_osc->buffer = calloc (1, 4000);

	
	return krad_osc;

}
