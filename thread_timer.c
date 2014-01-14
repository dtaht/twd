#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <sched.h>
#include <signal.h>

/* Only linux has TIMERFDs so... */

/* Heh. It turns out  gettimeofday uses a globally shared
   page with the current time on it, now. It's faster than
   hell.

   I'm so obsolete
   We still need to keep track of monotonic time tho.
*/

#define HAVE_TIMERFD 1

#if HAVE_TIMERFD
#include <sys/timerfd.h>

/* 
 * thread_timer: Keep global time updated to the nearest sane value
 */

/* In linux we can beat against two different clocks and get a grip
   on when ntp moves the time */

/* FIXME: need to write a test to see if we can get to 10us or less
   reliably */

int thread_timer(time_t *curr_time) {
  struct itimerspec new_value;
  struct itimerspec ms10_interval;
  struct itimerspec ms100_interval;
  struct itimerspec old_value;
  struct timespec now;
  struct timespec watchdog;
  sigset_t sigmask, empty_mask;
  struct sigaction sa;
  fd_set origfds;
  fd_set currfds;

  int rc = 0;
  int highfd = 0;
  int ms1, ms10, ms100; /* Setup timers to fire on these intervals */
  clockid_t clockid = CLOCK_MONOTONIC;
  int test = 0;
  /* When called with no flags the select only returns the first one */
  ms1 = timerfd_create(clockid,0);
  ms10 = timerfd_create(CLOCK_REALTIME,0);
  ms100 = timerfd_create(clockid,0);
  assert(ms1 > 0 || ms10 > 0 || ms100 > 0);

  if (clock_gettime(clockid, &now) == -1)
    handle_error("clock_gettime");

  watchdog.tv_sec = TWD_DEFAULT_DURATION;
  watchdog.tv_nsec = 0;

  new_value.it_value.tv_sec = 0; // now.tv_sec;
  new_value.it_value.tv_nsec = 1000L; // arm the timer
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 1000L * 1000L; // 1ms

  // Setting to a relative time doesn't appear to work. Perhaps
  // using TFD_TIMER_ABSTIME is better and looping til we make sure?

  if((rc = timerfd_settime(ms1, 0, &new_value, NULL)) != 0) 
  {
    printf("WTF\n");
  }

  new_value.it_interval.tv_nsec = 10L * 1000L *1000L; // 10ms
  if((rc = timerfd_settime(ms10, 0, &ms10_interval, NULL)) !=0 )
  {
    printf("WTF\n");
  }

  new_value.it_interval.tv_nsec = 100L * 1000L * 1000L; // 100ms
  if(( rc = timerfd_settime(ms100, 0, &ms100_interval, NULL)) !=0)
  {
    printf("WTF\n");
  }
  //  FD_ZERO(readfds);
  FD_ZERO(&origfds);
  FD_SET(ms1,&origfds);
  FD_SET(ms10,&origfds);
  FD_SET(ms100,&origfds);

  currfds = origfds;

  sigemptyset(&sigmask);
  sigfillset(&sigmask);

#define READ_OVER(f)\
  do {				 \
    if(FD_ISSET(f, &currfds))			     \
      printf( "" # f " timer fired %ld times\n", read_overrun(f)); \
  } while (0);

  highfd = ms1;
  if(ms10 > ms1) highfd = ms10; 
  if(ms100 > ms10) highfd = ms100; 
  highfd++;

  // Get the current time synced to the timerfd

  // So like, theoretically, we should get 10000 ms1, 1000 ms10, 100 ms100 
  // events...
  // FIXME do we have to block signals in order to not miss an update?
 
  currfds = origfds;
  while(test++ < 10000 && 
	((rc = pselect(highfd,&currfds,NULL,NULL,&watchdog,&sigmask))>0))
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
    currfds = origfds;
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

#endif /* HAVE_TIMERFD */

/* Other OSes can have a saner loop */
