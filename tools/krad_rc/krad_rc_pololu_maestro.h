#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <termios.h>


// "/dev/ttyACM0"

typedef struct krad_rc_pololu_maestro_St krad_rc_pololu_maestro_t;

struct krad_rc_pololu_maestro_St {

	char device[512];

	int fd;
	struct termios term_options;
	uint32_t last_cmd_time;

};



void krad_rc_pololu_maestro_destroy (krad_rc_pololu_maestro_t *krad_rc_pololu_maestro);
void krad_rc_pololu_maestro_command (krad_rc_pololu_maestro_t *krad_rc_pololu_maestro, void *command);
void krad_rc_pololu_maestro_safe (krad_rc_pololu_maestro_t *krad_rc_pololu_maestro);
krad_rc_pololu_maestro_t *krad_rc_pololu_maestro_create (char *device);
