#include <krad_ebml.h>

int main (int argc, char *argv[]) {

	krad_ebml_t *krad_ebml;	

	int count;

	nestegg_packet *pkt;
	size_t length, size;
	uint64_t duration, tstamp, pkt_tstamp;
	unsigned char * codec_data, * ptr;
	unsigned int cnt, i, j, track, tracks, pkt_cnt, pkt_track;

	if (argc != 2) {
		printf("Please specify a file\n");
		exit(1);
	}


	krad_ebml = kradebml_create();

	kradebml_open_input_file(krad_ebml, argv[1]);
	
	count = 0;
	
	while (nestegg_read_packet(krad_ebml->ctx, &pkt) > 0) {
	
		nestegg_packet_track(pkt, &pkt_track);
		nestegg_packet_count(pkt, &pkt_cnt);
		nestegg_packet_tstamp(pkt, &pkt_tstamp);

		if (count == 0) {
		//	fprintf(stderr, "t %u pts %f frames %u: \n", pkt_track, pkt_tstamp / 1e9, pkt_cnt);
		}
		
		//for (i = 0; i < pkt_cnt; ++i) {
			//nestegg_packet_data(pkt, i, &ptr, &size);
			//fprintf(stderr, "%u ", (unsigned int) size);
		//}
		
		//fprintf(stderr, "\n");

		nestegg_free_packet(pkt);
		
		count++;
		
		if ( count == 300 ) {
			count = 0;
		}
		
	}
	
	kradebml_destroy(krad_ebml);

	return 0;

}
