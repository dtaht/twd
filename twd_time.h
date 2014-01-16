#ifndef _TWD_TIME_H
#define _TWD_TIME_H

#include <sys/types.h>

/* truly ancient non-typedefs bug me */

typedef struct timespec timespec_t;
typedef struct timeval timeval_t;
typedef struct itimerspec itimerspec_t;

/* FIXME get these from the right clock */
static inline int twd_gettime(timespec_t *t1) {
  if (clock_gettime(CLOCK_MONOTONIC, t1) == -1)
    handle_error("clock_gettime");
  return(0);
}

static inline void convert_timeval2timespec(timespec_t *t1, timeval_t *t2)
{
  t1->tv_sec = t2->tv_sec;
  t1->tv_nsec = t2->tv_usec * 1000L;
}

static inline int timeval_subtract (timeval_t *result, 
				    timeval_t *x, 
				    timeval_t *y)
{
  int nsec =0;
  if (x->tv_usec < y->tv_usec) {
    nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000)
    {
      nsec = (x->tv_usec - y->tv_usec) / 1000000;
      y->tv_usec += 1000000 * nsec;
      y->tv_sec -= nsec;
    }
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
  return x->tv_sec < y->tv_sec;
}

#endif
