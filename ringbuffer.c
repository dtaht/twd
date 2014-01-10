#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>
#include <sys/mman.h>

#include "atomic.h"
#include "ringbuffer.h"

/************************************************************************
*
* See http://en.wikipedia.org/wiki/Ring_buffer for the idea behind this
* implementation.
*
* If using the mmap() trick isn't that portable, then as a fall back, we can
* allocate twice the requested memory (as is done now), then when rbuf->ridx
* falls beyond the given size then call
*
*	memmove(rbuf->address,&rbuf->address[rbuf->ridx],rbuf->used);
*	
* before adjusting the indecies in ringbuffer_read().  Sure, it's a bit of a
* slowdown, but it makes the code easier to write and understand.
*
* Sharing a ring buffer between threads is currently not safe.  I don't know
* enough about modern atomics to handle this, but in general, when you call
* ringbuffer_read() or ringbuffer_write, the first thing you need to do is
* obtain exclusive access to the ringbuffer__s.  I would put the required
* protection in the structure itself (obviously).
*
* The kNumTries in the original code appeared to be an attempt to try the
* operation a few times (it's a type of spin lock) and then give up on
* obtaining exclusive access to the ring buffer.  At least, that's my
* interpretation on the original code.
*
************************************************************************/

/****************************************************************************
* This exists as an inline function and not a macro.  Doing things this way
* gives us type safely, and ensures that each parameter is only evaluated
* once.  Since we're using C99, we might as well skip the old C macro
* system.
*****************************************************************************/

static inline size_t min_size_t(size_t a,size_t b)
{
  return a < b ? a : b ;
}

/************************************************************************/

int ringbuffer_init(ringbuffer__s *const buff,size_t size)
{
  unsigned char *address;
  char           path[FILENAME_MAX];
  int            fh;
    
  assert(buff != NULL);
  assert(size >= (size_t)sysconf(_SC_PAGE_SIZE));
  assert((size % (size_t)sysconf(_SC_PAGE_SIZE)) == 0);
  
  /*---------------------------------------------------------------------
  ; The basic outline---we allocate twice the memory via mmap().  Then we
  ; create a file (in shared memory I guess), set the size to the size we
  ; originally requested, then map this file twice into memory, overwriting
  ; the original mapping.  This will give us a region where the second half
  ; maps over the first, even though the addresses are different.
  ;
  ; Isn't virtual memory neat?
  ;
  ; Anyway, check http://en.wikipedia.org/wiki/Ring_buffer for more details.
  ;------------------------------------------------------------------------*/
  
  strcpy(path,"/dev/shm/ring-buffer-XXXXXX");
  fh = mkstemp(path);
  if (fh == -1)
    return errno;
  
  unlink(path);
  
  buff->size = size;
  buff->ridx = 0;
  buff->widx = 0;
  
  if (ftruncate(fh,buff->size) < 0)
  {
    close(fh);
    return errno;
  }
  
  buff->address = mmap(
  	NULL,
  	buff->size * 2,
  	PROT_NONE,
  	MAP_ANONYMOUS | MAP_PRIVATE,
  	-1,
  	0
  );
  
  if (buff->address == MAP_FAILED)
  {
    close(fh);
    return errno;
  }
  
  address = mmap(buff->address,
  	buff->size,
  	PROT_READ | PROT_WRITE,
  	MAP_FIXED | MAP_SHARED,
  	fh,
  	0
  );
  
  if (address != buff->address)
  {
    munmap(buff->address,buff->size * 2);
    close(fh);
    return ENOMEM;
  }
  
  address = mmap(
  	buff->address + buff->size,
  	buff->size,
  	PROT_READ | PROT_WRITE,
  	MAP_FIXED | MAP_SHARED,
  	fh,
  	0
  );
  
  if (address != buff->address + size)
  {
    munmap(buff->address,buff->size * 2);
    close(fh);
    return errno;
  }
  
  close(fh);
  return 0;
}

/************************************************************************/  

size_t ringbuffer_peek(ringbuffer__s *const restrict rbuf)
{
  return (rbuf->size - (rbuf->widx - rbuf->ridx));
}

/************************************************************************/  

size_t ringbuffer_used(ringbuffer__s *const restrict rbuf)
{
  return (rbuf->widx - rbuf->ridx);
}

/* Just left this in here because I don't trust the "used" change. 
   This locks up at 4096 and 8192 */

size_t old_ringbuffer_used(
		       ringbuffer__s *const restrict rbuf)
{
  size_t size = rbuf->size;
  size_t widx = rbuf->widx;
  size_t ridx = rbuf->ridx;
  long temp = 0;

  switch( ( (widx > size) << 1) | (ridx > size) )
  {
  case 0: /* if they both are on the same page just do the math */
  case 3: temp = widx - ridx; break;
  case 1: temp = widx - (ridx - size); break;
  case 2: temp = ridx - (widx - size); break;
  }

  if(temp < 0) { 
    temp = -temp; 
    //printf ("Temp: %d\n", temp); 
  }
  return(temp); 

}

/************************************************************************/  

size_t ringbuffer_write(
	ringbuffer__s *const restrict rbuf,
	const void    *restrict       src,
	size_t                        amount
)
{
  size_t len;
  
  assert(rbuf   != NULL);
  assert(src    != NULL);
  assert(amount >  0);
  
  len = min_size_t(rbuf->size - ringbuffer_used(rbuf),amount);
  if (len > 0)
  {
    memcpy(&rbuf->address[rbuf->widx],src,len);
    atomic_add(&rbuf->widx,len);
  }
  
  return len;
}

/************************************************************************/

size_t ringbuffer_read(
	ringbuffer__s *const restrict rbuf,
	void          *restrict       dest,
	size_t                        amount
)
{
  assert(rbuf   != NULL);
  assert(dest   != NULL);
  assert(amount >  0);
  size_t size = rbuf->size;
  size_t used = ringbuffer_used(rbuf);
  if (used > 0)
  {
    size_t len = min_size_t(used,amount);
    volatile size_t temp; 
    size_t temp2;
    size_t temp3;

    memcpy(dest,&rbuf->address[rbuf->ridx],len);
    rbuf->ridx += len;

    if (rbuf->ridx > size)
    {
      do {
       	temp3 = temp = __atomic_load_n(&rbuf->widx, __ATOMIC_ACQUIRE); // atomic_read()?
	temp2 = temp - size;
      }
      while((temp3 = atomic_compare_and_swap(&rbuf->widx,temp,temp2)) != temp) ;
      rbuf->ridx -= size;
    }
    return len;
  }
  else
    return 0;
}

/************************************************************************/

int ringbuffer_reset(ringbuffer__s *const buff)
{
  assert(buff   != NULL);
  if(buff)
    { 
      buff->ridx = 0;
      buff->widx = 0;
      return(EXIT_SUCCESS);
    }
  return(EXIT_FAILURE);
}

/************************************************************************/

int ringbuffer_destroy(ringbuffer__s *const rbuf)
{
  assert(rbuf != NULL);
  
  errno = 0;
  munmap(rbuf->address,rbuf->size * 2);
  return errno;
}

/************************************************************************/
