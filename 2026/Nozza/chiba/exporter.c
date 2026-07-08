#include "exporter.h"

CURL *curl_for_ch_init(void) {
    CURL *curl = curl_easy_init();
    if (curl != NULL) {
        /* destination endpoint config */
        curl_easy_setopt(curl, CURLOPT_URL, CH_TARGET_URL);
        /* 1 => post enabled for this handle */
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    }
    return curl;
}

u_int64_t get_time_ms(void) {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    
    return /* (sec * 1e3) + (nsec / 1e6) */
    (u_int64_t)tp.tv_sec * 1000 + (u_int64_t)tp.tv_nsec / 1000000;   
}

int export_buffer_init(ebuffer *eb) {
    eb->buffer = xmalloc(EXPORT_BUFFER_SIZE * sizeof(rbuffer_data));
    if (eb->buffer == NULL) {
        fprintf(stderr, "failed with export buffer memory allocation\n");
        return -1;
    }
    eb->size = EXPORT_BUFFER_SIZE;
    return 0;
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

int export_routine(rbuffer *rb, ebuffer *eb) {
    CURL *curl = curl_for_ch_init(); 
    
    if (curl == NULL) {
        fprintf(stderr, "failed with curl_easy_init()\n");
        return -1;
    }
    
    u_int64_t before, now;
    before = now = get_time_ms();
    while (running_flag) {
        now = get_time_ms();
        
        int batch_ret = 0;
        int sent      = 0;
        if (batch_transfer(rb, eb) > 0) 
            batch_ret = 1; 

        /* Volumetric trigger */
        if (!sent && (eb->nelem >= eb->size)) {
            //heree libcurl
            eb->nelem = 0;
            now = get_time_ms();
            sent = 1;
        }

        /* Time trigger */
        uint64_t delta = now - before;
        if (!sent && ((delta) >= EXPORT_MIN_TIME_MS)) {
            //[BLOCKING HTTP TRANSACTION]
            eb->nelem = 0;
            now = get_time_ms();
            sent = 1;
        }
        
        /* Sleep management */
        if (!sent && !batch_ret) {
            time_t sec  = (time_t)(delta / 1000);
            long nsec = (long)((delta % 1000) * 1000000);
            struct timespec request = { .tv_sec = sec, .tv_nsec = nsec };
            nanosleep(&request, NULL);
        }

        if (sent) before = now;
    }
    /* Flushing buffers */
    while (batch_transfer(rb, eb) > 0) {
        //[BLOCKING HTTP TRANSACTION]
    }
    if (eb->nelem > 0) {
        //http transaction with the last nelem elements
    }

    curl_easy_cleanup(curl);
    return 0;
}