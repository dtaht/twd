#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <sched.h>

#include "ringbuffer.h"

#define TWD_RINGBUFFER_SIZE 4096 // 1 memory page on most systems

#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_warning_en(en, msg) \
  do { errno = en; perror(msg); } while (0)

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct acks {
  uint64_t timestamp;
  uint32_t seqnum;
  uint32_t flags;
};

typedef struct acks ack_t;

const char SUCCESS[] = "SUCCESS";
const char FAIL[] = "FAIL";

#define TEST_LOOPS (64 * TWD_RINGBUFFER_SIZE)

ack_t test_data_struct[TEST_LOOPS] = {0};

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

/* Test to see what happens when we roll over the page boundry
   in a single thread */

static int test_rollover(ringbuffer__s *rbuf) {
  int a, b, c;
  int count;
  a = 1;
  b = 2;
  c = 1;
  int res = 0;
  int i = 0;
  for(i = 0; i < 1024; i++) {
    if((count = ringbuffer_write(rbuf,&a,sizeof(a))) != sizeof(a))
       {
	 res |= 1;
	 printf("We have an early problem: count = %d at %d\n",count,i);
       }
   }
  i = 0;
  do {}
  while((count = ringbuffer_read(rbuf,&c,sizeof(c)) == sizeof(c)) && 
	c == 1 && i++ < 1024) ;

  if(i == 1024 && c==1 && count == 0) {
    printf("All good before rollover\n");
  } else {
    res |= 1;
    printf("Read goes BOOM: i=%d c=%d count=%d\n",i,c,count);
  }
  a = 2;
  for(i = 0; i < 1024; i++) {
    if((count = ringbuffer_write(rbuf,&a,sizeof(a))) != sizeof(a)) 
       {
	 res |= 1;
	 printf("We have a post-rollover problem: count = %d at %d\n",count,i);
       }
   }
  i = 0;
  do {}
  while((count = ringbuffer_read(rbuf,&c,sizeof(c)) == sizeof(c)) && 
	c == 2 && i++ < 1024) ;
  if(i == 1024 && c==2 && count == 0)
    {
      printf("All good after rollover\n");
    } else 
    {
      res |= 1;
      printf("BOOM after rollover: i=%d c=%d count=%d\n",i,c,count);
      printf("Values for buffer: ridx, widx = ( %ld, %ld )", rbuf->ridx, rbuf->widx);
    }
 
  return res;
}

/* PTHREADED TESTS */

struct test_control {
  pthread_t thread_id; /* ID returned by pthread_create() */
  int thread_num;      /* Application-defined thread # */
  ringbuffer__s *ringbuf;

  cpu_set_t cpus;

  size_t fd;
  size_t cookie;
  size_t seqno_start;
  size_t limit;

};

static void *
thread_reader(void *arg)
{
  struct test_control *tinfo = arg;
  int count;
  int seqnums = 0;
  ack_t a;
  printf("Reader started\n");
  while(!(count = ringbuffer_read(tinfo->ringbuf,&a,sizeof(a))))
    sched_yield();
  if(count != sizeof(a)) { printf("TRUNCATED READ\n"); exit(-1); }
  while(a.flags != 1) {
    //    going quiet for a test
    //    printf("Seqnum: %d; flags: %d\n", a.seqnum, a.flags);
    if (a.seqnum != seqnums) 
      printf ("missing seqno expect %d got %d\n", seqnums, a.seqnum);
    seqnums++;
    while(!(count = ringbuffer_read(tinfo->ringbuf,&a,sizeof(a))))
      sched_yield();
    if(count != sizeof(a)) { printf("TRUNCATED READ\n"); exit(-1); }
  }
  printf("Done reading!\n");
  return SUCCESS;
}

static void *
thread_writer(void *arg)
{
  struct test_control *tinfo = arg;
  int i;
  int count = 0;
  printf("Writer started\n");
  // starts getting rescheduled around 962 entries
  for(i = 0; i<TEST_LOOPS-1; i++) { 
    test_data_struct[i].seqnum = i;
    test_data_struct[i].flags = 0;
    // and if you call it with this enabled it never hits the lock so
    // the test succeeds
    //    printf("Writing i:%d!\n",i);
    // not having this while on the write was why I was not seeing all the data

    while ((count = ringbuffer_write(tinfo->ringbuf,
				     &test_data_struct[i],
				     sizeof(ack_t))) != sizeof(ack_t))
      sched_yield();
  }
  printf("Done writing i!\n");
  test_data_struct[i].seqnum = i;
  test_data_struct[i].flags = 1;
  while(!ringbuffer_write(tinfo->ringbuf,&test_data_struct[i],sizeof(ack_t)))
    {
    sched_yield();
    }
  printf("Done writing!\n");
  fflush(stdout);
  return SUCCESS;
}

