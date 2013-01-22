#include <math.h>
#include <stdlib.h>
#include <inttypes.h>

typedef struct krad_perspective_St krad_perspective_t;
typedef struct krad_position_St krad_position_t;

struct krad_position_St {
  double x;
  double y;
};

struct krad_perspective_St {
  int w, h;
  krad_position_t tl;
  krad_position_t tr;
  krad_position_t bl;
  krad_position_t br;
  int *map;  
};

void krad_perspective_process (krad_perspective_t *krad_perspective, uint32_t *inframe, uint32_t *outframe);

void perspective_destroy (krad_perspective_t *krad_perspective);
krad_perspective_t *krad_perspective_create (int width, int height);
