#include "krad_radio_client.h"
#include "krad_radio_client_internal.h"
#include "krad_compositor_common.h"

typedef struct kr_videoport_St kr_videoport_t;

struct kr_videoport_St {

  int width;
  int height;
  kr_shm_t *kr_shm;
  kr_client_t *client;
  int sd;
  
  int (*callback)(void *, void *);
  void *pointer;
  
  int active;
  
  pthread_t process_thread;  
  
};

int kr_compositor_read_port ( kr_client_t *client, char *text) {

  uint32_t ebml_id;
  uint64_t ebml_data_size;
  int bytes_read;
  
  int source_width;
  
  char string[1024];
  memset (string, '\0', 1024);
  
  bytes_read = 0;

  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
  
  if (ebml_id != EBML_ID_KRAD_COMPOSITOR_PORT) {
    //printk ("hrm wtf1\n");
  } else {
    bytes_read += ebml_data_size + 11;
  }
  
  
  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);  

  if (ebml_id != EBML_ID_KRAD_COMPOSITOR_PORT_SOURCE_WIDTH) {
    //printk ("hrm wtf2\n");
  } else {
    //printk ("tag name size %zu\n", ebml_data_size);
  }

  source_width = krad_ebml_read_number (client->krad_ebml, ebml_data_size);
  
  
  sprintf (text, "Source Width: %d", source_width);
  
  
  return bytes_read;

}

int kr_compositor_read_sprite ( kr_client_t *client, char *text) {

  uint64_t ebml_data_size;
  int bytes_read;
  
  krad_sprite_rep_t *krad_sprite = krad_compositor_ebml_to_new_krad_sprite_rep (client->krad_ebml, &ebml_data_size);
  bytes_read = ebml_data_size + 10;
  
  sprintf (text, "Id: %d  Filename: \"%s\"  X: %d  Y: %d  Z: %d  Xscale: %f Yscale: %f  Rotation: %f  Opacity: %f", 
           krad_sprite->controls.number, krad_sprite->filename,
           krad_sprite->controls.x, krad_sprite->controls.y, krad_sprite->controls.z,
           krad_sprite->controls.xscale, krad_sprite->controls.yscale,
           krad_sprite->controls.rotation, krad_sprite->controls.opacity);
         
    
  return bytes_read;
}

int kr_compositor_read_text ( kr_client_t *client, char *text) {

  uint64_t ebml_data_size;
  int bytes_read;
  
  krad_text_rep_t *krad_text = krad_compositor_ebml_to_krad_text_rep (client->krad_ebml, &ebml_data_size, NULL);
  bytes_read = ebml_data_size + 9;

  sprintf (text, "Id: %d  Text:\"%s\"  Font: \"%s\"  Red: %f  Green: %f  Blue: %f  X: %d  Y: %d  Z: %d  Xscale: %f Yscale: %f  Rotation: %f  Opacity: %f", 
          krad_text->controls->number, krad_text->text, krad_text->font,
          krad_text->red, krad_text->green, krad_text->blue,
          krad_text->controls->x, krad_text->controls->y, krad_text->controls->z,
          krad_text->controls->xscale, krad_text->controls->yscale,
          krad_text->controls->rotation, krad_text->controls->opacity);
 
 return bytes_read;
}
  
int kr_compositor_read_frame_size ( kr_client_t *client, char *text, krad_compositor_rep_t **krad_compositor_rep) {

  uint32_t ebml_id;
  uint64_t ebml_data_size;
  int bytes_read;
  
  int width, height;
  
  char string[1024];
  memset (string, '\0', 1024);
  
  bytes_read = 6;

  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
  width = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
  bytes_read += ebml_data_size;
  
  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);  
  height = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
  bytes_read += ebml_data_size;

  sprintf (text, "Width: %d  Height: %d", 
           width, height);
    
  return bytes_read;
}

int kr_compositor_read_frame_rate ( kr_client_t *client, char *text, krad_compositor_rep_t **krad_compositor_rep) {

  uint32_t ebml_id;
  uint64_t ebml_data_size;
  int bytes_read;
  
  int numerator, denominator;
  
  char string[1024];
  memset (string, '\0', 1024);
  
  bytes_read = 6;

  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);
  numerator = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
  bytes_read += ebml_data_size;
  
  krad_ebml_read_element (client->krad_ebml, &ebml_id, &ebml_data_size);  
  denominator = krad_ebml_read_number( client->krad_ebml, ebml_data_size);
  bytes_read += ebml_data_size;

  sprintf (text, "Numerator: %d  Denominator: %d - %f", 
           numerator, denominator, (float) numerator / (float) denominator );
    
  return bytes_read;
}

