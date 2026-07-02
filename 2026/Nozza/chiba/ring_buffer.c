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
    rb->size   = RING_BUFFER_SIZE;
    rb->nelem  = 0;
    if (pthread_mutex_init(&rb->lock, NULL) != 0) {
        free(rb->buffer); return -1;
    }

    rb->head = 0;
    if (pthread_cond_init(&rb->not_full, NULL) != 0) {
        pthread_mutex_destroy(&rb->lock);
        free(rb->buffer);
        return -1;
    }

    rb->tail = 0;
    if (pthread_cond_init(&rb->not_empty, NULL) != 0) {
        pthread_cond_destroy(&rb->not_full);
        pthread_mutex_destroy(&rb->lock);
        free(rb->buffer);
        return -1;
    }

    return 0;
}

void ring_buffer_destroy(rbuffer *rb) {
    pthread_mutex_destroy(&rb->lock);
    pthread_cond_destroy(&rb->not_full);
    pthread_cond_destroy(&rb->not_empty);
    free(rb);
}

void ring_buffer_put(rbuffer *rb, rbuffer_data rbd) {
    pthread_mutex_lock(&rb->lock);
    
    while (rb->nelem >= rb->size) {
        pthread_cond_wait(&rb->not_full, &rb->lock);
    }
    rb->buffer[rb->tail] = rbd;
    rb->tail = (rb->tail + 1) & (rb->size - 1);
    rb->nelem++;
    pthread_cond_signal(&rb->not_empty);

    pthread_mutex_unlock(&rb->lock);
}

rbuffer_data ring_buffer_get(rbuffer *rb) {
    rbuffer_data rbd;
    pthread_mutex_lock(&rb->lock);

    while (rb->nelem == 0) {
        pthread_cond_wait(&rb->not_empty, &rb->lock);
    } 
    rbd = rb->buffer[rb->head];
    rb->head = (rb->head + 1) & (rb->size - 1);
    rb->nelem--;
    pthread_cond_signal(&rb->not_full);


    pthread_mutex_unlock(&rb->lock);
    return rbd;
}