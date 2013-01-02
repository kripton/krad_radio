#include "krad_transponder_graph.h"

/* Prototypes */

void krad_Xtransponder_subunit_destroy (krad_Xtransponder_subunit_t **krad_Xtransponder_subunit);

/* Internal */

char *transponder_subunit_type_to_string (krad_Xtransponder_subunit_type_t type) {

  switch (type) {
    case DEMUXER:
      return "Demuxer";
    case MUXER:
      return "Muxer";
    case DECODER:
      return "Decoder";
    case ENCODER:
      return "Encoder";
    case XCAPTURE:
      return "Capture";
  }
  
  return "Unknown Subunit";
}


void krad_Xcodec_slice_unref (krad_Xcodec_slice_t *krad_Xcodec_slice) {
  __sync_fetch_and_sub( &krad_Xcodec_slice->refs, 1 );
}

void krad_Xtransponder_port_read (krad_Xtransponder_input_port_t *inport, void *msg) {

  int ret;
  char buffer[1];

  ret = read (inport->socketpair[1], buffer, 1);
  if (ret != 1) {
    if (ret == 0) {
      printk ("Krad Transponder: port read got EOF");
      return;
    }
    printk ("Krad Transponder: port read unexpected read return value %d", ret);
  }

  if (msg != NULL) {
    printk ("Krad Transponder: msg read space %zu", krad_ringbuffer_read_space (inport->msg_ring));  
    ret = krad_ringbuffer_read (inport->msg_ring, (char *)msg, sizeof(void *));
    if (ret != sizeof(void *)) {
      printk ("Krad Transponder: invalid msg read len %d", ret);
    }    
  } else {
    printk ("uh oh nullzor!");
  }
  printk ("Krad Transponder: input port read");

}

void krad_Xtransponder_port_write (krad_Xtransponder_input_port_t *input, void *msg) {

  int wrote;

  if (input != NULL) {
    krad_ringbuffer_write (input->msg_ring, (char *)msg, sizeof(void *));
    wrote = write (input->socketpair[0], "0", 1);
    if (wrote != 1) {
      printk ("Krad Transponder: port write unexpected write return value %d", wrote);
    }
  }

  printk ("Krad Transponder: port write");

}

void krad_Xtransponder_port_broadcast (krad_Xtransponder_output_port_t *outport, void *msg) {

  int p;
  
  printk ("Krad Transponder: output port broadcasting");  
  
  for (p = 0; p < KRAD_TRANSPONDER_PORT_CONNECTIONS; p++) {
    if (outport->connections[p] != NULL) {
      krad_Xtransponder_port_write (outport->connections[p], msg);
    }
  }

  printk ("Krad Transponder: output port broadcasted");

}

void krad_Xtransponder_port_disconnect (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                    krad_Xtransponder_output_port_t *output,
                                    krad_Xtransponder_input_port_t *input) {

  printk ("Krad Transponder: sending disconnect ports msg");

  krad_Xtransponder_control_msg_t *msg;
  msg = calloc (1, sizeof (krad_Xtransponder_control_msg_t));
  msg->type = DISCONNECTPORTS;
  msg->input_port = input;
  msg->output_port = output;
  krad_Xtransponder_port_write (krad_Xtransponder_subunit->control, &msg);

  printk ("Krad Transponder: sent disconnect ports msg");
}

void krad_Xtransponder_port_connect (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                    krad_Xtransponder_subunit_t *from_krad_Xtransponder_subunit,
                                    krad_Xtransponder_output_port_t *output,
                                    krad_Xtransponder_input_port_t *input) {

  printk ("Krad Transponder: sending connecting ports msg");

  krad_Xtransponder_control_msg_t *msg;
  msg = calloc (1, sizeof (krad_Xtransponder_control_msg_t));
  msg->type = CONNECTPORTS;
  msg->input_port = input;
  msg->output_port = output;
  
  input->connected_to_subunit = krad_Xtransponder_subunit;
  input->subunit = from_krad_Xtransponder_subunit;
  krad_Xtransponder_port_write (krad_Xtransponder_subunit->control, &msg);

  printk ("Krad Transponder: sent connecting ports msg");
}

