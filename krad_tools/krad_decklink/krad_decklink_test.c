#include "krad_decklink.h"





int main ( int argc, char *argv[] ) {


	krad_decklink_t *krad_decklink;
	
	
	
	krad_decklink = krad_decklink_create ();


	usleep(3000000);

	
	krad_decklink_destroy ( krad_decklink );



	return 0;

}
