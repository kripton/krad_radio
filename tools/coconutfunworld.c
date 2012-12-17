#include "krad_radio.h"

#define KRAD_TRANSPONDER_SUBUNITS 32
#define KRAD_TRANSPONDER_PORT_CONNECTIONS 24

typedef struct kradx_transponder_St kradx_transponder_t;
typedef struct krad_transponder_subunit_St krad_transponder_subunit_t;
typedef struct krad_transponder_input_port_St krad_transponder_input_port_t;
typedef struct krad_transponder_output_port_St krad_transponder_output_port_t;
typedef struct krad_codec_slice_St krad_codec_slice_t;
typedef struct krad_transponder_control_msg_St krad_transponder_control_msg_t;

typedef enum {
  CONNECTPORTS = 77999,
  DISCONNECTPORTS,
  UPDATE,
  DESTROY,
} krad_transponder_control_msg_type_t;

typedef enum {
  DEMUXER = 1999,
  MUXER,
  DECODER,
  ENCODER,
  //OUTPUT,
  //STREAMIN
  //FILEIN
  //transmitter
  //listener
} krad_transponder_subunit_type_t;

struct krad_transponder_control_msg_St {
  
  krad_transponder_control_msg_type_t type;
  
  krad_transponder_input_port_t *input_port;
  krad_transponder_output_port_t *output_port;
    
};

struct krad_codec_slice_St {
  unsigned char *data;
  uint32_t size;
	int refs;
	int final;
};

struct krad_transponder_input_port_St {
  krad_ringbuffer_t *msg_ring;
  int socketpair[2];
  krad_transponder_subunit_t *subunit;  
  krad_transponder_subunit_t *connected_to_subunit;
};

struct krad_transponder_output_port_St {
  krad_transponder_input_port_t **connections;
};

struct krad_transponder_subunit_St {
  int destroy;
  kradx_transponder_t *kradx_transponder;
	pthread_t thread;
  krad_transponder_subunit_type_t type;
  krad_transponder_input_port_t *control;  
  krad_transponder_input_port_t **inputs;
  krad_transponder_output_port_t **outputs;
};

struct kradx_transponder_St {
	krad_transponder_subunit_t **subunits;
	krad_radio_t *krad_radio;
};

/* Prototypes */

void krad_transponder_subunit_destroy (krad_transponder_subunit_t **krad_transponder_subunit);

/* Internal */

char *transponder_subunit_type_to_string (krad_transponder_subunit_type_t type) {

  switch (type) {
    case DEMUXER:
      return "Demuxer";
    case MUXER:
      return "Muxer";
    case DECODER:
      return "Decoder";
    case ENCODER:
      return "Encoder";
  }
  
  return "Unknown Subunit";
}


void krad_codec_slice_unref (krad_codec_slice_t *krad_codec_slice) {
  __sync_fetch_and_sub( &krad_codec_slice->refs, 1 );
}

void krad_transponder_port_read (krad_transponder_input_port_t *inport, void *msg) {

  int ret;
  char buffer[1];

  ret = read (inport->socketpair[1], buffer, 1);
  if (ret != 1) {
    printf ("Krad Transponder: port read unexpected read return value %d\n", ret);
  }

  if (msg != NULL) {
    printf ("Krad Transponder: msg read space %zu\n", krad_ringbuffer_read_space (inport->msg_ring));  
    ret = krad_ringbuffer_read (inport->msg_ring, (char *)msg, sizeof(void *));
    if (ret != sizeof(void *)) {
      printf ("Krad Transponder: invalid msg read len %d\n", ret);
    }    
  } else {
    printf ("uh oh nullzor!\n");
  }
  printf ("Krad Transponder: input port read\n");

}

