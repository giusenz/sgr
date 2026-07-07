#include "exporter.h"

long delta_time(struct timeval *now, struct timeval *before) {
    
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
    while (running_flag) {
        int batch_ret, sent;
        batch_ret = sent = 0;
        size_t n = batch_transfer(rb, eb);
        
        if (n > 0) {
            batch_ret = 1;
            eb->nelem += n;
        }
        
        /* Volumetric trigger */
        if (eb->nelem >= eb->size) {
            //[BLOCKING HTTP TRANSACTION]
            eb->nelem = 0;
            //update last send time
            sent = 1;
        }
        /* Time trigger */
        // delta-time >= MIN. => SEND AND ALL THE CONSEQUENCES 
        //[BLOCKING HTTP TRANSACTION]
        
        /* Sleep management */
        if (batch_ret == 0 && sent == 0) {
            // CURR - LAST-SEND OPT
        }
    }
}