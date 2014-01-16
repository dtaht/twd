#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
#include <sched.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "ringbuffer.h"
#include "twd_time.h"
#include "protocol.h"
#include "log.h"
#include "test_control.h"
#include "dscp.h"
#include "parse_addr.h"

/* This defines the simplest 3 tests we can run, testing for
 * tos, diffserv, & ecn awareness.
 */

#define TWD_RINGBUFFER_SIZE 4096 // 1 memory page on most systems

const char SUCCESS[] = "SUCCESS";
const char FAIL[] = "FAIL";

#define TEST_LOOPS (64 * TWD_RINGBUFFER_SIZE)

ack_t test_data_struct[TEST_LOOPS] = {0};


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
  itimerspec_t new_value;
  itimerspec_t ms10_interval;
  itimerspec_t ms100_interval;
  itimerspec_t old_value;
  timespec_t now;
  timespec_t watchdog;
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
  new_value.it_interval.tv_nsec = 1000L * 1000L; // 1ms

  // Setting to a relative time doesn't appear to work. Perhaps
  // using TFD_TIMER_ABSTIME is better and looping til we make sure?

  if((rc = timerfd_settime(ms1, 0, &new_value, NULL)) != 0) 
  {
    printf("WTF\n");
  }

  ms10_interval.it_value.tv_sec = 0; // now.tv_sec;
  ms10_interval.it_value.tv_nsec = 1000L; // arm the timer
  ms10_interval.it_interval.tv_sec = 0;
  ms10_interval.it_interval.tv_nsec = 10L * 1000L *1000L; // 10ms
  if((rc = timerfd_settime(ms10, 0, &ms10_interval, NULL)) !=0 )
  {
    printf("WTF\n");
  }

  ms100_interval.it_value.tv_sec = 0;
  ms100_interval.it_value.tv_nsec = 1000; // arm the timer
  ms100_interval.it_interval.tv_sec = 0;
  ms100_interval.it_interval.tv_nsec = 100L * 1000L * 1000L; // 100ms
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

  printf("Starting test\n");
  highfd = ms1;
  if(ms10 > ms1) highfd = ms10; 
  if(ms100 > ms10) highfd = ms100; 
  highfd++;

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

  new_value.it_value.tv_sec = 0; 
  new_value.it_value.tv_nsec = 0; // disarm the timer

  if((rc = timerfd_settime(ms1, 0, &new_value, NULL)) != 0) 
  {
    printf("Can't disarm timer\n");
  }
  if((rc = timerfd_settime(ms10, 0, &new_value, NULL)) != 0) 
  {
    printf("Can't disarm timer\n");
  }
  if((rc = timerfd_settime(ms100, 0, &new_value, NULL)) != 0) 
  {
    printf("Can't disarm timer\n");
  }
  
}

int destroy_timers() {
  return(0);
}


/* Maintain global clock shared among all threads */

static void * thread_timer(void *arg) {
	timer_t *time;
	// See above tes code for details 
        return time;
}

static void * thread_reader(void *arg)
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
  int sfds[2];
  int cfds[2];

  num_threads = 4;
  
  cpu_set_t set;
  socketpair(AF_INET, SOCK_DGRAM, 0, (int *) &sfds);
  socketpair(AF_INET, SOCK_DGRAM, 0, (int *) &cfds);
  
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

typedef struct {
  int flags;
  timespec_t ts;
  timespec_t rtt;
  uint32_t seqno;
  uint8_t ttl;
  tos_t tos;
} per_packet_info_t;

typedef struct  {
  int fd;
  int type;
  ip_header_t header;
  int size;
  int ts_type;
  uint32_t seqno;
  int64_t nonce;
  timespec_t ts;
} pbuffer_t;

int settos(int p, int type, int tos, int ecn) {
  int dscp = tos | ecn;
  int rc = 0;
  switch(type)
  {
  default: 
  IPPROTO_IP: rc = setsockopt(p, IPPROTO_IP, IP_TOS, (char *) &dscp, 
			    sizeof(dscp));
           break;
  IPPROTO_IPV6: rc = setsockopt(p,IPPROTO_IPV6, IPV6_TCLASS, (int *)
				&dscp,  sizeof(dscp));
	   break;
  }
  return(rc);
}

// FIXME: check for ipv6 equivalents, and the correct meaning of set

//        if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, (char *)&dscp, 
//                       sizeof(dscp)) == SOCKET_ERROR) {
//

/* SO_TIMESTAMP
  Generate time stamp for each incoming packet using the (not necessarily
  monotonous!) system time. Result is returned via recv_msg() in a
  control message as timeval (usec resolution).

* SO_TIMESTAMPNS
  Same time stamping mechanism as SO_TIMESTAMP, but returns result as
  timespec (nsec resolution).

  Advanced API (RFC3542) (2) specifies RECVHOPLIMIT
  packet options were defined earlier. HOWEVER RECPKTOPTIONS
  is probably more portable.

RECVHOPLIMIT RECPATHMTU are good useful options too.

*/
 
