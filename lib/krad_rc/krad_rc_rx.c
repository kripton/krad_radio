#include "krad_rc_rx.h"

krad_rc_rx_t *krad_rc_rx_create (krad_rc_t *krad_rc, int port) {

	krad_rc_rx_t *krad_rc_rx = calloc(1, sizeof(krad_rc_rx_t));

	krad_rc_rx->data = calloc(1, 2048);
	strcpy ((char *)krad_rc_rx->data, "KRC");

	krad_rc_rx->port = port;

	krad_rc_rx->krad_rc = krad_rc;

	krad_rc_rx->sd = socket (AF_INET, SOCK_DGRAM, 0);

	memset((char *) &krad_rc_rx->local_address, 0, sizeof(krad_rc_rx->local_address));

	krad_rc_rx->local_address.sin_family = AF_INET;
	krad_rc_rx->local_address.sin_port = htons (krad_rc_rx->port);
	krad_rc_rx->local_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (krad_rc_rx->sd, (struct sockaddr *)&krad_rc_rx->local_address, sizeof(krad_rc_rx->local_address)) == -1 ) {
		krad_rc_rx_destroy (krad_rc_rx);
		return NULL;
	}

	return krad_rc_rx;

}

void krad_rc_rx_destroy (krad_rc_rx_t *krad_rc_rx) {

	krad_rc_rx->run = 0;
	
	usleep (500000);

	close (krad_rc_rx->sd);

	free (krad_rc_rx->data);
	free (krad_rc_rx);
}


void krad_rc_rx_run (krad_rc_rx_t *krad_rc_rx) {

	int ret;
	int rsize;

	krad_rc_rx->run = 1;
	
	while (	krad_rc_rx->run == 1 ) {
	
	
		ret = recvfrom(krad_rc_rx->sd, krad_rc_rx->data, 2000, 0, (struct sockaddr *)&krad_rc_rx->remote_address, (socklen_t *)&rsize);
		
		if (ret == -1) {
			krad_rc_rx->run = 0;
			break;
		}
		
		//printf ("got packet! %d\n", ret);		
		
		krad_rc_command ( krad_rc_rx->krad_rc, krad_rc_rx->data + 3);
	
	}

}
