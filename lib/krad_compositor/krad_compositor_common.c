
#include "krad_compositor_common.h"
#include "krad_ebml.h"
#include "krad_radio.h"

void krad_compositor_subunit_controls_to_ebml (krad_ebml_t *krad_ebml, kr_compositor_subunit_controls_t *krad_compositor_subunit_controls) {
  
  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_NUMBER, krad_compositor_subunit_controls->number);
  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_COMPOSITOR_X, krad_compositor_subunit_controls->x);
	krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, krad_compositor_subunit_controls->y);
  krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_COMPOSITOR_Y, krad_compositor_subunit_controls->z);
	krad_ebml_write_int32 (krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_TICKRATE, krad_compositor_subunit_controls->tickrate);
	
  krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_SCALE, krad_compositor_subunit_controls->xscale);
	krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_OPACITY, krad_compositor_subunit_controls->opacity);
	krad_ebml_write_float (krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_ROTATION, krad_compositor_subunit_controls->rotation);

}
void krad_compositor_subunit_controls_read (krad_ebml_t *krad_ebml, kr_compositor_subunit_controls_t *subunit_controls) {
  uint32_t ebml_id;
	
	uint64_t ebml_data_size;
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_SPRITE_NUMBER) {
    subunit_controls->number = krad_ebml_read_number (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_X) {
    subunit_controls->x = krad_ebml_read_number (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_Y) {
    subunit_controls->y = krad_ebml_read_number (krad_ebml, ebml_data_size);
  }
      
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_Y) {
    subunit_controls->z = krad_ebml_read_number (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_SPRITE_TICKRATE) {
    subunit_controls->tickrate = krad_ebml_read_number (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_SPRITE_SCALE) {
     subunit_controls->yscale = subunit_controls->xscale = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
  
//  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
//  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_SPRITE_SCALE) {
//    subunit_controls->yscale = krad_ebml_read_float (krad_ebml, ebml_data_size);
//  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_SPRITE_OPACITY) {
    subunit_controls->opacity = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_SPRITE_ROTATION) {
    subunit_controls->rotation = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }		
  
}

void krad_compositor_subunit_controls_destroy (kr_compositor_subunit_controls_t *krad_compositor_subunit_controls) {
  free (krad_compositor_subunit_controls);
}

kr_compositor_subunit_controls_t *krad_compositor_subunit_controls_create () {

	kr_compositor_subunit_controls_t *krad_compositor_subunit_controls;

	krad_compositor_subunit_controls = calloc (1, sizeof (kr_compositor_subunit_controls_t));
	
	krad_compositor_subunit_controls_reset (krad_compositor_subunit_controls);
	
	return krad_compositor_subunit_controls;

}

kr_compositor_subunit_controls_t *krad_compositor_subunit_controls_create_and_init (int number, int x, int y, int z, int tickrate, int width, int height,
                                                                                    float scale, float opacity, float rotation) {

	kr_compositor_subunit_controls_t *krad_compositor_subunit_controls;

	krad_compositor_subunit_controls = calloc (1, sizeof (kr_compositor_subunit_controls_t));
	
	krad_compositor_subunit_controls_reset (krad_compositor_subunit_controls);
  
  krad_compositor_subunit_controls->x = x;
	krad_compositor_subunit_controls->y = y;
  krad_compositor_subunit_controls->z = z;
  
  krad_compositor_subunit_controls->number = number;
  krad_compositor_subunit_controls->tickrate = tickrate;
  
  krad_compositor_subunit_controls->width = width;
  krad_compositor_subunit_controls->height = height;
  
  krad_compositor_subunit_controls->xscale = scale;
  krad_compositor_subunit_controls->yscale = scale;
  
  krad_compositor_subunit_controls->rotation = rotation;
  krad_compositor_subunit_controls->opacity = opacity;

  return krad_compositor_subunit_controls;

}

void krad_compositor_subunit_controls_reset (kr_compositor_subunit_controls_t *krad_compositor_subunit_controls) {
  krad_compositor_subunit_controls->x = 0;
	krad_compositor_subunit_controls->y = 0;
  krad_compositor_subunit_controls->z = 0;
  
  krad_compositor_subunit_controls->tickrate = KRAD_COMPOSITOR_SUBUNIT_DEFAULT_TICKRATE;
  
  krad_compositor_subunit_controls->number = 0;
  
  krad_compositor_subunit_controls->width = 0;
  krad_compositor_subunit_controls->height = 0;
  
  krad_compositor_subunit_controls->xscale = 1.0f;
  krad_compositor_subunit_controls->yscale = 1.0f;
  
  krad_compositor_subunit_controls->rotation = 0.0f;
  krad_compositor_subunit_controls->opacity = 0.0f;

}

void krad_compositor_validate_text_rep (krad_text_rep_t *krad_text_rep) {
  
  if ((krad_text_rep->red < 0.0) || (krad_text_rep->red > 0.255)) {
    printk("bad red value: %f", krad_text_rep->red);
  }
  
  if ((krad_text_rep->green < 0.0) || (krad_text_rep->green > 0.255)) {
    printk("bad green value: %f", krad_text_rep->green);
  }
  
  if ((krad_text_rep->blue < 0.0) || (krad_text_rep->blue > 0.255)) {
    printk("bad blue value: %f", krad_text_rep->blue);
  }

}

krad_text_rep_t *krad_compositor_text_rep_create () {
  
  krad_text_rep_t *krad_text_rep = calloc(1, sizeof (krad_text_rep_t));
  krad_text_rep->controls = krad_compositor_subunit_controls_create();
  return krad_text_rep;
}

void krad_compositor_text_rep_destroy (krad_text_rep_t *krad_text_rep) {
  krad_compositor_subunit_controls_destroy (krad_text_rep->controls);
  free (krad_text_rep);
}

void krad_compositor_text_rep_to_ebml (krad_text_rep_t *krad_text_rep, krad_ebml_t *krad_ebml) {
  
  uint64_t cmd;
  
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT, &cmd);	
	
	krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT, krad_text_rep->text);
	krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_COMPOSITOR_FONT, krad_text_rep->font);
	
	krad_ebml_write_float (krad_ebml,
	                       EBML_ID_KRAD_COMPOSITOR_RED,
	                       krad_text_rep->red);
	krad_ebml_write_float (krad_ebml,
	                       EBML_ID_KRAD_COMPOSITOR_GREEN,
	                       krad_text_rep->green);
	krad_ebml_write_float (krad_ebml,
	                       EBML_ID_KRAD_COMPOSITOR_BLUE,
	                       krad_text_rep->blue);
	

	krad_compositor_subunit_controls_to_ebml (krad_ebml, krad_text_rep->controls);

	krad_ebml_finish_element (krad_ebml, cmd);
}

