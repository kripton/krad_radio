#include "krad_rc.h"





int main ( int argc, char *argv[] ) {


	krad_rc_t *krad_rc;
	int c;
	uint32_t usec;
	uint32_t channel;
	int center;
	uint32_t start;	
	uint32_t end;
	unsigned char command[8];
	
	krad_rc = krad_rc_create ("Pololu Maestro", "/dev/ttyACM0");
	
	if (krad_rc == NULL) {
		printf ("krad rc was null :/\n");
		return 1;
	}
	

	channel = 0;
	
	start = 1219;
	center = 1661;
	end = 1985;
	
	memcpy (command, &channel, 4);	
	
	usec = start * 4;
	memcpy (command + 4, &usec, 4);
	krad_rc_command ( krad_rc, command);
	
	for (c = start; c < end; c++) {
		usec = c * 4;
		memcpy (command + 4, &usec, 4);
		krad_rc_command ( krad_rc, command);
		usleep (500);		
	}
	
	usec = center * 4;
	memcpy (command + 4, &usec, 4);
	krad_rc_command ( krad_rc, command);
	
	
	usec = 0;
	memcpy (command + 4, &usec, 4);
	krad_rc_command ( krad_rc, command);	
	
	krad_rc_destroy ( krad_rc );



	return 0;

}

