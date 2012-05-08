#include "krad_v4l2.h"


krad_v4l2_t *kradv4l2_create() {

	krad_v4l2_t *kradv4l2;

	kradv4l2 = calloc(1, sizeof(krad_v4l2_t));

	kradv4l2->jpeg_dec = tjInitDecompress();

	kradv4l2->jpeg_buffer = calloc(1, 4200000);

	return kradv4l2;

}

void kradv4l2_destroy(krad_v4l2_t *kradv4l2) {

	tjDestroy ( kradv4l2->jpeg_dec );
	free ( kradv4l2->jpeg_buffer );
	free ( kradv4l2 );

}

void kradv4l2_frame_done (krad_v4l2_t *kradv4l2) {
	if (-1 == xioctl (kradv4l2->fd, VIDIOC_QBUF, &kradv4l2->buf)) {
		errno_exit ("VIDIOC_QBUF");
	}
}
			
char *kradv4l2_read_frame_adv (krad_v4l2_t *kradv4l2) {
		

			kradv4l2->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			kradv4l2->buf.memory = V4L2_MEMORY_MMAP;

			if (-1 == xioctl (kradv4l2->fd, VIDIOC_DQBUF, &kradv4l2->buf)) {
				switch (errno) {
		    		case EAGAIN:
		            	return 0;

					case EIO:
		
					default:
						errno_exit ("VIDIOC_DQBUF");
				}
			}

			kradv4l2->timestamp = kradv4l2->buf.timestamp;
			
		//	printf("\n\ntimestamp %zu %zu \n\n", kradv4l2->timestamp.tv_sec, kradv4l2->timestamp.tv_usec);

			
			kradv4l2->jpeg_size = kradv4l2->buf.bytesused;

			
			//printf("\n\njpeg size %u \n\n", kradv4l2->jpeg_size);

			
			return kradv4l2->buffers[kradv4l2->buf.index].start;
}

char *kradv4l2_read_frame (krad_v4l2_t *kradv4l2) {
		
	unsigned int i;

	switch (kradv4l2->io) {
		
		case IO_METHOD_MMAP:

			CLEAR (kradv4l2->buf);

			kradv4l2->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			kradv4l2->buf.memory = V4L2_MEMORY_MMAP;

			if (-1 == xioctl (kradv4l2->fd, VIDIOC_DQBUF, &kradv4l2->buf)) {
				switch (errno) {
		    		case EAGAIN:
		            	return 0;

					case EIO:
		
					default:
						errno_exit ("VIDIOC_DQBUF");
				}
			}

//            assert (kradv4l2->buf.index < kradv4l2->n_buffers);

			return kradv4l2->buffers[kradv4l2->buf.index].start;
//	        process_image (kradcapture, kradv4l2->buffers[buf.index].start);

			if (-1 == xioctl (kradv4l2->fd, VIDIOC_QBUF, &kradv4l2->buf)) {
				errno_exit ("VIDIOC_QBUF");
			}
			
			break;

		case IO_METHOD_USERPTR:
		
			CLEAR (kradv4l2->buf);

			kradv4l2->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			kradv4l2->buf.memory = V4L2_MEMORY_USERPTR;

			if (-1 == xioctl (kradv4l2->fd, VIDIOC_DQBUF, &kradv4l2->buf)) {

				switch (errno) {
					case EAGAIN:
						return 0;

					case EIO:

					default:
						errno_exit ("VIDIOC_DQBUF");
					}
			}

			for (i = 0; i < kradv4l2->n_buffers; ++i) {
			
				if (kradv4l2->buf.m.userptr == (unsigned long) kradv4l2->buffers[i].start && kradv4l2->buf.length == kradv4l2->buffers[i].length) {
					break;
				}
			}

//			assert (i < kradv4l2->n_buffers);

//			process_image (kradcapture, (void *) buf.m.userptr);

			if (-1 == xioctl (kradv4l2->fd, VIDIOC_QBUF, &kradv4l2->buf)) {
				errno_exit ("VIDIOC_QBUF");
			}

			break;
	
		case IO_METHOD_READ:
    		
    		if (-1 == read (kradv4l2->fd, kradv4l2->buffers[0].start, kradv4l2->buffers[0].length)) {
				
				switch (errno) {
            		case EAGAIN:
                    	return 0;

					case EIO:

					default:
						errno_exit ("read");
				}
			}

//			process_image (kradcapture, kradv4l2->buffers[0].start);

			break;
	
	}

	return NULL;
}

