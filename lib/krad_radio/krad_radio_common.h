#ifndef KRAD_RADIO_COMMON_H
#define KRAD_RADIO_COMMON_H

typedef struct krad_radio_rep_St krad_radio_rep_t;

struct krad_radio_rep_St {
  
  uint32_t remote_port;
  uint32_t web_port;
  uint32_t osc_port;
  uint64_t start_time;
  
  char logname[256];
  
};

#endif // KRAD_RADIO_COMMON_H
