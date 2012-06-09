#include "krad_wayland.h"

/* static for once, more clean you say? I do prototype them, in order. */

static int krad_wayland_create_shm_buffer (krad_wayland_t *krad_wayland, int width, int height, int frames,
											uint32_t format, void **data_out);
static void krad_wayland_handle_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial);
static void krad_wayland_handle_configure (void *data, struct wl_shell_surface *shell_surface,
											uint32_t edges, int32_t width, int32_t height);
static void krad_wayland_handle_popup_done (void *data, struct wl_shell_surface *shell_surface);

static void krad_wayland_shm_format (void *data, struct wl_shm *wl_shm, uint32_t format);
static int krad_wayland_event_mask_update (uint32_t mask, void *data);
static void krad_wayland_handle_global (struct wl_display *display, uint32_t id,
												const char *interface, uint32_t version, void *data);

static void krad_wayland_destroy_display (krad_wayland_t *krad_wayland);
static void krad_wayland_create_display (krad_wayland_t *krad_wayland);

static void krad_wayland_destroy_window (krad_wayland_t *krad_wayland);
static int krad_wayland_create_window (krad_wayland_t *krad_wayland);

static void krad_wayland_frame_listener (void *data, struct wl_callback *callback, uint32_t time);
static void krad_wayland_render (krad_wayland_t *krad_wayland, void *image, int width, int height, uint32_t time);

/* end of protos */


static int krad_wayland_create_shm_buffer (krad_wayland_t *krad_wayland, int width, int height, int frames,
											uint32_t format, void **data_out) {


	char filename[] = "/tmp/wayland-shm-XXXXXX";
	struct wl_shm_pool *pool;
	int fd, size, stride;
	void *data;
	int b;
	
	b = 0;

	fd = mkstemp (filename);
	if (fd < 0) {
		fprintf(stderr, "open %s failed: %m\n", filename);
		return 1;
	}
	stride = width * 4;
	krad_wayland->frame_size = stride * height;
	size = krad_wayland->frame_size * frames;
	if (ftruncate (fd, size) < 0) {
		fprintf (stderr, "ftruncate failed: %m\n");
		close (fd);
		return 1;
	}

	data = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	unlink (filename);

	if (data == MAP_FAILED) {
		fprintf (stderr, "mmap failed: %m\n");
		close (fd);
		return 1;
	}

	pool = wl_shm_create_pool (krad_wayland->display->shm, fd, size);
	for (b = 0; b < KRAD_WAYLAND_BUFFER_COUNT; b++) {
		krad_wayland->buffer[b] = wl_shm_pool_create_buffer (pool, b * krad_wayland->frame_size, 
															 width, height, stride, format);
	}
	wl_shm_pool_destroy (pool);
	close (fd);

	*data_out = data;

	return 0;
}
											
static void krad_wayland_handle_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
	wl_shell_surface_pong (shell_surface, serial);
	//printf ("handle ping happened\n");	
}

static void krad_wayland_handle_configure (void *data, struct wl_shell_surface *shell_surface,
							  uint32_t edges, int32_t width, int32_t height) {
	//printf ("handle configure happened\n");
							  
}

static void krad_wayland_handle_popup_done (void *data, struct wl_shell_surface *shell_surface) {
	//printf ("handle popup_done happened\n");	
}

static void krad_wayland_shm_format (void *data, struct wl_shm *wl_shm, uint32_t format) {

	krad_wayland_t *krad_wayland = data;

	krad_wayland->display->formats |= (1 << format);

	//printf ("shm_format happened\n");

}

static int krad_wayland_event_mask_update (uint32_t mask, void *data) {

	krad_wayland_t *krad_wayland = data;

	krad_wayland->display->mask = mask;
	//printf ("event_mask_update happened %u\n", mask);
	return 0;
}

static void krad_wayland_handle_global (struct wl_display *display, uint32_t id,
												const char *interface, uint32_t version, void *data) {

	krad_wayland_t *krad_wayland = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		krad_wayland->display->compositor =
			wl_display_bind(display, id, &wl_compositor_interface);
	} else if (strcmp(interface, "wl_shell") == 0) {
		krad_wayland->display->shell = wl_display_bind(display, id, &wl_shell_interface);
	} else if (strcmp(interface, "wl_shm") == 0) {
		krad_wayland->display->shm = wl_display_bind(display, id, &wl_shm_interface);
		wl_shm_add_listener(krad_wayland->display->shm, &krad_wayland->display->shm_listener, krad_wayland);
	}

	//printf ("display_handle_global happened\n");

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

