#ifndef KRAD_RADIO_COMMON_H
#define KRAD_RADIO_COMMON_H

#include "krad_ebml.h"
#include "krad_radio_ipc.h"

typedef struct krad_radio_rep_St krad_radio_rep_t;
typedef struct krad_radio_rep_St kr_radio_t;

struct krad_radio_rep_St {
  

  uint64_t uptime;
  uint32_t cpu_usage;
  char sysinfo[256];
  char logname[256];
  
};





kr_radio_t *kr_radio_rep_create ();
void kr_radio_rep_destroy (kr_radio_t *radio_rep);







#endif // KRAD_RADIO_COMMON_H
