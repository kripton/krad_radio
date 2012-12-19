#include "krad_v4l2.h"

krad_v4l2_t *krad_v4l2_create() {

	krad_v4l2_t *krad_v4l2;

	krad_v4l2 = calloc(1, sizeof(krad_v4l2_t));
	krad_v4l2->mode = V4L2_PIX_FMT_YUYV;

	return krad_v4l2;
}

void krad_v4l2_destroy(krad_v4l2_t *krad_v4l2) {
  krad_v4l2_free_codec_buffer (krad_v4l2);
  free ( krad_v4l2 );
}

int krad_v4l2_is_h264_keyframe (unsigned char *buffer) {

  unsigned int nal_type = buffer[4] & 0x1F;

  if ((nal_type == 5) || (nal_type == 7) || (nal_type == 8)) {
    return 1;
  }
  return 0;
}

static void krad_v4l2_uvc_h264_set_bitrate (krad_v4l2_t *krad_v4l2, int bmin) {

  int bmax = bmin;
  int res;
  struct uvc_xu_control_query ctrl;
  uvcx_bitrate_layers_t  conf;
  ctrl.unit = 12;
  ctrl.size = 10;
  ctrl.selector = UVCX_BITRATE_LAYERS;
  ctrl.data = (unsigned char*)&conf;
  ctrl.query = UVC_GET_CUR;
  conf.wLayerID = 0;
  conf.dwPeakBitrate = conf.dwAverageBitrate = 0;
  res = xioctl(krad_v4l2->fd, UVCIOC_CTRL_QUERY, &ctrl);
  if (res)
    {
      printke("ctrl_query");
      return;
    }
  printk("Krad V4L2: get before br %d %d", conf.dwPeakBitrate, conf.dwAverageBitrate);
  conf.dwPeakBitrate = bmax;
  conf.dwAverageBitrate = bmin;
  ctrl.query = UVC_SET_CUR;
  res = xioctl(krad_v4l2->fd, UVCIOC_CTRL_QUERY, &ctrl);
  if (res)
    {
      printke("ctrl_query");
      return;
    }
  printk("Krad V4L2: set br %d %d", conf.dwPeakBitrate, conf.dwAverageBitrate);
  ctrl.query = UVC_GET_CUR;
  res = xioctl(krad_v4l2->fd, UVCIOC_CTRL_QUERY, &ctrl);
  if (res)
    {
      printke("ctrl_query");
      return;
    }
  printk("Krad V4L2: get after br %d %d", conf.dwPeakBitrate, conf.dwAverageBitrate);
}

/*
static void krad_v4l2_uvc_h264_reset (krad_v4l2_t *krad_v4l2) {

	int res;
	struct uvc_xu_control_query ctrl;
	uvcx_encoder_reset conf;
	ctrl.selector = UVCX_ENCODER_RESET;
	ctrl.size = 2;
	ctrl.unit = 12;
	ctrl.data = (unsigned char*)&conf;
	ctrl.query = UVC_SET_CUR;
	conf.wLayerID = 0;
	res = xioctl(krad_v4l2->fd, UVCIOC_CTRL_QUERY, &ctrl);
	if (res) { printke("uvc_h264_reset"); return;}

}
*/

static void krad_v4l2_uvc_h264_set_rc_mode (krad_v4l2_t *krad_v4l2, int mode) {

	int res;
	struct uvc_xu_control_query ctrl;

	uvcx_rate_control_mode_t conf;
	ctrl.selector = UVCX_RATE_CONTROL_MODE;
	ctrl.size = 3;
	ctrl.unit = 12;
	ctrl.data = (unsigned char*)&conf;
	ctrl.query = UVC_GET_CUR;
	conf.wLayerID = 0;
	res = xioctl(krad_v4l2->fd, UVCIOC_CTRL_QUERY, &ctrl);
	if (res) { printke("query"); return;}
	printk("Krad V4L2: mode %d\n", (int)conf.bRateControlMode);
	conf.bRateControlMode = mode;
	ctrl.query = UVC_SET_CUR;
	res = xioctl(krad_v4l2->fd, UVCIOC_CTRL_QUERY, &ctrl);
	if (res) { printke("query"); return;}
	printk("Krad V4L2: mode %d\n", (int)conf.bRateControlMode);
	ctrl.query = UVC_GET_CUR;
	res = xioctl(krad_v4l2->fd, UVCIOC_CTRL_QUERY, &ctrl);
	if (res) { printke("query"); return;}
	printk("Krad V4L2: mode %d\n", (int)conf.bRateControlMode);
}

