#include "exporter.h"

u_int64_t get_time_ms(void) {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    
    return 
    (u_int64_t)tp.tv_sec * 1000 + (u_int64_t)tp.tv_nsec / 1000000;  
}

void export_buffer_init(ebuffer *eb) {
    eb->buffer = xmalloc(EXPORT_BUFFER_SIZE * sizeof(rbuffer_data));
    eb->size   = EXPORT_BUFFER_SIZE;
}

void export_buffer_destroy(ebuffer *eb) {
    free(eb->buffer);
    free(eb);
}

size_t batch_transfer(rbuffer *rb, ebuffer *eb) {  
    pthread_mutex_lock(&rb->lock);
    
    /* non-blocking */
    if (rb->nelem == 0) {
        pthread_mutex_unlock(&rb->lock);
        return 0;
    }
    size_t eb_residual_cap = eb->size - eb->nelem;
    size_t n = (rb->nelem) > (eb_residual_cap) ? eb_residual_cap : rb->nelem;
    /* NO WRAP-AROUND: n <= av_space (available space in ring buffer). 
     * Single call to memcpy. Source starts at rb->head.
     * WRAP_AROUND: n > av_space => Double call to memcpy needed.
     * The source intervals to cover are 
     * [head, head + av_space] and [0, n - av_space]    
     */
    size_t av_space = rb->size - rb->head;
    size_t rdata_sz = sizeof(rbuffer_data);
    if (n <= av_space) {
        memcpy(&eb->buffer[eb->nelem], &rb->buffer[rb->head], (n * rdata_sz));
    } else {
        memcpy(&eb->buffer[eb->nelem], &rb->buffer[rb->head], (av_space * rdata_sz));
        memcpy(&eb->buffer[(eb->nelem + av_space)], &rb->buffer[0], ((n - av_space) * rdata_sz));       
    }
    
    rb->head  = (rb->head + n) & (rb->size - 1);
    rb->nelem = rb->nelem - n;
    eb->nelem = eb->nelem + n;
    
    pthread_mutex_unlock(&rb->lock);
    return n;
}

void export_routine(rbuffer *rb, ebuffer *eb) {
    u_int64_t before_ms = get_time_ms();
    u_int64_t now_ms    = get_time_ms();
    while (running_flag) {
        now_ms    = get_time_ms();
        int batch_ret = 0;
        int sent      = 0;
        if (batch_transfer(rb, eb) > 0) 
            batch_ret = 1; 

        /* Volumetric trigger */
        if (!sent && (eb->nelem >= eb->size)) {
            //[BLOCKING HTTP TRANSACTION]
            eb->nelem = 0;
            now_ms = get_time_ms();
            sent = 1;
        }

        /* Time trigger */
        if (!sent && ((now_ms - before_ms) >= EXPORT_MIN_TIME_MS)) {
            //[BLOCKING HTTP TRANSACTION]
            eb->nelem = 0;
            now_ms = get_time_ms();
            sent = 1;
        }
        
        /* Sleep management */
        if (!sent && !batch_ret) {
            //sleep(min-delta)
        }

        if (sent) before_ms = now_ms;
    }
}