void kr_compositor_port_list (kr_client_t *client) {

  uint64_t compositor_command;
  uint64_t list_ports;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LIST_PORTS, &list_ports);

  krad_ebml_finish_element (client->krad_ebml, list_ports);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_background (kr_client_t *client, char *filename) {

  uint64_t compositor_command;
  uint64_t background;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_BACKGROUND, &background);

  krad_ebml_write_string (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FILENAME, filename);

  krad_ebml_finish_element (client->krad_ebml, background);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_add_text (kr_client_t *client, char *text, int x, int y, int z, int tickrate, 
                  float scale, float opacity, float rotation, float red, float green, float blue, char *font) {

  uint64_t compositor_command;
  uint64_t textcmd;

  krad_text_rep_t *krad_text_rep = krad_compositor_text_rep_create();
  strcpy (krad_text_rep->text, text);
  strcpy (krad_text_rep->font, font);
  krad_text_rep->red = red;
  krad_text_rep->green = green;
  krad_text_rep->blue = blue;
  
  krad_compositor_subunit_controls_init (krad_text_rep->controls, 0, x, y, z, tickrate, 0, 0, scale, opacity, rotation);

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_ADD_TEXT, &textcmd);
 
  krad_compositor_text_rep_to_ebml (krad_text_rep, client->krad_ebml);

  krad_ebml_finish_element (client->krad_ebml, textcmd);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
  
  krad_ebml_write_sync (client->krad_ebml);

}

void kr_compositor_set_text (kr_client_t *client, int num, int x, int y, int z, int tickrate, 
                  float scale, float opacity, float rotation, float red, float green, float blue) {
/*
  uint64_t compositor_command;
  uint64_t text;
 
  krad_compositor_subunit_controls_init (krad_text_rep->controls, num, "", "", red, green, blue, x, y, z, tickrate, scale, opacity, rotation);
  
  compositor_command = 0;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_TEXT, &text);

  krad_compositor_text_rep_to_ebml (krad_text_rep, client->krad_ebml);
  
  krad_ebml_finish_element (client->krad_ebml, text);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);

  krad_ebml_write_sync (client->krad_ebml);

  krad_compositor_text_rep_destroy (krad_text_rep);
*/
}

void kr_compositor_remove_text (kr_client_t *client, int num) {

  uint64_t compositor_command;
  uint64_t text;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_REMOVE_TEXT, &text);


  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_TEXT_NUMBER, num);

  krad_ebml_finish_element (client->krad_ebml, text);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_list_texts (kr_client_t *client) {

  uint64_t compositor_command;
  uint64_t text;
  
  compositor_command = 0;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LIST_TEXTS, &text);

  krad_ebml_finish_element (client->krad_ebml, text);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_add_sprite (kr_client_t *client, char *filename, int x, int y, int z, int tickrate, 
                  float scale, float opacity, float rotation) {

  uint64_t compositor_command;
  uint64_t sprite;
  
  compositor_command = 0;
  
  krad_sprite_rep_t *krad_sprite_rep = 
      krad_compositor_sprite_rep_create_and_init (0, filename, x, y, z, tickrate, scale, opacity, rotation);

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_ADD_SPRITE, &sprite);
  
  krad_compositor_sprite_rep_to_ebml(krad_sprite_rep, client->krad_ebml);
  
  krad_ebml_finish_element (client->krad_ebml, sprite);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
  
  kr_compositor_sprite_rep_destroy (krad_sprite_rep);
}

void kr_compositor_set_sprite (kr_client_t *client, int num, 
                                     int x, int y, int z, int tickrate, 
                                     float scale, float opacity, float rotation) {

  uint64_t compositor_command;
  uint64_t sprite;
  
  compositor_command = 0;
  
  krad_sprite_rep_t *krad_sprite_rep = 
      krad_compositor_sprite_rep_create_and_init (num, "", x, y, z, tickrate, scale, opacity, rotation);

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_SPRITE, &sprite);
 
  krad_compositor_sprite_rep_to_ebml (krad_sprite_rep, client->krad_ebml);
  
  krad_ebml_finish_element (client->krad_ebml, sprite);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
  
  kr_compositor_sprite_rep_destroy (krad_sprite_rep);
}