void krad_v4l2_uvc_h264_keyframe_req (krad_v4l2_t *krad_v4l2) {

	struct uvc_xu_control_query ctrl;

	uvcx_picture_type_control_t conf;
	ctrl.selector = UVCX_PICTURE_TYPE_CONTROL;
	ctrl.size = 4;
	ctrl.unit = 12;
	ctrl.data = (unsigned char*)&conf;
	conf.wLayerID = 0;
	conf.wPicType = 0x01;
	ctrl.query = UVC_SET_CUR;
	xioctl(krad_v4l2->fd, UVCIOC_CTRL_QUERY, &ctrl);
}

void krad_v4l2_frame_done (krad_v4l2_t *krad_v4l2) {
  if (-1 == xioctl (krad_v4l2->fd, VIDIOC_QBUF, &krad_v4l2->buf)) {
    errno_exit ("Krad V4L2: VIDIOC_QBUF");
  }
}

char *krad_v4l2_read (krad_v4l2_t *krad_v4l2) {

  krad_v4l2->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  krad_v4l2->buf.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl (krad_v4l2->fd, VIDIOC_DQBUF, &krad_v4l2->buf)) {
    switch (errno) {
      case EAGAIN:
        return 0;
      case EIO:
      default:
        errno_exit ("Krad V4L2: VIDIOC_DQBUF");
    }
  }

  krad_v4l2->timestamp = krad_v4l2->buf.timestamp;
  krad_v4l2->encoded_size = krad_v4l2->buf.bytesused;

  return krad_v4l2->buffers[krad_v4l2->buf.index].start;
}

void krad_v4l2_start_capturing (krad_v4l2_t *krad_v4l2) {
        
  unsigned int i;
  enum v4l2_buf_type type;

  for (i = 0; i < krad_v4l2->n_buffers; ++i) {

    struct v4l2_buffer buf;

    CLEAR (buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;

    if (-1 == xioctl (krad_v4l2->fd, VIDIOC_QBUF, &buf)) {
      errno_exit ("Krad V4L2: VIDIOC_QBUF");
    }
  }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl (krad_v4l2->fd, VIDIOC_STREAMON, &type)) {
    errno_exit ("Krad V4L2: VIDIOC_STREAMON");
  }
		

	if (krad_v4l2->mode == V4L2_PIX_FMT_H264) {
		//krad_v4l2_uvc_h264_reset (krad_v4l2);
		krad_v4l2_uvc_h264_set_rc_mode (krad_v4l2, RATECONTROL_VBR);
		krad_v4l2_uvc_h264_set_bitrate (krad_v4l2, 300);
		printk ("Krad V4L2: Set H264 mode to VBR");
	}
}

void krad_v4l2_stop_capturing (krad_v4l2_t *krad_v4l2) {

  enum v4l2_buf_type type;

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl (krad_v4l2->fd, VIDIOC_STREAMOFF, &type)) {
    errno_exit ("Krad V4L2: VIDIOC_STREAMOFF");
  }
}

void krad_v4l2_uninit_device (krad_v4l2_t *krad_v4l2) {

  unsigned int i;

  for (i = 0; i < krad_v4l2->n_buffers; ++i)
		if (-1 == munmap (krad_v4l2->buffers[i].start, krad_v4l2->buffers[i].length))
					errno_exit ("Krad V4L2: munmap");

  free (krad_v4l2->buffers);
}

void krad_v4l2_init_mmap (krad_v4l2_t *krad_v4l2) {

  struct v4l2_requestbuffers req;

  CLEAR (req);

  req.count = 24;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (krad_v4l2->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
		    failfast ("Krad V4L2: %s does not support memory mapping", krad_v4l2->device);
		} else {
		        errno_exit ("Krad V4L2: VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		failfast ("Krad V4L2: Insufficient buffer memory on %s\n", krad_v4l2->device);
	}

	printk ("Krad V4L2: v4l2 says %d buffers", req.count);

	krad_v4l2->buffers = calloc (req.count, sizeof (*krad_v4l2->buffers));

	if (!krad_v4l2->buffers) {
		failfast ("Krad V4L2: Out of memory");
	}

	for (krad_v4l2->n_buffers = 0; krad_v4l2->n_buffers < req.count; ++krad_v4l2->n_buffers) {

		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = krad_v4l2->n_buffers;

		if (-1 == xioctl (krad_v4l2->fd, VIDIOC_QUERYBUF, &buf)) {
			errno_exit ("Krad V4L2: VIDIOC_QUERYBUF");
		}
		
		krad_v4l2->buffers[krad_v4l2->n_buffers].length = buf.length;
		krad_v4l2->buffers[krad_v4l2->n_buffers].offset = buf.m.offset;
		
		krad_v4l2->buffers[krad_v4l2->n_buffers].start = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, krad_v4l2->fd, buf.m.offset);

		if (MAP_FAILED == krad_v4l2->buffers[krad_v4l2->n_buffers].start) {
			errno_exit ("Krad V4L2: mmap");
		}
	}

	krad_v4l2->n_buffers = req.count;
}

