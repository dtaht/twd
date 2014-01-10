
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>
#include <sys/mman.h>

#include "atomic.h"

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

typedef struct
{
  unsigned char *address;
  size_t         size;
  size_t         used;
  size_t         ridx;
  size_t         widx;
} ringbuffer__s;

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
  buff->used = 0;
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
  
  len = min_size_t(rbuf->size - rbuf->used,amount);
  if (len > 0)
  {
    memcpy(&rbuf->address[rbuf->widx],src,len);
    rbuf->used += len;
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
  
  if (rbuf->used > 0)
  {
    size_t len = min_size_t(rbuf->used,amount);
    memcpy(dest,&rbuf->address[rbuf->ridx],len);
    rbuf->used -= len;
    rbuf->ridx += len;

    if (rbuf->ridx > rbuf->size)
    {
      rbuf->ridx -= rbuf->size;
      atomic_sub(&rbuf->widx,rbuf->size);
    }
    return len;
  }
  else
    return 0;
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

static int test_basic(ringbuffer__s *rbuf) {
  int a, b, c;
  a = 1;
  b = 2;
  ringbuffer_write(rbuf,&a,sizeof(a));
  ringbuffer_write(rbuf,&b,sizeof(b));
  if(ringbuffer_read(rbuf,&c,sizeof(c)) != sizeof(c)) return -1;
  if(c!=a) return -2;
  if(ringbuffer_read(rbuf,&c,sizeof(c)) != sizeof(c)) return -1;
  if(c!=b) return -2;
  if(ringbuffer_read(rbuf,&c,sizeof(c)) != 0) return -3;
  if(a != 1) return -4; // fail. we scribbled on a somehow
  if(b != 2) return -4; // fail. we scribbled on b somehow
  return 0;
}

int main(void)
{
  ringbuffer__s rbuf;
  int           rc;
  
  rc = ringbuffer_init(&rbuf,4096);
  if (rc != 0)
  {
    fprintf(stderr,"ringbuffer_init() = %s",strerror(rc));
    return EXIT_FAILURE;
  }
  
  /*----------------------------------------------
  ; just a quick check that our mapping worked
  ;-----------------------------------------------*/
  
  rbuf.address[0] = 0x5A;
  printf("%02X %02X\n",rbuf.address[0],rbuf.address[16384]);  
  
  /*------------------------------------------------------------------------
  ; the pause that refreshes.  This also allows us to check /proc/<pid>/maps
  ; to see the mapped regions.
  ;------------------------------------------------------------------------*/
  
  printf("pid: %lu\n",(unsigned long)getpid());

  if(test_basic(&rbuf)) printf("Basic test failed\n");  
  return EXIT_SUCCESS;
}

/************************************************************************/