int recv_setup(pbuffer_t *p) {
  int set = 1; // FIXME: check what this should be
  switch(p->type) {
  case IPPROTO_IP: 
    if(setsockopt(p->fd, IPPROTO_IP, IP_RECVTOS, &set,sizeof(set))<0) { } 
    if(setsockopt(p->fd, IPPROTO_IP, IP_RECVTTL, &set,sizeof(set))<0) { }  
    break;
  case IPPROTO_IPV6: 
    if(setsockopt(p->fd, IPPROTO_IPV6, IPV6_RECVTCLASS, &set,sizeof(set))<0) { } break;
    if(setsockopt(p->fd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &set,sizeof(set))<0) { } break;
  }
  p->ts_type = 0; /* user timestamping - note this is a bad idea on high rate flows */

#if defined(SO_TIMESTAMPNS)
  if(setsockopt(p->fd, SOL_SOCKET, SO_TIMESTAMPNS, &set,sizeof(set))<0) 
    {
#endif
    if(setsockopt(p->fd, IPPROTO_IPV6, SO_TIMESTAMP, &set,sizeof(set))<0) { 
      p->ts_type = 2; // usec resolution
    } else {
      p->ts_type = 0;
    }
#if defined(SO_TIMESTAMPNS)
    } else {
    p->ts_type = 1; // Nanosec resolutions
  } 
#endif
  break;

}

#define MAX_MTU 1500
//        int  on = 64;
//       setsockopt(fd, IPPROTO_IPV6, IPV6_HOPLIMIT, &on, sizeof(on));

//int send_packet(connections_t, headers_t, results_t, void *data);

int send_packet(pbuffer_t *p, ip_header_t *h, char *data, int size) {
  struct msghdr msg = {0}; 
  struct cmsghdr *c;
  struct iovec iov[3] = {0};
  char pkt[MAX_MTU];
  msg.msg_iov = iov;
  msg.msg_iovlen = 3;
  iov[0].iov_base = (char *) &pkt;
  iov[0].iov_len = sizeof(pkt);

  msg.msg_control=pkt;
  msg.msg_controllen=1500;
  c=CMSG_FIRSTHDR(&msg);
  assert(c);

// if using raw sockets (ipv4mapped(p->from);

  if(p->type == IPPROTO_IP)
    {
      c->cmsg_level=IPPROTO_IP;
      c->cmsg_type=IP_TOS;
      c->cmsg_len=CMSG_LEN(sizeof(int));
      *(int*)CMSG_DATA(c)=h->tos;
      msg.msg_controllen=c->cmsg_len;
      c=CMSG_NXTHDR(&msg,&c);
      c->cmsg_level=IPPROTO_IP;
      c->cmsg_type=IP_TTL;
      c->cmsg_len=CMSG_LEN(sizeof(int));
      *(int*)CMSG_DATA(c)=h->ttl;
      msg.msg_controllen+=c->cmsg_len;
      // c=CMSG_NEXTHDR(&msg);
    } else {
    if(p-> type = IPPROTO_IPV6) {
      c->cmsg_level=IPPROTO_IPV6;
      c->cmsg_type=IPV6_TCLASS;
      c->cmsg_len=CMSG_LEN(sizeof(int));
      *(int*)CMSG_DATA(c)=h->tos;
      msg.msg_controllen=c->cmsg_len;
      c=CMSG_NXTHDR(&msg,&c);
      c->cmsg_level=IPPROTO_IPV6;
      c->cmsg_type=IPV6_HOPLIMIT;
      c->cmsg_len=CMSG_LEN(sizeof(int));
      *(int*)CMSG_DATA(c)=h->ttl;
      msg.msg_controllen+=c->cmsg_len;
      //      c=CMSG_NEXTHDR(&msg);
    }
  }

  sendmsg(p->fd,msg,0);
}
 
/* 
 if (cmsg->cmsg_level == SOL_IP - might be needed
 && cmsg->cmsg_type == IP_TTL)
 return (int)CMSG_DATA(cmsg);

 */