void kradv4l2_read_frames (krad_v4l2_t *kradv4l2) {

	while(1) {

		for (;;) {

		    fd_set fds;
		    struct timeval tv;
		    int r;

		    FD_ZERO (&fds);
		    FD_SET (kradv4l2->fd, &fds);

		    /* Timeout. */
		    tv.tv_sec = 6;
		    tv.tv_usec = 0;

		    r = select (kradv4l2->fd + 1, &fds, NULL, NULL, &tv);

		    if (-1 == r) {
		        if (EINTR == errno) {
					continue;
		        }

				errno_exit ("select");
			}

		    if (0 == r) {
				fprintf (stderr, "select timeout\n");
				exit (EXIT_FAILURE);
		    }

			if (kradv4l2_read_frame (kradv4l2)) {
				break;
			}

		}
	}
}

char *kradv4l2_read_frame_wait_adv (krad_v4l2_t *kradv4l2) {

    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO (&fds);
    FD_SET (kradv4l2->fd, &fds);

    /* Timeout. */
    tv.tv_sec = 6;
    tv.tv_usec = 0;

    r = select (kradv4l2->fd + 1, &fds, NULL, NULL, &tv);

    if (-1 == r) {
        if (EINTR == errno) {
        	// retry?
        	return NULL;
        }

		errno_exit ("select");
	}

    if (0 == r) {
		fprintf (stderr, "select timeout\n");
		exit (EXIT_FAILURE);
    }

	return kradv4l2_read_frame_adv (kradv4l2);

}

char *kradv4l2_read_frame_wait (krad_v4l2_t *kradv4l2) {

    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO (&fds);
    FD_SET (kradv4l2->fd, &fds);

    /* Timeout. */
    tv.tv_sec = 6;
    tv.tv_usec = 0;

    r = select (kradv4l2->fd + 1, &fds, NULL, NULL, &tv);

    if (-1 == r) {
        if (EINTR == errno) {
        	// retry?
        	return NULL;
        }

		errno_exit ("select");
	}

    if (0 == r) {
		fprintf (stderr, "select timeout\n");
		exit (EXIT_FAILURE);
    }

	return kradv4l2_read_frame (kradv4l2);

}

void kradv4l2_start_capturing (krad_v4l2_t *kradv4l2) {
        
    unsigned int i;
    enum v4l2_buf_type type;

	switch (kradv4l2->io) {
	
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < kradv4l2->n_buffers; ++i) {

			struct v4l2_buffer buf;

    		CLEAR (buf);

    		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    		buf.memory      = V4L2_MEMORY_MMAP;
    		buf.index       = i;

    		if (-1 == xioctl (kradv4l2->fd, VIDIOC_QBUF, &buf)) {
				errno_exit ("VIDIOC_QBUF");
			}
		}
		
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (kradv4l2->fd, VIDIOC_STREAMON, &type)) {
			errno_exit ("VIDIOC_STREAMON");
		}
		
		break;

	case IO_METHOD_USERPTR:
	
		for (i = 0; i < kradv4l2->n_buffers; ++i) {

			struct v4l2_buffer buf;

    		CLEAR (buf);

    		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    		buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) kradv4l2->buffers[i].start;
			buf.length = kradv4l2->buffers[i].length;

			if (-1 == xioctl (kradv4l2->fd, VIDIOC_QBUF, &buf)) {
				errno_exit ("VIDIOC_QBUF");
			}
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (kradv4l2->fd, VIDIOC_STREAMON, &type)) {
			errno_exit ("VIDIOC_STREAMON");
		}

		break;
	}
}

