#ifndef _TWD_TEST_CONTROL_H
#define _TWD_TEST_CONTROL_H

/* The contents of this struct are still in flux */

struct test_control {
  pthread_t thread_id; /* ID returned by pthread_create() */
  int thread_num;      /* Application-defined thread # */
  ringbuffer__s *ringbuf;

  cpu_set_t cpus;

  size_t rfd;
  size_t wfd;
  size_t cookie;
  size_t seqno_start;
  size_t limit;

};

typedef struct test_control test_control_t;

#endif /* TWD_TEST_CONTROL_H */
