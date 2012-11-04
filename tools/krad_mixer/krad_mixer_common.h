#ifndef KRAD_MIXER_COMMON_H
#define KRAD_MIXER_COMMON_H

typedef struct krad_mixer_portgroup_rep_St krad_mixer_portgroup_rep_t;

typedef enum {
	OUTPUT,
	INPUT,
	MIX,
} krad_mixer_portgroup_direction_t;

struct krad_mixer_portgroup_rep_St {

	char sysname[256];

	int xmms2;

};


#endif
