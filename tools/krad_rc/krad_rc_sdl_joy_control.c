#include "krad_rc_tx.h"
#include "krad_rc_sdl_joy.h"




int main ( int argc, char *argv[] ) {


	krad_rc_tx_t *krad_rc_tx;
	krad_rc_sdl_joy_t *krad_rc_sdl_joy;

	unsigned char command[8];
	int usec;
	int target;
	int channel;
	
	int axis;
	int value;
	

	int center;

	double curr_pos;
	double min_pos, max_pos;
	float middle_pos, pos_range;	
	
	krad_rc_sdl_joy = krad_rc_sdl_joy_create (0);	
	krad_rc_tx = krad_rc_tx_create (argv[1], (atoi(argv[2])));
	
	if (krad_rc_tx == NULL) {
		printf ("krad rc tx was null :/\n");
		return 1;
	}
	

	channel = 0;
	memcpy (command, &channel, 4);
	
	target = 1219;
	//target = atoi(argv[3]);

	usec = target * 4;
	memcpy (command + 4, &usec, 4);

	krad_rc_tx_command ( krad_rc_tx, command, 8);
	
	
	axis = 0;
	value = 0;
	
	min_pos = 1219;
	center = 1631;
	max_pos = 1985;	

	middle_pos = (min_pos + max_pos) / 2.0;
	pos_range = max_pos - middle_pos;
	curr_pos = middle_pos;
	
	
	while (1) {
	
	
		krad_rc_sdl_joy_wait (krad_rc_sdl_joy, &axis, &value);
	
	
		printf ("got joy axis %d value %d\n", axis, value);
	
		if ((axis == 0) || (axis == 1)) {
		
			channel = axis;
			memcpy (command, &channel, 4);
			
			if (axis == 1) {
				value += -8600;
			}
			if (axis == 0) {
				value = value * -1;
			}			
			
			target = center + (value / 100);

			usec = target * 4;
			memcpy (command + 4, &usec, 4);

			krad_rc_tx_command ( krad_rc_tx, command, 8);
		
		}	
	
	
		//usleep (1500);
	
	}
	
	
	krad_rc_sdl_joy_destroy ( krad_rc_sdl_joy );
	
	krad_rc_tx_destroy ( krad_rc_tx );



	return 0;

}
