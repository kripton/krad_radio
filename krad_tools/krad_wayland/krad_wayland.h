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
#include <assert.h>

#include <wayland-client.h>
#include <wayland-egl.h>

typedef struct krad_wayland_St krad_wayland_t;
typedef struct krad_wayland_display_St krad_wayland_display_t;
typedef struct krad_wayland_window_St krad_wayland_window_t;

struct krad_wayland_display_St {
	struct wl_display *display;
	struct wl_compositor *compositor;
	struct wl_shell *shell;
	struct wl_shm *shm;
	uint32_t formats;
	uint32_t mask;
	
	struct wl_shm_listener shm_listenter;
	
};

struct krad_wayland_window_St {
	int width;
	int height;
	struct wl_surface *surface;
	struct wl_shell_surface *shell_surface;
	struct wl_buffer *buffer;
	void *shm_data;
	struct wl_callback *callback;
	
	struct wl_shell_surface_listener surface_listener;
	struct wl_callback_listener frame_listener;
};


struct krad_wayland_St {

	krad_wayland_window_t *window;
	krad_wayland_display_t *display;
	
	int running;

};

int krad_wayland_run (krad_wayland_t *krad_wayland);

void krad_wayland_destroy (krad_wayland_t *krad_wayland);
krad_wayland_t *krad_wayland_create ();