void kr_compositor_remove_sprite (kr_client_t *client, int num) {

  uint64_t compositor_command;
  uint64_t sprite;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_REMOVE_SPRITE, &sprite);

  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_SPRITE_NUMBER, num);

  krad_ebml_finish_element (client->krad_ebml, sprite);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_list_sprites (kr_client_t *client) {

  uint64_t compositor_command;
  uint64_t sprite;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LIST_SPRITES, &sprite);

  krad_ebml_finish_element (client->krad_ebml, sprite);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_close_display (kr_client_t *client) {

  uint64_t compositor_command;
  uint64_t display;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_CLOSE_DISPLAY, &display);

  krad_ebml_finish_element (client->krad_ebml, display);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_open_display (kr_client_t *client, int width, int height) {

  uint64_t compositor_command;
  uint64_t display;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_OPEN_DISPLAY, &display);

  krad_ebml_finish_element (client->krad_ebml, display);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_snapshot (kr_client_t *client) {

  uint64_t command;
  uint64_t snap_command;
  command = 0;
  snap_command = 0;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SNAPSHOT, &snap_command);
  krad_ebml_finish_element (client->krad_ebml, snap_command);

  krad_ebml_finish_element (client->krad_ebml, command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_snapshot_jpeg (kr_client_t *client) {

  uint64_t command;
  uint64_t snap_command;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SNAPSHOT_JPEG, &snap_command);
  krad_ebml_finish_element (client->krad_ebml, snap_command);
  krad_ebml_finish_element (client->krad_ebml, command);
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_set_frame_rate (kr_client_t *client, int numerator, int denominator) {

  uint64_t compositor_command;
  uint64_t set_frame_rate;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_FRAME_RATE, &set_frame_rate);

  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FPS_NUMERATOR, numerator);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_FPS_DENOMINATOR, denominator);

  krad_ebml_finish_element (client->krad_ebml, set_frame_rate);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_set_resolution (kr_client_t *client, int width, int height) {

  uint64_t compositor_command;
  uint64_t set_resolution;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_SET_RESOLUTION, &set_resolution);

  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_WIDTH, width);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_HEIGHT, height);

  krad_ebml_finish_element (client->krad_ebml, set_resolution);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_last_snap_name (kr_client_t *client) {

  uint64_t command;
  uint64_t snap_command;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_GET_LAST_SNAPSHOT_NAME, &snap_command);
  krad_ebml_finish_element (client->krad_ebml, snap_command);
  krad_ebml_finish_element (client->krad_ebml, command);
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_info (kr_client_t *client) {

  uint64_t command;
  uint64_t info_command;
  command = 0;
  info_command = 0;
  
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &command);

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_INFO, &info_command);
  krad_ebml_finish_element (client->krad_ebml, info_command);

  krad_ebml_finish_element (client->krad_ebml, command);
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_compositor_set_port_mode (kr_client_t *client, int number, uint32_t x, uint32_t y,
                    uint32_t width, uint32_t height, uint32_t crop_x, uint32_t crop_y,
                    uint32_t crop_width, uint32_t crop_height, float opacity, float rotation) {

  uint64_t compositor_command;
  uint64_t update_port;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_UPDATE_PORT, &update_port);

  krad_ebml_write_int8 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_NUMBER, number);

  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_X, x);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_Y, y);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_WIDTH, width);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_HEIGHT, height);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_CROP_X, crop_x);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_CROP_Y, crop_y);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_CROP_WIDTH, crop_width);
  krad_ebml_write_int32 (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_CROP_HEIGHT, crop_height);
  krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_OPACITY, opacity);
  krad_ebml_write_float (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_PORT_ROTATION, rotation);

  krad_ebml_finish_element (client->krad_ebml, update_port);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_ebml_to_compositor_rep (unsigned char *ebml_frag, kr_compositor_t **kr_compositor_rep_in) {

  uint32_t ebml_id;
	uint64_t ebml_data_size;
  kr_compositor_t *kr_compositor_rep;
  int item_pos;

  item_pos = 0;

  if (*kr_compositor_rep_in == NULL) {
    *kr_compositor_rep_in = kr_compositor_rep_create ();
  }
  
  kr_compositor_rep = *kr_compositor_rep_in;
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->width = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->height = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->fps_numerator = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->fps_denominator = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->sprites = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->texts = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->vectors = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->inputs = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->outputs = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  kr_compositor_rep->frames = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

}

void kr_ebml_to_comp_controls (unsigned char *ebml_frag, kr_comp_controls_t *controls) {

  uint32_t ebml_id;
  uint64_t ebml_data_size;
  int item_pos;

  item_pos = 0;
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->number = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->x = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->y = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->z = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->tickrate = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);

	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->xscale = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
 
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->yscale = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->opacity = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);	
  controls->rotation = krad_ebml_read_number_from_frag_add (ebml_frag + item_pos, ebml_data_size, &item_pos);
}

