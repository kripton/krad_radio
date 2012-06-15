#include "krad_ticker.h"


void krad_ticker_test (int count, int numerator, int denominator) {

	krad_ticker_t *krad_ticker;
	int t;

	krad_ticker = krad_ticker_create (numerator, denominator);

	krad_ticker_start (krad_ticker);

	for (t = 0; t < count; t++) {

		printk ("Tick! %d %d - %d/%d", numerator, denominator, t, count);

		fflush (stdout);

		krad_ticker_wait (krad_ticker);
	}

	krad_ticker_destroy (krad_ticker);

}

int main (int argc, char *argv[]) {

	int count;
	
	count = 300;
	
	krad_system_init ();
	
	krad_ticker_test (count, 30000, 1000);
	krad_ticker_test (count, 44100, 512);
	krad_ticker_test (count, 44100, 1024);
	krad_ticker_test (count, 48000, 1024);
	krad_ticker_test (count, 60000, 1000);

	printk ("Clean Exit");

	return 0;

}
