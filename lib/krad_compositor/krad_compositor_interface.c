#include "krad_compositor_interface.h"

void krad_compositor_subunit_to_ebml (krad_ipc_server_t *krad_ipc, krad_compositor_subunit_t *krad_compositor_subunit) {
  
  krad_ebml_write_int32 (krad_ipc->current_client->krad_ebml2, 
                         EBML_ID_KRAD_COMPOSITOR_X, 
                         krad_compositor_subunit->x);
  krad_ebml_write_int32 (krad_ipc->current_client->krad_ebml2, 
                         EBML_ID_KRAD_COMPOSITOR_Y, 
                         krad_compositor_subunit->y);
  krad_ebml_write_int32 (krad_ipc->current_client->krad_ebml2, 
                         EBML_ID_KRAD_COMPOSITOR_Y, 
                         krad_compositor_subunit->z);
  
  krad_ebml_write_float (krad_ipc->current_client->krad_ebml2,
                         EBML_ID_KRAD_COMPOSITOR_SPRITE_TICKRATE,
                         krad_compositor_subunit->tickrate);
  
  krad_ebml_write_float (krad_ipc->current_client->krad_ebml2,
                         EBML_ID_KRAD_COMPOSITOR_SPRITE_SCALE,
                         krad_compositor_subunit->xscale);               
  
  krad_ebml_write_float (krad_ipc->current_client->krad_ebml2, 
                         EBML_ID_KRAD_COMPOSITOR_SPRITE_OPACITY, 
                         krad_compositor_subunit->opacity);
  
  krad_ebml_write_float (krad_ipc->current_client->krad_ebml2, 
                       EBML_ID_KRAD_COMPOSITOR_SPRITE_ROTATION, 
                       krad_compositor_subunit->rotation);
}  

void krad_compositor_sprite_to_ebml ( krad_ipc_server_t *krad_ipc, krad_sprite_t *krad_sprite, int number) {

  krad_sprite_rep_t *ksr = krad_sprite_to_sprite_rep (krad_sprite);
  ksr->controls->number = number;
  krad_compositor_sprite_rep_to_ebml (ksr, krad_ipc->current_client->krad_ebml2);
  krad_compositor_sprite_rep_destroy(ksr);

}

void krad_compositor_port_to_ebml ( krad_ipc_server_t *krad_ipc, krad_compositor_port_t *krad_compositor_port) {

  uint64_t port;

  krad_ebml_start_element (krad_ipc->current_client->krad_ebml2, EBML_ID_KRAD_COMPOSITOR_PORT, &port);  

  krad_ipc_server_respond_number ( krad_ipc,
                   EBML_ID_KRAD_COMPOSITOR_PORT_SOURCE_WIDTH,
                   krad_compositor_port->source_width);
                   
  krad_ebml_finish_element (krad_ipc->current_client->krad_ebml2, port);
  
}


void krad_compositor_text_to_ebml ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc, krad_text_t *krad_text, int number) {

  krad_text_rep_t *ktr = krad_text_to_text_rep (krad_text, NULL);
  ktr->controls->number = number;
  krad_compositor_text_rep_to_ebml (ktr, krad_ipc->current_client->krad_ebml2);
  krad_compositor_text_rep_destroy(ktr);
  
}

