#include "krad_radio.h"

#ifndef KRAD_TRANSPONDER_GRAPH
#define KRAD_TRANSPONDER_GRAPH

#define KRAD_TRANSPONDER_SUBUNITS 99
#define KRAD_TRANSPONDER_PORT_CONNECTIONS 200

typedef struct krad_Xtransponder_St krad_Xtransponder_t;
typedef struct krad_Xtransponder_subunit_St krad_Xtransponder_subunit_t;
typedef struct krad_Xtransponder_input_port_St krad_Xtransponder_input_port_t;
typedef struct krad_Xtransponder_output_port_St krad_Xtransponder_output_port_t;
typedef struct krad_Xcodec_slice_St krad_Xcodec_slice_t;
typedef struct krad_Xtransponder_control_msg_St krad_Xtransponder_control_msg_t;
typedef struct krad_transponder_watch_St krad_transponder_watch_t;

typedef enum {
  CONNECTPORTS = 77999,
  DISCONNECTPORTS,
  UPDATE,
  DESTROY,
} krad_Xtransponder_control_msg_type_t;

typedef enum {
  DEMUXER = 1999,
  MUXER,
  DECODER,
  ENCODER,
  XCAPTURE,
  //OUTPUT,
  //STREAMIN
  //FILEIN
  //transmitter
  //listener
} krad_Xtransponder_subunit_type_t;

struct krad_transponder_watch_St {

	int (*readable_callback)(void *);
	void *callback_pointer;
  int fd;
	
};

struct krad_Xtransponder_control_msg_St {
  
  krad_Xtransponder_control_msg_type_t type;
  
  krad_Xtransponder_input_port_t *input_port;
  krad_Xtransponder_output_port_t *output_port;
    
};

struct krad_Xcodec_slice_St {
  unsigned char *data;
  uint32_t size;
	int refs;
	int final;
};

struct krad_Xtransponder_input_port_St {
  krad_ringbuffer_t *msg_ring;
  int socketpair[2];
  krad_Xtransponder_subunit_t *subunit;  
  krad_Xtransponder_subunit_t *connected_to_subunit;
};

struct krad_Xtransponder_output_port_St {
  krad_Xtransponder_input_port_t **connections;
};

struct krad_Xtransponder_subunit_St {
  int destroy;
  krad_Xtransponder_t *krad_Xtransponder;
  krad_transponder_watch_t *watch;
  pthread_t thread;
  krad_Xtransponder_subunit_type_t type;
  krad_Xtransponder_input_port_t *control;  
  krad_Xtransponder_input_port_t **inputs;
  krad_Xtransponder_output_port_t **outputs;
};

struct krad_Xtransponder_St {
	krad_Xtransponder_subunit_t **subunits;
	krad_radio_t *krad_radio;
};


int krad_Xtransponder_add_capture (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch);
int krad_Xtransponder_add_encoder (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch);
int krad_Xtransponder_add_decoder (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch);

void krad_Xtransponder_subunit_remove (krad_Xtransponder_t *krad_Xtransponder, int s);

void krad_Xtransponder_destroy (krad_Xtransponder_t **krad_Xtransponder);
krad_Xtransponder_t *krad_Xtransponder_create (krad_radio_t *krad_radio);

#endif
