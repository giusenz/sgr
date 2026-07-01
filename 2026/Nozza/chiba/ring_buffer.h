#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define RING_BUFFER_SIZE 16384

typedef struct ring_buffer_data {
    u_int32_t start_time;
    u_int32_t end_time;
    u_int32_t srcaddr;
    u_int32_t dstaddr;
    u_int32_t dPkts;
    u_int32_t dOctets;
    u_int16_t srcport;
    u_int16_t dstport;
    u_int8_t  prot;
} rbuffer_data;

typedef struct ring_buffer {
    struct ring_buffer_data *buffer;
    size_t size;
    size_t nelem;
    pthread_mutex_t lock;
    
    size_t head;
    pthread_cond_t not_full;  

    size_t tail;
    pthread_cond_t not_empty;
} rbuffer;

void *xmalloc(size_t size);

int ring_buffer_init(rbuffer *rb);
void ring_buffer_destroy(rbuffer *rb);

void ring_buffer_put(rbuffer *rb, rbuffer_data rbd);
rbuffer_data *ring_buffer_get(rbuffer *rb);

#endif