void krad_transponder_port_write (krad_transponder_input_port_t *input, void *msg) {

  int wrote;

  if (input != NULL) {
    krad_ringbuffer_write (input->msg_ring, (char *)msg, sizeof(void *));
    wrote = write (input->socketpair[0], "0", 1);
    if (wrote != 1) {
      printf ("Krad Transponder: port write unexpected write return value %d\n", wrote);
    }
  }

  printf ("Krad Transponder: port write\n");

}

void krad_transponder_port_broadcast (krad_transponder_output_port_t *outport, void *msg) {

  int p;
  
  printf ("Krad Transponder: output port broadcasting\n");  
  
  for (p = 0; p < KRAD_TRANSPONDER_PORT_CONNECTIONS; p++) {
    if (outport->connections[p] != NULL) {
      krad_transponder_port_write (outport->connections[p], msg);
    }
  }

  printf ("Krad Transponder: output port broadcasted\n");

}

void krad_transponder_port_disconnect (krad_transponder_subunit_t *krad_transponder_subunit,
                                    krad_transponder_output_port_t *output,
                                    krad_transponder_input_port_t *input) {

  printf ("Krad Transponder: sending disconnect ports msg\n");

  krad_transponder_control_msg_t *msg;
  msg = calloc (1, sizeof (krad_transponder_control_msg_t));
  msg->type = DISCONNECTPORTS;
  msg->input_port = input;
  msg->output_port = output;
  krad_transponder_port_write (krad_transponder_subunit->control, &msg);

  printf ("Krad Transponder: sent disconnect ports msg\n");
}

void krad_transponder_port_connect (krad_transponder_subunit_t *krad_transponder_subunit,
                                    krad_transponder_subunit_t *from_krad_transponder_subunit,
                                    krad_transponder_output_port_t *output,
                                    krad_transponder_input_port_t *input) {

  printf ("Krad Transponder: sending connecting ports msg\n");

  krad_transponder_control_msg_t *msg;
  msg = calloc (1, sizeof (krad_transponder_control_msg_t));
  msg->type = CONNECTPORTS;
  msg->input_port = input;
  msg->output_port = output;
  
  input->connected_to_subunit = krad_transponder_subunit;
  input->subunit = from_krad_transponder_subunit;
  krad_transponder_port_write (krad_transponder_subunit->control, &msg);

  printf ("Krad Transponder: sent connecting ports msg\n");
}

void krad_transponder_input_port_disconnect (krad_transponder_input_port_t *krad_transponder_input_port) {
  printf ("Krad Transponder: disconnecting ports\n");
	//close (krad_transponder_input_port->socketpair[0]);
  close (krad_transponder_input_port->socketpair[1]);
}

void krad_transponder_input_port_free (krad_transponder_input_port_t *krad_transponder_input_port) {
  krad_ringbuffer_free ( krad_transponder_input_port->msg_ring );
  free (krad_transponder_input_port);
}

void krad_transponder_input_port_destroy (krad_transponder_input_port_t *krad_transponder_input_port) {
  krad_transponder_input_port_disconnect (krad_transponder_input_port);
  krad_transponder_input_port_free (krad_transponder_input_port);
  printf ("Krad Transponder: input port destroyed\n");
}

void krad_transponder_output_port_free (krad_transponder_output_port_t *krad_transponder_output_port) {
  free (krad_transponder_output_port->connections);
  free (krad_transponder_output_port);
}

void krad_transponder_output_port_destroy (krad_transponder_output_port_t *krad_transponder_output_port) {

  //krad_transponder_port_disconnect (krad_transponder_port);
  krad_transponder_output_port_free (krad_transponder_output_port);
  printf ("Krad Transponder: output port destroyed\n");
}

krad_transponder_input_port_t *krad_transponder_input_port_create () {

  krad_transponder_input_port_t *krad_transponder_input_port = calloc (1, sizeof(krad_transponder_input_port_t));

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, krad_transponder_input_port->socketpair)) {
    printf ("Krad Transponder: subunit could not create socketpair errno: %d\n", errno);
    free (krad_transponder_input_port);
    return NULL;
  }

  krad_transponder_input_port->msg_ring =
  krad_ringbuffer_create ( 200 * sizeof(krad_codec_slice_t *) );

  printf ("Krad Transponder: input port created\n");

  return krad_transponder_input_port;
}

