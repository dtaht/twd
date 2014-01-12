
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include <sched.h>

#include "infbuffer.h"

/**************************************************************************
* This is only here because my GCC compiler is a bit old and doesn't have
* intrinsic atomics.
**************************************************************************/

static inline size_t FETCH_THEN_ADD(volatile size_t *p,size_t incr)
{
  size_t result;
  __asm__ volatile ("lock; xadd %0, %1" :
  	"=r"(result), "=m"(*p):
  	"0"(incr) , "m"(*p) :
  	"memory");
  return result;
}

/**************************************************************************/

int infbuffer_init(infbuffer__s *buf,size_t items)
{
  buf->ridx = 0;
  buf->widx = 0;
  buf->tidx = 0;
  buf->data = calloc(items,sizeof(void *));
  return buf->data == NULL
  	? ENOMEM
  	: 0;
}

/**************************************************************************/

void infbuffer_add(infbuffer__s *buf,void *item)
{
  size_t idx = FETCH_THEN_ADD(&buf->tidx,1);
  buf->data[idx] = item;
  FETCH_THEN_ADD(&buf->widx,1);
}

/**************************************************************************/

void infbuffer_rem(infbuffer__s *buf,void **pitem)
{
  size_t idx = FETCH_THEN_ADD(&buf->ridx,1);
  while(idx >= buf->widx)
    sched_yield();
  *pitem = buf->data[idx];
}

/**************************************************************************/

void infbuffer_destroy(infbuffer__s *buf)
{
  free(buf->data);
}

/**************************************************************************/