static void krad_wayland_create_display (krad_wayland_t *krad_wayland) {

	krad_wayland->display = calloc (1, sizeof (krad_wayland_display_t));
	krad_wayland->display->display = wl_display_connect (NULL);
	if (krad_wayland->display->display == NULL) {
		fprintf (stderr, "Can't connect to wayland\n");
		exit (1);
	}

	krad_wayland->display->shm_listener.format = krad_wayland_shm_format;

	krad_wayland->display->formats = 0;
	wl_display_add_global_listener (krad_wayland->display->display, krad_wayland_handle_global, krad_wayland);
	wl_display_iterate (krad_wayland->display->display, WL_DISPLAY_READABLE);
	wl_display_roundtrip (krad_wayland->display->display);

	if (!(krad_wayland->display->formats & (1 << WL_SHM_FORMAT_XRGB8888))) {
		fprintf(stderr, "WL_SHM_FORMAT_XRGB32 not available\n");
		exit(1);
	}

	wl_display_get_fd (krad_wayland->display->display, krad_wayland_event_mask_update, krad_wayland);

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

static int krad_wayland_create_window (krad_wayland_t *krad_wayland) {

	krad_wayland->current_buffer = 0;

	krad_wayland->window->buffer = krad_wayland->buffer[krad_wayland->current_buffer];

	if (!krad_wayland->window->buffer) {
		free (krad_wayland->window);
		return 1;
	}

	krad_wayland->window->surface_listener.ping = krad_wayland_handle_ping;
	krad_wayland->window->surface_listener.configure = krad_wayland_handle_configure;
	krad_wayland->window->surface_listener.popup_done = krad_wayland_handle_popup_done;

	krad_wayland->window->callback = NULL;
	krad_wayland->window->surface = wl_compositor_create_surface (krad_wayland->display->compositor);
	krad_wayland->window->shell_surface = wl_shell_get_shell_surface (krad_wayland->display->shell, 
																	  krad_wayland->window->surface);

	if (krad_wayland->window->shell_surface) {
		wl_shell_surface_add_listener (krad_wayland->window->shell_surface, 
									   &krad_wayland->window->surface_listener,
									   krad_wayland);
	}

	wl_shell_surface_set_toplevel (krad_wayland->window->shell_surface);

	return 0;
}

static void krad_wayland_frame_listener (void *data, struct wl_callback *callback, uint32_t time) {

	krad_wayland_t *krad_wayland = data;

	int updated;

	updated = 0;

	//printf ("redraw happened %u\n", time);

	if (krad_wayland->render_test_pattern == 1) {
		krad_wayland_render (krad_wayland, krad_wayland->window->shm_data,
				     krad_wayland->window->width, krad_wayland->window->height, time);
	}

	if (krad_wayland->frame_callback != NULL) {
		updated = krad_wayland->frame_callback (krad_wayland->callback_pointer, time);
	}

	wl_surface_attach (krad_wayland->window->surface, krad_wayland->window->buffer, 0, 0);

	if (updated) {
		wl_surface_damage (krad_wayland->window->surface, 0, 0, krad_wayland->window->width, krad_wayland->window->height);
	} else {
		wl_surface_damage (krad_wayland->window->surface, 0, 0, 1, 1);
	}

	if (callback) {
		wl_callback_destroy (callback);
	}

	krad_wayland->window->callback = wl_surface_frame (krad_wayland->window->surface);

	krad_wayland->window->frame_listener.done = krad_wayland_frame_listener;

	wl_callback_add_listener (krad_wayland->window->callback, &krad_wayland->window->frame_listener, krad_wayland);

	//printf ("redraw done\n");

}

static void krad_wayland_render (krad_wayland_t *krad_wayland, void *image, int width, int height, uint32_t time) {

	uint32_t *p;
	int i, end, offset;

	p = image;
	end = width * height;
	offset = time >> 4;
	for (i = 0; i < end; i++) {
		p[i] = (i + offset) * 0x0080401;
	}
}


int krad_wayland_prepare_window (krad_wayland_t *krad_wayland, int width, int height, void **buffer) {

	int ret;

	krad_wayland_create_display (krad_wayland);

	krad_wayland->window = calloc (1, sizeof (krad_wayland_window_t));

	if (krad_wayland->window == NULL) {
		return 1;
	}

	krad_wayland->window->width = width;
	krad_wayland->window->height = height;

	ret = krad_wayland_create_shm_buffer (krad_wayland, width, height, KRAD_WAYLAND_BUFFER_COUNT,
					      WL_SHM_FORMAT_XRGB8888, &krad_wayland->window->shm_data);

	if (ret == 0) {
		*buffer = krad_wayland->window->shm_data;
	} else {
		free (krad_wayland->window);
	}

	return ret;

}

int krad_wayland_open_window (krad_wayland_t *krad_wayland) {

	krad_wayland_create_window (krad_wayland);

	int count;

	count = 0;

	if (!krad_wayland->window) {
		return 1;
	}

	krad_wayland->running = 1;

	krad_wayland_frame_listener (krad_wayland, NULL, 0);

	while (krad_wayland->running) {
		//printf ("iterate start %d\n", count);	
		wl_display_iterate (krad_wayland->display->display, krad_wayland->display->mask);
		count++;
		//printf ("iterate happened %d\n", count);
	}

	printf ("Krad Wayland: window closing..\n");
	krad_wayland_destroy_window (krad_wayland);
	krad_wayland_destroy_display (krad_wayland);

	return 0;
}

void krad_wayland_set_frame_callback (krad_wayland_t *krad_wayland, int frame_callback (void *, uint32_t), void *pointer) {

	krad_wayland->frame_callback = frame_callback;
	krad_wayland->callback_pointer = pointer;

}

void krad_wayland_destroy (krad_wayland_t *krad_wayland) {

	free (krad_wayland);
}

krad_wayland_t *krad_wayland_create () {

	krad_wayland_t *krad_wayland = calloc (1, sizeof(krad_wayland_t));
	
	return krad_wayland;
}
