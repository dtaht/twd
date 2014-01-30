/* Test packet handling */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "packet.h"

int main()
{
  int rfd,wfd;
  int rc;
  ip_header_t h;
  int size = 1500;
  int on = 1;
  char data[size];

/* create a reader socket and a writer socket. No need to fork.
   This is a simple test */

/* Turn on recv timestamping, tos, and ttl via setsockopt */

  rc = setsockopt(rfd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on));
  rc = setsockopt(rfd, IPPROTO_IPV6, IPV6_RECVFLOW, &on, sizeof(on));
  rc = setsockopt(rfd, IPPROTO_IPV6, IPV6_RECVTCLASS, &on, sizeof(on));

/* write a packet */

  h.ttl = 64;
  h.tos.dscp = 8;
  h.tos.ecn = 3;

  h.sport = X;
  h.dport = Y;


  send_packet(sfd,&h,&data,&size);

/* read a packet to see if the headers are preserved and the above 
   sendmsg/recmsg code correct */

  recv_packet(rfd,&h,&data,&size);

  if (h.ttl != 63 || h.tos.dscp != 8 || h.tos.ecn != 3) {
    printf("Expected: ttl=%d dscp=%d ecn=%d, got ttl=%d dscp=%d ecn=%d\n",
	   63,8,3,h.ttl,h.tos.dscp,h.tos.ecn);
    exit(-1);
  }
  
  close(wfd);
  close(rfd);

  return(0);
}
