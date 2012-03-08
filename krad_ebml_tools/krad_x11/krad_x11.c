#include "krad_x11.h"


int saveimage (char *filename, int width, int height, uint8_t *imagedata) {
                                              
	cairo_surface_t *surface;
	cairo_t *cr;				  
			
	int stride;
	int bytes;
	unsigned char *data;
				
	stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, width);
	bytes = stride * height;
	data = calloc (1, bytes);
	surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_RGB24, width, height, stride);

	cr = cairo_create (surface);

	//memcpy(data, imagedata, width * height * 4);

	cairo_set_source_rgb (cr, 2, 3, 0.5);
	cairo_move_to (cr, 0, 0);
	cairo_line_to (cr, 4, 4);
	//cairo_line_to (cr, 975, 111);
	//cairo_set_line_width (cr, 7);
	cairo_stroke (cr);

	memcpy(data, imagedata, width * height * 4);

	cairo_surface_write_to_png (surface, filename);

	cairo_destroy(cr);

	cairo_surface_destroy(surface);
	
	free(data);
	
	return 0;

}


void krad_x11_glx_render (krad_x11_t *krad_x11) {

	//glClearColor(0.2, 0.4, 0.9, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT);
    
	//float hud_aspectRatio = krad_sdl_opengl_display->hud_width / (float) krad_sdl_opengl_display->hud_height;

	// get the window aspect ratio
	float aspectRatio = krad_x11->width / (float) krad_x11->height;
	float windowAspectRatio = krad_x11->width / (float) krad_x11->height;
	float halfWindowAspectRatio = windowAspectRatio * 0.5f;
	// aspectRatio is the aspect ratio of your frame or texture
	float halfAspectRatio = aspectRatio * 0.5f;
	float left, right, top, bottom;
	if (aspectRatio < windowAspectRatio) {
		// top and bottom should be flush with window
		top = 0.5f; // remember this is the top of the texture
		left = -halfWindowAspectRatio;
	} else {
		// left and right should be flush with window
		left = -halfAspectRatio;
		// we are keeping top relative to the windowAspectRatio (normalizing here)
		top = halfAspectRatio / windowAspectRatio;
	}
	
	right = -left;
	bottom = -top;

	glBindTexture (GL_TEXTURE_2D, 4);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//pthread_rwlock_rdlock(&krad_sdl_opengl_display->frame_lock);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, krad_x11->width, krad_x11->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, krad_x11->pixels);
	//pthread_rwlock_unlock(&krad_sdl_opengl_display->frame_lock);

	/*
	if (krad_sdl_opengl_display->hud_data != NULL) {
		glBindTexture (GL_TEXTURE_2D, 5);
		glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, krad_sdl_opengl_display->hud_width, krad_sdl_opengl_display->hud_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, krad_sdl_opengl_display->hud_data);
	}
	*/

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW); 	
	glLoadIdentity ();

	static float speed = 0.035f;
	static float k = 0.0f;
	static float zoom = 0.0f;
	static float raise = 0.0f;
	static float lastzoom = 0.0f;
	static float zoomrange = 1.0f;
	static float rotaterange = 35.0f;
	
	static int animate_test = 1;
	static int zoom_in = 1;

	k += speed;

	if (animate_test) {
		if (!(zoom_in)) {
			//k += speed;
			zoomrange = 3.0f;
			zoom = (zoomrange * -1) - (-(zoomrange) * sinf(k));
			//printf("zoom is %f zoomrange is %f final is %f \n", zoom, zoomrange, zoom + -2.0f);	
			glTranslatef (0.0f, 0.0f, zoom + -1.5f);
			glRotatef (rotaterange * sinf(k), 1.0f, 0.3f, 0.4f);
			//glScalef(zoomrange*sinf(k) + zoomrange, zoomrange*sinf(k) + zoomrange, 1.0f);
		} else {
			if (zoom != zoomrange) {

				if (zoom == 0.0f) {
					zoomrange = 3.0f;
					zoom = zoomrange * 2.0f;
				}
				
				//k += speed;
				zoom = (zoomrange * -1) - (-(zoomrange)*sinf(k));
				
				if (lastzoom == 0.0f) {
					lastzoom = zoom;
				}
				
				if (lastzoom > zoom) {
					glTranslatef(0.0f, 0.0f, -1.0f);
					zoom_in = 0;
					animate_test = 0;
				} else {
					//printf("zoom is %f zoomrange is %f final is %f k is %f\n", zoom, zoomrange, zoom + -2.0f, k);	
					glTranslatef(0.0f, 0.0f, zoom + -1.0f);
					//glRotatef(rotaterange*sinf(k), 1.0f, 0.3f, 0.4f);
					raise += speed;
					if (raise > 1.0f) {
						raise = 1.0f;
					}
					glScalef(1.0f, raise, 1.0f);
				}
				
				lastzoom = zoom;
			}
		}
	} else {
		glTranslatef (0.0f, 0.0f, -1.0f);
	}

	
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glFrustum (left, right, bottom, top, 1.00, 50.0);
	glMatrixMode (GL_MODELVIEW);

	//glUseProgram(program_object);

	// reset color
	glColor3f(1.0f, 1.0f, 1.0f);
	
	// video display
	glEnable (GL_TEXTURE_2D);
	glBindTexture (GL_TEXTURE_2D, 4);

	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-aspectRatio * 0.5f, 0.5f); 
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(-aspectRatio  * 0.5f, -0.5f); 
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(aspectRatio  * 0.5f, -0.5f); 
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(aspectRatio  * 0.5f, 0.5f);
	}
	glEnd();
	
	glDisable (GL_TEXTURE_2D);

	glXSwapBuffers (krad_x11->display, krad_x11->drawable);
	krad_x11_glx_check_for_input (krad_x11);
}