void krad_Xtransponder_input_port_disconnect (krad_Xtransponder_input_port_t *krad_Xtransponder_input_port) {
  printk ("Krad Transponder: disconnecting ports");
	//close (krad_Xtransponder_input_port->socketpair[0]);
  close (krad_Xtransponder_input_port->socketpair[1]);
}

void krad_Xtransponder_input_port_free (krad_Xtransponder_input_port_t *krad_Xtransponder_input_port) {
  krad_ringbuffer_free ( krad_Xtransponder_input_port->msg_ring );
  free (krad_Xtransponder_input_port);
}

void krad_Xtransponder_input_port_destroy (krad_Xtransponder_input_port_t *krad_Xtransponder_input_port) {
  krad_Xtransponder_input_port_disconnect (krad_Xtransponder_input_port);
  krad_Xtransponder_input_port_free (krad_Xtransponder_input_port);
  printk ("Krad Transponder: input port destroyed");
}

void krad_Xtransponder_output_port_free (krad_Xtransponder_output_port_t *krad_Xtransponder_output_port) {
  free (krad_Xtransponder_output_port->connections);
  free (krad_Xtransponder_output_port);
}

void krad_Xtransponder_output_port_destroy (krad_Xtransponder_output_port_t *krad_Xtransponder_output_port) {

  //krad_Xtransponder_port_disconnect (krad_Xtransponder_port);
  krad_Xtransponder_output_port_free (krad_Xtransponder_output_port);
  printk ("Krad Transponder: output port destroyed");
}

krad_Xtransponder_input_port_t *krad_Xtransponder_input_port_create () {

  krad_Xtransponder_input_port_t *krad_Xtransponder_input_port = calloc (1, sizeof(krad_Xtransponder_input_port_t));

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, krad_Xtransponder_input_port->socketpair)) {
    printk ("Krad Transponder: subunit could not create socketpair errno: %d", errno);
    free (krad_Xtransponder_input_port);
    return NULL;
  }

  krad_Xtransponder_input_port->msg_ring =
  krad_ringbuffer_create ( 200 * sizeof(krad_Xcodec_slice_t *) );

  printk ("Krad Transponder: input port created");

  return krad_Xtransponder_input_port;
}

krad_Xtransponder_output_port_t *krad_Xtransponder_output_port_create () {
  krad_Xtransponder_output_port_t *krad_Xtransponder_output_port = calloc (1, sizeof(krad_Xtransponder_output_port_t));
  krad_Xtransponder_output_port->connections = calloc (KRAD_TRANSPONDER_PORT_CONNECTIONS, sizeof(krad_Xtransponder_input_port_t *));
  printk ("Krad Transponder: output port created");
  return krad_Xtransponder_output_port;
}

void krad_Xtransponder_subunit_connect_ports_actual (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                                    krad_Xtransponder_output_port_t *output_port,
                                                    krad_Xtransponder_input_port_t *input_port) {

  int p;

  for (p = 0; p < KRAD_TRANSPONDER_PORT_CONNECTIONS; p++) {
    if (output_port->connections[p] == NULL) {
      output_port->connections[p] = input_port;
      printk ("Krad Transponder: output port actually connected!");
      break;
    }
  }
}

void krad_Xtransponder_subunit_disconnect_ports_actual (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                                    krad_Xtransponder_output_port_t *output_port,
                                                    krad_Xtransponder_input_port_t *input_port) {

  int p;

  for (p = 0; p < KRAD_TRANSPONDER_PORT_CONNECTIONS; p++) {
    if (output_port->connections[p] == input_port) {
      output_port->connections[p] = NULL;
      
      krad_Xcodec_slice_t *codec_slice;
      codec_slice = calloc (1, sizeof(krad_Xcodec_slice_t));
      codec_slice->final = 1;
      krad_Xtransponder_port_write (input_port, &codec_slice);
      
      close (input_port->socketpair[0]);
      printk ("Krad Transponder: output port actually disconnected!");
      break;
    }
  }
}

