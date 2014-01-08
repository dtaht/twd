#ifndef _twd_h
#define _twd_h

#ifndef __GNUC__
#  define __attribute__(x)
#endif

#define MAX_MTU 1280

typedef union
{
  struct sockaddr     sa;
  struct sockaddr_in  sin;
  struct sockaddr_in6 sin6;
} sockaddr__u;

extern int parse_ipqos(const char *cp);
extern const char * iptos2str(int iptos);

struct output_type {
  char *desc;
  int id;
};

typedef struct output_type OutputType_t;

struct twd_options {
  unsigned int verbose:1;
  unsigned int ipv4:1;
  unsigned int ipv6:1;
  unsigned int up:1;
  unsigned int dn:1;
  unsigned int bidir:1;
  unsigned int dontfork:1;
  unsigned int help:1;
  unsigned int randomize_data:1;
  unsigned int randomize_size:1;
  unsigned int passfail:1;
  unsigned int ecn:1;
  unsigned int test_owd:1;
  unsigned int test_ecn:1;
  unsigned int test_diffserv:1;
  unsigned int test_tos:1;
  unsigned int test_all:1;
  unsigned int test_self:1;
  unsigned int multicast:1;
  unsigned int server:1;
  int debug;
  int packet_size;
  int tests;
  int length;
  int interval;
  int diffserv;
  int format;
  char *logdir;
  char *filename;
  char **hosts;
};

typedef struct twd_options TWD_Options_t;
const OutputType_t output_type[] = { { "plain",0 },
				     { "csv",1 },
				     { "json",2} };

struct packet_stats {
  int rtt;
};

struct packet_log {
  int serial;
  int rtt;
  int loss;
};

#endif