krad_transponder_output_port_t *krad_transponder_output_port_create () {
  krad_transponder_output_port_t *krad_transponder_output_port = calloc (1, sizeof(krad_transponder_output_port_t));
  krad_transponder_output_port->connections = calloc (KRAD_TRANSPONDER_PORT_CONNECTIONS, sizeof(krad_transponder_input_port_t *));
  printf ("Krad Transponder: output port created\n");
  return krad_transponder_output_port;
}

void krad_transponder_subunit_connect_ports_actual (krad_transponder_subunit_t *krad_transponder_subunit,
                                                    krad_transponder_output_port_t *output_port,
                                                    krad_transponder_input_port_t *input_port) {

  int p;

  for (p = 0; p < KRAD_TRANSPONDER_PORT_CONNECTIONS; p++) {
    if (output_port->connections[p] == NULL) {
      output_port->connections[p] = input_port;
      printf ("Krad Transponder: output port actually connected!\n");
      break;
    }
  }
}

void krad_transponder_subunit_disconnect_ports_actual (krad_transponder_subunit_t *krad_transponder_subunit,
                                                    krad_transponder_output_port_t *output_port,
                                                    krad_transponder_input_port_t *input_port) {

  int p;

  for (p = 0; p < KRAD_TRANSPONDER_PORT_CONNECTIONS; p++) {
    if (output_port->connections[p] == input_port) {
      output_port->connections[p] = NULL;
      
      krad_codec_slice_t *codec_slice;
      codec_slice = calloc (1, sizeof(krad_codec_slice_t));
      codec_slice->final = 1;
      krad_transponder_port_write (input_port, &codec_slice);
      
      close (input_port->socketpair[0]);
      printf ("Krad Transponder: output port actually disconnected!\n");
      break;
    }
  }
}

void krad_transponder_subunit_handle_control_msg (krad_transponder_subunit_t *krad_transponder_subunit,
                                             krad_transponder_control_msg_t *msg) {
  if (msg->type == CONNECTPORTS) {
  	printf("Krad Transponder Subunit: got CONNECTPORTS msg!\n");
  	krad_transponder_subunit_connect_ports_actual (krad_transponder_subunit, msg->output_port, msg->input_port);
    free (msg);
    return;
  }
  if (msg->type == DISCONNECTPORTS) {
  	printf("Krad Transponder Subunit: got DISCONNECTPORTS msg!\n");
  	krad_transponder_subunit_disconnect_ports_actual (krad_transponder_subunit, msg->output_port, msg->input_port);  	
    free (msg);
    return;
  }
  if (msg->type == UPDATE) {
  	printf("Krad Transponder Subunit: got UPDATE msg!\n");
    free (msg);
    return;
  }

  printf("Krad Transponder Subunit: got some other message!\n");
  
}