void krad_Xtransponder_subunit_handle_control_msg (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                             krad_Xtransponder_control_msg_t *msg) {
  if (msg->type == CONNECTPORTS) {
  	printk("Krad Transponder Subunit: got CONNECTPORTS msg!");
  	krad_Xtransponder_subunit_connect_ports_actual (krad_Xtransponder_subunit, msg->output_port, msg->input_port);
    free (msg);
    return;
  }
  if (msg->type == DISCONNECTPORTS) {
  	printk("Krad Transponder Subunit: got DISCONNECTPORTS msg!");
  	krad_Xtransponder_subunit_disconnect_ports_actual (krad_Xtransponder_subunit, msg->output_port, msg->input_port);  	
    free (msg);
    return;
  }
  if (msg->type == UPDATE) {
  	printk("Krad Transponder Subunit: got UPDATE msg!");
    free (msg);
    return;
  }

  printk("Krad Transponder Subunit: got some other message!");
  
}

int krad_Xtransponder_subunit_poll (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit) {

	int n;
  int nfds;
  int ret;
  int timeout;
	struct pollfd pollfds[4];
	
	timeout = -1;
	n = 0;

  pollfds[n].fd = krad_Xtransponder_subunit->control->socketpair[1];
  pollfds[n].events = POLLIN;
  n++;
  
  if (krad_Xtransponder_subunit->type == MUXER) {
    if (krad_Xtransponder_subunit->inputs[0]->connected_to_subunit != NULL) {
      pollfds[n].fd = krad_Xtransponder_subunit->inputs[0]->socketpair[1];
      pollfds[n].events = POLLIN;
      n++;
    }
    if (krad_Xtransponder_subunit->inputs[1]->connected_to_subunit != NULL) {
      pollfds[n].fd = krad_Xtransponder_subunit->inputs[1]->socketpair[1];
      pollfds[n].events = POLLIN;
      n++;
    }
  }
  
  if (krad_Xtransponder_subunit->type == DECODER) {
    if (krad_Xtransponder_subunit->inputs[0]->connected_to_subunit != NULL) {
      pollfds[n].fd = krad_Xtransponder_subunit->inputs[0]->socketpair[1];
      pollfds[n].events = POLLIN;
      n++;
    }
  }
  
  if (krad_Xtransponder_subunit->watch != NULL) {
    if (krad_Xtransponder_subunit->watch->fd > 0) {
      pollfds[n].fd = krad_Xtransponder_subunit->watch->fd;
      pollfds[n].events = POLLIN;
      n++;
    } else {
      if (krad_Xtransponder_subunit->watch->idle_callback_interval > 0) {
        timeout = krad_Xtransponder_subunit->watch->idle_callback_interval;
      }
    }
  }
  

  nfds = n;

	ret = poll (pollfds, nfds, timeout);
	if (ret < 0) {
    // error
		return 0;
	}
	
	if (ret == 0) {
    if (krad_Xtransponder_subunit->watch->idle_callback_interval > 0) {
      krad_Xtransponder_subunit->watch->readable_callback (krad_Xtransponder_subunit->watch->callback_pointer);
    }
		return 1;
	}	

	if (ret > 0) {
		for (n = 0; n < nfds; n++) {
			if (pollfds[n].revents) {
			  if (pollfds[n].revents & POLLERR) {
				  printk("Krad Transponder Subunit: Err we got Err in Hrr!");
          if (pollfds[n].fd == krad_Xtransponder_subunit->control->socketpair[1]) {
			      printk("Krad Transponder Subunit: Err on control socket!");
          }
          return 0;
			  }

			  if (pollfds[n].revents & POLLIN) {
			  
			    if ((krad_Xtransponder_subunit->watch != NULL) && 
			        (krad_Xtransponder_subunit->watch->fd > 0) &&
			        (pollfds[n].fd == krad_Xtransponder_subunit->watch->fd)) {
            krad_Xtransponder_subunit->watch->readable_callback (krad_Xtransponder_subunit->watch->callback_pointer);			    
			    }

          if (pollfds[n].fd == krad_Xtransponder_subunit->control->socketpair[1]) {
            krad_Xtransponder_control_msg_t *msg;
            msg = NULL;
				    krad_Xtransponder_port_read (krad_Xtransponder_subunit->control, (void **)&msg);
				    if (msg->type == DESTROY) {
				    	printk("Krad Transponder: Subunit Got Destroy MSG!");
				      free (msg);
              krad_Xtransponder_subunit->destroy = 1;
              if ((krad_Xtransponder_subunit->type == MUXER) || (krad_Xtransponder_subunit->type == DECODER)) {
                if (krad_Xtransponder_subunit->inputs[0]->connected_to_subunit != NULL) {
                  krad_Xtransponder_subunit->destroy++;
				          krad_Xtransponder_port_disconnect (krad_Xtransponder_subunit->inputs[0]->connected_to_subunit,
				                                            krad_Xtransponder_subunit->inputs[0]->connected_to_subunit->outputs[0],
				                                            krad_Xtransponder_subunit->inputs[0]);
                }
                if (krad_Xtransponder_subunit->type == MUXER) {
                  if (krad_Xtransponder_subunit->inputs[1]->connected_to_subunit != NULL) {
                    krad_Xtransponder_subunit->destroy++;
				            krad_Xtransponder_port_disconnect (krad_Xtransponder_subunit->inputs[1]->connected_to_subunit,
				                                              krad_Xtransponder_subunit->inputs[1]->connected_to_subunit->outputs[1],
				                                              krad_Xtransponder_subunit->inputs[1]);
                  }
                }
				      }
              if ((krad_Xtransponder_subunit->type == ENCODER) ||
                  (krad_Xtransponder_subunit->type == DEMUXER) ||
                  (krad_Xtransponder_subunit->type == XCAPTURE)) {
				        return 0;
              }

				    } else {
				      krad_Xtransponder_subunit_handle_control_msg (krad_Xtransponder_subunit, msg);
				    }
          }

          if (krad_Xtransponder_subunit->type == MUXER) {
            krad_Xcodec_slice_t *codec_slice;
            codec_slice = NULL;
            if (pollfds[n].fd == krad_Xtransponder_subunit->inputs[0]->socketpair[1]) {
				      krad_Xtransponder_port_read (krad_Xtransponder_subunit->inputs[0], (void **)&codec_slice);
            }
            if (pollfds[n].fd == krad_Xtransponder_subunit->inputs[1]->socketpair[1]) {
				      krad_Xtransponder_port_read (krad_Xtransponder_subunit->inputs[1], (void **)&codec_slice);
            }
            if (codec_slice != NULL) {
              printk ("Krad Transponder Subunit: Got a packet!");
              
              if (codec_slice->final == 1) {
                printk ("Krad Transponder Subunit: Got FINAL packet!");
              }
              
              krad_Xcodec_slice_unref (codec_slice);
            }
          }
			  }

			  if (pollfds[n].revents & POLLOUT) {
				  // out on something
			  }
			  
			  if (pollfds[n].revents & POLLHUP) {
				  printk ("Krad Transponder Subunit: Got Hangup");
          if (pollfds[n].fd == krad_Xtransponder_subunit->control->socketpair[1]) {
			      printk("Err! Hangup on control socket!");
            return 0;
          }
          if (krad_Xtransponder_subunit->type == MUXER) {
            if (pollfds[n].fd == krad_Xtransponder_subunit->inputs[0]->socketpair[1]) {
			        printk ("Krad Transponder Subunit: Encoded Video Input Disconnected");
			        krad_Xtransponder_subunit->inputs[0]->connected_to_subunit = NULL;
			        if (krad_Xtransponder_subunit->destroy > 1) {
			          krad_Xtransponder_subunit->destroy--;
			        }
            }
            if (pollfds[n].fd == krad_Xtransponder_subunit->inputs[1]->socketpair[1]) {
			        printk ("Krad Transponder Subunit: Encoded Audio Input Disconnected");
			        krad_Xtransponder_subunit->inputs[1]->connected_to_subunit = NULL;
			        if (krad_Xtransponder_subunit->destroy > 1) {
			          krad_Xtransponder_subunit->destroy--;
			        }
            }
          }
				}
			  
      }
		}
	}
	return 1;
}

