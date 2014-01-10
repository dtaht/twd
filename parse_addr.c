#define _GNU_SOURCE

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "parse_addr.h"

/*************************************************************************/

int to_addr_port(
	sockaddr__u *restrict sockaddr,
	const char  *restrict addrport,
	twdip__t              atype
)
{
  struct addrinfo  hints;
  struct addrinfo *results;
  size_t           len = strlen(addrport);
  char             ipaddr[len + 1];
  char            *taddr;
  char            *tport;
  int              rc;
  
  assert(sockaddr != NULL);
  assert(addrport != NULL);
  assert((atype == TWDIP_v4) || (atype == TWDIP_v6) || (atype == TWDIP_any));
  
  memcpy(ipaddr,addrport,len + 1);

  /*--------------------------------------------------------------------
  ; We search backwards through the string, instead of forwards, because
  ; IPv6 addresses has embedded colons (but are embedded in brackets, with
  ; the port outside the brackets.  It will look like:
  ;
  ;	[fc00::1]:2222
  ;
  ;-----------------------------------------------------------------------*/  
  
  tport = memrchr(ipaddr,':',len);
  
  if (tport != NULL)
    *tport++ = '\0';
  else
    return EINVAL; /* when we get an IANA port, we can specify it here */

  /*--------------------------------------------------------------------
  ; if the first character of the addrport is a bracket, this is an IPv6
  ; address.  Using a hack, if the two characters previous to the port text
  ; is NOT a closing bracket, we return an error, otherwise, we NUL out the
  ; closing bracket, and skip the addrport pointer past the opening bracket,
  ; thus leaving us with a straight IPv6 address.
  ;-------------------------------------------------------------------------*/

  if (ipaddr[0] == '[')
  {
    if (tport[-2] != ']')
      return EINVAL;
    tport[-2] = '\0';
    taddr = &ipaddr[1];
  }
  else
    taddr = ipaddr;
  
  memset(&hints,0,sizeof(hints));
  results           = NULL;
  hints.ai_family   = atype;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  
  rc = getaddrinfo(taddr,tport,&hints,&results);
  
  /*-----------------------------------------------------------------------
  ; Woot!  We finally have an address!  Now grovel through the results for
  ; something we can use.
  ;------------------------------------------------------------------------*/
  
  if (rc == 0)
    memcpy(&sockaddr->sa,results->ai_addr,results->ai_addrlen);
  else
    rc = EADDRNOTAVAIL;
    
  freeaddrinfo(results);
  return rc;
}

/*************************************************************************/

#ifdef TEST

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>

int main(int argc, char *argv[])
{
  twdip__t atype;
  int      ret;
  int      i;
  
  if (argc == 1)
  {
    fprintf(stderr,"usage: %s [a46] hosts...\n",argv[0]);
    return EXIT_FAILURE;
  }
  
  switch(argv[1][0])
  {
    case '4': atype = TWDIP_v4;  break;
    case '6': atype = TWDIP_v6;  break;
    case 'a': atype = TWDIP_any; break;
    default:
         fprintf(stderr,"usage: %s [a46] hosts...\n",argv[0]);
         return EXIT_FAILURE;
  }
  
  for (i = 2 , ret = EXIT_SUCCESS ; i < argc ; i++)
  {
    sockaddr__u addr;
    char        tipv4[INET_ADDRSTRLEN];
    char        tipv6[INET6_ADDRSTRLEN];
    int         rc;
    
    rc = to_addr_port(&addr,argv[i],atype);
    if (rc != 0)
    {
      printf("NONE %s: %s\n",argv[i],strerror(rc));
      ret = EXIT_FAILURE;
      continue;
    }
    
    switch(addr.sa.sa_family)
    {
      case AF_INET:
           printf(
           	"IPv4 %s %s:%d\n",
           	argv[i],
           	inet_ntop(AF_INET,&addr.sin.sin_addr.s_addr,tipv4,sizeof(tipv4)),
           	ntohs(addr.sin.sin_port)
           );
           break;
           
      case AF_INET6:
           printf(
           	"IPv6: %s [%s]:%d\n",
           	argv[i],
           	inet_ntop(AF_INET6,&addr.sin6.sin6_addr.s6_addr,tipv6,sizeof(tipv6)),
           	ntohs(addr.sin6.sin6_port)
           );
           break;

      default:
           fprintf(stderr,"this should not happen\n");
           ret = EXIT_FAILURE;
           break;
    }
  }
  
  return ret;
}

#endif