void krad_v4l2_init_device (krad_v4l2_t *krad_v4l2) {

	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;

  if (-1 == xioctl (krad_v4l2->fd, VIDIOC_QUERYCAP, &cap)) {
    if (EINVAL == errno) {
      failfast ("Krad V4L2: %s is no V4L2 device", krad_v4l2->device);
    } else {
      errno_exit ("Krad V4L2: VIDIOC_QUERYCAP");
    }
  }

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    failfast ("Krad V4L2: %s is no video capture device", krad_v4l2->device);
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		failfast ("Krad V4L2: %s does not support streaming i/o", krad_v4l2->device);
	}

	/* Select video input, video standard and tune here. */

	CLEAR (cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (0 == xioctl (krad_v4l2->fd, VIDIOC_CROPCAP, &cropcap)) {

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if (-1 == xioctl (krad_v4l2->fd, VIDIOC_S_CROP, &crop)) {
      switch (errno) {
        case EINVAL:
          /* Cropping not supported. */
          break;
        default:
        /* Errors ignored. */
          break;
      }
    }
  } else {	
    /* Errors ignored. */
  }

	CLEAR (fmt);

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = krad_v4l2->width; 
	fmt.fmt.pix.height      = krad_v4l2->height;
	fmt.fmt.pix.bytesperline = krad_v4l2->width;
	fmt.fmt.pix.sizeimage = 96000;
	
	fmt.fmt.pix.pixelformat = krad_v4l2->mode;
	fmt.fmt.pix.field       = V4L2_FIELD_ANY;

	if (-1 == xioctl (krad_v4l2->fd, VIDIOC_S_FMT, &fmt)) {
		errno_exit ("Krad V4L2: VIDIOC_S_FMT");
	}

	char fourcc[5];
	fourcc[4] = '\0';
	memcpy(&fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);

	printkd ("Krad V4L2: %ux%u FMT %s Stride: %u Size: %u", fmt.fmt.pix.width, fmt.fmt.pix.height, fourcc, 
														fmt.fmt.pix.bytesperline, fmt.fmt.pix.sizeimage);

	krad_v4l2->width = fmt.fmt.pix.width; 
	krad_v4l2->height = fmt.fmt.pix.height;

	struct v4l2_streamparm stream_parameters;

	CLEAR (stream_parameters);
	stream_parameters.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (krad_v4l2->fd, VIDIOC_G_PARM, &stream_parameters)) {
		errno_exit ("Krad V4L2: VIDIOC_G_PARM");
	}

	printkd ("Krad V4L2: G Frameinterval %u/%u", stream_parameters.parm.capture.timeperframe.numerator, 
										  stream_parameters.parm.capture.timeperframe.denominator);
	
	stream_parameters.parm.capture.timeperframe.numerator = 1;
	stream_parameters.parm.capture.timeperframe.denominator = krad_v4l2->fps;

	if (-1 == xioctl (krad_v4l2->fd, VIDIOC_S_PARM, &stream_parameters)) {
    printke ("Krad V4L2: unable to set stream parameters as speced");
    printke ("Krad V4L2: error %d, %s", errno, strerror (errno));
	}

	printkd ("Krad V4L2: S Frameinterval %u/%u", stream_parameters.parm.capture.timeperframe.numerator, 
										  stream_parameters.parm.capture.timeperframe.denominator);
									  
	if (stream_parameters.parm.capture.timeperframe.denominator != krad_v4l2->fps) {
		printkd ("Krad V4L2: failed to get proper capture fps!");
	}					

	krad_v4l2_init_mmap (krad_v4l2);
}

void errno_exit (const char *s) {
  failfast ("%s error %d, %s", s, errno, strerror (errno));
}

int xioctl (int fd, int request, void *arg) {

  int r;

  do r = ioctl (fd, request, arg);
  while (-1 == r && EINTR == errno);

  return r;

}

