#ifndef EXPORTER_H
#define EXPORTER_H  

#include "ring_buffer.h"

#define EXPORT_BUFFER_SIZE 32768

typedef struct export_buffer {
    struct ring_buffer_data *buffer;
    size_t size;
    size_t nelem; 
} ebuffer;

void export_buffer_init(ebuffer *eb);
void export_buffer_destroy(ebuffer *eb);

/* Batch transfer from shared buffer to exportation buffer 
 * The number of bytes to transfer (let us call it N) is the minimum 
 * between rb->nelem, which are the elements ready to be consumed, and
 * eb residual capacity, i.e. eb->size - eb->nelem.
 * Here is the formula implemented:
 *     N = min(rb->elem, eb->size - eb->nelem)  
 */
void batch_transfer(rbuffer *rb, ebuffer *eb);

#endif

 