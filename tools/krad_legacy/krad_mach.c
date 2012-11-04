//#ifdef __MACH__

#include "krad_mach.h"

void clock_gettime (int clocktype, struct timespec *ts) {

  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;

}


int prctl (int n0, int n1, int n2, int n3, int n4) {
  return 0;
}

int clock_nanosleep (int clocktype, int timertype, struct timespec *ts, void *noptr) {

  struct timespec mine;

  clock_gettime (CLOCK_REALTIME, &mine);

  if (mine.tv_sec > ts->tv_sec) {
	  return 0;
  }

  if (mine.tv_nsec > ts->tv_nsec) {
	  if (mine.tv_sec == ts->tv_sec) {
		  return 0;
    }
	  mine.tv_nsec = 1000000000 + ts->tv_nsec - mine.tv_nsec;
	  mine.tv_sec++;
  } else {
    mine.tv_nsec = ts->tv_nsec - mine.tv_nsec;
  }

  mine.tv_sec = ts->tv_sec - mine.tv_sec;

  return nanosleep (&mine, NULL);
}

int epoll_create (int __size) {
  return 0;
}

int epoll_create1 (int __flags) {
  return 0;
}

int epoll_ctl (int __epfd, int __op, int __fd,
		      struct epoll_event *__event) {
  return 0;
}

int epoll_wait (int __epfd, struct epoll_event *__events,
		       int __maxevents, int __timeout) {
  return 0;
}

int kradv4l2_detect_devices () {
  return 0;
}

int kradv4l2_get_device_filename (int device_num, char *device_name) {
  return 0;
}

//#endif