int krad_transponder_subunit_poll (krad_transponder_subunit_t *krad_transponder_subunit) {

	int n;
  int nfds;
  int ret;
	struct pollfd pollfds[4];
	
	n = 0;

  pollfds[n].fd = krad_transponder_subunit->control->socketpair[1];
  pollfds[n].events = POLLIN;
  n++;
  
  if (krad_transponder_subunit->type == MUXER) {
    if (krad_transponder_subunit->inputs[0]->connected_to_subunit != NULL) {
      pollfds[n].fd = krad_transponder_subunit->inputs[0]->socketpair[1];
      pollfds[n].events = POLLIN;
      n++;
    }
    if (krad_transponder_subunit->inputs[1]->connected_to_subunit != NULL) {
      pollfds[n].fd = krad_transponder_subunit->inputs[1]->socketpair[1];
      pollfds[n].events = POLLIN;
      n++;
    }
  }
  
  if (krad_transponder_subunit->type == DECODER) {
    if (krad_transponder_subunit->inputs[0]->connected_to_subunit != NULL) {
      pollfds[n].fd = krad_transponder_subunit->inputs[0]->socketpair[1];
      pollfds[n].events = POLLIN;
      n++;
    }
  }

  nfds = n;

	ret = poll (pollfds, nfds, 1000);
	if (ret < 0) {
    // error
		return 0;
	}
	
	if (ret == 0) {
    // yawn
		return 1;
	}	

	if (ret > 0) {
		for (n = 0; n < nfds; n++) {
			if (pollfds[n].revents) {
			  if (pollfds[n].revents & POLLERR) {
				  printf("Krad Transponder Subunit: Err we got Err in Hrr!\n");
          if (pollfds[n].fd == krad_transponder_subunit->control->socketpair[1]) {
			      printf("Krad Transponder Subunit: Err on control socket!\n");
          }
          return 0;
			  }

			  if (pollfds[n].revents & POLLIN) {

          if (pollfds[n].fd == krad_transponder_subunit->control->socketpair[1]) {
            krad_transponder_control_msg_t *msg;
            msg = NULL;
				    krad_transponder_port_read (krad_transponder_subunit->control, (void **)&msg);
				    if (msg->type == DESTROY) {
				    	printf("Krad Transponder Subunit: Got Destroy MSG!\n");
				      free (msg);
              if ((krad_transponder_subunit->type == MUXER) || (krad_transponder_subunit->type == DECODER)) {
                krad_transponder_subunit->destroy = 1;
                if (krad_transponder_subunit->inputs[0]->connected_to_subunit != NULL) {
                  krad_transponder_subunit->destroy++;
				          krad_transponder_port_disconnect (krad_transponder_subunit->inputs[0]->connected_to_subunit,
				                                            krad_transponder_subunit->inputs[0]->connected_to_subunit->outputs[0],
				                                            krad_transponder_subunit->inputs[0]);
                }
                if (krad_transponder_subunit->inputs[1]->connected_to_subunit != NULL) {
                  krad_transponder_subunit->destroy++;
				          krad_transponder_port_disconnect (krad_transponder_subunit->inputs[1]->connected_to_subunit,
				                                            krad_transponder_subunit->inputs[1]->connected_to_subunit->outputs[1],
				                                            krad_transponder_subunit->inputs[1]);
                }
				      }
              if ((krad_transponder_subunit->type == ENCODER) || (krad_transponder_subunit->type == DEMUXER)) {
				        return 0;
              }

				    } else {
				      krad_transponder_subunit_handle_control_msg (krad_transponder_subunit, msg);
				    }
          }

          if (krad_transponder_subunit->type == MUXER) {
            krad_codec_slice_t *codec_slice;
            codec_slice = NULL;
            if (pollfds[n].fd == krad_transponder_subunit->inputs[0]->socketpair[1]) {
				      krad_transponder_port_read (krad_transponder_subunit->inputs[0], (void **)&codec_slice);
            }
            if (pollfds[n].fd == krad_transponder_subunit->inputs[1]->socketpair[1]) {
				      krad_transponder_port_read (krad_transponder_subunit->inputs[1], (void **)&codec_slice);
            }
            if (codec_slice != NULL) {
              printf ("Krad Transponder Subunit: Got a packet!\n");
              
              if (codec_slice->final == 1) {
                printf ("Krad Transponder Subunit: Got FINAL packet!\n");
              }
              
              krad_codec_slice_unref (codec_slice);
            }
          }
			  }

			  if (pollfds[n].revents & POLLOUT) {
				  // out on something
			  }
			  
			  if (pollfds[n].revents & POLLHUP) {
				  printf ("Krad Transponder Subunit: Got Hangup\n");
          if (pollfds[n].fd == krad_transponder_subunit->control->socketpair[1]) {
			      printf("Err! Hangup on control socket!\n");
            return 0;
          }
          if (krad_transponder_subunit->type == MUXER) {
            if (pollfds[n].fd == krad_transponder_subunit->inputs[0]->socketpair[1]) {
			        printf ("Krad Transponder Subunit: Encoded Video Input Disconnected\n");
			        krad_transponder_subunit->inputs[0]->connected_to_subunit = NULL;
			        if (krad_transponder_subunit->destroy > 1) {
			          krad_transponder_subunit->destroy--;
			        }
            }
            if (pollfds[n].fd == krad_transponder_subunit->inputs[1]->socketpair[1]) {
			        printf ("Krad Transponder Subunit: Encoded Audio Input Disconnected\n");
			        krad_transponder_subunit->inputs[1]->connected_to_subunit = NULL;
			        if (krad_transponder_subunit->destroy > 1) {
			          krad_transponder_subunit->destroy--;
			        }
            }
          }
				}
			  
      }
		}
	}
	
	return 1;
	
}

