/* protocol parser */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

typedef int32_t obj_t; // I don't know what struct this needs to be yet
typedef int8_t packet_t; // Same here

static bool parse_NONCE(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
} 

static bool parse_ATIME(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

static bool parse_REGISTER(obj_t *o, uint8_t *b, uint8_t size)
{ 
return(false);
}

static bool parse_COUNTERPROPOSAL(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

static bool parse_PROBE(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

static bool parse_PROBEACK(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

static bool parse_LSEEN(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

static bool parse_CPU(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

static bool parse_LOSSRECORD(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

static bool parse_TOSRECORD(obj_t *o, uint8_t *b, uint8_t size)
{ 
return(false);
}

static bool parse_CLOSEWAIT(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

static bool parse_CLOSE(obj_t *o, uint8_t *b, uint8_t size)
{
return(false);
}

/* The actual packet processing bits */

#define doADV(b) b = b + b[0];

// FIXME needs to check against end of packet

#define doIT(T) do { rc = parse_ ## T ( o,bytes,bytes[0] );\
                     doADV(bytes); } while (0)

int parse_packet(obj_t *o, packet_t *p, size_t size)
{

  uint8_t length;
  uint8_t * bytes = p;
  int processed = 0;
  int rc = 0;

  while(bytes < p) {
    switch(*bytes)
      {
      case PAD:    bytes++; break; 
      case PADN:  
      case RANDN: doADV(bytes); break; 
      case NONCE: doIT(NONCE); break; 
      case ATIME: doIT(ATIME); break;
      case REGISTER: doIT(REGISTER); break; 
      case COUNTERPROPOSAL: doIT(COUNTERPROPOSAL); break;
      case PROBE: doIT(PROBE); break;
      case PROBEACK: doIT(PROBEACK); break;
      case LSEEN: doIT(LSEEN); break; 
      case CPU: doIT(CPU); break; 
      case LOSSRECORD: doIT(LOSSRECORD); break; 
      case TOSRECORD: doIT(TOSRECORD); break; 
      case CLOSEWAIT: doIT(CLOSEWAIT); break;
      case CLOSE: doIT(CLOSE); break;
      }
    if(rc != 0)
      printf("Error in message processing ignored\n");
  }
}

#ifdef TEST
main() {
}
#endif
