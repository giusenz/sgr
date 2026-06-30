#include "collector.h"

volatile sig_atomit_t running_flag = 1;

void sigproc(int sig) {
  static int called = 0;

  fprintf(stderr, "Leaving...\n");
  if (called) return; else called = 1;

  running_flag = 0;
}

void *get_in_addr(struct sockaddr *sa) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

int init_collector_socket(const char *port) {
    int sockfd;
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
    
    puts("Collector: listening...");
    
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
        
        printf("Collector: got packet from %s\n", 
            inet_ntop(their_addr.ss_family, 
                get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
        
        /* Raw exported data parsing logic*/
        struct NF5_header *header = (struct NF5_header *) buf;
        u_int16_t version = ntohs(header->version);
        u_int16_t count   = ntohs(header->count);
        u_int32_t uptime  = ntohl(header->uptime); 

        size_t offset = sizeof(struct NF5_header);
        for (int i = 0; i < count; i++) {
            /* Buffer over-reading handler */
            if (offset + sizeof(struct NF5_record) > (size_t)numbytes) {
                
            }
        }
    }
    
    close(sockfd);
    return NULL;
}