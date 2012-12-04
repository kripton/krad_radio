#ifndef KRAD_COMPOSITOR_COMMON_H
#define KRAD_COMPOSITOR_COMMON_H

//#include "krad_compositor.h"

typedef struct krad_text_rep_St krad_text_rep_t;
typedef struct krad_sprite_rep_St krad_sprite_rep_t;
typedef struct kr_compositor_subunit_controls_St kr_compositor_subunit_controls_t;
typedef struct krad_compositor_rep_St krad_compositor_rep_t;

struct kr_compositor_subunit_controls_St {

	int x;
	int y;
	int z;

	int width;
	int height;

	float xscale;
	float yscale;

	float rotation;
	float opacity;

};

struct krad_sprite_rep_St {
	char filename[256];
	kr_compositor_subunit_controls_t *controls;
};

struct krad_text_rep_St {
	
	char text[1024];
	char font[128];
	
	float red;
	float green;
	float blue;
	
	kr_compositor_subunit_controls_t *controls;
};

struct krad_compositor_rep_St {
	
	uint32_t width;
	uint32_t height;
	uint32_t fps_numerator;
	uint32_t fps_denominator;
	uint64_t current_frame_number;
	
};

#endif // KRAD_COMPOSITOR_COMMON_H