int krad_compositor_handler ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc ) {
  
  uint32_t ebml_id;
  
  uint32_t command;
  uint64_t ebml_data_size;
  uint64_t element;
  uint64_t response;
  uint64_t info_loc;
  uint64_t payload_loc;
  krad_sprite_rep_t *krad_sprite_rep;
  krad_text_rep_t *krad_text_rep;
  
  uint64_t numbers[32];    
  float floats[32];      
  
  char string[1024];
//  char string2[1024];  
  
  int p;
  int s;
  
  int sd1;
  int sd2;
      
  sd1 = 0;
  sd2 = 0;  
  
  p = 0;
  s = 0;
  
  string[0] = '\0';
  //string2[0] = '\0';
  
  krad_ipc_server_read_command ( krad_ipc, &command, &ebml_data_size);

  switch ( command ) {
  
    case EBML_ID_KRAD_COMPOSITOR_CMD_UPDATE_PORT:

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_NUMBER) {
        numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_X) {
        numbers[1] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_Y) {
        numbers[2] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_WIDTH) {
        numbers[3] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_HEIGHT) {
        numbers[4] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_CROP_X) {
        numbers[5] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_CROP_Y) {
        numbers[6] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_CROP_WIDTH) {
        numbers[7] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_CROP_HEIGHT) {
        numbers[8] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }      
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_OPACITY) {
        floats[0] = krad_ebml_read_float (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_PORT_ROTATION) {
        floats[1] = krad_ebml_read_float (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }      
      
      printkd ("comp params ipc cmd");
      
      krad_compositor_port_set_comp_params (&krad_compositor->port[numbers[0]],
                             numbers[1], numbers[2], numbers[3], numbers[4],
                          numbers[5], numbers[6], numbers[7], numbers[8],
                          floats[0], floats[1]);

      break;    
  
    case EBML_ID_KRAD_COMPOSITOR_CMD_INFO:

      krad_ipc_server_response_start_with_address_and_type ( krad_ipc,
                                                             &krad_compositor->address,
                                                             EBML_ID_KRAD_UNIT_INFO,
                                                             &response);

      krad_ipc_server_payload_start ( krad_ipc, &payload_loc);

      krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_INFO, &info_loc);

      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_WIDTH,
                       krad_compositor->width);

      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_HEIGHT,
                       krad_compositor->height);  
      
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_FPS_NUMERATOR,
                       krad_compositor->frame_rate_numerator);

      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_FPS_DENOMINATOR,
                       krad_compositor->frame_rate_denominator);
                       
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_SPRITE_COUNT,
                                       krad_compositor->active_sprites);
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_TEXT_COUNT,
                                       krad_compositor->active_texts);
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_VECTOR_COUNT,
                                       0);
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_PORT_COUNT,
                                       krad_compositor->active_input_ports);
      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_PORT_COUNT,
                                       krad_compositor->active_output_ports);

      krad_ipc_server_respond_number ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_PORT_COUNT,
                                       krad_compositor->frame_num);
                       
      krad_ipc_server_response_finish ( krad_ipc, info_loc);
      
      krad_ipc_server_payload_finish ( krad_ipc, payload_loc );
      krad_ipc_server_response_finish ( krad_ipc, response );
      
      return 1;

    case EBML_ID_KRAD_COMPOSITOR_CMD_SNAPSHOT:
      krad_compositor->snapshot++;
      break;
    case EBML_ID_KRAD_COMPOSITOR_CMD_SNAPSHOT_JPEG:
      krad_compositor->snapshot_jpeg++;
      break;  
      
    case EBML_ID_KRAD_COMPOSITOR_CMD_SET_FRAME_RATE:

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_FPS_NUMERATOR) {
        numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_FPS_DENOMINATOR) {
        numbers[1] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_compositor_set_frame_rate (krad_compositor, numbers[0], numbers[1]);

      break;

    case EBML_ID_KRAD_COMPOSITOR_CMD_SET_RESOLUTION:

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_WIDTH) {
        numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }

      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_HEIGHT) {
        numbers[1] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
      
      krad_compositor_update_resolution (krad_compositor, numbers[0], numbers[1]);

      break;

  
    case EBML_ID_KRAD_COMPOSITOR_CMD_LIST_PORTS:

      
      krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_MSG, &response);
      krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_PORT_LIST, &element);  
      

      for (p = 0; p < KC_MAX_PORTS; p++) {
        if (krad_compositor->port[p].krad_compositor_subunit->active == 1) {
          //printf("Link %d Active: %s\n", k, krad_linker->krad_link[k]->mount);
          krad_compositor_port_to_ebml ( krad_ipc, &krad_compositor->port[p]);
        }
      }
      
      krad_ipc_server_response_list_finish ( krad_ipc, element );
      krad_ipc_server_response_finish ( krad_ipc, response );  
        
      break;  
      
    case EBML_ID_KRAD_COMPOSITOR_CMD_SET_BACKGROUND:
      
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);  

      if (ebml_id != EBML_ID_KRAD_COMPOSITOR_FILENAME) {
        
      }

      krad_ebml_read_string (krad_ipc->current_client->krad_ebml, string, ebml_data_size);

      krad_compositor_set_background (krad_compositor, string);
    
      break;      
  
    case EBML_ID_KRAD_COMPOSITOR_CMD_CLOSE_DISPLAY:
    
      krad_compositor_close_display (krad_compositor);    
      
      break;
    case EBML_ID_KRAD_COMPOSITOR_CMD_OPEN_DISPLAY:
      
      krad_compositor->display_width = krad_compositor->width;
      krad_compositor->display_height = krad_compositor->height;
      
      krad_compositor_open_display (krad_compositor);
      
      break;    

  case EBML_ID_KRAD_COMPOSITOR_CMD_ADD_SPRITE:
   
    krad_sprite_rep = krad_compositor_ebml_to_new_krad_sprite_rep (krad_ipc->current_client->krad_ebml, numbers);
    
    krad_compositor_add_sprite (krad_compositor, krad_sprite_rep);
    
    krad_compositor_sprite_rep_destroy (krad_sprite_rep);
    
    break;

  case EBML_ID_KRAD_COMPOSITOR_CMD_SET_SPRITE:
    
    krad_sprite_rep = krad_compositor_ebml_to_new_krad_sprite_rep (krad_ipc->current_client->krad_ebml, numbers);
    krad_compositor_set_sprite (krad_compositor, krad_sprite_rep);
    krad_compositor_sprite_rep_destroy (krad_sprite_rep);
    break;
    
  case EBML_ID_KRAD_COMPOSITOR_CMD_REMOVE_SPRITE:
    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_SPRITE_NUMBER) {
        numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
    
      krad_compositor_remove_sprite (krad_compositor, numbers[0]);
    
      break;
      
  case EBML_ID_KRAD_COMPOSITOR_CMD_LIST_SPRITES:
    
    krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_MSG, &response);
    krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_SPRITE_LIST, &element);  

    for (s = 0; s < KC_MAX_SPRITES; s++) {
      if (krad_compositor->krad_sprite[s].krad_compositor_subunit->active) {
        krad_compositor_sprite_to_ebml ( krad_ipc, &krad_compositor->krad_sprite[s], s);
      }
    }
    
    krad_ipc_server_response_list_finish ( krad_ipc, element );
    krad_ipc_server_response_finish ( krad_ipc, response );  
          
    break;

  case EBML_ID_KRAD_COMPOSITOR_CMD_ADD_TEXT:
    
    krad_text_rep = krad_compositor_ebml_to_krad_text_rep (krad_ipc->current_client->krad_ebml, numbers, NULL);
    krad_compositor_add_text (krad_compositor, krad_text_rep);
    krad_compositor_text_rep_destroy (krad_text_rep);
    
    break;
      
  case EBML_ID_KRAD_COMPOSITOR_CMD_SET_TEXT:
    
    krad_compositor_ebml_to_krad_text_rep (krad_ipc->current_client->krad_ebml, numbers, krad_compositor->krad_text_rep_ipc);
    krad_compositor_set_text(krad_compositor, krad_compositor->krad_text_rep_ipc);
    
    break;
    
  case EBML_ID_KRAD_COMPOSITOR_CMD_REMOVE_TEXT:
    
      krad_ebml_read_element (krad_ipc->current_client->krad_ebml, &ebml_id, &ebml_data_size);
      if (ebml_id == EBML_ID_KRAD_COMPOSITOR_TEXT_NUMBER) {
        numbers[0] = krad_ebml_read_number (krad_ipc->current_client->krad_ebml, ebml_data_size);
      }
    
      krad_compositor_remove_text (krad_compositor, numbers[0]);
    
      break;
      
    case EBML_ID_KRAD_COMPOSITOR_CMD_LIST_TEXTS:
    
      printk ("krad compositor handler: LIST_TEXTS");
    
      krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_MSG, &response);
      //krad_ipc_server_respond_string ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_TEXT, "this is a test name");
      krad_ipc_server_response_list_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_TEXT_LIST, &element);  
    

      for (s = 0; s < KC_MAX_TEXTS; s++) {
        if (krad_compositor->krad_text[s].krad_compositor_subunit->active) {
          krad_compositor_text_to_ebml ( krad_compositor, krad_ipc, &krad_compositor->krad_text[s], s);
        }
      }
    
      krad_ipc_server_response_list_finish ( krad_ipc, element );
      krad_ipc_server_response_finish ( krad_ipc, response );  
      break;
      
    case EBML_ID_KRAD_COMPOSITOR_CMD_GET_LAST_SNAPSHOT_NAME:

      krad_compositor_get_last_snapshot_name (krad_compositor, string);
      krad_ipc_server_response_start ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_MSG, &response);
      krad_ipc_server_respond_string ( krad_ipc, EBML_ID_KRAD_COMPOSITOR_LAST_SNAPSHOT_NAME, string);
      krad_ipc_server_response_finish ( krad_ipc, response);
    
      break;
      
    case EBML_ID_KRAD_COMPOSITOR_CMD_LOCAL_VIDEOPORT_DESTROY:

      for (p = 0; p < KC_MAX_PORTS; p++) {
        if (krad_compositor->port[p].local == 1) {
          krad_compositor_port_destroy (krad_compositor, &krad_compositor->port[p]);
          break;
        }
      }
        
      break;

    case EBML_ID_KRAD_COMPOSITOR_CMD_LOCAL_VIDEOPORT_CREATE:
    
  
      sd1 = 0;
      sd2 = 0;
    
      sd1 = krad_ipc_server_recvfd (krad_ipc->current_client);
      sd2 = krad_ipc_server_recvfd (krad_ipc->current_client);
        
      printk ("VIDEOPORT_CREATE Got FD's %d and %d\n", sd1, sd2);
        
      krad_compositor_local_port_create (krad_compositor, "localport", INPUT, sd1, sd2);
        
      break;
      
  }

  return 0;
}