krad_text_rep_t *krad_compositor_ebml_to_krad_text_rep (krad_ebml_t *krad_ebml, uint64_t *data_read, krad_text_rep_t *krad_text_rep) {
  
  uint32_t ebml_id;
	uint64_t ebml_data_size;
  krad_text_rep_t *krad_text_rep_ret;
  
  if (krad_text_rep == NULL) {
    krad_text_rep_ret = krad_compositor_text_rep_create();
  } else {
    krad_text_rep_ret = krad_text_rep;
  }
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_TEXT) {
    *data_read = ebml_data_size;
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_TEXT) {
    krad_ebml_read_string (krad_ebml, krad_text_rep_ret->text, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_FONT) {
    krad_ebml_read_string (krad_ebml, krad_text_rep_ret->font, ebml_data_size);		  
  } 
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_RED) {
    krad_text_rep_ret->red = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_GREEN) {
    krad_text_rep_ret->green = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_BLUE) {
    krad_text_rep_ret->blue = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }

  krad_compositor_subunit_controls_read (krad_ebml, krad_text_rep_ret->controls);
  
  return krad_text_rep_ret;
}

void krad_compositor_text_rep_reset (krad_text_rep_t *krad_text_rep) {
  //kr_compositor_subunit_controls_t *krad_controls = krad_text_rep->controls;
  //krad_compositor_subunit_controls_reset (krad_controls);
  //memset (krad_text_rep, 0, sizeof (krad_text_rep_t));
  //krad_text_rep->controls = krad_controls;
}


krad_text_rep_t *krad_compositor_text_rep_create_and_init (int number, char *text, char *font, float red, float green, float blue, int x, int y, int z, int tickrate, float scale, float opacity, float rotation) {

  krad_text_rep_t *krad_text_rep = calloc(1, sizeof (krad_text_rep_t));
  strcpy (krad_text_rep->text, text);
  strcpy (krad_text_rep->font, font);
  krad_text_rep->red = red;
  krad_text_rep->green = green;
  krad_text_rep->blue = blue;
  
  krad_text_rep->controls = krad_compositor_subunit_controls_create_and_init (number, x, y, z, tickrate, 0, 0, scale, opacity, rotation); 
  return krad_text_rep;
}

