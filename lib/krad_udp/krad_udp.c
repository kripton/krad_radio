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

	memset((char *) &remote_client, 0, sizeof(remote_client));
	remote_client.sin_port = htons(port);
	remote_client.sin_family = AF_INET;
	
	if (inet_pton(remote_client.sin_family, ip, &(remote_client.sin_addr)) != 1) {
		failfast ("inet_pton() failed");
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

		memcpy (krad_slicer->data + 19, data + sent, payload_size);

		//printk("track: %d slice: %d size: %d packet: %d range: %d - %d packet size: %d payload size: %d\n", 
		//		track, krad_slicer->track_seq[track], size, packet, sent, sent + payload_size - 1, packet_size, payload_size);
		
		ret = sendto(krad_slicer->sd, krad_slicer->data, packet_size, 0, 
					 (struct sockaddr *) &remote_client, sizeof(remote_client));
		
		if (ret != packet_size) {
			failfast ("udp sending error");
		}
		
		sent += payload_size;
	}	

	krad_slicer->track_seq[track]++;

}




krad_rebuilder_t *krad_rebuilder_create () {

	krad_rebuilder_t *krad_rebuilder;
	int s;
	
	krad_rebuilder = calloc(1, sizeof(krad_rebuilder_t));
	
	krad_rebuilder->slice_count = 10;
	
	krad_rebuilder->slices = calloc(krad_rebuilder->slice_count, sizeof(krad_slice_t));

	for (s = 0; s < krad_rebuilder->slice_count; s++) {
		krad_rebuilder->slices[s].data = malloc(500000);
	}

	return krad_rebuilder;

}

void krad_rebuilder_destroy (krad_rebuilder_t *krad_rebuilder) {

	int s;
	
	for (s = 0; s < krad_rebuilder->slice_count; s++) {
		free (krad_rebuilder->slices[s].data);
	}

	free (krad_rebuilder->slices);
	free (krad_rebuilder);

}



void krad_rebuilder_write (krad_rebuilder_t *krad_rebuilder, unsigned char *data, int length) {

	int s;
	int track;
	int slice_num;
	int slice_size;
	int payload_size;
	int payload_start;
	
	if (memcmp(data, "KQN", 3) != 0) {
		failfast ("krad packet error!");
	}
	
	// header|track number|sequence number|start_byte|total_bytes
	memcpy (&track, data + 3, 4);
	memcpy (&slice_num, data + 7, 4);
	memcpy (&payload_start, data + 11, 4);
	memcpy (&slice_size, data + 15, 4);	

	payload_size = length - KRAD_UDP_HEADER_SIZE;
	
	//printf("packet size: %d track %d slice num %d slice size %d %s\n", length, track, slice_num, slice_size, data);

	if (slice_num < krad_rebuilder->slice_position - 7) {
		printk ("krad packet too damn old\n");
		return;
	}
	
	for (s = 0; s < krad_rebuilder->slice_count; s++) {
		if (krad_rebuilder->slices[s].seq == slice_num) {
			break;
		}
	}

	
	if ((s == krad_rebuilder->slice_count) || (slice_num == 0)) {
		for (s = 0; s < krad_rebuilder->slice_count; s++) {
			if ((krad_rebuilder->slices[s].seq == 0) || 
			    (krad_rebuilder->slices[s].seq < krad_rebuilder->slice_position - (krad_rebuilder->slice_count - 2))) {
			    krad_rebuilder->slices[s].size = slice_size;
			    krad_rebuilder->slices[s].fill = 0;
			    krad_rebuilder->slices[s].seq = slice_num;
				break;
			}
		}
	}
	
	if (slice_size != krad_rebuilder->slices[s].size) {
		failfast("krad packet error2! %d", s);
	}
	
	memcpy (krad_rebuilder->slices[s].data + payload_start, data + KRAD_UDP_HEADER_SIZE, payload_size);
	krad_rebuilder->slices[s].fill += payload_size;

	/*
	if (krad_rebuilder->slices[s].fill == krad_rebuilder->slices[s].size) {
		printkd("slice %d recieved %d bytes\n", krad_rebuilder->slices[s].seq, krad_rebuilder->slices[s].size);
	}
	*/

	if (krad_rebuilder->slice_position < slice_num) {
		krad_rebuilder->slice_position = slice_num;
	}
}


int krad_rebuilder_read_packet (krad_rebuilder_t *krad_rebuilder, unsigned char *data, int track) {

	int s;
	
	if (krad_rebuilder->slice_read_position < (krad_rebuilder->slice_position - 5)) {
		printkd("fell behind and readjusted %d ", krad_rebuilder->slice_read_position);
		krad_rebuilder->slice_read_position = krad_rebuilder->slice_position - 5;
		printkd("%d\n", krad_rebuilder->slice_read_position);
	}

	for (s = 0; s < krad_rebuilder->slice_count; s++) {
		if (krad_rebuilder->slices[s].seq == krad_rebuilder->slice_read_position) {
			if (krad_rebuilder->slices[s].fill == krad_rebuilder->slices[s].size) {
				break;
			}
		}
	}

	if (s == krad_rebuilder->slice_count) {
		return 0;
	}

	memcpy (data, krad_rebuilder->slices[s].data, krad_rebuilder->slices[s].size);

	krad_rebuilder->slice_read_position++;

	return krad_rebuilder->slices[s].size;

}

