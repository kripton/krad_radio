#include "krad_udp.h"


krad_slicer_t *krad_slicer_create () {

	krad_slicer_t *krad_slicer = calloc(1, sizeof(krad_slicer_t));

	krad_slicer->data = calloc(1, 2048);
	strcpy ((char *)krad_slicer->data, "KQN");

	krad_slicer->sd = socket (AF_INET, SOCK_DGRAM, 0);

	return krad_slicer;

}

void krad_slicer_destroy (krad_slicer_t *krad_slicer) {
	
	close (krad_slicer->sd);
	
	free (krad_slicer->data);
	free (krad_slicer);
}

void krad_slicer_sendto (krad_slicer_t *krad_slicer, unsigned char *data, int size, int track, char *ip, int port) {

	int ret;
	int remaining;
	int payload_size;
	int packet_size;
	int sent;
	int packet;
	struct sockaddr_in remote_client;
		
	ret = 0;
	sent = 0;
	payload_size = 0;
	packet_size = 0;
	packet = 0;
	remaining = size;

	krad_slicer->track_seq[track]++;

	memset((char *) &remote_client, 0, sizeof(remote_client));
	remote_client.sin_port = htons(port);
	remote_client.sin_family = AF_INET;
	
	if (inet_pton(remote_client.sin_family, ip, &(remote_client.sin_addr)) != 1) {
		fprintf(stderr, "inet_pton() failed\n");
		exit(1);
	}

	while (remaining) {
		
		packet++;
	
		if (remaining > KRAD_UDP_MAX_PAYOAD_SIZE) {
			payload_size = KRAD_UDP_MAX_PAYOAD_SIZE;
		} else {
			payload_size = remaining;
		}
		
		remaining -= payload_size;
		packet_size = payload_size + KRAD_UDP_HEADER_SIZE;
		
		// header|track number|sequence number|start_byte|total_bytes
		memcpy (krad_slicer->data + 3, &track, 4);
		memcpy (krad_slicer->data + 7, &krad_slicer->track_seq[track], 4);
		memcpy (krad_slicer->data + 11, &sent, 4);
		memcpy (krad_slicer->data + 15, &size, 4);
		
		printf("track: %d slice: %d size: %d packet: %d range: %d - %d packet size: %d payload size: %d\n", 
				track, krad_slicer->track_seq[track], size, packet, sent, sent + payload_size - 1, packet_size, payload_size);
		
		ret = sendto(krad_slicer->sd, krad_slicer->data, packet_size, 0, 
					 (struct sockaddr *) &remote_client, sizeof(remote_client));
		
		if (ret != packet_size) {
			printf("udp sending error\n");
			exit(1);
		}
		
		sent += payload_size;
	}	


}




krad_rebuilder_t *krad_rebuilder_create () {

	krad_rebuilder_t *krad_rebuilder = calloc(1, sizeof(krad_rebuilder_t));
	
	krad_rebuilder->slices = calloc(10, sizeof(krad_slice_t));

	return krad_rebuilder;

}

void krad_rebuilder_destroy (krad_rebuilder_t *krad_rebuilder) {

	free (krad_rebuilder->slices);
	free (krad_rebuilder);

}



void krad_rebuilder_write (krad_rebuilder_t *krad_rebuilder, unsigned char *data, int length) {


}


int krad_rebuilder_read_packet (krad_rebuilder_t *krad_rebuilder, unsigned char *data, int track) {


}