void *krad_Xtransponder_subunit_thread (void *arg) {

	krad_Xtransponder_subunit_t *krad_Xtransponder_subunit = (krad_Xtransponder_subunit_t *)arg;

	krad_system_set_thread_name ("kr_txp_su");

	while (krad_Xtransponder_subunit_poll (krad_Xtransponder_subunit)) {
	
	  if (krad_Xtransponder_subunit->destroy == 1) {
	    break;
	  }
	
    //if (krad_Xtransponder_subunit->type == ENCODER) {
    //  krad_Xcodec_slice_t *codec_slice;
    //  codec_slice = calloc (1, sizeof(krad_Xcodec_slice_t));
    //  krad_Xtransponder_port_broadcast (krad_Xtransponder_subunit->outputs[0], &codec_slice);
    //}
    //if (krad_Xtransponder_subunit->type == DEMUXER) {
    //  krad_Xcodec_slice_t *codec_slice;
    //  codec_slice = calloc (1, sizeof(krad_Xcodec_slice_t));    
    //  krad_Xtransponder_port_broadcast (krad_Xtransponder_subunit->outputs[0], &codec_slice);
    //}    
	}

	return NULL;

}

void krad_Xtransponder_subunit_send_destroy_msg (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit) {
  krad_Xtransponder_control_msg_t *msg;
  msg = calloc (1, sizeof (krad_Xtransponder_control_msg_t));
  msg->type = DESTROY;
  krad_Xtransponder_port_write (krad_Xtransponder_subunit->control, &msg);
}