void krad_v4l2_close (krad_v4l2_t *krad_v4l2) {
  krad_v4l2_uninit_device (krad_v4l2);
  close (krad_v4l2->fd);
}

void krad_v4l2_open (krad_v4l2_t *krad_v4l2, char *device, int width, int height, int fps) {

	struct stat st; 

	strncpy(krad_v4l2->device, device, 512);

	krad_v4l2->width = width;
	krad_v4l2->height = height;
	krad_v4l2->fps = fps;
	
	if (-1 == stat (krad_v4l2->device, &st)) {
		failfast ("Krad V4L2: Cannot identify '%s': %d, %s", krad_v4l2->device, errno, strerror (errno));
	}

	if (!S_ISCHR (st.st_mode)) {
		failfast ("Krad V4L2: %s is no device", krad_v4l2->device);
	}

	krad_v4l2->fd = open (krad_v4l2->device, O_RDWR | O_NONBLOCK, 0);

	if (-1 == krad_v4l2->fd) {
		failfast ("Krad V4L2: Cannot open '%s': %d, %s", krad_v4l2->device, errno, strerror (errno));
	}
	
	krad_v4l2_init_device (krad_v4l2);
	
}

void krad_v4l2_alloc_codec_buffer (krad_v4l2_t *krad_v4l2) {
	if (krad_v4l2->codec_buffer == NULL) {
		krad_v4l2->codec_buffer = calloc(1, 4200000);
	}
}

void krad_v4l2_free_codec_buffer (krad_v4l2_t *krad_v4l2) {
	if (krad_v4l2->jpeg_dec != NULL) {
		tjDestroy ( krad_v4l2->jpeg_dec );
		krad_v4l2->jpeg_dec = NULL;
	}
	if (krad_v4l2->codec_buffer != NULL) {
		free ( krad_v4l2->codec_buffer );
		krad_v4l2->codec_buffer = NULL;
	}
}

void krad_v4l2_yuv_mode (krad_v4l2_t *krad_v4l2) {
	krad_v4l2->mode = V4L2_PIX_FMT_YUYV;
	krad_v4l2_free_codec_buffer (krad_v4l2);	
}

void krad_v4l2_mjpeg_mode (krad_v4l2_t *krad_v4l2) {
	if (krad_v4l2->jpeg_dec == NULL) {
		krad_v4l2->jpeg_dec = tjInitDecompress();
	}
	krad_v4l2_alloc_codec_buffer (krad_v4l2);
	krad_v4l2->mode = V4L2_PIX_FMT_MJPEG;
}

void krad_v4l2_h264_mode (krad_v4l2_t *krad_v4l2) {
	if (krad_v4l2->jpeg_dec != NULL) {
		tjDestroy ( krad_v4l2->jpeg_dec );
		krad_v4l2->jpeg_dec = NULL;
	}
	krad_v4l2_alloc_codec_buffer (krad_v4l2);
	krad_v4l2->mode = V4L2_PIX_FMT_H264;
}


static char jpeg_header[] =
{
0xff, 0xd8,                  // SOI
0xff, 0xe0,                  // APP0
0x00, 0x10,                  // APP0 Hdr size
0x4a, 0x46, 0x49, 0x46, 0x00, // ID string
0x01, 0x01,                  // Version
0x00,                      // Bits per type
0x00, 0x00,                  // X density
0x00, 0x00,                  // Y density
0x00,                      // X Thumbnail size
0x00                      // Y Thumbnail size
};

static char mjpg_dht[0x1A4] =
{
/* JPEG DHT Segment for YCrCb omitted from MJPG data */
0xFF, 0xC4, 0x01, 0xA2,
0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00,
0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61,
0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24,
0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34,
0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56,
0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,
0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9,
0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
0xF8, 0xF9, 0xFA, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01,
0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62,
0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A,
0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56,
0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8,
0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
0xF9, 0xFA
};


