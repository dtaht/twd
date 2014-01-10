#ifndef _TWD_RINGBUFFER_H
#define _TWD_RINGBUFFER_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>
#include <sys/mman.h>

#include "atomic.h"

typedef struct
{
  unsigned char *address;
  size_t         size;
  size_t         used;
  size_t         ridx;
  size_t         widx;
} ringbuffer__s;

int ringbuffer_init(ringbuffer__s *const buff,size_t size);
size_t ringbuffer_write(
	ringbuffer__s *const restrict rbuf,
	const void    *restrict       src,
	size_t                        amount
			);
size_t ringbuffer_read(
	ringbuffer__s *const restrict rbuf,
	void          *restrict       dest,
	size_t                        amount
		       );
int ringbuffer_destroy(ringbuffer__s *const rbuf);
int ringbuffer_reset(ringbuffer__s *const buff);
size_t ringbuffer_used(ringbuffer__s *const restrict rbuf);
size_t ringbuffer_peek(ringbuffer__s *const restrict rbuf);
#endif
