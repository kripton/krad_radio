#include "krad_wayland.h"

static struct wl_buffer *create_shm_buffer (krad_wayland_display_t *krad_wayland_display,
											int width, int height, uint32_t format, void **data_out) {
											
											
	char filename[] = "/tmp/wayland-shm-XXXXXX";
	struct wl_shm_pool *pool;
	struct wl_buffer *buffer;
	int fd, size, stride;
	void *data;

	fd = mkstemp (filename);
	if (fd < 0) {
		fprintf(stderr, "open %s failed: %m\n", filename);
		return NULL;
	}
	stride = width * 4;
	size = stride * height;
	if (ftruncate (fd, size) < 0) {
		fprintf (stderr, "ftruncate failed: %m\n");
		close (fd);
		return NULL;
	}

	data = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	unlink (filename);

	if (data == MAP_FAILED) {
		fprintf (stderr, "mmap failed: %m\n");
		close (fd);
		return NULL;
	}

	pool = wl_shm_create_pool (krad_wayland_display->shm, fd, size);
	buffer = wl_shm_pool_create_buffer(pool, 0,
					   width, height, stride, format);
	wl_shm_pool_destroy (pool);
	close (fd);

	*data_out = data;

	return buffer;
}

static void handle_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
	wl_shell_surface_pong (shell_surface, serial);
}

static void handle_configure (void *data, struct wl_shell_surface *shell_surface,
							  uint32_t edges, int32_t width, int32_t height) {
}

static void handle_popup_done (void *data, struct wl_shell_surface *shell_surface) {
}

static const struct wl_shell_surface_listener shell_surface_listener = {
	handle_ping,
	handle_configure,
	handle_popup_done
};

int krad_wayland_create_window (krad_wayland_t *krad_wayland, int width, int height) {

	krad_wayland->window = calloc (1, sizeof (krad_wayland_window_t));

	krad_wayland->window->buffer = create_shm_buffer (krad_wayland->display, width, height, 
													  WL_SHM_FORMAT_XRGB8888, &krad_wayland->window->shm_data);

	if (!krad_wayland->window->buffer) {
		free (krad_wayland->window);
		return 1;
	}

	krad_wayland->window->callback = NULL;
	krad_wayland->window->width = width;
	krad_wayland->window->height = height;
	krad_wayland->window->surface = wl_compositor_create_surface (krad_wayland->display->compositor);
	krad_wayland->window->shell_surface = wl_shell_get_shell_surface (krad_wayland->display->shell, 
																	  krad_wayland->window->surface);

	if (krad_wayland->window->shell_surface) {
		wl_shell_surface_add_listener (krad_wayland->window->shell_surface, &shell_surface_listener, krad_wayland->window);
	}

	wl_shell_surface_set_toplevel (krad_wayland->window->shell_surface);

	return 0;
}

static void krad_wayland_destroy_window (krad_wayland_t *krad_wayland) {

	if (krad_wayland->window->callback) {
		wl_callback_destroy (krad_wayland->window->callback);
	}

	wl_buffer_destroy (krad_wayland->window->buffer);
	wl_shell_surface_destroy (krad_wayland->window->shell_surface);
	wl_surface_destroy (krad_wayland->window->surface);
	free (krad_wayland->window);
}

static void krad_wayland_render (void *image, int width, int height, uint32_t time) {

	uint32_t *p;
	int i, end, offset;

	p = image;
	end = width * height;
	offset = time >> 4;
	for (i = 0; i < end; i++) {
		p[i] = (i + offset) * 0x0080401;
	}
}

static const struct wl_callback_listener frame_listener;

static void redraw (void *data, struct wl_callback *callback, uint32_t time) {

	krad_wayland_window_t *window = data;

	krad_wayland_render (window->shm_data, window->width, window->height, time);
	wl_surface_attach (window->surface, window->buffer, 0, 0);
	wl_surface_damage (window->surface, 0, 0, window->width, window->height);

	if (callback) {
		wl_callback_destroy(callback);
	}

	window->callback = wl_surface_frame (window->surface);
	wl_callback_add_listener (window->callback, &frame_listener, window);

}

static const struct wl_callback_listener frame_listener = {
	redraw
};

static void shm_format (void *data, struct wl_shm *wl_shm, uint32_t format) {

	krad_wayland_display_t *d = data;

	d->formats |= (1 << format);
}

struct wl_shm_listener shm_listenter = {
	shm_format
};

static void display_handle_global (struct wl_display *display, uint32_t id,
								   const char *interface, uint32_t version, void *data) {

	krad_wayland_display_t *d = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor =
			wl_display_bind(display, id, &wl_compositor_interface);
	} else if (strcmp(interface, "wl_shell") == 0) {
		d->shell = wl_display_bind(display, id, &wl_shell_interface);
	} else if (strcmp(interface, "wl_shm") == 0) {
		d->shm = wl_display_bind(display, id, &wl_shm_interface);
		wl_shm_add_listener(d->shm, &shm_listenter, d);
	}
}

static int event_mask_update (uint32_t mask, void *data) {

	krad_wayland_display_t *d = data;

	d->mask = mask;

	return 0;
}

static krad_wayland_display_t *krad_wayland_create_display (void) {

	krad_wayland_display_t *display;

	display = calloc (1, sizeof (krad_wayland_display_t));
	display->display = wl_display_connect (NULL);
	assert (display->display);

	display->formats = 0;
	wl_display_add_global_listener (display->display, display_handle_global, display);
	wl_display_iterate (display->display, WL_DISPLAY_READABLE);
	wl_display_roundtrip (display->display);

	if (!(display->formats & (1 << WL_SHM_FORMAT_XRGB8888))) {
		fprintf(stderr, "WL_SHM_FORMAT_XRGB32 not available\n");
		exit(1);
	}

	wl_display_get_fd (display->display, event_mask_update, display);
	
	return display;
}

static void krad_wayland_destroy_display (krad_wayland_t *krad_wayland) {

	if (krad_wayland->display->shm) {
		wl_shm_destroy (krad_wayland->display->shm);
	}

	if (krad_wayland->display->shell) {
		wl_shell_destroy (krad_wayland->display->shell);
	}

	if (krad_wayland->display->compositor) {
		wl_compositor_destroy (krad_wayland->display->compositor);
	}

	wl_display_flush (krad_wayland->display->display);
	wl_display_disconnect (krad_wayland->display->display);
	free (krad_wayland->display);
}


int krad_wayland_run (krad_wayland_t *krad_wayland) {

	krad_wayland->display = krad_wayland_create_display ();
	krad_wayland_create_window (krad_wayland, 1280, 720);

	if (!krad_wayland->window) {
		return 1;
	}

	krad_wayland->running = 1;

	redraw (krad_wayland->window, NULL, 0);

	while (krad_wayland->running) {
		wl_display_iterate (krad_wayland->display->display, krad_wayland->display->mask);
	}

	printf ("simple-shm exiting\n");
	krad_wayland_destroy_window (krad_wayland);
	krad_wayland_destroy_display (krad_wayland);

	return 0;
}

void krad_wayland_destroy (krad_wayland_t *krad_wayland) {

	free (krad_wayland);

}

krad_wayland_t *krad_wayland_create () {

	krad_wayland_t *krad_wayland = calloc (1, sizeof(krad_wayland_t));




	
	return krad_wayland;

}