void *krad_transponder_subunit_thread (void *arg) {

	krad_transponder_subunit_t *krad_transponder_subunit = (krad_transponder_subunit_t *)arg;

	krad_system_set_thread_name ("kr_txp_su");

	while (krad_transponder_subunit_poll (krad_transponder_subunit)) {
	
	  if (krad_transponder_subunit->destroy == 1) {
	    break;
	  }
	
    if (krad_transponder_subunit->type == ENCODER) {
      krad_codec_slice_t *codec_slice;
      codec_slice = calloc (1, sizeof(krad_codec_slice_t));
      krad_transponder_port_broadcast (krad_transponder_subunit->outputs[0], &codec_slice);
    }
    if (krad_transponder_subunit->type == DEMUXER) {
      krad_codec_slice_t *codec_slice;
      codec_slice = calloc (1, sizeof(krad_codec_slice_t));    
      krad_transponder_port_broadcast (krad_transponder_subunit->outputs[0], &codec_slice);
    }    
	}

	return NULL;

}

void krad_transponder_subunit_send_destroy_msg (krad_transponder_subunit_t *krad_transponder_subunit) {
  krad_transponder_control_msg_t *msg;
  msg = calloc (1, sizeof (krad_transponder_control_msg_t));
  msg->type = DESTROY;
  krad_transponder_port_write (krad_transponder_subunit->control, &msg);
}

void krad_transponder_subunit_start (krad_transponder_subunit_t *krad_transponder_subunit) {
  krad_transponder_subunit->control = krad_transponder_input_port_create ();
	pthread_create (&krad_transponder_subunit->thread, NULL, krad_transponder_subunit_thread, (void *)krad_transponder_subunit);
}


void krad_transponder_subunit_stop (krad_transponder_subunit_t *krad_transponder_subunit) {

  int p;
  int m;

  if ((krad_transponder_subunit->type == ENCODER) || (krad_transponder_subunit->type == DEMUXER)) {
    for (p = 0; p < KRAD_TRANSPONDER_PORT_CONNECTIONS; p++) {
      if (krad_transponder_subunit->outputs[0] != NULL) {
        if (krad_transponder_subunit->outputs[0]->connections[p] != NULL) {
          for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
            if (krad_transponder_subunit->kradx_transponder->subunits[m] == krad_transponder_subunit->outputs[0]->connections[p]->subunit) {
              krad_transponder_subunit_destroy (&krad_transponder_subunit->kradx_transponder->subunits[m]);
              break;
            }
          }
        }
      }
      if (krad_transponder_subunit->outputs[1] != NULL) {
        if (krad_transponder_subunit->outputs[1]->connections[p] != NULL) {
          for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
            if (krad_transponder_subunit->kradx_transponder->subunits[m] == krad_transponder_subunit->outputs[1]->connections[p]->subunit) {
              krad_transponder_subunit_destroy (&krad_transponder_subunit->kradx_transponder->subunits[m]);
              break;
            }
          }
        }
      }
    }
  }


  krad_transponder_subunit_send_destroy_msg (krad_transponder_subunit);
	pthread_join (krad_transponder_subunit->thread, NULL);
  krad_transponder_input_port_destroy (krad_transponder_subunit->control);	
}

