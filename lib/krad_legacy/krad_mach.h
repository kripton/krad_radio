#ifdef __MACH__

#ifndef KRAD_MACH_H
#define KRAD_MACH_H


#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#include <mach/clock.h>
#include <mach/mach.h>

#define CLOCK_MONOTONIC 0
#define CLOCK_THREAD_CPUTIME_ID 1
#define CLOCK_REALTIME 2

#define PR_SET_NAME 0
#define TIMER_ABSTIME 0

#define SOCK_CLOEXEC 0

#define DEFAULT_V4L2_DEVICE ""
#define DEFAULT_ALSA_CAPTURE_DEVICE ""
#define DEFAULT_ALSA_PLAYBACK_DEVICE ""

typedef struct krad_alsa_St krad_alsa_t;
typedef struct krad_v4l2_St krad_v4l2_t;

void clock_gettime (int clocktype, struct timespec *ts);
int clock_nanosleep (int clocktype, int timertype, struct timespec *ts, void *noptr);
int prctl (int n0, int n1, int n2, int n3, int n4);

enum EPOLL_EVENTS
  {
    EPOLLIN = 0x001,
#define EPOLLIN EPOLLIN
    EPOLLPRI = 0x002,
#define EPOLLPRI EPOLLPRI
    EPOLLOUT = 0x004,
#define EPOLLOUT EPOLLOUT
    EPOLLRDNORM = 0x040,
#define EPOLLRDNORM EPOLLRDNORM
    EPOLLRDBAND = 0x080,
#define EPOLLRDBAND EPOLLRDBAND
    EPOLLWRNORM = 0x100,
#define EPOLLWRNORM EPOLLWRNORM
    EPOLLWRBAND = 0x200,
#define EPOLLWRBAND EPOLLWRBAND
    EPOLLMSG = 0x400,
#define EPOLLMSG EPOLLMSG
    EPOLLERR = 0x008,
#define EPOLLERR EPOLLERR
    EPOLLHUP = 0x010,
#define EPOLLHUP EPOLLHUP
    EPOLLRDHUP = 0x2000,
#define EPOLLRDHUP EPOLLRDHUP
    EPOLLONESHOT = (1 << 30),
#define EPOLLONESHOT EPOLLONESHOT
    EPOLLET = (1 << 31)
#define EPOLLET EPOLLET
  };

/* Valid opcodes ( "op" parameter ) to issue to epoll_ctl().  */
#define EPOLL_CTL_ADD 1	/* Add a file decriptor to the interface.  */
#define EPOLL_CTL_DEL 2	/* Remove a file decriptor from the interface.  */
#define EPOLL_CTL_MOD 3	/* Change file decriptor epoll_event structure.  */

typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event
{
  uint32_t events;	/* Epoll events */
  epoll_data_t data;	/* User data variable */
};

int epoll_create (int __size);
int epoll_create1 (int __flags);
int epoll_ctl (int __epfd, int __op, int __fd,
		      struct epoll_event *__event);

int epoll_wait (int __epfd, struct epoll_event *__events,
		       int __maxevents, int __timeout);

int kradv4l2_detect_devices ();
int kradv4l2_get_device_filename (int device_num, char *device_name);

#endif
#endif
