#include <krad_vpx.h>
#include <krad_v4l2.h>
#include <krad_ebml.h>

#define DEFAULT_DEVICE "/dev/video0"
#define APPVERSION "Krad V4L2 VPX EBML TEST 0.2"
#define TEST_COUNT 75


int main (int argc, char *argv[]) {

	krad_vpx_encoder_t *krad_vpx_encoder;
	//krad_vpx_decoder_t *krad_vpx_decoder;
	krad_ebml_t *ebml;

	char *filename = "/home/oneman/kode/testfilex.webm";
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
	unsigned char *vpx_packet;
	int packet_size;
	int videotrack;
	
	if (argc < 2) {
		device = DEFAULT_DEVICE;
		printf("no device provided, using %s , to provide a device, example: krad_v4l2_test /dev/video0\n", device);
	} else {
		device = argv[1];
		printf("Using %s\n", device);
	}

	ebml = kradebml_create();
	//kradebml_open_output_stream(ebml, "192.168.1.2", 9080, "/teststream.webm", "secretkode");
	kradebml_open_output_file(ebml, filename);
	kradebml_header(ebml, "webm", APPVERSION);
	videotrack = kradebml_add_video_track(ebml, "V_VP8", 10, width, height);
	kradebml_write(ebml);
	
	krad_vpx_encoder = krad_vpx_encoder_create(width, height);

	kradv4l2 = kradv4l2_create();

	kradv4l2_open(kradv4l2, device, width, height);
	
	kradv4l2_start_capturing (kradv4l2);
	
	while (count < TEST_COUNT) {
		
		if (count > 0) {
		//	kradv4l2_frame_done (kradv4l2);
		}
		frame = kradv4l2_read_frame_wait (kradv4l2);
		krad_vpx_convert_uyvy2yv12(krad_vpx_encoder->image, frame, width, height);
		kradv4l2_frame_done (kradv4l2);
		//krad_vpx_convert_frame_for_local_gl_display(krad_vpx_encoder);
		count++;
		printf("count is %d\n", count);
		packet_size = krad_vpx_encoder_write(krad_vpx_encoder, &vpx_packet, &keyframe);
		printf("packet size was %d\n", packet_size);

		if (packet_size) {
			kradebml_add_video(ebml, videotrack, vpx_packet, packet_size, keyframe);
			kradebml_write(ebml);
		}
		//exit(0);

	}
	
	kradebml_destroy(ebml);

	kradv4l2_stop_capturing (kradv4l2);

	kradv4l2_close(kradv4l2);

	kradv4l2_destroy(kradv4l2);
	
	krad_vpx_encoder_destroy(krad_vpx_encoder);
	
	return 0;

}



