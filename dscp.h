#ifndef _dscp_h
#define _dscp_h
extern int parse_ipqos(const char *cp);
extern const char * iptos2str(int iptos);

struct ipqos_data {
	const char *name;
	int value;
};

typedef struct ipqos_data ipqos_t;

extern ipqos_t ipqos[];
#endif

