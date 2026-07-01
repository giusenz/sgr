#include "collector.h"

void *get_in_addr(struct sockaddr *sa) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

int init_collector_socket(const char *port) {
    int sockfd, rv;
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        perror("failed to bind socket");
        return 1;
    }
    
    freeaddrinfo(servinfo);
    return sockfd;
}

void *collector_worker_thread(void *args) {
    puts("Collector: booting up...");

    int sockfd = *(int *)args;
    struct sockaddr_storage their_addr; 
    socklen_t addr_len;
    uint8_t buf[MAXBUFLEN]; 
    ssize_t numbytes;       
    char s[INET_ADDRSTRLEN];

    while (running_flag) {
        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN, 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            if (running_flag == 0) break;
            perror("recvfrom");
            continue;
        }
        /*Packet under-reading */
        if (numbytes < NF5_HEADER_LENGTH) {
            //dropped_packets++;
            continue;
        }
        struct NF5_header *header = (struct NF5_header *) buf;
        u_int16_t count = ntohs(header->count);
        /*Packet over-reading */
        if (NF5_HEADER_LENGTH + NF5_RECORD_LENGTH * count > (size_t)numbytes) {
            //dropped_packets++; 
            continue;
        }

        u_int32_t SysUptime    = ntohl(header->SysUptime); 
        u_int32_t unix_secs    = ntohl(header->unix_secs);
        u_int32_t unix_nsecs   = ntohl(header->unix_nsecs);
        u_int64_t boot_time_ms = ((u_int64_t)unix_secs * 1000) - SysUptime;
        struct NF5_record *records = (struct NF5_record *) (buf + NF5_HEADER_LENGTH);
        for (int i = 0; i < count; i++) {
            struct NF5_record *rec = &records[i];
            rbuffer_data nrdata;
            nrdata.srcaddr = rec->srcaddr;
            nrdata.dstaddr = rec->dstaddr;
            nrdata.dPkts   = ntohl(rec->dPkts);
            nrdata.dOctets = ntohl(rec->dOctets);
            nrdata.srcport = ntohs(rec->srcport);
            nrdata.dstport = ntohs(rec->dstport);
            nrdata.prot    = rec->prot;
            uint64_t start_time_ms = boot_time_ms + ntohl(rec->first);
            uint64_t end_time_ms   = boot_time_ms + ntohl(rec->last);
            nrdata.start_time = (uint32_t)(start_time_ms / 1000);
            nrdata.end_time   = (uint32_t)(end_time_ms / 1000);
        }
    }
    
    close(sockfd);
    puts("Collector: shutting down...");
    return NULL;
}