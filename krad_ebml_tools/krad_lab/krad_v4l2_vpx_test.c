#include <krad_vpx.h>
#include <krad_v4l2.h>

#define DEFAULT_DEVICE "/dev/video0"

#define TEST_COUNT 100


int main (int argc, char *argv[]) {

	krad_vpx_encoder_t *krad_vpx_encoder;
	//krad_vpx_decoder_t *krad_vpx_decoder;

	char *device;
	krad_v4l2_t *kradv4l2;

	int width;
	int height;
	int count;
	int keyframe;
	
	width = 1280;
	height = 720;
	count = 0;
	
	void *frame = NULL;
	void *vpx_packet;
	int packet_size;
	
	if (argc < 2) {
		device = DEFAULT_DEVICE;
		printf("no device provided, using %s , to provide a device, example: krad_v4l2_test /dev/video0\n", device);
	} else {
		device = argv[1];
		printf("Using %s\n", device);
	}


	/*
	while (count < TEST_COUNT) {

		krad_vpx_decoder = krad_vpx_decoder_create();
		krad_vpx_decoder_destroy(krad_vpx_decoder);

		krad_vpx_encoder = krad_vpx_encoder_create(width, height);
		krad_vpx_encoder_destroy(krad_vpx_encoder);
	
		count++;
	
	}
	*/

	krad_vpx_encoder = krad_vpx_encoder_create(width, height);

	kradv4l2 = kradv4l2_create();

	kradv4l2_open(kradv4l2, device, width, height);
	
	kradv4l2_start_capturing (kradv4l2);
	
	while (count < TEST_COUNT) {
		
		if (count > 0) {
			kradv4l2_frame_done (kradv4l2);
		}
		frame = kradv4l2_read_frame_wait (kradv4l2);
		krad_vpx_convert_uyvy2yv12(krad_vpx_encoder->image, frame, width, height);
		krad_vpx_convert_frame_for_local_gl_display(krad_vpx_encoder);
		count++;
		printf("count is %d\n", count);
		packet_size = krad_vpx_encoder_write(krad_vpx_encoder, &vpx_packet, &keyframe);
		printf("packet size was %d\n", packet_size);

	}

	kradv4l2_stop_capturing (kradv4l2);

	kradv4l2_close(kradv4l2);

	kradv4l2_destroy(kradv4l2);
	
	krad_vpx_encoder_destroy(krad_vpx_encoder);
	
	return 0;

}