void krad_Xtransponder_subunit_start (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit) {
  krad_Xtransponder_subunit->control = krad_Xtransponder_input_port_create ();
	pthread_create (&krad_Xtransponder_subunit->thread, NULL, krad_Xtransponder_subunit_thread, (void *)krad_Xtransponder_subunit);
}

void krad_Xtransponder_subunit_stop (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit) {

  int p;
  int m;

  // Recursive Dep Destroy

  if ((krad_Xtransponder_subunit->type == ENCODER) || (krad_Xtransponder_subunit->type == DEMUXER)) {
    for (p = 0; p < KRAD_TRANSPONDER_PORT_CONNECTIONS; p++) {
      if (krad_Xtransponder_subunit->outputs[0] != NULL) {
        if (krad_Xtransponder_subunit->outputs[0]->connections[p] != NULL) {
          for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
            if (krad_Xtransponder_subunit->krad_Xtransponder->subunits[m] == krad_Xtransponder_subunit->outputs[0]->connections[p]->subunit) {
              krad_Xtransponder_subunit_destroy (&krad_Xtransponder_subunit->krad_Xtransponder->subunits[m]);
              break;
            }
          }
        }
      }
      if (krad_Xtransponder_subunit->outputs[1] != NULL) {
        if (krad_Xtransponder_subunit->outputs[1]->connections[p] != NULL) {
          for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
            if (krad_Xtransponder_subunit->krad_Xtransponder->subunits[m] == krad_Xtransponder_subunit->outputs[1]->connections[p]->subunit) {
              krad_Xtransponder_subunit_destroy (&krad_Xtransponder_subunit->krad_Xtransponder->subunits[m]);
              break;
            }
          }
        }
      }
    }
  }


  krad_Xtransponder_subunit_send_destroy_msg (krad_Xtransponder_subunit);
	pthread_join (krad_Xtransponder_subunit->thread, NULL);
  krad_Xtransponder_input_port_destroy (krad_Xtransponder_subunit->control);	
}

