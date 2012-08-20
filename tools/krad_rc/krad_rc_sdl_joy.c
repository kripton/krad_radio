#include "krad_rc_sdl_joy.h"


void krad_rc_sdl_joy_wait (krad_rc_sdl_joy_t *krad_rc_sdl_joy, int *axis, int *value) {


	SDL_WaitEvent (&krad_rc_sdl_joy->evt);
	
	switch (krad_rc_sdl_joy->evt.type) {
		case SDL_KEYDOWN:
            switch ( krad_rc_sdl_joy->evt.key.keysym.sym ) {
                case SDLK_q:
                    break;
                default:
                    break;
            }

		case SDL_QUIT:
			break;

		case SDL_JOYAXISMOTION:
			*axis = krad_rc_sdl_joy->evt.jaxis.axis;
			*value = krad_rc_sdl_joy->evt.jaxis.value;
			break;
	}


}

void krad_rc_sdl_joy_destroy (krad_rc_sdl_joy_t *krad_rc_sdl_joy) {

	SDL_JoystickClose (krad_rc_sdl_joy->joystick);
	free (krad_rc_sdl_joy);

}


krad_rc_sdl_joy_t *krad_rc_sdl_joy_create (int joynum) {


	krad_rc_sdl_joy_t *krad_rc_sdl_joy = calloc(1, sizeof(krad_rc_sdl_joy_t));
	int num_axes, num_buttons, num_balls, num_hats;

	krad_rc_sdl_joy->joynum = joynum;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return NULL;
	}

	SDL_JoystickEventState (SDL_ENABLE);
	krad_rc_sdl_joy->joystick = SDL_JoystickOpen (krad_rc_sdl_joy->joynum);

	num_axes = SDL_JoystickNumAxes (krad_rc_sdl_joy->joystick);
	num_buttons = SDL_JoystickNumButtons (krad_rc_sdl_joy->joystick);
	num_balls = SDL_JoystickNumBalls (krad_rc_sdl_joy->joystick);
	num_hats = SDL_JoystickNumHats (krad_rc_sdl_joy->joystick);

	printf("%s: %d axes, %d buttons, %d balls, %d hats\n",
		   SDL_JoystickName(krad_rc_sdl_joy->joynum),
		   num_axes, num_buttons, num_balls, num_hats);

	return krad_rc_sdl_joy;

}

