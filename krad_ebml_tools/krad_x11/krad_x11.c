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

	memcpy(data, imagedata, width * height * 4);

	cairo_set_source_rgb (cr, 2, 3, 0.5);
	cairo_move_to (cr, 0, 0);
	cairo_line_to (cr, 11, 44);
	//cairo_line_to (cr, 975, 111);
	//cairo_set_line_width (cr, 7);
	cairo_stroke (cr);

	cairo_surface_write_to_png (surface, filename);

	cairo_destroy(cr);

	cairo_surface_destroy(surface);
	
	free(data);
	
	return 0;

}


void krad_x11_gl_draw() {
    glClearColor(0.2, 0.4, 0.9, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}


int krad_x11_gl_run (krad_x11_t *krad_x11) {

    int running = 1;
    xcb_generic_event_t *event;
    
    running = 1;
    
	while (running) {

		event = xcb_wait_for_event(krad_x11->connection);

		if (!event) {
			fprintf(stderr, "i/o error in xcb_wait_for_event");
			return -1;
		}

		switch (event->response_type & ~0x80) {

			case XCB_KEY_PRESS:
				/* Quit on key press */
				running = 0;
				break;
			case XCB_EXPOSE:
				/* Handle expose event, draw and swap buffers */
				krad_x11_gl_draw();
				glXSwapBuffers (krad_x11->display, krad_x11->drawable);
				break;
			default:
				break;
		}

		free (event);
	}

    return 0;
}

int krad_x11_create_glx_window (krad_x11_t *krad_x11) {

	int ret;
	int visualID = 0;
	GLXFBConfig *fb_configs = 0;
	int num_fb_configs = 0;
	GLXFBConfig fb_config;
	GLXContext context;
	xcb_colormap_t colormap;
	xcb_window_t window;
	GLXWindow glx_window;
	uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
	uint32_t valuemask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	
	fb_configs = glXGetFBConfigs(krad_x11->display, krad_x11->screen_number, &num_fb_configs);
	
	if (!fb_configs || num_fb_configs == 0) {
		fprintf(stderr, "glXGetFBConfigs failed\n");
		return -1;
	}

	fb_config = fb_configs[0];
	glXGetFBConfigAttrib(krad_x11->display, fb_config, GLX_VISUAL_ID , &visualID);
	context = glXCreateNewContext(krad_x11->display, fb_config, GLX_RGBA_TYPE, 0, True);
	
	if (!context) {
		fprintf(stderr, "glXCreateNewContext failed\n");
		return -1;
	}

	colormap = xcb_generate_id (krad_x11->connection);
	window = xcb_generate_id (krad_x11->connection);

	xcb_create_colormap ( krad_x11->connection, XCB_COLORMAP_ALLOC_NONE, colormap, krad_x11->screen->root, visualID );

	uint32_t valuelist[] = { eventmask, colormap, 0 };

	xcb_create_window (krad_x11->connection, XCB_COPY_FROM_PARENT, window, krad_x11->screen->root, 0, 0, 150, 150, 0,
					   XCB_WINDOW_CLASS_INPUT_OUTPUT, visualID, valuemask, valuelist );

	xcb_map_window (krad_x11->connection, window); 

	glx_window = glXCreateWindow ( krad_x11->display, fb_config, window, 0 );

	if (!window) {
		xcb_destroy_window (krad_x11->connection, window);
		glXDestroyContext (krad_x11->display, context);
		fprintf(stderr, "glXDestroyContext failed\n");
		exit(1);
	}

	krad_x11->drawable = glx_window;

	/* make OpenGL context current */
	if (!glXMakeContextCurrent(krad_x11->display, krad_x11->drawable, krad_x11->drawable, context)) {
		xcb_destroy_window(krad_x11->connection, window);
		glXDestroyContext(krad_x11->display, context);
		fprintf(stderr, "glXMakeContextCurrent failed\n");
		exit(1);
	}

	/* run main loop */
	ret = krad_x11_gl_run(krad_x11);

	/* Cleanup */
	glXDestroyWindow (krad_x11->display, glx_window);
	xcb_destroy_window (krad_x11->connection, window);
	glXDestroyContext (krad_x11->display, context);

	return ret;

}



void krad_x11_destroy (krad_x11_t *krad_x11) {

	XCloseDisplay (krad_x11->display);
	
	free(krad_x11);
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
		}
	}

	return krad_x11;

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

		number = xcb_image_shm_get(krad_x11->connection, krad_x11->screen->root, img, shminfo, 0, 0, AllPlanes);
		reply = xcb_shm_get_image_reply(krad_x11->connection, cookie, NULL);
	
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
