#define _GNU_SOURCE

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "ringbuffer.h"

#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct test_control {
  pthread_t thread_id; /* ID returned by pthread_create() */
  int thread_num;      /* Application-defined thread # */
  queue_t *ringbuf;

  size_t fd;
  size_t cookie;
  size_t seqno;
  size_t limit;

};

/* Thread start function: does nothing yet.
   and return upper-cased copy of argv_string */

const char uargv[] = "WTF";

struct acks {
  long long timestamp;
  int seqnum;
  int flags;
};

typedef struct acks ack_t;

ack_t test_data_struct[1024] = {0};

static void *
thread_reader(void *arg)
{
  struct test_control *tinfo = arg;
  ack_t *a;
  //   while(tinfo->ringbuf->readPos == tinfo->ringbuf->writePos) pthread_yield();
  while(!(a = ring_dequeue(tinfo->ringbuf))) pthread_yield();
  while(a->flags != 1) {
    printf("Seqnum: %ld; flags: %d\n", a->seqnum, a->flags);
    while(!(a = ring_dequeue(tinfo->ringbuf))) pthread_yield();
  }
  printf("Done reading!\n");
  
  return uargv;
}

static void *
thread_writer(void *arg)
{
  struct test_control *tinfo = arg;
  int i;
  for(i = 0; i<256; i++) { 
    test_data_struct[i].seqnum = i;
    test_data_struct[i].flags = 0;
    if(!ring_enqueue(tinfo->ringbuf,&test_data_struct[i])) {
      pthread_yield();
    }
  }
  printf("Done writing i!\n");
  test_data_struct[i].seqnum = i;
  test_data_struct[i].flags = 1;
  while(!ring_enqueue(tinfo->ringbuf,&test_data_struct[i])) pthread_yield();
  printf("Done writing!\n");
  return uargv;
}

int
main(int argc, char *argv[])
{
  int s, tnum, opt, num_threads;
  pthread_attr_t attr;
  int stack_size;
  void *res;
  int i;

  /* The "-s" option specifies a stack size for our threads */
  
  stack_size = -1;
  while ((opt = getopt(argc, argv, "s:")) != -1) {
    switch (opt) {
    case 's':
      stack_size = strtoul(optarg, NULL, 0);
      break;
      
    default:
      fprintf(stderr, "Usage: %s [-s stack-size] arg...\n",
	      argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  
  num_threads = 2;

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

    s = pthread_attr_setschedpolicy(&attr,SCHED_RR);
    if (s != 0)
      handle_error_en(s, "pthread_attr_setschedpolicy");
    s = pthread_attr_getschedpolicy(&attr,&i);
    if (s != 0)
      handle_error_en(s, "pthread_attr_getschedpolicy");
    printf("%sScheduling policy   = %s\n", "wtf",
    	   (i == SCHED_OTHER) ? "SCHED_OTHER" :
    	   (i == SCHED_FIFO)  ? "SCHED_FIFO" :
    	   (i == SCHED_RR)    ? "SCHED_RR" :
    	   "???");


      /* Create one thread for each command-line argument */
      
      for (tnum = 0; tnum < num_threads; tnum++) {
	tinfo[tnum].thread_num = tnum + 1;
	
	/* The pthread_create() call stores the thread ID into
	   corresponding element of tinfo[] */

	tinfo[0].ringbuf = tinfo[1].ringbuf = ring_createQueue(64);
	  
	if((s = pthread_create(&tinfo[tnum].thread_id, &attr,
			       &thread_writer, &tinfo[tnum])) != 0)
	  handle_error_en(s, "pthread_create");
  	if((s = pthread_create(&tinfo[tnum].thread_id, &attr,
			       &thread_reader, &tinfo[tnum])) != 0)
	  handle_error_en(s, "pthread_create");
      }
  
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
	//	ring_destroyQueue(tinfo[1].ringbuf);
	// tinfo[0].ringbuf = 0;
	//	if(res != NULL) free(res); result is a constant currently
	//	we will return test results instead after processing
      }
    }  
  exit(EXIT_SUCCESS);
}

int client_start() {

}
