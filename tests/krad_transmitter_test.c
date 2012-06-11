#include "krad_system.h"
#include "krad_transmitter.h"

void krad_transmitter_test (int port) {

	krad_transmitter_t *krad_transmitter;
	krad_transmission_t *krad_transmission;

	krad_transmitter = krad_transmitter_create ();
	
	krad_transmitter_listen_on (krad_transmitter, port);
	
	krad_transmission = krad_transmitter_transmission_create (krad_transmitter, "testass", "fake/type");
	
	usleep (2555000);
	
	krad_transmitter_transmission_set_header (krad_transmission, (unsigned char *)"header\n", strlen("header\n"));

	usleep (2555000);
	
	krad_transmitter_transmission_sync_point (krad_transmission);
	
	krad_transmitter_transmission_add_data (krad_transmission, (unsigned char *)"data\n", strlen("data\n"));
	
	usleep (2555000);	
	
	krad_transmitter_transmission_add_data (krad_transmission, (unsigned char *)"data\n", strlen("data\n"));
	
	usleep (2555000);	
	
	krad_transmitter_transmission_add_data (krad_transmission, (unsigned char *)"data\n", strlen("data\n"));	
	
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