void krad_x11_glx_check_for_input (krad_x11_t *krad_x11) {
	krad_x11_glx_wait_or_poll (krad_x11, 0);
}

void krad_x11_glx_wait (krad_x11_t *krad_x11) {
	krad_x11_glx_wait_or_poll (krad_x11, 1);
}


void krad_x11_glx_wait_or_poll (krad_x11_t *krad_x11, int wait) {

	if (wait) {
		krad_x11->event = xcb_wait_for_event (krad_x11->connection);
	} else {
		krad_x11->event = xcb_poll_for_event (krad_x11->connection);
	}
	
	if ((wait) && (!krad_x11->event)) {
		fprintf(stderr, "\n\ni/o error in xcb_wait_for_event\n\n");
		krad_x11->close_window = 1;
	}
	
	if (krad_x11->event) {

		switch (krad_x11->event->response_type & ~0x80) {

			case XCB_KEY_PRESS:
				krad_x11->press = (xcb_button_press_event_t *)krad_x11->event;
				printf("\nX11 Keypress: %d\n\n", krad_x11->press->detail);
				krad_x11_capture(krad_x11, NULL);
				break;
			case XCB_EXPOSE:
				/* Handle expose event, draw and swap buffers */
				krad_x11_glx_render (krad_x11);
				break;
			default:
				break;
		}
	
		if ((krad_x11->event->response_type & 0x7f) == XCB_CLIENT_MESSAGE) {
			xcb_client_message_event_t* evt = (xcb_client_message_event_t*)krad_x11->event;
			if (evt->type == krad_x11->wm_protocols && evt->data.data32[0] == krad_x11->wm_delete_window) {
				printf("\n\nWM_DELETE_WINDOW received from the Window Manager\n\n");
				krad_x11->close_window = 1;
				if (krad_x11->krad_x11_shutdown != NULL) {
					*krad_x11->krad_x11_shutdown = 1;
				}
			} else {
				printf("\n\nunhandled x11 message\n\n");
			}
		}
	
		free (krad_x11->event);
	}
}

void krad_x11_destroy_glx_window (krad_x11_t *krad_x11) {

	glXDestroyWindow (krad_x11->display, krad_x11->glx_window);
	xcb_destroy_window (krad_x11->connection, krad_x11->window);
	glXDestroyContext (krad_x11->display, krad_x11->context);
	free (krad_x11->pixels);
	krad_x11->window_open = 0;

}

static inline xcb_intern_atom_cookie_t intern_string (xcb_connection_t *c, const char *s) {
	return xcb_intern_atom (c, 0, strlen (s), s);
}

static xcb_atom_t get_atom (xcb_connection_t *conn, xcb_intern_atom_cookie_t ck) {
    xcb_intern_atom_reply_t *reply;
    xcb_atom_t atom;

    reply = xcb_intern_atom_reply (conn, ck, NULL);
    if (reply == NULL)
        return 0;

    atom = reply->atom;
    free (reply);
    return atom;
}

