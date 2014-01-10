#ifndef _ringbuffer_h
#define _ringbuffer_h
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <memory.h>
#include "atomic.h"

struct queue
{
    void** buf;
    size_t num;
    uint64_t writePos;
    uint64_t readPos;
};

typedef struct queue queue_t;

static inline queue_t* ring_createQueue(size_t num)
{
    queue_t * new = malloc(sizeof(*new));
    new->buf = malloc(sizeof(void*) * num);
    memset(new->buf, 0, sizeof(void*)*num);
    new->readPos = 0;
    new->writePos = 0;
    new->num = num;
    return new;
}

static inline void ring_destroyQueue(queue_t* queue)
{
    if(queue)
    {
        if(queue->buf) free(queue->buf);
    }
    free(queue);
}

/* Fixme. Need a peek routine */

#define kNumTries 3 /* Have no idea what this does */

static inline int ring_enqueue(queue_t* queue, void* item)
{
    for(int i = 0; i < kNumTries; i++)
    {
      if(atomic_compare_and_swap(&queue->buf[queue->writePos % queue->num], NULL, item))  // this is is basically wrong
        {
            atomic_fetch_and_add(&queue->writePos, 1);
            return 0;
        }
    }
    return -1;
}

static inline void* ring_dequeue(queue_t* queue)
{
    for(int i = 0; i < kNumTries; i++)
    {
        void* value = queue->buf[queue->readPos % queue->num];
        if(value && atomic_compare_and_swap(&queue->buf[queue->readPos % queue->num], value, NULL)) // basically wrong
        {
            atomic_fetch_and_add(&queue->readPos, 1);
            return value;
        }
    }
    return NULL;
}

#undef kNumTries

#endif
