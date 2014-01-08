#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
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
