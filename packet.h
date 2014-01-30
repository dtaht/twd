#ifndef _TWD_PACKET_H
#define _TWD_PACKET_H

#define ISECNMARK(a) (a & 2)
#define ISECNSEEN(a) (a & 3)

// watch out for endian problems here

typedef struct {
  uint8_t tos:6;
  uint8_t ecn:2;
} dscp_t;

typedef union {
  uint8_t tos;
  dscp_t dscp;
} tos_t;

typedef struct {
  sockaddr__u from;
  sockaddr__u to;
  uint16_t sport;
  uint16_t dport;
  tos_t tos;
  uint8_t ttl;
  uint8_t proto;
  uint32_t csum;
  uint32_t flow;  
} ip_header_t;

typedef struct {
  int flags;
  timespec_t ts;
  timespec_t rtt;
  uint32_t seqno;
  uint8_t ttl;
  tos_t tos;
} per_packet_info_t;

typedef struct  {
  int fd;
  int type;
  ip_header_t header;
  int size;
  int ts_type;
  uint32_t seqno;
  int64_t nonce;
  timespec_t ts;
} pbuffer_t;

#endif
