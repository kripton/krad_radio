#include <krad_easing.h>




static void easing_tests () {

	int position;
	int time;
	int start;
	int end;
	int duration;
	
	time = 0;
	position = 0;
	start = 0;
	end = 500;
	duration = 345;
	
	while (time <= duration) {
	
		position = krad_ease (LINEAR, time, start, end, duration);
		
		printf ("LINEAR Time: %d/%d Position is: %d/%d/%d\n", time, duration, start, position, end );
	
		time++;
	
	}	
	
	time = 0;
	position = 0;
	start = 0;
	end = 500;
	duration = 345;
	
	while (time <= duration) {
	
		position = krad_ease (EASEINSINE, time, start, end, duration);
		
		printf ("EASEINSINE Time: %d/%d Position is: %d/%d/%d\n", time, duration, start, position, end );
	
		time++;
	
	}
	
	time = 0;
	position = 0;
	start = 0;
	end = 500;
	duration = 345;
	
	
	while (time <= duration) {
	
		position = krad_ease (EASEOUTSINE, time, start, end, duration);
		
		printf ("EASEOUTSINE Time: %d/%d Position is: %d/%d/%d\n", time, duration, start, position, end );
	
		time++;
	
	}	
	
	
	time = 0;
	position = 0;
	start = 0;
	end = 500;
	duration = 345;
	
	while (time <= duration) {
	
		position = krad_ease (EASEINOUTSINE, time, start, end, duration);
		
		printf ("EASEINOUTSINE Time: %d/%d Position is: %d/%d/%d\n", time, duration, start, position, end );
	
		time++;
	
	}	


	
	time = 0;
	position = 0;
	start = 0;
	end = 500;
	duration = 345;
	
	while (time <= duration) {
	
		position = krad_ease (EASEINCUBIC, time, start, end, duration);
		
		printf ("EASEINCUBIC Time: %d/%d Position is: %d/%d/%d\n", time, duration, start, position, end );
	
		time++;
	
	}
	
	time = 0;
	position = 0;
	start = 0;
	end = 500;
	duration = 345;
	
	
	while (time <= duration) {
	
		position = krad_ease (EASEOUTCUBIC, time, start, end, duration);
		
		printf ("EASEOUTCUBIC Time: %d/%d Position is: %d/%d/%d\n", time, duration, start, position, end );
	
		time++;
	
	}	
	
	
	time = 0;
	position = 0;
	start = 0;
	end = -500;
	duration = 345;
	
	while (time <= duration) {
	
		position = krad_ease (EASEINOUTCUBIC, time, start, end, duration);
		
		printf ("EASEINOUTCUBIC Time: %d/%d Position is: %d/%d/%d\n", time, duration, start, position, end );
	
		time++;
	
	}	

}

int main ( int argc, char *argv[] ) {


	krad_easing_t *krad_easing;
	
	krad_easing = krad_easing_create ();
	krad_easing_destroy ( krad_easing );



	easing_tests ();


	return 0;

}
