#ifndef _parse_addr_h
#define _parse_addr_h
// FIXME find max hostname size 
#define MAX_HOSTNAME 255

struct addrport {
	int port;
	char host[MAX_HOSTNAME+8];
};

typedef union
{
  struct sockaddr     sa;
  struct sockaddr_in  sin;
  struct sockaddr_in6 sin6;
} sockaddr__u;

typedef struct addrport AddrPort_t;

extern int parse_address(AddrPort_t *address);
extern int to_port_num(const char *tport);
extern int to_addr_port(
	sockaddr__u *restrict sockaddr,
	const char  *restrict addrport
			);

#endif
