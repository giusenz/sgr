#include "exporter.h"

void export_buffer_init(ebuffer *eb) {
    eb->buffer = xmalloc(EXPORT_BUFFER_SIZE * sizeof(rbuffer_data));
    eb->size   = EXPORT_BUFFER_SIZE;
}

void export_buffer_destroy(ebuffer *eb) {
    free(eb->buffer);
    free(eb);
}

void batch_transfer(rbuffer *rb, ebuffer *eb) {
    
}