void krad_transponder_subunit_destroy (krad_transponder_subunit_t **krad_transponder_subunit) {

  int p;
  
  if (*krad_transponder_subunit != NULL) {
  
    krad_transponder_subunit_stop (*krad_transponder_subunit);
    
    // maybe this happens elsewere
    for (p = 0; p < 2; p++) {
      if ((*krad_transponder_subunit)->inputs[p] != NULL) {
        krad_transponder_input_port_destroy ((*krad_transponder_subunit)->inputs[p]);
      }
      if ((*krad_transponder_subunit)->outputs[p] != NULL) {
        krad_transponder_output_port_destroy ((*krad_transponder_subunit)->outputs[p]);
      }
    }  
    
    printf ("Krad Transponder: %s subunit destroyed\n",
            transponder_subunit_type_to_string((*krad_transponder_subunit)->type));

    free ((*krad_transponder_subunit)->inputs);
    free ((*krad_transponder_subunit)->outputs);
    free (*krad_transponder_subunit);
    *krad_transponder_subunit = NULL;
  }
}

krad_transponder_subunit_t *krad_transponder_subunit_create (kradx_transponder_t *kradx_transponder, krad_transponder_subunit_type_t type) {

  int p;
  krad_transponder_subunit_t *krad_transponder_subunit = calloc (1, sizeof(krad_transponder_subunit_t));
  krad_transponder_subunit->inputs = calloc (2, sizeof(krad_transponder_input_port_t *));
  krad_transponder_subunit->outputs = calloc (2, sizeof(krad_transponder_output_port_t *));
  krad_transponder_subunit->kradx_transponder = kradx_transponder;
  krad_transponder_subunit->type = type;

  printf ("Krad Transponder: creating %s subunit\n",
          transponder_subunit_type_to_string(krad_transponder_subunit->type));

  switch (type) {
    case DEMUXER:
      for (p = 0; p < 2; p++) {
        krad_transponder_subunit->outputs[p] = krad_transponder_output_port_create ();
      }
      break;
    case MUXER:
      for (p = 0; p < 2; p++) {
        krad_transponder_subunit->inputs[p] = krad_transponder_input_port_create ();
        //krad_transponder_subunit->outputs[p] = krad_transponder_port_create ();    
      }
      break;
    case DECODER:
      krad_transponder_subunit->inputs[0] = krad_transponder_input_port_create ();
      break;      
    case ENCODER:
      krad_transponder_subunit->outputs[0] = krad_transponder_output_port_create ();
      break;      
  }

  krad_transponder_subunit_start (krad_transponder_subunit);
  printf ("Krad Transponder: %s subunit created\n",
          transponder_subunit_type_to_string(krad_transponder_subunit->type));

  return krad_transponder_subunit;
}

int kradx_transponder_subunit_add (kradx_transponder_t *kradx_transponder, krad_transponder_subunit_type_t type) {

  int m;

  for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
    if (kradx_transponder->subunits[m] == NULL) {
      kradx_transponder->subunits[m] = krad_transponder_subunit_create (kradx_transponder, type);
      return m;
    }
  }
  
  return -1;
  
}

/* Public API */

void kradx_transponder_subunit_remove (kradx_transponder_t *kradx_transponder, int s) {
  if ((s > -1) && (s < KRAD_TRANSPONDER_SUBUNITS)) {
    if (kradx_transponder->subunits[s] != NULL) {
      printf ("Krad Transponder: removing subunit %d\n", s);
      krad_transponder_subunit_destroy (&kradx_transponder->subunits[s]);    
    }
  }
}