void kr_ebml_to_sprite_rep (unsigned char *ebml_frag, kr_sprite_t **kr_sprite_rep_in) {

  uint32_t ebml_id;
	uint64_t ebml_data_size;
  kr_sprite_t *kr_sprite_rep;
  int item_pos;

  item_pos = 0;

  if (*kr_sprite_rep_in == NULL) {
    *kr_sprite_rep_in = kr_compositor_sprite_rep_create ();
  }
  
  kr_sprite_rep = *kr_sprite_rep_in;
  
	item_pos += krad_ebml_read_element_from_frag (ebml_frag + item_pos, &ebml_id, &ebml_data_size);
	item_pos += krad_ebml_read_string_from_frag (ebml_frag + item_pos, ebml_data_size, kr_sprite_rep->filename);
  
  kr_ebml_to_comp_controls (ebml_frag + item_pos, &kr_sprite_rep->controls);
  
}

int kr_compositor_response_get_string_from_compositor (unsigned char *ebml_frag, char **string) {

	int pos;
  kr_compositor_t *kr_compositor;

  pos = 0;
  kr_compositor = NULL;

  kr_ebml_to_compositor_rep (ebml_frag, &kr_compositor);
  pos += sprintf (*string + pos, "Resolution: %ux%u\n", kr_compositor->width, kr_compositor->height);
  pos += sprintf (*string + pos, "Frame Rate: %u / %u\n", kr_compositor->fps_numerator, kr_compositor->fps_denominator);
  pos += sprintf (*string + pos, "Sprites: %u\n", kr_compositor->sprites);
  pos += sprintf (*string + pos, "Vectors: %u\n", kr_compositor->vectors);
  pos += sprintf (*string + pos, "Texts: %u\n", kr_compositor->texts);
  pos += sprintf (*string + pos, "Inputs: %u\n", kr_compositor->inputs);
  pos += sprintf (*string + pos, "Outputs: %u\n", kr_compositor->outputs);
  pos += sprintf (*string + pos, "Frames: %"PRIu64"\n", kr_compositor->frames);
        
  kr_compositor_rep_destroy (kr_compositor);
  
  return pos; 
}

int kr_compositor_response_get_string_from_subunit_controls (kr_comp_controls_t *controls, char *string) {

  int pos;

  pos = 0;

  pos += sprintf (string + pos, "X: %d\n", controls->x);
  pos += sprintf (string + pos, "Y: %d\n", controls->y);
  pos += sprintf (string + pos, "Z: %d\n", controls->z);

  pos += sprintf (string + pos, "Y Scale: %f\n", controls->yscale);
  pos += sprintf (string + pos, "X Scale: %f\n", controls->xscale);

  pos += sprintf (string + pos, "Opacity: %f\n", controls->opacity);
  pos += sprintf (string + pos, "Rotation: %f\n", controls->rotation);
  pos += sprintf (string + pos, "Tickrate: %d\n", controls->tickrate);
  
  return pos;

}

int kr_compositor_response_get_string_from_sprite (unsigned char *ebml_frag, char **string) {

	int pos;
  kr_sprite_t *kr_sprite;

  pos = 0;
  kr_sprite = NULL;

  kr_ebml_to_sprite_rep (ebml_frag, &kr_sprite);
  pos += sprintf (*string + pos, "Filename: %s\n", kr_sprite->filename);
  pos += kr_compositor_response_get_string_from_subunit_controls (&kr_sprite->controls, *string + pos);

  kr_compositor_sprite_rep_destroy (kr_sprite);
  
  return pos; 
}

int kr_compositor_response_get_string_from_subunit (kr_response_t *kr_response, unsigned char *ebml_frag, char **string) {

  switch ( kr_response->address.path.subunit.compositor_subunit ) {
    case KR_SPRITE:
      return kr_compositor_response_get_string_from_sprite (ebml_frag, string);
      break;
    default:
      return sprintf (*string, "Annoying Placeholder Text\n");
  }
  
  return 0; 
}

int kr_compositor_response_to_string (kr_response_t *kr_response, char **string) {

  switch ( kr_response->type ) {
    case EBML_ID_KRAD_UNIT_INFO:
      *string = kr_response_alloc_string (kr_response->size * 4);
      return kr_compositor_response_get_string_from_compositor (kr_response->buffer, string);
    case EBML_ID_KRAD_SUBUNIT_INFO:
      *string = kr_response_alloc_string (kr_response->size * 4);
      return kr_compositor_response_get_string_from_subunit (kr_response, kr_response->buffer, string);
    case EBML_ID_KRAD_SUBUNIT_CREATED:
      *string = kr_response_alloc_string (kr_response->size * 4);
      return kr_compositor_response_get_string_from_subunit (kr_response, kr_response->buffer, string);
  }
  return 0;
}

