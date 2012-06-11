#include "krad_system.h"
#include "krad_transmitter.h"

void krad_transmitter_test (int port) {

	krad_transmitter_t *krad_transmitter;

	krad_transmitter = krad_transmitter_create ();
	
	krad_transmitter_listen_on (krad_transmitter, port);
	
	krad_transmitter_transmission_create (krad_transmitter, "testass", "fake/type");
	
	usleep (2555000);
	
	krad_transmitter_destroy (krad_transmitter);

}


int main (int argc, char *argv[]) {

	krad_system_init ();

	if (argc == 2) {
		krad_transmitter_test (atoi(argv[1]));
	} else {
		failfast ("Need port");
	}

	printk ("Clean exit");

	return 0;

}
