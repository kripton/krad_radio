#include "krad_x11.h"

void krad_x11_destroy (krad_x11_t *krad_x11) {
	
	if (krad_x11->capture_enabled == 1) {
		krad_x11_disable_capture(krad_x11);
	}
	
	XCloseDisplay (krad_x11->display);
	free (krad_x11);
}

krad_x11_t *krad_x11_create () {

	krad_x11_t *krad_x11;
	int s;
	
	if ((krad_x11 = calloc (1, sizeof (krad_x11_t))) == NULL) {
		failfast ("krad_x11 mem alloc fail");
	}

	if (KRAD_X11_XCB_ONLY) {
		krad_x11->connection = xcb_connect (NULL, &krad_x11->screen_number);
	} else {
	
		krad_x11->display = XOpenDisplay (0);
	
		if (!krad_x11->display) {
			failfast ("Can't open display");
		}

		krad_x11->connection = XGetXCBConnection (krad_x11->display);

		if (!krad_x11->connection) {
			XCloseDisplay (krad_x11->display);
			failfast ("Can't get xcb connection from display\n");
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
		failfast ("shminfo fail");
	}

	krad_x11->shminfo.shmaddr = shmat(krad_x11->shminfo.shmid, 0, 0);
	krad_x11->img->data = krad_x11->shminfo.shmaddr;

	if (krad_x11->img->data == (uint8_t *)-1) {
		xcb_image_destroy(krad_x11->img);
		failfast ("xcb image fail");
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

	//char filename[512];
	
	krad_x11->number = xcb_image_shm_get (krad_x11->connection, krad_x11->screen->root, krad_x11->img, krad_x11->shminfo, 0, 0, 0xffffffff);
	krad_x11->reply = xcb_shm_get_image_reply (krad_x11->connection, krad_x11->cookie, NULL);
	
	if (krad_x11->reply) {
		free(krad_x11->reply);
	}

	//printf("And the numbers are: %d %d %d %d \n", krad_x11->img->width, krad_x11->img->height, krad_x11->screen_bit_depth, krad_x11->number);         
	
	if (buffer == NULL) {
		//sprintf(filename, "%s/krad_link_test_capture_%"PRIuMAX".png", getenv ("HOME"), (uintmax_t)time(NULL));
		//saveimage (filename, krad_x11->img->width, krad_x11->img->height, krad_x11->img->data);
		//printk ("\nSaved image to %s\n", filename);
		return 0;
	} else {
		memcpy (buffer, krad_x11->img->data, krad_x11->img->width * krad_x11->img->height * 4);
		return krad_x11->img->width * krad_x11->img->height * 4;
	}
}

