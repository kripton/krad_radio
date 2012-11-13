#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <malloc.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#include <pthread.h>

#include <SDL/SDL.h>

typedef struct krad_rc_sdl_joy_St krad_rc_sdl_joy_t;

struct krad_rc_sdl_joy_St {

	int joynum;

	SDL_Event evt;
	SDL_Joystick *joystick;
	int num_axes, num_buttons, num_balls, num_hats;

	pthread_t joy_thread;
	
	int axis[16];
	
	int run;

};

void *krad_rc_sdl_joy_thread (void *arg);

void krad_rc_sdl_joy_poll (krad_rc_sdl_joy_t *krad_rc_sdl_joy);
void krad_rc_sdl_joy_destroy (krad_rc_sdl_joy_t *krad_rc_sdl_joy);
krad_rc_sdl_joy_t *krad_rc_sdl_joy_create (int joynum);
