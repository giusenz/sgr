#include "collector.h"

void *get_in_addr(struct sockaddr *sa) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

int set_nonblocking_fd(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) { 
        fprintf(stderr, "fcntl F_GETFL\n"); return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        fprintf(stderr, "fcntl F_SETFL\n"); return -1;
    }
    return 0;
}

int init_collector_socket() {
    int sockfd, rv;
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;
    if (((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            fprintf(stderr, "socket failed\n"); 
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            fprintf(stderr, "bind failed: %s\n", strerror(errno));
            continue;
        }
        break;
    }
    
    freeaddrinfo(servinfo);
    
    if (p == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        return -1;
    }    

    if (set_nonblocking_fd(sockfd) != 0) {
        fprintf(stderr, "failed to set non-blocking socket\n");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int deserialize_NF5_header(struct NF5_header *header, u_int8_t *buf) {
    if (header == NULL || buf == NULL) {
        fprintf(stderr, "failed with NF5 header deserialization\n");
        return -1;
    }
    memcpy(header, buf, sizeof(struct NF5_header)); 
    return 0;
}

void parse_NF5_record(struct NF5_record *rec, u_int64_t boot_time_ms, rbuffer_data *rbd) {
    rbd->srcaddr = rec->srcaddr;
    rbd->dstaddr = rec->dstaddr;
    rbd->dPkts   = ntohl(rec->dPkts);
    rbd->dOctets = ntohl(rec->dOctets);
    rbd->srcport = ntohs(rec->srcport);
    rbd->dstport = ntohs(rec->dstport);
    rbd->prot    = rec->prot;

    uint64_t start_time_ms = boot_time_ms + ntohl(rec->first);
    uint64_t end_time_ms   = boot_time_ms + ntohl(rec->last);
    
    rbd->start_time = (uint32_t)(start_time_ms / 1000);
    rbd->end_time   = (uint32_t)(end_time_ms / 1000);
}

void process_NF5_records(u_int8_t *buf, u_int16_t count, u_int64_t boot_time_ms, rbuffer *rbuffer) {
    u_int8_t *rec_base_addr = buf + NF5_HEADER_LENGTH;
    for (int i = 0; i < count; i++) {
        struct NF5_record record;
        u_int8_t *curr_base_addr = rec_base_addr + (i * NF5_RECORD_LENGTH);
        memcpy(&record, curr_base_addr, sizeof(struct NF5_record));
        
        rbuffer_data rbd;
        parse_NF5_record(&record, boot_time_ms, &rbd);
        ring_buffer_put(rbuffer, rbd);
    }
}

void *collector_thread_routine(void *args) {
    ct_data *ctd = (ct_data *) args;
    struct sockaddr_storage their_addr; 
    socklen_t addr_len;
    uint8_t buf[MAXBUFLEN]; 
    ssize_t numbytes;       
    
    struct pollfd pfd;
    pfd.fd     = ctd->sockfd;
    pfd.events = POLLIN;
    while (running_flag) {
        int rtval = poll(&pfd, 1, 1000);
        if (rtval < 0) {
            if (errno == EINTR) continue;
            else {
                fprintf(stderr, "polling failed: %s\n", strerror(errno));
                break;
            }
        }
        if (rtval == 0) continue;
        if (pfd.revents & POLLIN) {
            addr_len = sizeof their_addr;
            if ((numbytes = recvfrom(ctd->sockfd, buf, MAXBUFLEN, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                if (running_flag == 0) break;
                fprintf(stderr, "recvfrom\n");
                continue;
            }

            /* Packet under-reading */
            if (numbytes < NF5_HEADER_LENGTH) {
                continue;
            }
            
            struct NF5_header header;
            if (deserialize_NF5_header(&header, buf) != 0) {
                continue;
            }
            
            /* Packet over-reading */
            u_int16_t count = ntohs(header.count);
            if (NF5_HEADER_LENGTH + NF5_RECORD_LENGTH * count > (size_t)numbytes) {
                continue;
            }  
            
            u_int32_t SysUptime    = ntohl(header.SysUptime); 
            u_int32_t unix_secs    = ntohl(header.unix_secs);
            u_int64_t boot_time_ms = ((u_int64_t)unix_secs * 1000) - SysUptime;
            
            process_NF5_records(buf, count, boot_time_ms, ctd->buffer);
        }    
    }
    return NULL;
}