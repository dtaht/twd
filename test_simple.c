#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <sched.h>
#include <signal.h>
#include <sys/timerfd.h>

#include "ringbuffer.h"
#include "protocol.h"
#include "log.h"

/* This defines the simplest 3 tests we can run, testing for 
 * tos, diffserv, & ecn awareness.
 */

#define TWD_RINGBUFFER_SIZE 4096 // 1 memory page on most systems

const char SUCCESS[] = "SUCCESS";
const char FAIL[] = "FAIL";

#define TEST_LOOPS (64 * TWD_RINGBUFFER_SIZE)

ack_t test_data_struct[TEST_LOOPS] = {0};

/* The contents of this struct are still in flux */

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

typedef struct test_control test_control_t;

/* Non blocking read protected by a select call */

uint64_t read_overrun(int fd) {
  uint64_t err = 0;
  int rc = read(fd,&err,8);
  if(rc < 1) { if(errno == EAGAIN) err = 0; }
  return err;
}

/*           sigemptyset(&sigmask);
           sigaddset(&sigmask, SIGCHLD);
           if (sigprocmask(SIG_BLOCK, &sigmask, NULL) == -1) {
               perror("sigprocmask");
               exit(EXIT_FAILURE);
           }
*/

int create_timerfds(fd_set *exceptfds) {
  struct itimerspec new_value;
  struct itimerspec ms10_interval;
  struct itimerspec ms100_interval;
  struct itimerspec old_value;
  struct timespec now;
  struct timespec watchdog;
  sigset_t sigmask, empty_mask;
  struct sigaction sa;
  int rc = 0;
  int highfd = 0;
  int ms1, ms10, ms100; /* Setup timers to fire on these intervals */
  clockid_t clockid = CLOCK_MONOTONIC;
  int test = 0;
  /* When called with no flags the select only returns the first one */
  ms1 = timerfd_create(clockid,0);
  ms10 = timerfd_create(clockid,0);
  ms100 = timerfd_create(clockid,0);
  /* This needs a separate fcntl in older linuxes. Might still. Fixme.
     but it is safe presently to call this with blocking reads */

  /* ms1 = timerfd_create(clockid,TFD_NONBLOCK|TFD_CLOEXEC); */
  /* ms10 = timerfd_create(clockid,TFD_NONBLOCK|TFD_CLOEXEC); */
  /* ms100 = timerfd_create(clockid,TFD_NONBLOCK|TFD_CLOEXEC); */

  assert(ms1 > 0 || ms10 > 0 || ms100 > 0);

  if (clock_gettime(clockid, &now) == -1)
    handle_error("clock_gettime");

  watchdog.tv_sec = 5;
  watchdog.tv_nsec = 0;

  //  now.tv_sec += 1; /* start 1 sec in the future */

  new_value.it_value.tv_sec = 0; // now.tv_sec;
  new_value.it_value.tv_nsec = 1000L; // arm the timer
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 1000L; // 1us

  // Setting to a relative time doesn't appear to work. Perhaps
  // using TFD_TIMER_ABSTIME is better and looping til we make sure?

  if((rc = timerfd_settime(ms1, 0, &new_value, NULL)) != 0) 
  {
    printf("WTF\n");
  }
  /* OK this is either a bug in my code or in the implementation.
     If you set the ms1 and ms10 timers to the same value they fire
     for a while. If you set it 10x bigger, less while.

     perhaps setting the flags to be this low doesn't work?
  */

  ms10_interval.it_value.tv_sec = 0; // now.tv_sec;
  ms10_interval.it_value.tv_nsec = 1000L; // arm the timer
  ms10_interval.it_interval.tv_sec = 0;
  ms10_interval.it_interval.tv_nsec = 10*1000L; // 10us
  if((rc = timerfd_settime(ms10, 0, &ms10_interval, NULL)) !=0 )
  {
    printf("WTF\n");
  }

  ms100_interval.it_value.tv_sec = 0;
  ms100_interval.it_value.tv_nsec = 1000; // arm the timer
  ms100_interval.it_interval.tv_sec = 0;
  ms100_interval.it_interval.tv_nsec = 100L * 1000L; // 100us
  if(( rc = timerfd_settime(ms100, 0, &ms100_interval, NULL)) !=0)
  {
    printf("WTF\n");
  }
  //  FD_ZERO(readfds);
  FD_ZERO(exceptfds);
  FD_SET(ms1,exceptfds);
  FD_SET(ms10,exceptfds);
  FD_SET(ms100,exceptfds);

  sigemptyset(&sigmask);
  sigfillset(&sigmask);

#define READ_OVER(f)\
  do {				 \
    if(FD_ISSET(f, exceptfds))			     \
      printf( "" # f " timer fired %ld times\n", read_overrun(f)); \
  } while (0);

  printf("Starting test\n");
  highfd = ms1;
  if(ms10 > ms1) highfd = ms10; 
  if(ms100 > ms10) highfd = ms100; 
  highfd++;

  // So like, theoretically, we should get 10000 ms1, 1000 ms10, 100 ms100 
  // events...
  // FIXME Maybe we have to block signals in order to not miss an update?

  while(test++ < 10000 && 
	((rc = pselect(highfd,exceptfds,NULL,NULL,&watchdog,&sigmask))>0))
  {
    if(rc > 0) {
    READ_OVER(ms100);
    READ_OVER(ms10);
    READ_OVER(ms1);
    rc = 0;
    }
    if (rc == -1 && errno != EINTR) {
      /* Handle error... if I wasn't blocking everything */
      printf("Got an EINTR\n");
    }
  }

 if(rc > 0)
    {
    READ_OVER(ms100);
    READ_OVER(ms10);
    READ_OVER(ms1);
    }
  if(rc == 0 && test < 1000) { printf("Error, watchdog fired\n"); }
  if(rc == -1) { printf("Error in select\n"); }

#undef READ_OVER
   
}

int destroy_timers() {
  return(0);
}

static void *
thread_reader(void *arg)
{
  test_control_t *tinfo = arg;
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
  test_control_t *tinfo = arg;
  int i;
  int count = 0;
  printf("Writer started\n");
  // starts getting rescheduled around 962 entries
  for(i = 0; i<TEST_LOOPS-1; i++) { 
    test_data_struct[i].seqnum = i;
    test_data_struct[i].flags = 0;
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
  fd_set exceptfds;

  rc = ringbuffer_init(&rbuf,4096);
  if (rc != 0)
  {
    fprintf(stderr,"ringbuffer_init() = %s",strerror(rc));
    return EXIT_FAILURE;
  }
  
  create_timerfds(&exceptfds);
  exit(0);

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
  /*
  printf("BASIC: %s\n", rc |= test_basic(&rbuf) ? FAIL : SUCCESS);
  ringbuffer_reset(&rbuf);
  printf("ROLLOVER: %s\n", rc |= test_rollover(&rbuf) ? FAIL : SUCCESS);
  ringbuffer_reset(&rbuf);
  printf("Starting pthread test be prepared for weirdness\n");
  ringbuffer_reset(&rbuf);
  printf("PTHREAD1: %s\n", rc |= test_pthread1(&rbuf) ? FAIL : SUCCESS);
*/
  return(rc);
}

/************************************************************************/
