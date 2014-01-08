#define _GNU_SOURCE

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <locale.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <iconv.h>
#include <malloc.h>

#include "parse_addr.h"

/* the ipv6 separator is a colon. So is the port number. The convention
   is to separate out an ipv6 address with [] if a :port is present. 
   Then there is ipv4, which can use :port without the []. Headache. */

int parse_address(AddrPort_t *address)
{
	char addy[MAX_HOSTNAME+8];
	bool had_brackets = false;
	char *a;
	char *c;
	char *b=strrchr(address->host,':'); /* Find the last ':' */

	if ((a=strrchr(address->host,']')) != NULL)
	  if((c=strchr(address->host,'[')) != NULL) {
			had_brackets = true;
	  		strncpy(addy, c+1, (a-c)); 
			addy[(a-c)-1]=0;
			b=strrchr(a+1,':');
			}
	if(b != NULL) {
		if(had_brackets) {
			address->port = strtoul(b+1,NULL,10);
	  		strcpy(address->host, addy); 
		} else {
		  /* if we didn't we still could have just seen a bare ip */
	  	  c=strchr(address->host,':');
		  if(c == b) {
			address->port = strtoul(b+1,NULL,10);
	  		address->host[b-address->host] = 0; 
		  }
		}
	} else {
	  if(had_brackets) strcpy(address->host, addy); 
	}

	return 0;
}

int to_port_num(const char *tport)
{
  struct servent *presult;
  struct servent  result;
  char            tmp[BUFSIZ];
  
  /*-------------------------------------------------------------------
  ; assume a port number, not a name.  Hopefully, no port names start
  ; with a digit.
  ;--------------------------------------------------------------------*/
  
  if (isdigit(*tport))
  {
    unsigned long  port;
    char          *p;
    
    errno = 0;
    port  = strtoul(tport,&p,10);
    if ((errno != 0) || (port > USHRT_MAX) || (p == tport))
      return -1;
    return port;
  }
  
  if (getservbyname_r(tport,"udp",&result,tmp,sizeof(tmp),&presult) != 0)
    return -1;

  return ntohs((short)result.s_port);
}

int to_addr_port(
	sockaddr__u *restrict sockaddr,
	const char  *restrict addrport
)
{
  assert(sockaddr != NULL);
  assert(addrport != NULL);
  
  size_t  len = strlen(addrport);
  char    ipaddr[len + 1];
  char   *tport;
  int     port;

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
  if (tport == NULL)
    return EINVAL;
  
  *tport++ = '\0';
  
  port = to_port_num(tport);
  if (port < 0)
    return EINVAL;
  
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
    memmove(ipaddr,&ipaddr[1],len);
    
    if (inet_pton(AF_INET6,ipaddr,sockaddr->sin6.sin6_addr.s6_addr))
    {
      sockaddr->sin6.sin6_family = AF_INET6;
      sockaddr->sin6.sin6_port   = htons(port);
      return 0;
    }
  }
  
  /*--------------------------------------------------------------------
  ; else assume we have an IPv4 address and carry on.
  ;--------------------------------------------------------------------*/
  
  else
  {
    if (inet_pton(AF_INET,ipaddr,&sockaddr->sin.sin_addr.s_addr))
    {
      sockaddr->sin.sin_family = AF_INET;
      sockaddr->sin.sin_port   = htons(port);
      return 0;
    }
  }
  return EINVAL;
}

#ifdef TEST

int main(int argc, char *argv[])
{
    struct addrinfo hint, *res = NULL;
    int ret;
    AddrPort_t addr;
    memset(&hint, '\0', sizeof hint);

    hint.ai_family = PF_UNSPEC;
    hint.ai_flags = AI_NUMERICHOST;

    /* Save a DNS lookup first */
    for(int i=1; i < argc; i++) {
      strcpy(addr.host,argv[i]);
      addr.port = 0;
      parse_address(&addr);
      printf("corrected addr = %s\n", addr.host);
      ret = getaddrinfo(addr.host, NULL, &hint, &res);
      if (ret) {
    	hint.ai_flags = 0;
        ret = getaddrinfo(addr.host, NULL, &hint, &res);
	if(ret) {
	  puts("Invalid address");
	  puts(gai_strerror(ret));
	  return 1;
	}
      }
      switch(res->ai_family) {
      case AF_INET:  printf("%s is an ipv4 address\n",argv[1]); break;
      case AF_INET6: printf("%s is an ipv6 address\n",argv[1]); break;
      default: printf("%s is unknown address format %d\n",argv[1],
		      res->ai_family);
      }
      
      freeaddrinfo(res);
    }
    return 0;
}

#endif
