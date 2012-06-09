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

#define KRAD_WAYLAND_BUFFER_COUNT 2

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
	
	struct wl_shm_listener shm_listener;

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
	
	int frame_size;
	struct wl_buffer *buffer[KRAD_WAYLAND_BUFFER_COUNT];
	int current_buffer;
	
	int (*frame_callback)(void *, void *);	
	void *callback_pointer;
	
	int running;

};


void krad_wayland_set_frame_callback (krad_wayland_t *krad_wayland, int frame_callback (void *, void *), void *pointer);

int krad_wayland_open_window (krad_wayland_t *krad_wayland, int width, int height);

void krad_wayland_destroy (krad_wayland_t *krad_wayland);
krad_wayland_t *krad_wayland_create ();

