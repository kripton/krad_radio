#include "krad_udp.h"

#define TEST_PORT 42666

void receiver_test() {

	krad_rebuilder_t *krad_rebuilder;
	int port;
	int sd;
	int ret;
	int slen;
	unsigned char *buffer;
	unsigned char *packet_buffer;
	struct sockaddr_in local_address;
	struct sockaddr_in remote_address;
	
	slen = sizeof(remote_address);
	
	buffer = calloc (1, 2000);
	packet_buffer = calloc (1, 200000);
	sd = socket (AF_INET, SOCK_DGRAM, 0);
	port = TEST_PORT;
	krad_rebuilder = krad_rebuilder_create ();

	memset((char *) &local_address, 0, sizeof(local_address));
	local_address.sin_family = AF_INET;
	local_address.sin_port = htons (port);
	local_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1 ) {
		printf("bind error\n");
		exit(1);
	}

	while (1) {
	
		ret = recvfrom(sd, buffer, 2000, 0, (struct sockaddr *)&remote_address, (socklen_t *)&slen);
		
		if (ret == -1) {
			printf("failed recvin udp\n");
			exit(1);
		}
		
		//printf("Received packet from %s:%d\n", 
		//		inet_ntoa(remote_address.sin_addr), ntohs(remote_address.sin_port));


		krad_rebuilder_write (krad_rebuilder, buffer, ret);

		ret = krad_rebuilder_read_packet (krad_rebuilder, packet_buffer, 1);
		if (ret != 0) {
			printf("read a packet with %d bytes: -%s-\n", ret, packet_buffer);
		}

	}

	krad_rebuilder_destroy (krad_rebuilder);
	close (sd);
	free (packet_buffer);
	free (buffer);
}

void sender_test() {

	krad_slicer_t *krad_slicer;
	unsigned char *buffer;
	int count;
	int length;
	count = 0;
	
	buffer = malloc(50000);
	
	sprintf(buffer, "the wonderfull world of networking");
	
	length = strlen(buffer) + 1;
	
	krad_slicer = krad_slicer_create ();
	
	while (count < 555) {
		krad_slicer_sendto (krad_slicer, buffer, length, 1, "127.0.0.1", TEST_PORT);
		//krad_slicer_sendto (krad_slicer, buffer, 50000, 3, "127.0.0.1", TEST_PORT);		
		count++;
	}
	
	krad_slicer_destroy (krad_slicer);
	
	free (buffer);

}


int main (int argc, char *argv[]) {

	if (argc > 1) {
		receiver_test();
	} else {
		sender_test();
	}
	
	return 0;
	
}
