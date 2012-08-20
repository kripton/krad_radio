#include "krad_rc_tx.h"





int main ( int argc, char *argv[] ) {


	krad_rc_tx_t *krad_rc_tx;
	unsigned char command[8];
	int usec;
	int target;
	int channel;
	
	krad_rc_tx = krad_rc_tx_create (argv[1], (atoi(argv[2])));
	
	if (krad_rc_tx == NULL) {
		printf ("krad rc tx was null :/\n");
		return 1;
	}
	

	channel = 0;
	memcpy (command, &channel, 4);
	
	target = 1219;
	target = atoi(argv[3]);

	usec = target * 4;
	memcpy (command + 4, &usec, 4);

	krad_rc_tx_command ( krad_rc_tx, command, 8);
	
	
	
	krad_rc_tx_destroy ( krad_rc_tx );



	return 0;

}

