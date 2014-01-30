/* Test packet handling */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

#include "packet.h"

int main()
{
int rfd,wfd;
int rc;
ip_header_t h;
int size = 1500;
int on = 1;

socklen_t salen;
struct sockaddr *sa;
struct addrinfo hints, *res, *ressave;
struct addrinfo hints, *wres, *wressave;
char *host = "ip6-localhost";
char *port = NULL;
char *count = "1";
char data[size];

/* create a reader socket and a writer socket. No need to fork.
   This is a simple test */

hints = {0};

hints.ai_family=AF_INET6;
hints.ai_socktype=SOCK_DGRAM;
hints.ai_protocol=IPPROTO_UDP;
hints.ai_protocol = 0;          /* Any protocol */
hints.ai_canonname = NULL;
hints.ai_addr = NULL;
hints.ai_next = NULL;

if((n=getaddrinfo(host, port, &hints, &wres)) != 0)
  printf("udp write error for %s, %s: %s", host, port, gai_strerror(n));

wressave=wres;

do {
  wfd=socket(wres->ai_family, wres->ai_socktype, wres->ai_protocol); 
  if(wfd == -1) continue;
  if (bind(wfd, wres->ai_addr, wres->ai_addrlen) == 0)
    break;
  } while ((wres=wres->ai_next) != NULL);

if(wfd == -1) { perror("Can't open socket"); }

// FIXME get ephemeral port number from above, 

if((n=getaddrinfo(host, port, &hints, &res)) != 0)
  printf("udp setup error for %s, %s: %s", host, port, gai_strerror(n));

ressave=res;

do {
  rfd=socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(rfd >= 0)
    break; /*success*/
  } while ((res=res->ai_next) != NULL);

/* Turn on recv timestamping, tos, and ttl via setsockopt */

if(setsockopt(rfd, SOL_SOCKET, SO_TIMESTAMPNS, &on, sizeof(on)) == -1) {
  perror("can't timestamp in ns res");
  if(rc = setsockopt(rfd, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on)) == -1) {
    perror("can't timestamp in us res");
    }
  }

if(setsockopt(rfd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on)) == -1)
  perror("can't set hoplimit");

if(setsockopt(rfd, IPPROTO_IPV6, IPV6_RECVFLOW, &on, sizeof(on)) == -1)
  perror("can't set recvflow");

if(setsockopt(rfd, IPPROTO_IPV6, IPV6_RECVTCLASS, &on, sizeof(on)) == -1);
  perror("can't set recvtclass");

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

  freeaddrinfo(wres);
  freeaddrinfo(res);

  close(wfd);
  close(rfd);

  return(0);
}
