#include "krad_rc_pololu_maestro.h"

void krad_rc_pololu_maestro_set_target (krad_rc_pololu_maestro_t *krad_rc_pololu_maestro, uint32_t channel, uint32_t target) {

	int wrote;

	unsigned char channel_actual;
	unsigned short target_actual;

	channel_actual = channel;
	target_actual = target;

	unsigned char command[] = {0x84, channel_actual, target_actual & 0x7F, target_actual >> 7 & 0x7F};


	//printf ("commanding %c %c %c %c\n", command[0], command[1], command[2], command[3]);

	wrote = write (krad_rc_pololu_maestro->fd, command, sizeof(command));

	//printf ("wrote was %d\n", wrote);

}

void krad_rc_pololu_maestro_safe (krad_rc_pololu_maestro_t *krad_rc_pololu_maestro) {

	uint32_t target;
	uint32_t channel;
	
	target = 0;
	
	for (channel = 0; channel < 12; channel++) {
		krad_rc_pololu_maestro_set_target (krad_rc_pololu_maestro, channel, target);
	}
	
}

void krad_rc_pololu_maestro_command (krad_rc_pololu_maestro_t *krad_rc_pololu_maestro, void *command) {
 
	uint32_t target;
	uint32_t channel;
	unsigned char *command_actual;

	target = 0;
	channel = 0;
	
	command_actual = command;
	
	memcpy (&channel, command_actual, 4);
	memcpy (&target, command_actual + 4, 4);
	
	//printf ("commanding %u %u\n", channel, target);
	
	krad_rc_pololu_maestro_set_target (krad_rc_pololu_maestro, channel, target);

}

void krad_rc_pololu_maestro_destroy (krad_rc_pololu_maestro_t *krad_rc_pololu_maestro) {

	if (krad_rc_pololu_maestro->fd != 0) {
		krad_rc_pololu_maestro_safe (krad_rc_pololu_maestro);
		close (krad_rc_pololu_maestro->fd);
		krad_rc_pololu_maestro->fd = 0;
	}

	free (krad_rc_pololu_maestro);

}

krad_rc_pololu_maestro_t *krad_rc_pololu_maestro_create (char *device) {

	krad_rc_pololu_maestro_t *krad_rc_pololu_maestro = calloc (1, sizeof(krad_rc_pololu_maestro_t));

	strcpy (krad_rc_pololu_maestro->device, device);

	krad_rc_pololu_maestro->fd = open (krad_rc_pololu_maestro->device, O_RDWR | O_NOCTTY);

	if (krad_rc_pololu_maestro->fd < 0) {
		free (krad_rc_pololu_maestro);
		return NULL;
	}

	tcgetattr (krad_rc_pololu_maestro->fd, &krad_rc_pololu_maestro->term_options);
	krad_rc_pololu_maestro->term_options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	krad_rc_pololu_maestro->term_options.c_oflag &= ~(ONLCR | OCRNL);
	tcsetattr (krad_rc_pololu_maestro->fd, TCSANOW, &krad_rc_pololu_maestro->term_options);
	
	return krad_rc_pololu_maestro;

}