krad_sprite_rep_t *krad_compositor_sprite_rep_create() {
  krad_sprite_rep_t *krad_sprite_rep = calloc(1, sizeof (krad_sprite_rep_t));
  krad_sprite_rep->controls = krad_compositor_subunit_controls_create();
  return krad_sprite_rep;
}

krad_sprite_rep_t *krad_compositor_sprite_rep_create_and_init ( int number, char *filename, int x, int y, int z, int tickrate, 
                                                            float scale, float opacity, float rotation) {
  
  krad_sprite_rep_t *krad_sprite_rep = calloc(1, sizeof (krad_sprite_rep_t));
  
  strcpy (krad_sprite_rep->filename, filename);
      
  krad_sprite_rep->controls = krad_compositor_subunit_controls_create_and_init (number, x, y, z, tickrate, 0, 0, scale, opacity, rotation); 
  return krad_sprite_rep;
}

void krad_compositor_sprite_rep_destroy (krad_sprite_rep_t *krad_sprite_rep) {
  krad_compositor_subunit_controls_destroy (krad_sprite_rep->controls);
  free (krad_sprite_rep);
}

void krad_compositor_sprite_rep_to_ebml (krad_sprite_rep_t *krad_sprite_rep, krad_ebml_t *krad_ebml) {
  uint64_t cmd;
  
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_LIST, &cmd);
  
  krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_COMPOSITOR_FILENAME, krad_sprite_rep->filename);
  krad_compositor_subunit_controls_to_ebml (krad_ebml, krad_sprite_rep->controls);

  krad_ebml_finish_element (krad_ebml, cmd);
}

krad_sprite_rep_t *krad_compositor_ebml_to_new_krad_sprite_rep (krad_ebml_t *krad_ebml, uint64_t *bytes_read) {
  
  uint32_t ebml_id;
  uint64_t ebml_data_size;

  krad_sprite_rep_t *krad_sprite_rep = krad_compositor_sprite_rep_create();
 
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_SPRITE_LIST) {
    *bytes_read = ebml_data_size;
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_FILENAME) {
    krad_ebml_read_string (krad_ebml, krad_sprite_rep->filename, ebml_data_size);
  }

  krad_compositor_subunit_controls_read (krad_ebml, krad_sprite_rep->controls);
  
  return krad_sprite_rep;
}

void krad_compositor_validate_vector_rep (krad_vector_rep_t *krad_vector_rep) {
  
  if ((krad_vector_rep->red < 0.0) || (krad_vector_rep->red > 0.255)) {
    printk("bad red value: %f", krad_vector_rep->red);
  }
  
  if ((krad_vector_rep->green < 0.0) || (krad_vector_rep->green > 0.255)) {
    printk("bad green value: %f", krad_vector_rep->green);
  }
  
  if ((krad_vector_rep->blue < 0.0) || (krad_vector_rep->blue > 0.255)) {
    printk("bad blue value: %f", krad_vector_rep->blue);
  }

}

krad_vector_rep_t *krad_compositor_vector_rep_create () {
  
  krad_vector_rep_t *krad_vector_rep = calloc(1, sizeof (krad_vector_rep_t));
  krad_vector_rep->controls = krad_compositor_subunit_controls_create();
  return krad_vector_rep;
}

void krad_compositor_vector_rep_destroy (krad_vector_rep_t *krad_vector_rep) {
  krad_compositor_subunit_controls_destroy (krad_vector_rep->controls);
  free (krad_vector_rep);
}

void krad_compositor_vector_rep_to_ebml (krad_vector_rep_t *krad_vector_rep, krad_ebml_t *krad_ebml) {
  
  uint64_t cmd;
  char vector_type_string [128];
  
  krad_ebml_start_element (krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT, &cmd);	
  krad_vector_type_to_string(krad_vector_rep->krad_vector_type, vector_type_string);
	krad_ebml_write_string (krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT, vector_type_string);
	
	krad_ebml_write_float (krad_ebml,
	                       EBML_ID_KRAD_COMPOSITOR_RED,
	                       krad_vector_rep->red);
	krad_ebml_write_float (krad_ebml,
	                       EBML_ID_KRAD_COMPOSITOR_GREEN,
	                       krad_vector_rep->green);
	krad_ebml_write_float (krad_ebml,
	                       EBML_ID_KRAD_COMPOSITOR_BLUE,
	                       krad_vector_rep->blue);
	

	krad_compositor_subunit_controls_to_ebml (krad_ebml, krad_vector_rep->controls);

	krad_ebml_finish_element (krad_ebml, cmd);
}