void kradv4l2_stop_capturing (krad_v4l2_t *kradv4l2) {

	enum v4l2_buf_type type;

	switch (kradv4l2->io) {
		case IO_METHOD_READ:
			/* Nothing to do. */
			break;

		case IO_METHOD_MMAP:
		
		case IO_METHOD_USERPTR:
		
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			if (-1 == xioctl (kradv4l2->fd, VIDIOC_STREAMOFF, &type)) {
				errno_exit ("VIDIOC_STREAMOFF");
			}

			break;
	}
}

void kradv4l2_uninit_device (krad_v4l2_t *kradv4l2) {

	unsigned int i;

	switch (kradv4l2->io) {
		case IO_METHOD_READ:
			free (kradv4l2->buffers[0].start);
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < kradv4l2->n_buffers; ++i)
				if (-1 == munmap (kradv4l2->buffers[i].start, kradv4l2->buffers[i].length))
					errno_exit ("munmap");
			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < kradv4l2->n_buffers; ++i)
				free (kradv4l2->buffers[i].start);
			break;
	}

	free (kradv4l2->buffers);
}

void kradv4l2_init_read (krad_v4l2_t *kradv4l2, unsigned int buffer_size) {

    kradv4l2->buffers = calloc (1, sizeof (*kradv4l2->buffers));

    if (!kradv4l2->buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
    }

	kradv4l2->buffers[0].length = buffer_size;
	kradv4l2->buffers[0].start = malloc (buffer_size);

	if (!kradv4l2->buffers[0].start) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
}