krad_x11_t *krad_x11_create_glx_window (krad_x11_t *krad_x11, char *title, int width, int height, int *closed) {

	int visualID = 0;
	GLXFBConfig *fb_configs = 0;
	int num_fb_configs = 0;
	GLXFBConfig fb_config;
	//uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[1] = { XCB_EVENT_MASK_KEY_PRESS, };
	
	if (krad_x11 == NULL) {
		krad_x11 = krad_x11_create();
	}	
	
	krad_x11->krad_x11_shutdown = closed;
	
	strncpy(krad_x11->window_title, title, sizeof(krad_x11->window_title));
	
	krad_x11->width = width;
	krad_x11->height = height;
	
	fb_configs = glXGetFBConfigs(krad_x11->display, krad_x11->screen_number, &num_fb_configs);
	
	if (!fb_configs || num_fb_configs == 0) {
		fprintf(stderr, "glXGetFBConfigs failed\n");
		exit(1);
	}

	fb_config = fb_configs[0];
	glXGetFBConfigAttrib(krad_x11->display, fb_config, GLX_VISUAL_ID , &visualID);
	krad_x11->context = glXCreateNewContext(krad_x11->display, fb_config, GLX_RGBA_TYPE, 0, True);
	
	if (!krad_x11->context) {
		fprintf(stderr, "glXCreateNewContext failed\n");
		exit(1);
	}

	krad_x11->colormap = xcb_generate_id (krad_x11->connection);
	krad_x11->window = xcb_generate_id (krad_x11->connection);

	xcb_create_colormap ( krad_x11->connection, XCB_COLORMAP_ALLOC_NONE, krad_x11->colormap, krad_x11->screen->root, visualID );

	//uint32_t valuelist[] = { eventmask, krad_x11->colormap, 0 };

	xcb_create_window (krad_x11->connection, XCB_COPY_FROM_PARENT, krad_x11->window, krad_x11->screen->root, 0, 0, krad_x11->width, krad_x11->height, 0,
					   XCB_WINDOW_CLASS_INPUT_OUTPUT, visualID, mask, values );

	xcb_change_property (krad_x11->connection, XCB_PROP_MODE_REPLACE, krad_x11->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, 
						 strlen (krad_x11->window_title), krad_x11->window_title);
						 
	xcb_change_property (krad_x11->connection, XCB_PROP_MODE_REPLACE, krad_x11->window, XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, 
						 strlen(krad_x11->window_title), krad_x11->window_title);

    const static uint32_t values2[] = { 60, 60, 0 };

    xcb_configure_window (krad_x11->connection, krad_x11->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values2);

	xcb_intern_atom_cookie_t wm_protocols_ck, wm_delete_window_ck,
	                          wm_state_ck, wm_state_above_ck,
                              wm_state_below_ck, wm_state_fs_ck;
 
	wm_protocols_ck = intern_string (krad_x11->connection, "WM_PROTOCOLS");
	wm_delete_window_ck = intern_string (krad_x11->connection, "WM_DELETE_WINDOW");
	wm_state_ck = intern_string (krad_x11->connection, "_NET_WM_STATE");
	wm_state_above_ck = intern_string (krad_x11->connection, "_NET_WM_STATE_ABOVE");
	wm_state_below_ck = intern_string (krad_x11->connection, "_NET_WM_STATE_BELOW");
	wm_state_fs_ck = intern_string (krad_x11->connection, "_NET_WM_STATE_FULLSCREEN");
	
	krad_x11->wm_protocols = get_atom (krad_x11->connection, wm_protocols_ck);
	krad_x11->wm_delete_window = get_atom (krad_x11->connection, wm_delete_window_ck);
	krad_x11->wm_state = get_atom (krad_x11->connection, wm_state_ck);
	krad_x11->wm_state_above = get_atom (krad_x11->connection, wm_state_above_ck);
	krad_x11->wm_state_below = get_atom (krad_x11->connection, wm_state_below_ck);
	krad_x11->wm_state_fullscreen = get_atom (krad_x11->connection, wm_state_fs_ck);


	const xcb_atom_t wm_protocols[1] = { krad_x11->wm_delete_window, };
	
	xcb_change_property (krad_x11->connection, XCB_PROP_MODE_REPLACE, krad_x11->window,
						 krad_x11->wm_protocols, XA_ATOM, 32, 1, wm_protocols);

	xcb_map_window (krad_x11->connection, krad_x11->window);

	krad_x11->glx_window = glXCreateWindow ( krad_x11->display, fb_config, krad_x11->window, 0 );

	if (!krad_x11->window) {
		xcb_destroy_window (krad_x11->connection, krad_x11->window);
		glXDestroyContext (krad_x11->display, krad_x11->context);
		fprintf(stderr, "glXDestroyContext failed\n");
		exit(1);
	}

	krad_x11->drawable = krad_x11->glx_window;

	/* make OpenGL context current */
	if (!glXMakeContextCurrent(krad_x11->display, krad_x11->drawable, krad_x11->drawable, krad_x11->context)) {
		xcb_destroy_window(krad_x11->connection, krad_x11->window);
		glXDestroyContext(krad_x11->display, krad_x11->context);
		fprintf(stderr, "glXMakeContextCurrent failed\n");
		exit(1);
	}
	
	krad_x11->pixels_size = krad_x11->height * krad_x11->width * 4;
	krad_x11->pixels = calloc (1, krad_x11->pixels_size);
	
	krad_x11->window_open = 1;

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
	//glHint(GL_POLYGON_SMOOTH, GL_NICEST);
	//glEnable(GL_POLYGON_SMOOTH);
    
	glClearColor( 0, 0, 0, 0 );
	glViewport ( 0, 0, krad_x11->width, krad_x11->height );

	glMatrixMode ( GL_PROJECTION );
	glLoadIdentity ();
	
	krad_x11_enable_capture(krad_x11, krad_x11->screen_width, krad_x11->screen_height);
	
	return krad_x11;
	
}



