#include "krad_wayland.h"

/* static for once, more clean you say? I do prototype them, in order. */

static int krad_wayland_create_shm_buffer (krad_wayland_t *krad_wayland, int width, int height, int frames,
											uint32_t format, void **data_out);
static void krad_wayland_handle_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial);
static void krad_wayland_handle_configure (void *data, struct wl_shell_surface *shell_surface,
											uint32_t edges, int32_t width, int32_t height);
static void krad_wayland_handle_popup_done (void *data, struct wl_shell_surface *shell_surface);

static void
krad_wayland_pointer_handle_enter(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface,
		     wl_fixed_t sx_w, wl_fixed_t sy_w);


static void krad_wayland_pointer_handle_leave(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface);

static void krad_wayland_pointer_handle_motion(void *data, struct wl_pointer *pointer,
		      uint32_t time, wl_fixed_t sx_w, wl_fixed_t sy_w);

static void krad_wayland_pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial,
		      uint32_t time, uint32_t button, uint32_t state_w);

static void krad_wayland_pointer_handle_axis(void *data, struct wl_pointer *pointer,
		    uint32_t time, uint32_t axis, wl_fixed_t value);

static void krad_wayland_seat_handle_capabilities (void *data, struct wl_seat *seat, enum wl_seat_capability caps);

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

static void
krad_wayland_pointer_handle_enter(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface,
		     wl_fixed_t sx_w, wl_fixed_t sy_w)
{
	krad_wayland_t *krad_wayland = data;
	//struct window *window;
	//struct widget *widget;
	//float sx = wl_fixed_to_double(sx_w);
	//float sy = wl_fixed_to_double(sy_w);

	krad_wayland->display->pointer_x = wl_fixed_to_int(sx_w);
	krad_wayland->display->pointer_y = wl_fixed_to_int(sy_w);

	krad_wayland->mousein = 1;

	/*

	if (!surface) {
		// enter event for a window we've just destroyed 
		return;
	}

	input->display->serial = serial;
	input->pointer_enter_serial = serial;
	input->pointer_focus = wl_surface_get_user_data(surface);
	window = input->pointer_focus;

	if (window->pool) {
		shm_pool_destroy(window->pool);
		window->pool = NULL;
		//Schedule a redraw to free the pool 
		window_schedule_redraw(window);
	}

	input->sx = sx;
	input->sy = sy;

	widget = widget_find_widget(window->widget, sx, sy);
	input_set_focus_widget(input, widget, sx, sy);
	*/
}

static void
krad_wayland_pointer_handle_leave(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface)
{
	krad_wayland_t *krad_wayland = data;

	krad_wayland->display->pointer_x = -1;
	krad_wayland->display->pointer_y = -1;

	krad_wayland->mousein = 0;

	//input->display->serial = serial;
	//input_remove_pointer_focus(input);
}

static void
krad_wayland_pointer_handle_motion(void *data, struct wl_pointer *pointer,
		      uint32_t time, wl_fixed_t sx_w, wl_fixed_t sy_w)
{
	krad_wayland_t *krad_wayland = data;
	/*
	struct window *window = input->pointer_focus;
	struct widget *widget;
	int cursor = CURSOR_LEFT_PTR;
	*/
	//float sx = wl_fixed_to_double(sx_w);
	//float sy = wl_fixed_to_double(sy_w);

	krad_wayland->display->pointer_x = wl_fixed_to_int(sx_w);
	krad_wayland->display->pointer_y = wl_fixed_to_int(sy_w);

	/*
	input->sx = sx;
	input->sy = sy;

	if (!(input->grab && input->grab_button)) {
		widget = widget_find_widget(window->widget, sx, sy);
		input_set_focus_widget(input, widget, sx, sy);
	}

	if (input->grab)
		widget = input->grab;
	else
		widget = input->focus_widget;
	if (widget && widget->motion_handler)
		cursor = widget->motion_handler(input->focus_widget,
						input, time, sx, sy,
						widget->user_data);

	input_set_pointer_image(input, cursor);
	*/
}

