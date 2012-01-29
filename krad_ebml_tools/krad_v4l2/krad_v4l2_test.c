#include "krad_v4l2.h"

#define DEFAULT_DEVICE "/dev/video0"

int main (int argc, char *argv[]) {

	char *device;
	krad_v4l2_t *kradv4l2;
	
	if (argc < 2) {
		device = DEFAULT_DEVICE;
		printf("no device provided, using %s , to provide a device, example: krad_v4l2_test /dev/video0\n", device);
	} else {
		device = argv[1];
		printf("Using %s\n", device);
	}


	kradv4l2 = kradv4l2_create();

	kradv4l2_open(kradv4l2, device, 640, 480);

	//usleep(2000000);

	//kradv4l2_read_frames (kradv4l2);
	
	kradv4l2_start_capturing (kradv4l2);
	
//	kradv4l2_read_frame_wait (kradv4l2);


	usleep(400000);
	

	kradv4l2_stop_capturing (kradv4l2);

	kradv4l2_close(kradv4l2);

	kradv4l2_destroy(kradv4l2);
	
	printf("It worked\n");
	
	return 0;

}
