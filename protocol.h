#ifndef _TWD_PROTOCOL_H
#define _TWD_PROTOCOL_H

/* taken from the in-progress rfc */

enum TWD_protocol {
  PAD, PADN, RANDN, NONCE, ATIME, REGISTER, COUNTERPROPOSAL,
  PROBE, PROBEACK, LSEEN, CPU, LOSSRECORD, TOSRECORD, 
  CLOSEWAIT = 254,
  CLOSE = 255
};

struct ACKS {
  uint32_t dt:30; /* Time in ns since last large timestamp */
  uint32_t ecn:1; /* ecn seen */
  uint32_t loss:1; /* loss seen */
};

/* a 24 bit seqnum might be enough = 16 million packets.
   have to think about it */

struct seqnum {
  uint32_t seqnum:24;
  uint32_t flags:8;
};

typedef struct ACKS acks_t;

struct wire_acks {
  uint32_t seqnum;
  union {
    uint32_t dt;
    acks_t ack;
  } ts;
  uint32_t flags;
};

struct internal_acks {
  uint32_t seqnum;
  union {
    uint32_t dt;
    acks_t ack;
  } ts;
  uint32_t flags;
};

typedef struct wire_acks ack_t;

#endif