void krad_v4l2_mjpeg_to_rgb (krad_v4l2_t *krad_v4l2, unsigned char *argb_buffer, unsigned char *mjpeg_buffer, unsigned int mjpeg_size) {

	unsigned long jpeg_size_long;
	int stride;
	int ret;
	
	jpeg_size_long = mjpeg_size;
	stride = krad_v4l2->width * 4;
	
	int jpg_hedsize = sizeof(jpeg_header);
	int hufsize = sizeof(mjpg_dht);
 
 	//printf("sizeof jpeghed %d \n", jpg_hedsize);
 	//printf("sizeof huf %d \n", hufsize);
 
	memcpy (krad_v4l2->codec_buffer, jpeg_header, jpg_hedsize);

 	jpeg_size_long += jpg_hedsize;
 	jpeg_size_long += hufsize;
 	
	//removing avi header
	int tmp = *((char *)mjpeg_buffer + 4);
	// 	printf("sizeof temp %d \n", tmp);
	tmp <<= 8;
	//	printf("sizeof temp %d \n", tmp);
	tmp += *((char *)mjpeg_buffer + 5) + 4;
 	
 	//printf("sizeof temp %d \n", tmp);

 	int x = 0;
 	
 	while (!((mjpeg_buffer[tmp + x] == 0xFF) && (mjpeg_buffer[tmp + x + 1] == 0xDA))) {
 		x++;
 	}

 	//printf("got %d for X!\n", x);

	memcpy (krad_v4l2->codec_buffer + jpg_hedsize, mjpeg_buffer + tmp, x);
	memcpy (krad_v4l2->codec_buffer + jpg_hedsize + x, mjpg_dht, hufsize);
	memcpy (krad_v4l2->codec_buffer + jpg_hedsize + x + hufsize, mjpeg_buffer + tmp + x, mjpeg_size - tmp - x);
 	
 	jpeg_size_long = jpg_hedsize + x + hufsize + (mjpeg_size - tmp - x);
 	
	ret = tjDecompress2 ( krad_v4l2->jpeg_dec, krad_v4l2->codec_buffer, jpeg_size_long, argb_buffer, krad_v4l2->width, 
						  stride, krad_v4l2->height, TJPF_BGRA, 0 );

	if (ret != 0) {
		printke ("JPEG decoding error: %s\n", tjGetErrorStr());
	}
}


int krad_v4l2_mjpeg_to_jpeg (krad_v4l2_t *krad_v4l2, unsigned char *jpeg_buffer, unsigned char *mjpeg_buffer, unsigned int mjpeg_size) {

	unsigned long jpeg_size_long;
	
	jpeg_size_long = mjpeg_size;

	
	int jpg_hedsize = sizeof(jpeg_header);
	int hufsize = sizeof(mjpg_dht);
 
	memcpy (jpeg_buffer, jpeg_header, jpg_hedsize);
 	jpeg_size_long += jpg_hedsize;
 	jpeg_size_long += hufsize;

	//removing avi header
	int tmp = *((char *)mjpeg_buffer + 4);
	// 	printf("sizeof temp %d \n", tmp);
	tmp <<= 8;
	//	printf("sizeof temp %d \n", tmp);
	tmp += *((char *)mjpeg_buffer + 5) + 4;
 	
 	//printf("sizeof temp %d \n", tmp);

 	int x = 0;
 	
 	while (!((mjpeg_buffer[tmp + x] == 0xFF) && (mjpeg_buffer[tmp + x + 1] == 0xDA))) {
 		x++;
 	}

 	//printf("got %d for X!\n", x);

	memcpy (jpeg_buffer + jpg_hedsize, mjpeg_buffer + tmp, x);
	memcpy (jpeg_buffer + jpg_hedsize + x, mjpg_dht, hufsize);
	memcpy (jpeg_buffer + jpg_hedsize + x + hufsize, mjpeg_buffer + tmp + x, mjpeg_size - tmp - x);
 	
 	jpeg_size_long = jpg_hedsize + x + hufsize + (mjpeg_size - tmp - x);
 	
 	
 	return jpeg_size_long;
}


int krad_v4l2_detect_devices () {

	DIR *dp;
	struct dirent *ep;
	int count;
	
	count = 0;

	dp = opendir ("/dev");
	
	if (dp == NULL) {
		printke ("Couldn't open the /dev directory");
		return 0;
	}
	
	while ((ep = readdir(dp))) {
		if (memcmp(ep->d_name, "video", 5) == 0) {
			printk("Found V4L2 Device: /dev/%s", ep->d_name);
			count++;
		}
	}
	closedir (dp);

	return count;

}

int krad_v4l2_get_device_filename (int device_num, char *device_name) {

	DIR *dp;
	struct dirent *ep;
	int count;
	
	count = 0;

	dp = opendir ("/dev");
	
	if (dp == NULL) {
		printke ("Couldn't open the /dev directory");
		return 0;
	}
	
	while ((ep = readdir(dp))) {
		if (memcmp(ep->d_name, "video", 5) == 0) {
			if (count == device_num) {
				sprintf (device_name, "/dev/%s", ep->d_name);
				closedir (dp);
				return 1;
			}
			count++;
		}
	}
	closedir (dp);

	return 0;

}