void kradv4l2_init_mmap (krad_v4l2_t *kradv4l2) {

	struct v4l2_requestbuffers req;

    CLEAR (req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (kradv4l2->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
		    fprintf (stderr, "%s does not support memory mapping\n", kradv4l2->device);
		    exit (EXIT_FAILURE);
		} else {
		        errno_exit ("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n", kradv4l2->device);
		exit (EXIT_FAILURE);
	}

	kradv4l2->buffers = calloc (req.count, sizeof (*kradv4l2->buffers));

	if (!kradv4l2->buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (kradv4l2->n_buffers = 0; kradv4l2->n_buffers < req.count; ++kradv4l2->n_buffers) {

		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = kradv4l2->n_buffers;

		if (-1 == xioctl (kradv4l2->fd, VIDIOC_QUERYBUF, &buf)) {
			errno_exit ("VIDIOC_QUERYBUF");
		}
		
		kradv4l2->buffers[kradv4l2->n_buffers].length = buf.length;
		kradv4l2->buffers[kradv4l2->n_buffers].start = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, kradv4l2->fd, buf.m.offset);

		if (MAP_FAILED == kradv4l2->buffers[kradv4l2->n_buffers].start) {
			errno_exit ("mmap");
		}
	}
}

void kradv4l2_init_userp (krad_v4l2_t *kradv4l2, unsigned int buffer_size) {

	struct v4l2_requestbuffers req;
	unsigned int page_size;

    page_size = getpagesize();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

    CLEAR (req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl (kradv4l2->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support user pointer i/o\n", kradv4l2->device);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
    }

    kradv4l2->buffers = calloc (4, sizeof (*kradv4l2->buffers));

    if (!kradv4l2->buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
    }

    for (kradv4l2->n_buffers = 0; kradv4l2->n_buffers < 4; ++kradv4l2->n_buffers) {

		kradv4l2->buffers[kradv4l2->n_buffers].length = buffer_size;
		kradv4l2->buffers[kradv4l2->n_buffers].start = memalign (page_size, buffer_size);

		if (!kradv4l2->buffers[kradv4l2->n_buffers].start) {
			fprintf (stderr, "Out of memory\n");
			exit (EXIT_FAILURE);
		}
	}
}

void kradv4l2_init_device (krad_v4l2_t *kradv4l2) {

	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;

	//unsigned int min;


	kradv4l2->io = IO_METHOD_MMAP;

	if (-1 == xioctl (kradv4l2->fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n", kradv4l2->device);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		    fprintf (stderr, "%s is no video capture device\n", kradv4l2->device);
		    exit (EXIT_FAILURE);
	}

	switch (kradv4l2->io) {
		case IO_METHOD_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
				fprintf (stderr, "%s does not support read i/o\n", kradv4l2->device);
				exit (EXIT_FAILURE);
			}

		break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
				fprintf (stderr, "%s does not support streaming i/o\n", kradv4l2->device);
				exit (EXIT_FAILURE);
			}

			break;
	}


	/* Select video input, video standard and tune here. */

	CLEAR (cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl (kradv4l2->fd, VIDIOC_CROPCAP, &cropcap)) {
		
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl (kradv4l2->fd, VIDIOC_S_CROP, &crop)) {
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
	fmt.fmt.pix.width       = kradv4l2->width; 
	fmt.fmt.pix.height      = kradv4l2->height;
	
	if (kradv4l2->mjpeg_mode) {
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	} else {
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	}
		
	//	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	if (-1 == xioctl (kradv4l2->fd, VIDIOC_S_FMT, &fmt)) {
		errno_exit ("VIDIOC_S_FMT");
	}

	char fourcc[5];
	fourcc[4] = '\0';
	memcpy(&fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);

	printf("V4L2: %ux%u FMT %s Stride: %u Size: %u\n", fmt.fmt.pix.width, fmt.fmt.pix.height, fourcc, 
														fmt.fmt.pix.bytesperline, fmt.fmt.pix.sizeimage);

	struct v4l2_streamparm stream_parameters;

	CLEAR (stream_parameters);
	stream_parameters.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (kradv4l2->fd, VIDIOC_G_PARM, &stream_parameters)) {
		errno_exit ("VIDIOC_G_PARM");
	}

	printf("V4L2: G Frameinterval %u/%u\n", stream_parameters.parm.capture.timeperframe.numerator, 
										  stream_parameters.parm.capture.timeperframe.denominator);
	
	stream_parameters.parm.capture.timeperframe.numerator = 1;
	stream_parameters.parm.capture.timeperframe.denominator = kradv4l2->fps;


	if (-1 == xioctl (kradv4l2->fd, VIDIOC_S_PARM, &stream_parameters)) {
		errno_exit ("VIDIOC_S_PARM");
	}

	printf("V4L2: S Frameinterval %u/%u\n", stream_parameters.parm.capture.timeperframe.numerator, 
										  stream_parameters.parm.capture.timeperframe.denominator);
										  
	if (stream_parameters.parm.capture.timeperframe.denominator != kradv4l2->fps) {
		printf("failed to get proper capture fps!\n");
	}					
	
	

					  

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	/*
	min = fmt.fmt.pix.width * 2;

	if (fmt.fmt.pix.bytesperline < min) {
		fmt.fmt.pix.bytesperline = min;
	}

	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;

	if (fmt.fmt.pix.sizeimage < min) {
		fmt.fmt.pix.sizeimage = min;
	}
	*/
	switch (kradv4l2->io) {
		case IO_METHOD_READ:
			kradv4l2_init_read (kradv4l2, fmt.fmt.pix.sizeimage);
			break;

		case IO_METHOD_MMAP:
			kradv4l2_init_mmap (kradv4l2);
			break;

		case IO_METHOD_USERPTR:
			kradv4l2_init_userp (kradv4l2, fmt.fmt.pix.sizeimage);
			break;
	}
}

void errno_exit (const char *s) {

	fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));
	exit (EXIT_FAILURE);

}

int xioctl (int fd, int request, void *arg) {

	int r;

	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;

}

void kradv4l2_close (krad_v4l2_t *kradv4l2) {

	kradv4l2_uninit_device (kradv4l2);

	close (kradv4l2->fd);

	if (kradv4l2->fd == -1) {
		fprintf (stderr, "error closing device");
		exit (EXIT_FAILURE);
	}

}

void kradv4l2_open (krad_v4l2_t *kradv4l2, char *device, int width, int height, int fps) {

	struct stat st; 

	strncpy(kradv4l2->device, device, 512);

	kradv4l2->width = width;
	kradv4l2->height = height;
	kradv4l2->fps = fps;
	
	if (-1 == stat (kradv4l2->device, &st)) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n", kradv4l2->device, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}

	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is no device\n", kradv4l2->device);
		exit (EXIT_FAILURE);
	}

	kradv4l2->fd = open (kradv4l2->device, O_RDWR | O_NONBLOCK, 0);

	if (-1 == kradv4l2->fd) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n", kradv4l2->device, errno, strerror (errno));
	    exit (EXIT_FAILURE);
	}
	
	kradv4l2_init_device (kradv4l2);
	
	
	
	
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


