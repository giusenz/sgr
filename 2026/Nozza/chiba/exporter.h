#ifndef EXPORTER_H
#define EXPORTER_H  

#include "ring_buffer.h"
#include <signal.h>
#include <time.h>

/* The linear export buffer 
 * is dimensioned on ClickHouse writing efficiency
 * and on the optimal throughput of the HTTP payload. 
 * 
 * Double buffering optimization was adopted. 
 * Here is a practical example. 
 * export_buffer: **
 * ring_buffer  : ****
 * (see ring_buffer.h for further explanation)
 */
#define EXPORT_BUFFER_SIZE 16384 

#define EXPORT_MIN_TIME_MS 5000

extern volatile sig_atomic_t running_flag;

typedef struct export_buffer {
    struct ring_buffer_data *buffer;
    size_t size;
    size_t nelem; 
} ebuffer;

/* Compute time difference in milliseconds */
long delta_time(struct timeval *now, struct timeval *before);

void export_buffer_init(ebuffer *eb);
void export_buffer_destroy(ebuffer *eb);

size_t batch_transfer(rbuffer *rb, ebuffer *eb);

void export_routine(rbuffer *rb, ebuffer *eb);

#endif

 