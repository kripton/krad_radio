
#ifndef KRAD_COMPOSITOR_COMMON_H
#define KRAD_COMPOSITOR_COMMON_H


#include <stdint.h>
#include "krad_ebml.h"

typedef struct krad_text_rep_St krad_text_rep_t;
typedef struct krad_sprite_rep_St krad_sprite_rep_t;
typedef struct krad_vector_rep_St krad_vector_rep_t;
typedef struct kr_compositor_subunit_controls_St kr_compositor_subunit_controls_t;
typedef struct krad_compositor_rep_St krad_compositor_rep_t;

struct kr_compositor_subunit_controls_St {

  int number;

	int x;
	int y;
	int z;

  int tickrate;
  
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

typedef enum {
  HEX,
  SELECTOR,
  CIRCLE,
  RECT,
  TRIANGLE,
  VIPER,
  METER,
  GRID,
} krad_vector_type_t;

struct krad_vector_rep_St {
	
  krad_vector_type_t krad_vector_type;
  
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


kr_compositor_subunit_controls_t *krad_compositor_subunit_controls_create ();
kr_compositor_subunit_controls_t *krad_compositor_subunit_controls_create_and_init ( int number, int x, int y, int z, int tickrate, int width, int height, float scale, float opacity, float rotation);
void krad_compositor_subunit_controls_reset (kr_compositor_subunit_controls_t *krad_compositor_subunit_controls);
void krad_compositor_subunit_controls_destroy (kr_compositor_subunit_controls_t *krad_compositor_subunit_controls);
void krad_compositor_subunit_controls_to_ebml (krad_ebml_t *krad_ebml, kr_compositor_subunit_controls_t *krad_compositor_subunit_controls);
void krad_compositor_subunit_controls_read (krad_ebml_t *krad_ebml, kr_compositor_subunit_controls_t *subunit_controls);


krad_text_rep_t *krad_compositor_text_rep_create ();
krad_text_rep_t *krad_compositor_text_rep_create_and_init (int number, char *text, char *font, float red, float green, float blue, int x, int y, int z, int tickrate, float scale, float opacity, float rotation);
krad_text_rep_t *krad_compositor_ebml_to_krad_text_rep (krad_ebml_t *krad_ebml, uint64_t *ebml_data_size, krad_text_rep_t *krad_text_rep);
void krad_compositor_text_rep_destroy (krad_text_rep_t *krad_text_rep);
void krad_compositor_validate_text_rep (krad_text_rep_t *krad_text_rep);
void krad_compositor_text_rep_to_ebml (krad_text_rep_t *krad_text_rep, krad_ebml_t *krad_ebml);
void krad_compositor_text_rep_reset (krad_text_rep_t *krad_text_rep);

krad_sprite_rep_t *krad_compositor_sprite_rep_create ();
void krad_compositor_sprite_rep_destroy (krad_sprite_rep_t *krad_sprite_rep);
krad_sprite_rep_t *krad_compositor_ebml_to_new_krad_sprite_rep (krad_ebml_t *krad_ebml, uint64_t *bytes_read);
krad_sprite_rep_t *krad_compositor_sprite_rep_create_and_init ( int number, char *filename, int x, int y, int z, int tickrate, float scale, float opacity, float rotation);
void krad_compositor_sprite_rep_to_ebml (krad_sprite_rep_t *krad_sprite_rep, krad_ebml_t *krad_ebml);
#endif // KRAD_COMPOSITOR_COMMON_H

krad_vector_rep_t *krad_compositor_vector_rep_create ();
krad_vector_rep_t *krad_compositor_vector_rep_create_and_init (int number, char *vector_type, float red, float green, float blue, int x, int y, int z, int tickrate, float scale, float opacity, float rotation);
krad_vector_rep_t *krad_compositor_ebml_to_krad_vector_rep (krad_ebml_t *krad_ebml, uint64_t *ebml_data_size, krad_vector_rep_t *krad_vector_rep);
void krad_compositor_vector_rep_destroy (krad_vector_rep_t *krad_vector_rep);
void krad_compositor_validate_vector_rep (krad_vector_rep_t *krad_vector_rep);
void krad_compositor_vector_rep_to_ebml (krad_vector_rep_t *krad_vector_rep, krad_ebml_t *krad_ebml);
void krad_compositor_vector_rep_reset (krad_vector_rep_t *krad_vector_rep);
krad_vector_type_t krad_string_to_vector_type (char *string); 
void krad_vector_type_to_string (krad_vector_type_t krad_vector_type, char *string);
