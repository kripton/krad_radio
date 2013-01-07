#include "krad_radio.h"

#ifndef KRAD_TRANSPONDER_GRAPH
#define KRAD_TRANSPONDER_GRAPH

#define KRAD_TRANSPONDER_SUBUNITS 99
#define KRAD_TRANSPONDER_PORT_CONNECTIONS 200

typedef struct krad_Xtransponder_St krad_Xtransponder_t;
typedef struct krad_Xtransponder_subunit_St krad_Xtransponder_subunit_t;
typedef struct krad_Xtransponder_input_port_St krad_Xtransponder_input_port_t;
typedef struct krad_Xtransponder_output_port_St krad_Xtransponder_output_port_t;
typedef struct krad_slice_St krad_slice_t;
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

  int idle_callback_interval;
	int (*readable_callback)(void *);
	void (*destroy_callback)(void *);
	krad_codec_header_t *(*encoder_header_callback)(void *);	
	void *callback_pointer;
  int fd;
	
};

struct krad_Xtransponder_control_msg_St {
  
  krad_Xtransponder_control_msg_type_t type;
  
  krad_Xtransponder_input_port_t *input_port;
  krad_Xtransponder_output_port_t *output_port;
    
};

struct krad_slice_St {
  unsigned char *data;
  int32_t size;
  int frames;
  int keyframe;
  uint64_t timecode;  
	int refs;
	int final;
	int header;
	krad_codec_t codec;
};

struct krad_Xtransponder_input_port_St {
  krad_ringbuffer_t *msg_ring;
  int socketpair[2];
  krad_Xtransponder_subunit_t *subunit;  
  krad_Xtransponder_subunit_t *connected_to_subunit;
};

struct krad_Xtransponder_output_port_St {
  krad_Xtransponder_input_port_t **connections;
  krad_slice_t *krad_slice[4];
  int headers;
  krad_codec_header_t *krad_codec_header;  
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
  
  krad_slice_t *krad_slice;
 
};

struct krad_Xtransponder_St {
	krad_Xtransponder_subunit_t **subunits;
	krad_radio_t *krad_radio;
};


krad_slice_t *krad_slice_create_with_data (unsigned char *data, uint32_t size);
krad_slice_t *krad_slice_create ();
int krad_slice_set_data (krad_slice_t *krad_slice, unsigned char *data, uint32_t size);
void krad_slice_ref (krad_slice_t *krad_slice);
void krad_slice_unref (krad_slice_t *krad_slice);

void krad_Xtransponder_subunit_connect2 (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                         krad_Xtransponder_subunit_t *from_krad_Xtransponder_subunit);
void krad_Xtransponder_subunit_connect (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                        krad_Xtransponder_subunit_t *from_krad_Xtransponder_subunit);

krad_codec_header_t *krad_Xtransponder_get_audio_header (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit);
krad_codec_header_t *krad_Xtransponder_get_header (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit);

krad_codec_header_t *krad_Xtransponder_get_subunit_output_header (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                                                  int port);

int krad_Xtransponder_set_header (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit,
                                  krad_codec_header_t *krad_codec_header);

krad_slice_t *krad_Xtransponder_get_slice (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit);
int krad_Xtransponder_slice_broadcast (krad_Xtransponder_subunit_t *krad_Xtransponder_subunit, krad_slice_t **krad_slice);

void krad_Xtransponder_list (krad_Xtransponder_t *krad_Xtransponder);

int krad_Xtransponder_add_capture (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch);
int krad_Xtransponder_add_muxer (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch);
int krad_Xtransponder_add_demuxer (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch);
int krad_Xtransponder_add_encoder (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch);
int krad_Xtransponder_add_decoder (krad_Xtransponder_t *krad_Xtransponder, krad_transponder_watch_t *watch);

krad_Xtransponder_subunit_t *krad_Xtransponder_get_subunit (krad_Xtransponder_t *krad_Xtransponder, int sid);

void krad_Xtransponder_subunit_remove (krad_Xtransponder_t *krad_Xtransponder, int sid);

void krad_Xtransponder_destroy (krad_Xtransponder_t **krad_Xtransponder);
krad_Xtransponder_t *krad_Xtransponder_create (krad_radio_t *krad_radio);

#endif