int kradx_transponder_add_encoder (kradx_transponder_t *kradx_transponder, krad_codec_t codec) {
  return kradx_transponder_subunit_add (kradx_transponder, ENCODER);;
}

int kradx_transponder_add_muxer (kradx_transponder_t *kradx_transponder, int videoport, int audioport) {
  
  int m;

  m = kradx_transponder_subunit_add (kradx_transponder, MUXER);
  
  if (m > -1) {
    
    if (audioport > -1) {
      krad_transponder_port_connect (kradx_transponder->subunits[audioport],
                                     kradx_transponder->subunits[m],
                                     kradx_transponder->subunits[audioport]->outputs[0], 
                                     kradx_transponder->subunits[m]->inputs[0]);
    }
    
    if (videoport > -1) {
      krad_transponder_port_connect (kradx_transponder->subunits[videoport],
                                     kradx_transponder->subunits[m],
                                     kradx_transponder->subunits[videoport]->outputs[0], 
                                     kradx_transponder->subunits[m]->inputs[1]);
    }
    
    return m;
    
  }
  
  return -1;
  
}

void kradx_transponder_list (kradx_transponder_t *kradx_transponder) {
  
  int m;

  printf ("Krad Transponder: Listing Subunits\n");

  for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
    if (kradx_transponder->subunits[m] != NULL) {
      printf ("Krad Transponder: Subunit %d - %s\n",
              m,
              transponder_subunit_type_to_string(kradx_transponder->subunits[m]->type));      
    }
  }

  printf ("\n");

}

void kradx_transponder_destroy (kradx_transponder_t **kradx_transponder) {

  int m;
  
  if (*kradx_transponder != NULL) {

    printf ("Krad Transponder: Destroying\n");

    for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
      if ((*kradx_transponder)->subunits[m] != NULL) {
        krad_transponder_subunit_destroy (&(*kradx_transponder)->subunits[m]);
      }
    }

    free ((*kradx_transponder)->subunits);
    free (*kradx_transponder);
    *kradx_transponder = NULL;
    
    printf ("Krad Transponder: Destroyed\n");
    
  }
  
}

kradx_transponder_t *kradx_transponder_create (krad_radio_t *krad_radio) {

	kradx_transponder_t *kradx_transponder;
  
  printf ("Krad Transponder: Creating\n");
  
	kradx_transponder = calloc(1, sizeof(kradx_transponder_t));
	kradx_transponder->krad_radio = krad_radio;
  kradx_transponder->subunits = calloc(KRAD_TRANSPONDER_SUBUNITS, sizeof(krad_transponder_subunit_t *));

  printf ("Krad Transponder: Created\n");

	return kradx_transponder;

}


int main (int argc, char *argv[]) {

  krad_radio_t *krad_radio;
  kradx_transponder_t *kradx_transponder;  
  int t;
  int s;

  krad_radio = NULL;
  kradx_transponder = NULL;

  //freopen("/dev/null", "w", stdout);

  for (t = 0; t < 1; t++) {
    kradx_transponder = kradx_transponder_create (krad_radio);  
    kradx_transponder_add_encoder (kradx_transponder, FLAC);
    s = kradx_transponder_add_muxer (kradx_transponder, -1, 0);
    usleep (500000);
    kradx_transponder_list (kradx_transponder);  
    usleep (500000);
    kradx_transponder_add_muxer (kradx_transponder, -1, 0);
    usleep (500000);
    kradx_transponder_list (kradx_transponder);  
    usleep (500000);    
    kradx_transponder_subunit_remove (kradx_transponder, s);
    usleep (500000);
    kradx_transponder_list (kradx_transponder);  
    usleep (500000);    
    kradx_transponder_destroy (&kradx_transponder);
  }

  return 0;

}
