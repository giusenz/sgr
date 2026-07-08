#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "wrappers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

/* The ring buffer is dimensioned 
 * exclusively to absorb processed NF5 records (see collector.c)
 * while the exporter is performing a blocking HTTP transaction.
 * 
 * Thanks to double buffering, when a batch_transfer() is performed (exporter.c),
 * the collector has a free half of the ring buffer where incoming data
 * can be written without risking to overwrite unread data at the tail. 
 * This prevents data loss due to buffer saturation */
#define RING_BUFFER_SIZE 32768

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
    pthread_mutex_t lock;
    struct ring_buffer_data *buffer;
    size_t size;
    size_t nelem;    
    size_t head;
    size_t tail;
} rbuffer;

int ring_buffer_init(rbuffer *rb);
void ring_buffer_destroy(rbuffer *rb);

void ring_buffer_put(rbuffer *rb, rbuffer_data rbd);

#endif