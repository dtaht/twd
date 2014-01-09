#ifndef _twd_h
#define _twd_h

#include "parse_addr.h"
#include <stdint.h>

#ifndef __GNUC__
#  define __attribute__(x)
#endif

#define MAX_MTU 1280
#define TWD_DEFAULT_INTERVAL (1000*100*25) /* 2.5ms */
#define TWD_DEFAULT_PORT 0 		   /* FIXME Get port from IANA */
#define TWD_DEFAULT_DURATION (1000uLL * 1000uLL * 1000uLL * 30uLL) /* 30sec */
#define TWD_DEFAULT_PACKET_SIZE 200

struct output_type {
  char *desc;
  int id;
};

typedef struct output_type OutputType_t;

struct twd_options {
  uint32_t verbose:1;
  uint32_t up:1;
  uint32_t dn:1;
  uint32_t bidir:1;
  uint32_t dontfork:1;
  uint32_t help:1;
  uint32_t randomize_data:1;
  uint32_t randomize_size:1;
  uint32_t passfail:1;
  uint32_t ecn:1;
  uint32_t ipv4:1;
  uint32_t ipv6:1;
  uint32_t server:1;
  uint32_t multicast:1;
  uint32_t test_owd:1;
  uint32_t test_ecn:1;
  uint32_t test_diffserv:1;
  uint32_t test_tos:1;
  uint32_t test_all:1;
  uint32_t test_self:1;
  uint32_t test_fq:1;
  uint32_t test_bw:1;
  uint32_t debug;
  uint32_t packet_size;
  uint32_t tests;
  uint64_t length;
  uint64_t interval;
  uint32_t diffserv;
  uint32_t format;
  char *logdir;
  char *filename;
  char **hosts;
  sockaddr__u server_address;
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