void krad_x11_destroy (krad_x11_t *krad_x11) {

	if (krad_x11->window_open == 1) {
		krad_x11_destroy_glx_window (krad_x11);
	}
	
	if (krad_x11->capture_enabled == 1) {
		krad_x11_disable_capture(krad_x11);
	}
	
	XCloseDisplay (krad_x11->display);
	
	free (krad_x11);
}

krad_x11_t *krad_x11_create() {

	krad_x11_t *krad_x11;
	int s;
	
	if ((krad_x11 = calloc (1, sizeof (krad_x11_t))) == NULL) {
		fprintf(stderr, "krad_x11 mem alloc fail\n");
		exit (1);
	}


	if (KRAD_X11_XCB_ONLY) {
		krad_x11->connection = xcb_connect (NULL, &krad_x11->screen_number);
	} else {
	
		krad_x11->display = XOpenDisplay (0);
	
		if (!krad_x11->display) {
			fprintf(stderr, "Can't open display\n");
			exit(1);
		}

		krad_x11->connection = XGetXCBConnection (krad_x11->display);

		if (!krad_x11->connection) {
			XCloseDisplay (krad_x11->display);
			fprintf(stderr, "Can't get xcb connection from display\n");
			exit(1);
		}

		XSetEventQueueOwner (krad_x11->display, XCBOwnsEventQueue);
	}
	
	krad_x11->iter = xcb_setup_roots_iterator (xcb_get_setup (krad_x11->connection));
		
	for (s = krad_x11->screen_number; krad_x11->iter.rem; --s, xcb_screen_next (&krad_x11->iter)) {
		if (s == 0) {
			krad_x11->screen = krad_x11->iter.data;
			krad_x11->screen_width = krad_x11->screen->width_in_pixels;
			krad_x11->screen_height = krad_x11->screen->height_in_pixels;
			krad_x11->screen_bit_depth = krad_x11->screen->root_depth;
		}
	}

	return krad_x11;

}

void krad_x11_enable_capture(krad_x11_t *krad_x11, int width, int height) {
	
	if (width == 0) {
		width = krad_x11->screen_width;
	}
	
	if (height == 0) {
		height = krad_x11->screen_height;
	}
	
	krad_x11->img = xcb_image_create_native(krad_x11->connection, width, height, XCB_IMAGE_FORMAT_Z_PIXMAP, krad_x11->screen_bit_depth, 0, ~0, 0);
	if (!krad_x11->img) {
		exit(1);
	}
	
	krad_x11->shminfo.shmid = shmget(IPC_PRIVATE, krad_x11->img->stride * krad_x11->img->height, (IPC_CREAT | 0666));

	if (krad_x11->shminfo.shmid == (uint32_t)-1) {
		xcb_image_destroy(krad_x11->img);
		printf("shminfo fail\n");         
		exit(1);
	}

	krad_x11->shminfo.shmaddr = shmat(krad_x11->shminfo.shmid, 0, 0);
	krad_x11->img->data = krad_x11->shminfo.shmaddr;

	if (krad_x11->img->data == (uint8_t *)-1) {
		xcb_image_destroy(krad_x11->img);
		printf("xcb image fail\n");         
		exit(1);
	}

	krad_x11->shminfo.shmseg = xcb_generate_id(krad_x11->connection);
	xcb_shm_attach(krad_x11->connection, krad_x11->shminfo.shmseg, krad_x11->shminfo.shmid, 0);
	krad_x11->capture_enabled = 1;
}


