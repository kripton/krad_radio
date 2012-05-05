#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <signal.h>

#include <wayland-client.h>
#include <wayland-egl.h>

typedef struct krad_wayland_St krad_wayland_t;

struct krad_wayland_St {


};



void krad_wayland_destroy(krad_wayland_t *wayland);
krad_wayland_t *krad_wayland_create();