void krad_Xtransponder_subunit_destroy (krad_Xtransponder_subunit_t **krad_Xtransponder_subunit) {

  int p;
  
  if (*krad_Xtransponder_subunit != NULL) {
  
    krad_Xtransponder_subunit_stop (*krad_Xtransponder_subunit);
    
    // maybe this happens elsewere
    for (p = 0; p < 2; p++) {
      if ((*krad_Xtransponder_subunit)->inputs[p] != NULL) {
        krad_Xtransponder_input_port_destroy ((*krad_Xtransponder_subunit)->inputs[p]);
      }
      if ((*krad_Xtransponder_subunit)->outputs[p] != NULL) {
        krad_Xtransponder_output_port_destroy ((*krad_Xtransponder_subunit)->outputs[p]);
      }
    }  
    
    printk ("Krad Transponder: %s subunit destroyed",
            transponder_subunit_type_to_string((*krad_Xtransponder_subunit)->type));

    free ((*krad_Xtransponder_subunit)->inputs);
    free ((*krad_Xtransponder_subunit)->outputs);
    if ((*krad_Xtransponder_subunit)->watch != NULL) {
      free ((*krad_Xtransponder_subunit)->watch);
    }
    
    free (*krad_Xtransponder_subunit);
    *krad_Xtransponder_subunit = NULL;
  }
}

krad_Xtransponder_subunit_t *krad_Xtransponder_subunit_create (krad_Xtransponder_t *krad_Xtransponder, krad_Xtransponder_subunit_type_t type, krad_transponder_watch_t *watch) {

  int p;
  krad_Xtransponder_subunit_t *krad_Xtransponder_subunit = calloc (1, sizeof(krad_Xtransponder_subunit_t));
  krad_Xtransponder_subunit->inputs = calloc (2, sizeof(krad_Xtransponder_input_port_t *));
  krad_Xtransponder_subunit->outputs = calloc (2, sizeof(krad_Xtransponder_output_port_t *));
  krad_Xtransponder_subunit->krad_Xtransponder = krad_Xtransponder;
  krad_Xtransponder_subunit->type = type;

  printk ("Krad Transponder: creating %s subunit",
          transponder_subunit_type_to_string(krad_Xtransponder_subunit->type));

  switch (type) {
    case DEMUXER:
      for (p = 0; p < 2; p++) {
        krad_Xtransponder_subunit->outputs[p] = krad_Xtransponder_output_port_create ();
      }
      break;
    case MUXER:
      for (p = 0; p < 2; p++) {
        krad_Xtransponder_subunit->inputs[p] = krad_Xtransponder_input_port_create ();
        //krad_Xtransponder_subunit->outputs[p] = krad_Xtransponder_port_create ();    
      }
      break;
    case DECODER:
      krad_Xtransponder_subunit->inputs[0] = krad_Xtransponder_input_port_create ();
      break;      
    case ENCODER:
      krad_Xtransponder_subunit->outputs[0] = krad_Xtransponder_output_port_create ();
      break;
    case XCAPTURE:
      break;
  }
  
  if (watch != NULL) {
    krad_Xtransponder_subunit->watch = calloc(1, sizeof(krad_transponder_watch_t));
    memcpy (krad_Xtransponder_subunit->watch, watch, sizeof(krad_transponder_watch_t));        
  }

  krad_Xtransponder_subunit_start (krad_Xtransponder_subunit);
  printk ("Krad Transponder: %s subunit created",
          transponder_subunit_type_to_string(krad_Xtransponder_subunit->type));

  return krad_Xtransponder_subunit;
}

int krad_Xtransponder_subunit_add (krad_Xtransponder_t *krad_Xtransponder, krad_Xtransponder_subunit_type_t type, krad_transponder_watch_t *watch) {

  int m;

  for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
    if (krad_Xtransponder->subunits[m] == NULL) {
      krad_Xtransponder->subunits[m] = krad_Xtransponder_subunit_create (krad_Xtransponder, type, watch);
      return m;
    }
  }
  
  return -1;
  
}

/* Public API */

void krad_Xtransponder_subunit_remove (krad_Xtransponder_t *krad_Xtransponder, int s) {
  if ((s > -1) && (s < KRAD_TRANSPONDER_SUBUNITS)) {
    if (krad_Xtransponder->subunits[s] != NULL) {
      printk ("Krad Transponder: removing subunit %d", s);
      krad_Xtransponder_subunit_destroy (&krad_Xtransponder->subunits[s]);    
    } else {
      printke ("Krad Transponder: can't remove subunit %d, not found", s);
    }
  }
}

int krad_Xtransponder_add_decoder (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch) {
  return krad_Xtransponder_subunit_add (krad_Xtransponder, DECODER, watch);
}