void kr_videoport_destroy_cmd (kr_client_t *client) {

  uint64_t compositor_command;
  uint64_t destroy_videoport;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LOCAL_VIDEOPORT_DESTROY, &destroy_videoport);

  krad_ebml_finish_element (client->krad_ebml, destroy_videoport);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}

void kr_videoport_create_cmd (kr_client_t *client) {

  uint64_t compositor_command;
  uint64_t create_videoport;
  
  compositor_command = 0;

  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD, &compositor_command);
  krad_ebml_start_element (client->krad_ebml, EBML_ID_KRAD_COMPOSITOR_CMD_LOCAL_VIDEOPORT_CREATE, &create_videoport);

  krad_ebml_finish_element (client->krad_ebml, create_videoport);
  krad_ebml_finish_element (client->krad_ebml, compositor_command);
    
  krad_ebml_write_sync (client->krad_ebml);
}


void kr_videoport_set_callback (kr_videoport_t *kr_videoport, int callback (void *, void *), void *pointer) {

  kr_videoport->callback = callback;
  kr_videoport->pointer = pointer;

}

void *kr_videoport_process_thread (void *arg) {

  kr_videoport_t *kr_videoport = (kr_videoport_t *)arg;
  int ret;
  char buf[1];

  krad_system_set_thread_name ("krc_videoport");

  while (kr_videoport->active == 1) {
  
    // wait for socket to have a byte
    ret = read (kr_videoport->sd, buf, 1);
    if (ret != 1) {
      printke ("krad compositor client: unexpected read return value %d in kr_videoport_process_thread", ret);
    }
    kr_videoport->callback (kr_videoport->kr_shm->buffer, kr_videoport->pointer);

    // write a byte to socket
    ret = write (kr_videoport->sd, buf, 1);
    if (ret != 1) {
      printke ("krad compositor client: unexpected write return value %d in kr_videoport_process_thread", ret);
    }

  }


  return NULL;

}



void kr_videoport_activate (kr_videoport_t *kr_videoport) {
  if ((kr_videoport->active == 0) && (kr_videoport->callback != NULL)) {
    pthread_create (&kr_videoport->process_thread, NULL, kr_videoport_process_thread, (void *)kr_videoport);
    kr_videoport->active = 1;
  }
}

void kr_videoport_deactivate (kr_videoport_t *kr_videoport) {

  if (kr_videoport->active == 1) {
    kr_videoport->active = 2;
    pthread_join (kr_videoport->process_thread, NULL);
    kr_videoport->active = 0;
  }
}

kr_videoport_t *kr_videoport_create (kr_client_t *client) {

  kr_videoport_t *kr_videoport;
  int sockets[2];

  if (!kr_client_local (client)) {
    // Local clients only
    return NULL;
  }

  kr_videoport = calloc (1, sizeof(kr_videoport_t));

  if (kr_videoport == NULL) {
    return NULL;
  }

  kr_videoport->client = client;

  kr_videoport->kr_shm = kr_shm_create (kr_videoport->client);

  sprintf (kr_videoport->kr_shm->buffer, "waa hoo its yaytime");

  if (kr_videoport->kr_shm == NULL) {
    free (kr_videoport);
    return NULL;
  }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    kr_shm_destroy (kr_videoport->kr_shm);
    free (kr_videoport);
    return NULL;
    }

  kr_videoport->sd = sockets[0];
  
  printf ("sockets %d and %d\n", sockets[0], sockets[1]);
  
  kr_videoport_create_cmd (kr_videoport->client);
  //FIXME use a return message from daemon to indicate ready to receive fds
  usleep (33000);
  kr_send_fd (kr_videoport->client, kr_videoport->kr_shm->fd);
  usleep (33000);
  kr_send_fd (kr_videoport->client, sockets[1]);
  usleep (33000);
  

  return kr_videoport;

}

void kr_videoport_destroy (kr_videoport_t *kr_videoport) {

  if (kr_videoport->active == 1) {
    kr_videoport_deactivate (kr_videoport);
  }

  kr_videoport_destroy_cmd (kr_videoport->client);

  if (kr_videoport != NULL) {
    if (kr_videoport->sd != 0) {
      close (kr_videoport->sd);
      kr_videoport->sd = 0;
    }
    if (kr_videoport->kr_shm != NULL) {
      kr_shm_destroy (kr_videoport->kr_shm);
      kr_videoport->kr_shm = NULL;
    }
    free(kr_videoport);
  }

}

