#ifndef TWD_PARSE_ADDR_H
#define TWD_PARSE_ADDR_H

typedef enum
{
  TWDIP_any = AF_UNSPEC,
  TWDIP_v4  = AF_INET,
  TWDIP_v6  = AF_INET6
} twdip__t;

typedef union
{
  struct sockaddr     sa;
  struct sockaddr_in  sin;
  struct sockaddr_in6 sin6;
} sockaddr__u;

extern int to_addr_port(
		sockaddr__u *restrict sockaddr,
		const char  *restrict addrport,
		twdip__t
	);
#endif
