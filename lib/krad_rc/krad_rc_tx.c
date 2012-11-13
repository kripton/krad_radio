#include "krad_rc_tx.h"

krad_rc_tx_t *krad_rc_tx_create (char *ip, int port) {

	krad_rc_tx_t *krad_rc_tx = calloc(1, sizeof(krad_rc_tx_t));

	krad_rc_tx->data = calloc(1, 2048);
	strcpy ((char *)krad_rc_tx->data, "KRC");

	krad_rc_tx->ip = strdup (ip);
	krad_rc_tx->port = port;

	krad_rc_tx->sd = socket (AF_INET, SOCK_DGRAM, 0);

	return krad_rc_tx;

}

void krad_rc_tx_destroy (krad_rc_tx_t *krad_rc_tx) {
	
	close (krad_rc_tx->sd);
	free (krad_rc_tx->ip);
	free (krad_rc_tx->data);
	free (krad_rc_tx);
}

void krad_rc_tx_command (krad_rc_tx_t *krad_rc_tx, unsigned char *data, int size) {

	int ret;

	int packet_size;
	struct sockaddr_in remote_client;
		
	ret = 0;
	packet_size = 0;
	
	memset((char *) &remote_client, 0, sizeof(remote_client));
	remote_client.sin_port = htons(krad_rc_tx->port);
	remote_client.sin_family = AF_INET;
	
	if (inet_pton(remote_client.sin_family, krad_rc_tx->ip, &(remote_client.sin_addr)) != 1) {
		fprintf(stderr, "inet_pton() failed\n");
		exit(1);
	}

	packet_size = size + 3;
	
	memcpy (krad_rc_tx->data + 3, data, size);

	ret = sendto(krad_rc_tx->sd, krad_rc_tx->data, packet_size, 0, 
				 (struct sockaddr *) &remote_client, sizeof(remote_client));
	
	if (ret != packet_size) {
		printf("udp sending error\n");
		exit(1);
	}

}