int krad_Xtransponder_add_encoder (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch) {
  return krad_Xtransponder_subunit_add (krad_Xtransponder, ENCODER, watch);
}

int krad_Xtransponder_add_capture (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch) {
  return krad_Xtransponder_subunit_add (krad_Xtransponder, XCAPTURE, watch);
}

int krad_Xtransponder_add_muxer (krad_Xtransponder_t *krad_Xtransponder, int videoport, int audioport) {
  
  int m;

  m = krad_Xtransponder_subunit_add (krad_Xtransponder, MUXER, NULL);
  
  if (m > -1) {
    
    if (audioport > -1) {
      krad_Xtransponder_port_connect (krad_Xtransponder->subunits[audioport],
                                     krad_Xtransponder->subunits[m],
                                     krad_Xtransponder->subunits[audioport]->outputs[0], 
                                     krad_Xtransponder->subunits[m]->inputs[0]);
    }
    
    if (videoport > -1) {
      krad_Xtransponder_port_connect (krad_Xtransponder->subunits[videoport],
                                     krad_Xtransponder->subunits[m],
                                     krad_Xtransponder->subunits[videoport]->outputs[0], 
                                     krad_Xtransponder->subunits[m]->inputs[1]);
    }
    
    return m;
    
  }
  
  return -1;
  
}

void krad_Xtransponder_list (krad_Xtransponder_t *krad_Xtransponder) {
  
  int m;

  printk ("Krad Transponder: Listing Subunits");

  for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
    if (krad_Xtransponder->subunits[m] != NULL) {
      printk ("Krad Transponder: Subunit %d - %s",
              m,
              transponder_subunit_type_to_string(krad_Xtransponder->subunits[m]->type));      
    }
  }

}

void krad_Xtransponder_destroy (krad_Xtransponder_t **krad_Xtransponder) {

  int m;
  
  if (*krad_Xtransponder != NULL) {

    printk ("Krad Transponder: Destroying");

    for (m = 0; m < KRAD_TRANSPONDER_SUBUNITS; m++) {
      if ((*krad_Xtransponder)->subunits[m] != NULL) {
        krad_Xtransponder_subunit_destroy (&(*krad_Xtransponder)->subunits[m]);
      }
    }

    free ((*krad_Xtransponder)->subunits);
    free (*krad_Xtransponder);
    *krad_Xtransponder = NULL;
    
    printk ("Krad Transponder: Destroyed");
    
  }
  
}

krad_Xtransponder_t *krad_Xtransponder_create (krad_radio_t *krad_radio) {

	krad_Xtransponder_t *krad_Xtransponder;
  
  printk ("Krad Transponder: Creating");
  
	krad_Xtransponder = calloc(1, sizeof(krad_Xtransponder_t));
	krad_Xtransponder->krad_radio = krad_radio;
  krad_Xtransponder->subunits = calloc(KRAD_TRANSPONDER_SUBUNITS, sizeof(krad_Xtransponder_subunit_t *));

  printk ("Krad Transponder: Created");

	return krad_Xtransponder;

}

/*
int main (int argc, char *argv[]) {

  krad_radio_t *krad_radio;
  krad_Xtransponder_t *krad_Xtransponder;  
  int t;
  int s;

  krad_radio = NULL;
  krad_Xtransponder = NULL;

  //freopen("/dev/null", "w", stdout);

  krad_Xtransponder = krad_Xtransponder_create (krad_radio);  

  for (t = 0; t < 10; t++) {
    krad_Xtransponder_add_encoder (krad_Xtransponder, FLAC);
    s = krad_Xtransponder_add_muxer (krad_Xtransponder, -1, 0);
    krad_Xtransponder_list (krad_Xtransponder);
    krad_Xtransponder_add_muxer (krad_Xtransponder, -1, 0);
    krad_Xtransponder_list (krad_Xtransponder);
    krad_Xtransponder_subunit_remove (krad_Xtransponder, s);
    krad_Xtransponder_list (krad_Xtransponder);
  }

  krad_Xtransponder_destroy (&krad_Xtransponder);

  return 0;

}
*/