static void
krad_wayland_pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial,
		      uint32_t time, uint32_t button, uint32_t state_w)
{
	krad_wayland_t *krad_wayland = data;
	
	//struct widget *widget;
	enum wl_pointer_button_state state = state_w;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		//printf ("clicky!\n");
		krad_wayland->click = 1;
	}
	
	if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
		//printf ("unclicky..\n");
		krad_wayland->click = 0;		
	}

	/*
	input->display->serial = serial;
	if (input->focus_widget && input->grab == NULL &&
	    state == WL_POINTER_BUTTON_STATE_PRESSED)
		input_grab(input, input->focus_widget, button);

	widget = input->grab;
	if (widget && widget->button_handler)
		(*widget->button_handler)(widget,
					  input, time,
					  button, state,
					  input->grab->user_data);

	if (input->grab && input->grab_button == button &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED)
		input_ungrab(input);
	*/
}

static void
krad_wayland_pointer_handle_axis(void *data, struct wl_pointer *pointer,
		    uint32_t time, uint32_t axis, wl_fixed_t value)
{
	//krad_wayland_t *krad_wayland = data;

	/*
	struct widget *widget;

	widget = input->focus_widget;
	if (input->grab)
		widget = input->grab;
	if (widget && widget->axis_handler)
		(*widget->axis_handler)(widget,
					input, time,
					axis, value,
					widget->user_data);
	*/
}

static void krad_wayland_seat_handle_capabilities (void *data, struct wl_seat *seat, enum wl_seat_capability caps) {

	krad_wayland_t *krad_wayland = data;

	if ((caps & WL_SEAT_CAPABILITY_POINTER) && !krad_wayland->display->pointer) {
		krad_wayland->display->pointer = wl_seat_get_pointer(seat);
		//wl_pointer_set_user_data (krad_wayland->display->pointer, krad_wayland);
		wl_pointer_add_listener (krad_wayland->display->pointer, &krad_wayland->display->pointer_listener, krad_wayland);
	} else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && krad_wayland->display->pointer) {
		wl_pointer_destroy(krad_wayland->display->pointer);
		krad_wayland->display->pointer = NULL;
	}
	/*
	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !input->keyboard) {
		input->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_set_user_data(input->keyboard, input);
		wl_keyboard_add_listener(input->keyboard, &keyboard_listener,
					 input);
	} else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && input->keyboard) {
		wl_keyboard_destroy(input->keyboard);
		input->keyboard = NULL;
	}
	*/
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
	} else if (strcmp(interface, "wl_seat") == 0) {
		krad_wayland->display->seat = wl_display_bind(display, id, &wl_seat_interface);
		wl_seat_add_listener(krad_wayland->display->seat, &krad_wayland->display->seat_listener, krad_wayland);
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

	krad_wayland->display->pointer_x = -1;
	krad_wayland->display->pointer_y = -1;

	krad_wayland->display->pointer_listener.enter = krad_wayland_pointer_handle_enter;
	krad_wayland->display->pointer_listener.leave = krad_wayland_pointer_handle_leave;
	krad_wayland->display->pointer_listener.motion = krad_wayland_pointer_handle_motion;
	krad_wayland->display->pointer_listener.button = krad_wayland_pointer_handle_button;
	krad_wayland->display->pointer_listener.axis = krad_wayland_pointer_handle_axis;

	krad_wayland->display->seat_listener.capabilities = krad_wayland_seat_handle_capabilities;
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

	wl_shell_surface_set_title (krad_wayland->window->shell_surface, krad_wayland->window->title);

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

		updated = 1;
	} else {

		if (krad_wayland->frame_callback != NULL) {
			updated = krad_wayland->frame_callback (krad_wayland->callback_pointer, time);
		}
	}

	wl_surface_attach (krad_wayland->window->surface, krad_wayland->window->buffer, 0, 0);

	if (updated) {
		wl_surface_damage (krad_wayland->window->surface, 0, 0, krad_wayland->window->width, krad_wayland->window->height);
	} else {
		wl_surface_damage (krad_wayland->window->surface, 0, 0, 10, 10);
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

void krad_wayland_set_window_title (krad_wayland_t *krad_wayland, char *title) {
	strncpy (krad_wayland->window->title, title, sizeof(krad_wayland->window->title));
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

	if (!krad_wayland->window) {
		return 1;
	}

	krad_wayland_frame_listener (krad_wayland, NULL, 0);

	return 0;
}

void krad_wayland_iterate (krad_wayland_t *krad_wayland) {
	wl_display_iterate (krad_wayland->display->display, krad_wayland->display->mask);
}

void krad_wayland_close_window (krad_wayland_t *krad_wayland) {
	printkd ("Krad Wayland: destroy_window..");
	krad_wayland_destroy_window (krad_wayland);
	printkd ("Krad Wayland: destroy_display..");	
	krad_wayland_destroy_display (krad_wayland);
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
