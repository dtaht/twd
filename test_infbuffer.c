
#ifdef __GNUC__
#  define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>

#include "infbuffer.h"

#define MEMSTATIC
#define MAGIC	0x00435053

/**************************************************************************/

typedef struct
{
  uint32_t magic;
  size_t   id;
  size_t   seqno;
  char     data[100];
} payload__s;

typedef struct
{
  infbuffer__s *ibuf;
  payload__s   *loads;	/* only used if MEMSTATIC defined */
  size_t        id;
  size_t        maxseq;
} writer__s;

typedef struct
{
  infbuffer__s *ibuf;
  size_t        writers;
  size_t        readers;
  size_t       *seqnos;  
  size_t        maxseq;
} reader__s;

/**************************************************************************/

void *thread_writer(void *data)
{
  writer__s *info  = data;

  for (size_t seqno = 0 ; seqno < info->maxseq ; seqno++)
  {
#ifdef MEMSTATIC
    payload__s *load = &info->loads[seqno];
#else
    payload__s *load = calloc(1,sizeof(payload__s));
#endif

    /*-------------------------------------------------------------------
    ; fill in our structure.  The reader is checking for a higher sequence
    ; number than the previous packet.  The sequence number IT starts with
    ; is 0, so here, we ensure that we send sequence numbers from 1 on up.
    ;--------------------------------------------------------------------*/
    
    load->magic = MAGIC;
    load->id    = info->id;
    load->seqno = seqno + 1;
    infbuffer_add(info->ibuf,load);
  }

  return NULL;
}

/**************************************************************************/

void *thread_reader(void *data)
{
  reader__s  *info = data;
  payload__s *load;
  void       *p;
  size_t      seq;
  
  do
  {
    infbuffer_rem(info->ibuf,&p);
    load = p;
    if (load == NULL) break;

    assert(load        != NULL);
    assert(load->magic == MAGIC);
    assert(load->id    <  info->writers);

    /*----------------------------------------------------------------------
    ; if there are multiple readers, we can't be sure of getting the actual
    ; next sequence from a particular writer.  We will, however, get a
    ; sequence number that is higher.
    ;-----------------------------------------------------------------------*/
    
    assert(load->seqno > info->seqnos[load->id]);
    info->seqnos[load->id] = load->seqno;

    seq = load->seqno;
#ifndef MEMSTATIC
    free(load);
#endif
  } while(seq < info->maxseq);
  
  return NULL;
}

/**************************************************************************/    

int main(int argc,char *argv[])
{
  size_t writers = 1;
  size_t readers = 1;
  size_t items   = 16384;
  int    c;
  
  while((c = getopt(argc,argv,"w:r:i:h")) != EOF)
  {
    switch(c)
    {
      case 'w':
           writers = strtoul(optarg,NULL,10);
           break;
           
      case 'r':
           readers = strtoul(optarg,NULL,10);
           break;
           
      case 'i':
           items = strtoul(optarg,NULL,10);
           break;
           
      case 'h':
      default:
           fprintf(
           	stderr,
           	"usage: %s [-w writers (%lu)] [-r readers (%lu)] [-i items (%lu)]\n",
           	argv[0],
           	(unsigned long)writers,
           	(unsigned long)readers,
           	(unsigned long)items
           );
           return EXIT_FAILURE;
    }
  }
  
  printf(
  	"writers: %lu\n"
  	"readers: %lu\n"
  	"items:   %lu\n"
  	"pid:     %lu\n"
  	"\n",
  	(unsigned long)writers,
  	(unsigned long)readers,
  	(unsigned long)items,
  	(unsigned long)getpid()
  );
  
  infbuffer__s  buffer;
  writer__s     wdata[writers];
  reader__s     rdata[readers];
  pthread_t     wids [writers];
  pthread_t     rids [readers];
  int           rc;
  
  rc = infbuffer_init(&buffer,items);
  if (rc != 0)
  {
    fprintf(stderr,"infbuffer_init() = %s\n",strerror(rc));
    return EXIT_FAILURE;
  }
  
  for (size_t i = 0 ; i < readers ; i++)
  {
    rdata[i].ibuf    = &buffer;
    rdata[i].writers = writers;
    rdata[i].readers = readers;
    rdata[i].maxseq  = (items / writers) - 1;
    rdata[i].seqnos  = calloc(writers,sizeof(unsigned long));
    if (rdata[i].seqnos == NULL)
    {
      fprintf(stderr,"calloc(seqnos) = %s\n",strerror(errno));
      return EXIT_FAILURE;
    }
    
    rc = pthread_create(&rids[i],NULL,thread_reader,&rdata[i]);
    if (rc != 0)
    {
      fprintf(stderr,"pthread_create() = %s\n",strerror(errno));
      return EXIT_FAILURE;
    }
  }
  
  for (size_t i = 0 ; i < writers ; i++)
  {
    wdata[i].ibuf   = &buffer;
    wdata[i].id     = i;
    wdata[i].maxseq = items / writers;

#ifdef MEMSTATIC
    wdata[i].loads  = calloc(wdata[i].maxseq,sizeof(payload__s));
#endif

    rc = pthread_create(&wids[i],NULL,thread_writer,&wdata[i]);
    if (rc != 0)
    {
      fprintf(stderr,"pthread_create() = %s\n",strerror(errno));
      return EXIT_FAILURE;
    }
  }
  
  for (size_t i = 0 ; i < writers ; i++)
    pthread_join(wids[i],NULL);

  for (size_t i = 0 ; i < readers ; i++)
    pthread_join(rids[i],NULL);

  return EXIT_SUCCESS;
}