krad_vector_rep_t *krad_compositor_ebml_to_krad_vector_rep (krad_ebml_t *krad_ebml, uint64_t *data_read, krad_vector_rep_t *krad_vector_rep) {
  
  uint32_t ebml_id;
	uint64_t ebml_data_size;
  krad_vector_rep_t *krad_vector_rep_ret;
  char vector_type_string[128];
  
  if (krad_vector_rep == NULL) {
    krad_vector_rep_ret = krad_compositor_vector_rep_create();
  } else {
    krad_vector_rep_ret = krad_vector_rep;
  }
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_TEXT) {
    *data_read = ebml_data_size;
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);	
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_TEXT) {
    krad_ebml_read_string (krad_ebml, vector_type_string, ebml_data_size);
  }
  
  krad_vector_rep->krad_vector_type = krad_string_to_vector_type (vector_type_string);
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_RED) {
    krad_vector_rep_ret->red = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_GREEN) {
    krad_vector_rep_ret->green = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }
  
  krad_ebml_read_element (krad_ebml, &ebml_id, &ebml_data_size);
  if (ebml_id == EBML_ID_KRAD_COMPOSITOR_BLUE) {
    krad_vector_rep_ret->blue = krad_ebml_read_float (krad_ebml, ebml_data_size);
  }

  krad_compositor_subunit_controls_read (krad_ebml, krad_vector_rep_ret->controls);
  
  return krad_vector_rep_ret;
}

void krad_compositor_vector_rep_reset (krad_vector_rep_t *krad_vector_rep) {
  //kr_compositor_subunit_controls_t *krad_controls = krad_vector_rep->controls;
  //krad_compositor_subunit_controls_reset (krad_controls);
  //memset (krad_vector_rep, 0, sizeof (krad_vector_rep_t));
  //krad_vector_rep->controls = krad_controls;
}


krad_vector_rep_t *krad_compositor_vector_rep_create_and_init (int number, char *vector_type_string, float red, float green, float blue, int x, int y, int z, int tickrate, float scale, float opacity, float rotation) {

  krad_vector_rep_t *krad_vector_rep = calloc(1, sizeof (krad_vector_rep_t));
  krad_vector_rep->krad_vector_type = krad_string_to_vector_type (vector_type_string);
  krad_vector_rep->red = red;
  krad_vector_rep->green = green;
  krad_vector_rep->blue = blue;
  
  krad_vector_rep->controls = krad_compositor_subunit_controls_create_and_init (number, x, y, z, tickrate, 0, 0, scale, opacity, rotation); 
  return krad_vector_rep;
}
krad_vector_type_t krad_string_to_vector_type (char *string) {
  if (strcmp (string, "hex") == 0) {
    return HEX;
  } 
  if (strcmp (string, "selector") == 0) {
    return SELECTOR;
  }
  if (strcmp (string, "circle") == 0) {
    return CIRCLE;
  } 
  if (strcmp (string, "rect") == 0) {
    return RECT;
  } 
  if (strcmp (string, "triangle") == 0) {
    return TRIANGLE;
  } 
  if (strcmp (string, "viper") == 0) {
    return VIPER;
  } 
  if (strcmp (string, "meter") == 0) {
    return METER;
  } 
  if (strcmp (string, "grid") == 0) {
    return GRID;
  } 

  return 0;
}

void krad_vector_type_to_string (krad_vector_type_t krad_vector_type, char *string) {
  if (krad_vector_type == HEX) {
    strcpy ( string, "hex");
  }
  if (krad_vector_type == SELECTOR) {
    strcpy ( string, "selector");
  }
  if (krad_vector_type == CIRCLE) {
    strcpy ( string, "circle");
  }
  if (krad_vector_type == RECT) {
    strcpy ( string, "rect");
  }
  if (krad_vector_type == TRIANGLE) {
    strcpy ( string, "triangle");
  }
  if (krad_vector_type == VIPER) {
    strcpy ( string, "viper");
  }
  if (krad_vector_type == METER) {
    strcpy ( string, "meter");
  }
  if (krad_vector_type == GRID) {
    strcpy ( string, "grid");
  }
  
}