static int test_pthread1(ringbuffer__s *rbuf) {
  int s, tnum, num_threads;
  pthread_attr_t attr;
  int stack_size = 0;
  void *res;
  int i;

  num_threads = 2;

  cpu_set_t set;

//  FIXME, setup the cpu stuff via the control structs
//  and the correct pthread stuff
//  A good test will make sure things are on different cores
  CPU_ZERO( &set );
  CPU_SET( 1, &set );
  if(0) {
  if (sched_setaffinity( getpid(), sizeof( cpu_set_t ), &set ))
    {
      perror( "sched_setaffinity" );
      return NULL;
    }
  }

  /* Allocate memory for pthread_create() arguments in c99 */
  {
    struct test_control tinfo[num_threads *
			     sizeof(struct test_control)];
    memset(tinfo,num_threads*sizeof(struct test_control),0);
    /* Initialize thread creation attributes */
    
    if((s = pthread_attr_init(&attr)) != 0) 
      handle_error_en(s, "pthread_attr_init");
    
    if (stack_size > 0) {
      if((s = pthread_attr_setstacksize(&attr, stack_size) != 0))
	handle_error_en(s, "pthread_attr_setstacksize");
    }
  
    /* FIXME this isn't quite enough to acquire real time privs
       and the answers on bsd, darwin, and linux are arcane. */
    
    if(!(s = pthread_attr_setschedpolicy(&attr,SCHED_RR)))
      handle_warning_en(s, "pthread_attr_setschedpolicy");
    if(!(s = pthread_attr_getschedpolicy(&attr,&i)))
      handle_warning_en(s, "pthread_attr_getschedpolicy");
 
    printf("Scheduling policy   = %s\n",
    	   (i == SCHED_OTHER) ? "SCHED_OTHER" :
    	   (i == SCHED_FIFO)  ? "SCHED_FIFO" :
    	   (i == SCHED_RR)    ? "SCHED_RR" :
    	   "???");

    tinfo[0].thread_num = 1;
    printf("Starting thread %d\n", 1);

    /* The pthread_create() call stores the thread ID into
       corresponding element of tinfo[] */

    tinfo[1].ringbuf = tinfo[0].ringbuf = rbuf;
    
    if((s = pthread_create(&tinfo[0].thread_id, &attr,
			   &thread_reader, &tinfo[0])) != 0)
      handle_error_en(s, "pthread_create");
    
    //    sleep(1);
    tinfo[1].thread_num = 2;
    printf("Starting thread %d\n", 2);
    
    if((s = pthread_create(&tinfo[1].thread_id, &attr,
			   &thread_writer,& tinfo[1])) != 0)
      handle_error_en(s, "pthread_create");
    
    /* Destroy the thread attributes object, since it is no
       longer needed */
    
    if((s = pthread_attr_destroy(&attr)) != 0)
      handle_error_en(s, "pthread_attr_destroy");
    
      /* Now join with each thread, and display its returned value */
      
      for (tnum = 0; tnum < num_threads; tnum++) {
	if((s = pthread_join(tinfo[tnum].thread_id, &res)) !=0)
	  handle_error_en(s, "pthread_join");
	
	printf("Joined with thread %d; returned value was %s\n",
	       tinfo[tnum].thread_num, (char *) res);
      }
  }
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
  printf("%02X %02X\n",rbuf.address[0],rbuf.address[TWD_RINGBUFFER_SIZE]);  
  
  /*------------------------------------------------------------------------
  ; the pause that refreshes.  This also allows us to check /proc/<pid>/maps
  ; to see the mapped regions.
  ;------------------------------------------------------------------------*/
  
  printf("pid: %lu\n",(unsigned long)getpid());

  rc = test_basic(&rbuf);
  printf("BASIC: %s\n", rc ? "FAIL" : "SUCCESS");
  ringbuffer_reset(&rbuf);
  rc = test_rollover(&rbuf);
  printf("ROLLOVER: %s\n", rc ? "FAIL" : "SUCCESS");
  ringbuffer_reset(&rbuf);
  printf("Starting pthread test be prepared for weirdness\n");
  rc = test_pthread1(&rbuf);
  printf("PTHREAD1: %s\n", rc ? "FAIL" : "SUCCESS");
  return(rc);
}

/************************************************************************/
