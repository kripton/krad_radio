#include "krad_system.h"
#include "krad_transmitter.h"

void krad_transmitter_test (int port) {

	krad_transmitter_t *krad_transmitter;
	krad_transmission_t *krad_transmission;

	int count;
	int test_count;
	char data[256];
	int data_len;

	test_count = 60000;
	count = 0;

	char *stream_name = "stream.txt";
	char *content_type = "text/plain";

	char *stream_header = "This is the header of the text stream.\n";

	krad_transmitter = krad_transmitter_create ();
	
	krad_transmitter_listen_on (krad_transmitter, port);
	
	krad_transmission = krad_transmitter_transmission_create (krad_transmitter, stream_name, content_type);
	
	usleep (100000);
	
	krad_transmitter_transmission_set_header (krad_transmission, (unsigned char *)stream_header, strlen(stream_header));

	usleep (100000);
	
	krad_transmitter_transmission_sync_point (krad_transmission);
	
	usleep (100000);	
	
	data_len = sprintf (data, "The first sentence ever in the stream, key of course.\n");
	krad_transmitter_transmission_add_data (krad_transmission, (unsigned char *)data, data_len);
	
	usleep (100000);
	
	for (count = 1; count < test_count; count++) {

		if ((count % 5) == 0) {
			krad_transmitter_transmission_sync_point (krad_transmission);
			data_len = sprintf (data, "\nSentence number %d, a very key sentence.\n", count);
		} else {
			data_len = sprintf (data, "Sentence number %d.\n", count);
		}

		krad_transmitter_transmission_add_data (krad_transmission, (unsigned char *)data, data_len);

		usleep (100000);
	}
	
	krad_transmitter_transmission_destroy (krad_transmission);
	
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
