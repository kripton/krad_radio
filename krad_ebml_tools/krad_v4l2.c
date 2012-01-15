#include <krad_v4l2.h>


krad_v4l2_t *kradv4l2_create() {

	krad_v4l2_t *kradv4l2;

	kradv4l2 = calloc(1, sizeof(krad_v4l2_t));

	return kradv4l2;

}

void kradv4l2_destroy(krad_v4l2_t *kradv4l2) {

	free(kradv4l2);

}

void kradv4l2_frame_done (krad_v4l2_t *kradv4l2) {
	if (-1 == xioctl (kradv4l2->fd, VIDIOC_QBUF, &kradv4l2->buf)) {
		errno_exit ("VIDIOC_QBUF");
	}
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

            assert (kradv4l2->buf.index < kradv4l2->n_buffers);

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

			assert (i < kradv4l2->n_buffers);

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
		    tv.tv_sec = 2;
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


char *kradv4l2_read_frame_wait (krad_v4l2_t *kradv4l2) {

    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO (&fds);
    FD_SET (kradv4l2->fd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
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

	unsigned int min;


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
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
//	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
//	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	if (-1 == xioctl (kradv4l2->fd, VIDIOC_S_FMT, &fmt)) {
		errno_exit ("VIDIOC_S_FMT");
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

void kradv4l2_open (krad_v4l2_t *kradv4l2, char *device, int width, int height) {

	struct stat st; 

	strncpy(kradv4l2->device, device, 512);

	kradv4l2->width = width;
	kradv4l2->height = height;
	
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



