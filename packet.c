#include <sys/types.h>
#include <sys/socket.h>

#include "packet.h"

#define MAX_MTU 1500

//        int  on = 64;
//       setsockopt(fd, IPPROTO_IPV6, IPV6_HOPLIMIT, &on, sizeof(on));

// send_packet_raw needed too I think

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

#ifdef TEST
int main()
{

/* create a reader socket and a writer socket. No need to fork
   this is a simple test */

/* Turn on timestamping, tos, and ttl via setsockopt */

/* write a packet */

/* read a packet to see if the headers are preserved and the above 
   sendmsg/recmsg code correct */

/* end */

}
#endif
