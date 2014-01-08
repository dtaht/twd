#ifndef _parse_addr_h
#define _parse_addr_h
// FIXME find max hostname size 
#define MAX_HOSTNAME 255

struct addrport {
	int port;
	char host[MAX_HOSTNAME+8];
};

typedef struct addrport AddrPort_t;

extern int parse_address(AddrPort_t *address);

#endif