void krad_x11_disable_capture(krad_x11_t *krad_x11) {

	xcb_shm_detach(krad_x11->connection, krad_x11->shminfo.shmseg);
	xcb_image_destroy(krad_x11->img);
	shmdt(krad_x11->shminfo.shmaddr);
	shmctl(krad_x11->shminfo.shmid, IPC_RMID, 0);
	krad_x11->capture_enabled = 0;
}


int krad_x11_capture(krad_x11_t *krad_x11, unsigned char *buffer) {

	char filename[512];
	
	krad_x11->number = xcb_image_shm_get (krad_x11->connection, krad_x11->screen->root, krad_x11->img, krad_x11->shminfo, 0, 0, 0xffffffff);
	krad_x11->reply = xcb_shm_get_image_reply (krad_x11->connection, krad_x11->cookie, NULL);
	
	if (krad_x11->reply) {
		free(krad_x11->reply);
	}

	//printf("And the numbers are: %d %d %d %d \n", krad_x11->img->width, krad_x11->img->height, krad_x11->screen_bit_depth, krad_x11->number);         
	
	if (buffer == NULL) {
		sprintf(filename, "%s/krad_link_test_capture_%zu.png", getenv ("HOME"), time(NULL));
		saveimage (filename, krad_x11->img->width, krad_x11->img->height, krad_x11->img->data);
		printf("\nSaved image to %s\n", filename);
		return 0;
	} else {
		memcpy (buffer, krad_x11->img->data, krad_x11->img->width * krad_x11->img->height * 4);
		return krad_x11->img->width * krad_x11->img->height * 4;
	}
}


void krad_x11_test_capture(krad_x11_t *krad_x11) {


	xcb_shm_segment_info_t shminfo;
	xcb_shm_get_image_cookie_t cookie;
	xcb_shm_get_image_reply_t *reply;
	xcb_image_t *img;

	uint8_t depth;
	int count;
	char filename[512];
	int number;

	depth = 0;
	count = 0;
	number = 0;

	depth = krad_x11->screen->root_depth;
	
	img = xcb_image_create_native(krad_x11->connection, 1280, 720, XCB_IMAGE_FORMAT_Z_PIXMAP, depth, 0, ~0, 0);
	if (!img) {
		exit(1);
	}
	
	shminfo.shmid = shmget(IPC_PRIVATE, img->stride * img->height, (IPC_CREAT | 0666));

	if (shminfo.shmid == (uint32_t)-1) {
		xcb_image_destroy(img);
		printf("shminfo fail\n");         
		exit(1);
	}

	shminfo.shmaddr = shmat(shminfo.shmid, 0, 0);
	img->data = shminfo.shmaddr;

	if (img->data == (uint8_t *)-1) {
		xcb_image_destroy(img);
		printf("xcb image fail\n");         
		exit(1);
	}

	shminfo.shmseg = xcb_generate_id(krad_x11->connection);
	xcb_shm_attach(krad_x11->connection, shminfo.shmseg, shminfo.shmid, 0);

	while (count < 100) {

		number = xcb_image_shm_get (krad_x11->connection, krad_x11->screen->root, img, shminfo, 0, 0, 0xffffffff);
		reply = xcb_shm_get_image_reply (krad_x11->connection, cookie, NULL);
	
		if (reply) {
			free(reply);
		}

		printf("And the numbers are: %s %d %d %d %d \n", img->data, img->width, img->height, depth, number);         
		sprintf(filename, "/home/oneman/testshots/ts%d.png", count++);
		saveimage (filename, img->width, img->height, img->data);
	}

	xcb_shm_detach(krad_x11->connection, shminfo.shmseg);
	xcb_image_destroy(img);
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmid, IPC_RMID, 0);

}