int recv_pbuffer(pbuffer_t *p, void * data)
{
  struct msghdr msg; 
  struct iovec iov[4];
  char pkt[MAX_MTU];
  memset(&msg, '\0', sizeof(msg));
  msg.msg_iov = iov;
  msg.msg_iovlen = 4;
  iov[0].iov_base = (char *) &pkt;
  iov[0].iov_len = sizeof(pkt);
  p->header.ttl = 0;
  p->header.tos = 0;
  p->ts = {0};
  /* FIXME: wrong */

  int message_size = sizeof(struct cmsghdr)+sizeof(p->tos)+sizeof(p->ttl)+sizeof(p->ts);
  int cmsg_size = message_size;
  char buf[CMSG_SPACE(sizeof(p->ts) + packet_size)];
  msg.msg_control = buf; // Assign buffer space for control header + header data/value
  msg.msg_controllen = sizeof(buf); //just initializing it

  int nRet = recvmsg(udpSocket, &msg, packet_size);
  p->ts = {0};
  if (nRet > 0) {
	struct cmsghdr *cmsg;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg,cmsg)) {

	  switch(cmsg->cmsg_level)
	    {
	    case IPPROTO_IP: if(cmsg->cmsg_len) switch (cmg->cmsg_type) {
		case IP_TTL: p->header.ttl = (int *) CMSG_DATA(cmsg); break; 
		case IP_TOS: p->header.tos = (int *) CMSG_DATA(cmsg); break;
		case SO_TIMESTAMP: converttimeval2timespec((timeval_t *) CMSG_DATA(cmsg), p->ts); break;
		case SO_TIMESTAMPNS: memcpy(&p->ts,CMSG_DATA(cmsg),sizeof(timespec_t)); break;
		}
	      break;
	    case IPPROTO_IPV6: if(cmsg->cmsg_len) switch (cmg->cmsg_type) {
		case IPV6_HOPLIMIT: p->header.ttl = (int *) CMSG_DATA(cmsg); break;
		case IPV6_TCLASS: p->header.tos = (int *) CMSG_DATA(cmsg); break;
		case SO_TIMESTAMP: converttimeval2timespec((timeval_t *) CMSG_DATA(cmsg), p->ts); break;
		case SO_TIMESTAMPNS: memcpy(&p->ts,CMSG_DATA(cmsg),sizeof(timespec_t)); break;
		}
	      break;
	    default: // FIXME: It's data now
	    }
	}
  }
  /* if we didn't get a timestamp with the packet, fill out the ts
     field portably. */
     if(p->ts.tv_sec == 0 && p->ts.tv_nsec == 0)
       twd_gettime(&p->ts);
}

int send_setup(pbuffer_t *p)
{
   int dscp;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_TCLASS, (char *)&dscp, 
                       sizeof(dscp)) == SOCKET_ERROR) {
	}
}

int send_pbuffer(pbuffer_t *p)
{
}

int send_packet_opt(pbuffer *p, char *data, int tos, int ecn) {
  /* setsockopt version */
  settos(p->fd,p->tos,p->ecn);
  sendmsg();
}

/* DSCP Test */

int test_recv_dscp(int p) {
  recv_packet(p, &buffer);
}

int test_sender_dscp(int p) {
  send_packet(p, tos, &buffer);
}

#define MAX_MTU 1500

/* TOS Test */
int test_tos() {
  per_packet_info_t results[64];
}

/* ECN Test */
int recv_ecn(pbuffer_t *p)
{
  per_packet_info_t results[4];
  pbuffer_t l = {*p};
  
}

ringbuffer__s collector;

/* We try to size the collector ring so large that it will never
   block, but... */

void send_result_to(ringbuffer__s rbuf; per_packet_info_t *p)
{
  while(!ringbuffer_write(rbuf,p,sizeof(*p))) sched_yield();
}


// FIXME, this assumes we are only getting one packet 
// per seqno, which is sort of right at this point
// (need to work in the nonce)

int recv_dscp(pbuffer_t *p, ringbuffer__s collector)
  {
  per_packet_info_t results = {0};
  pbuffer_t l = {*p};
  int rc;
  char buf[MAX_MTU];
  while((rc = recv_pbuffer(&l,&buf,sizeof(buf)) > 0))
  {
    results.seqno = l.seqno;
    results.ttl = l.ttl;
    results.ts = l.ts;
    results.tos = l.tos;
    send_result_to(&collector,&results);
    if(l.flags != 0) break;
  }
  if (rc < -1) 
  { // something went wrong 
  } 
  return l.flags;
}

int send_dscp(pbuffer_t *p) {
  pbuffer_t l = {*p};
  char data[MAX_MTU];
  results_t l = {0};
  for(int i = 0; ipqos[i].name != NULL; i++) {
    l.tos = ipqos[i].value;
    // these two should move to send_packet
    l.seqno++;
    twd_gettime(&l.ts);
    send_packet(&l,&data);
  }
  l.flags = 1;
  send_packet(&l,&data);
  return l.flags;
}

void test_setup_dscp()
{
  /* 
     create related threads
     
   */
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

  /* see: http://www.spinics.net/lists/arm-kernel/msg47075.html */
  
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
