#include "ringbuffer.h"

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

  rc = test_basic(&rbuf);
  printf("BASIC: %s\n", rc ? "FAIL" : "SUCCESS");
  return(rc);
}

/************************************************************************/