void kradv4l2_mjpeg_to_rgb (krad_v4l2_t *kradv4l2, unsigned char *argb_buffer, unsigned char *mjpeg_buffer, unsigned int mjpeg_size) {


	unsigned long jpeg_size_long;
	int stride;
	int ret;
	
	//static int count = 1;
	
	jpeg_size_long = mjpeg_size;
	stride = kradv4l2->width * 4;
	
	int jpg_hedsize = sizeof(jpeg_header);
	int hufsize = sizeof(mjpg_dht);
 
 	//printf("sizeof jpeghed %d \n", jpg_hedsize);
 	//printf("sizeof huf %d \n", hufsize);
 
	memcpy (kradv4l2->jpeg_buffer, jpeg_header, jpg_hedsize);


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

	memcpy (kradv4l2->jpeg_buffer + jpg_hedsize, mjpeg_buffer + tmp, x);


 	
	memcpy (kradv4l2->jpeg_buffer + jpg_hedsize + x, mjpg_dht, hufsize);
	memcpy (kradv4l2->jpeg_buffer + jpg_hedsize + x + hufsize, mjpeg_buffer + tmp + x, mjpeg_size - tmp - x);
 	
 	jpeg_size_long = jpg_hedsize + x + hufsize + (mjpeg_size - tmp - x);
 	
	ret = tjDecompress2 ( kradv4l2->jpeg_dec, kradv4l2->jpeg_buffer, jpeg_size_long, argb_buffer, kradv4l2->width, 
						  stride, kradv4l2->height, TJPF_BGRA, 0 );

	if (ret != 0) {
		printf("JPEG decoding error: %s\n", tjGetErrorStr());
	}
	/*
	int fd;
	char filename[512];
	sprintf(filename, "/home/oneman/kode/testfilex_%d.jpg", count++);

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(fd, kradv4l2->jpeg_buffer, jpeg_size_long);
	close(fd);
	
	if (count > 30) {
		exit(0);
	}
	*/
}


void kradv4l2_mjpeg_to_jpeg (krad_v4l2_t *kradv4l2, unsigned char *jpeg_buffer, unsigned char *mjpeg_buffer, unsigned int mjpeg_size) {


	unsigned long jpeg_size_long;
	int stride;
	int ret;
	
	//static int count = 1;
	
	jpeg_size_long = mjpeg_size;
	stride = kradv4l2->width * 4;
	
	int jpg_hedsize = sizeof(jpeg_header);
	int hufsize = sizeof(mjpg_dht);
 
 	//printf("sizeof jpeghed %d \n", jpg_hedsize);
 	//printf("sizeof huf %d \n", hufsize);
 
	memcpy (kradv4l2->jpeg_buffer, jpeg_header, jpg_hedsize);


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

	memcpy (kradv4l2->jpeg_buffer + jpg_hedsize, mjpeg_buffer + tmp, x);


 	
	memcpy (kradv4l2->jpeg_buffer + jpg_hedsize + x, mjpg_dht, hufsize);
	memcpy (kradv4l2->jpeg_buffer + jpg_hedsize + x + hufsize, mjpeg_buffer + tmp + x, mjpeg_size - tmp - x);
 	
 	jpeg_size_long = jpg_hedsize + x + hufsize + (mjpeg_size - tmp - x);
 	
	//ret = tjDecompress2 ( kradv4l2->jpeg_dec, kradv4l2->jpeg_buffer, jpeg_size_long, argb_buffer, kradv4l2->width, 
	//					  stride, kradv4l2->height, TJPF_BGRA, 0 );

	//if (ret != 0) {
	//	printf("JPEG decoding error: %s\n", tjGetErrorStr());
	//}
	/*
	int fd;
	char filename[512];
	sprintf(filename, "/home/oneman/kode/testfilex_%d.jpg", count++);

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(fd, kradv4l2->jpeg_buffer, jpeg_size_long);
	close(fd);
	
	if (count > 30) {
		exit(0);
	}
	*/
}

