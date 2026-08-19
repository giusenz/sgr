#include "collector.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>

/* Socket lifecycle*/

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

/* 
 * This is the Cisco Netflow(tm) version 5 packet format
 * Based on:
 * https://www.cisco.com/c/en/us/td/docs/net_mgmt/netflow_collection_engine/3-6/user/guide/format.html#wp1007472 
 */

#define NF5_HEADER_LENGTH 24U
#define NF5_RECORD_LENGTH 48U
#define NF5_MAX_RECORDS   30U

/* The following NF5 structs are packed to ensure consistent layout across platforms.
 * The packed type attribute specifies 
 * that a type must have the smallest possible alignment. */
struct NF5_header {
    u_int16_t version;
    u_int16_t count;
    u_int32_t SysUptime;
    u_int32_t unix_secs;
    u_int32_t unix_nsecs;
    u_int32_t flow_sequence;
    u_int8_t  engine_type;
    u_int8_t  engine_id;
    u_int16_t sampling_interval;  
} __attribute__((packed)); 

struct NF5_record {
    u_int32_t srcaddr;
    u_int32_t dstaddr;
    u_int32_t nexthop;
    u_int16_t input;
    u_int16_t output;
    u_int32_t dPkts;
    u_int32_t dOctets;
    u_int32_t first;
    u_int32_t last;
    u_int16_t srcport;
    u_int16_t dstport;
    u_int8_t  pad1;
    u_int8_t  tcp_flags;
    u_int8_t  prot;
    u_int8_t  tos;
    u_int16_t src_as;
    u_int16_t dst_as;
    u_int8_t  src_mask;
    u_int8_t  dst_mask;
    u_int16_t pad2;
} __attribute__((packed));

static int deserialize_NF5_header(struct NF5_header *header, u_int8_t *buf) {
    if (header == NULL || buf == NULL) {
        fprintf(stderr, "failed with NF5 header deserialization\n");
        return -1;
    }
    memcpy(header, buf, sizeof(struct NF5_header)); 
    return 0;
}

static void parse_NF5_record(struct NF5_record *rec, u_int64_t boot_time_ms, flow_data *rbd) {
    rbd->srcaddr = ntohl(rec->srcaddr);
    rbd->dstaddr = ntohl(rec->dstaddr);
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

static void process_NF5_records(u_int8_t *buf, u_int16_t count, u_int64_t boot_time_ms, rbuffer *rb) {
    u_int8_t *rec_base_addr = buf + NF5_HEADER_LENGTH;
    for (int i = 0; i < count; i++) {
        struct NF5_record record = {0};
        u_int8_t *curr_base_addr = rec_base_addr + (i * NF5_RECORD_LENGTH);
        memcpy(&record, curr_base_addr, sizeof(struct NF5_record));
        
        flow_data rbd = {0};
        parse_NF5_record(&record, boot_time_ms, &rbd);
        ring_buffer_put(rb, rbd);
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
    while (1) {
        /* The collector thread must drain the socket 
         * completely before it is allowed to exit. */
        int timeout = running_flag ? 1000 : 0;
        
        int rtval = poll(&pfd, 1, timeout);
        if (rtval < 0) {
            if (errno == EINTR) continue;
            else {
                fprintf(stderr, "polling failed: %s\n", strerror(errno));
                break;
            }
        }
        if (rtval == 0) {
            if (running_flag == 0) break;
            continue;
        }
        //printf("[DEBUG] poll returned %d, revents: %d\n", rtval, pfd.revents);
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