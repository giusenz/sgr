#include "ring_buffer.h"

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "malloc error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ptr;
}

int ring_buffer_init(rbuffer *rb){
    rb->buffer = xmalloc(RING_BUFFER_SIZE * sizeof(rbuffer_data));
    if (pthread_mutex_init(&rb->lock, NULL) != 0) {
        free(rb->buffer); return -1;
    }

    rb->size   = RING_BUFFER_SIZE;
    rb->nelem  = 0;

    rb->head = 0;
    rb->tail = 0;

    return 0;
}

void ring_buffer_destroy(rbuffer *rb) {
    free(rb->buffer);
    pthread_mutex_destroy(&rb->lock);
    free(rb);
}

void ring_buffer_put(rbuffer *rb, rbuffer_data rbd) {
    pthread_mutex_lock(&rb->lock);
    
    rb->buffer[rb->tail] = rbd;
    rb->tail = (rb->tail + 1) & (rb->size - 1);
    rb->nelem++;

    pthread_mutex_unlock(&rb->lock);
}
