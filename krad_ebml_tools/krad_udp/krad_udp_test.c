#include "krad_udp.h"




int main () {

	krad_slicer_t *krad_slicer;
	unsigned char *buffer;
	int count;
	
	count = 0;
	
	buffer = malloc(50000);
	
	krad_slicer = krad_slicer_create ();
	
	while (count < 200) {
		krad_slicer_sendto (krad_slicer, buffer, 50000, 1, "127.0.0.1", 666);
		krad_slicer_sendto (krad_slicer, buffer, 50000, 3, "127.0.0.1", 1666);		
		count++;
	}
	
	krad_slicer_destroy (krad_slicer);
	
	free (buffer);
	
	return 0;
	
}
