#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define PORT "9995"
#define MAXBUFLEN 2048

/* 
 * This is the Cisco Netflow(tm) version 5 packet format
 * Based on:
 * https://www.cisco.com/c/en/us/td/docs/net_mgmt/netflow_collection_engine/3-6/user/guide/format.html#wp1007472
 */

#define NF5_HEADER_LENGTH 24U
#define NF5_RECORD_LENGTH 48U
#define NF5_MAX_RECORDS   30U

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

struct normalized_NF5_record_data {
    u_int32_t start_time;
    u_int32_t end_time;
    u_int32_t srcaddr;
    u_int32_t dstaddr;
    u_int32_t dPkts;
    u_int32_t dOctets;
    u_int16_t srcport;
    u_int16_t dstport;
    u_int8_t  prot;
};

extern volatile sig_atomic_t running_flag;
void sigproc(int sig);

/* Given a sockaddr, this function returns its IPv4 socket address */
void *get_in_addr(struct sockaddr *sa);

/* Given a port, this function listens on socket file descriptor, 
 * establishes a new connection on it and performs the binding. 
 * On success, the opened socket file descriptor is returned. */
int init_collector_socket(const char *port);

/* This function implements the packets collection logic by processing NF5 packets 
 * and by producing them on a ring bounded buffer */
// DEFINE RING BUFFER STRUCTURE 
// DEFINE THREAD DATA (see SYNCHRONIZATION)
// DROPPED UNDER READ PARAMETER (IF NECESSARY)
void *collector_worker_thread(void *args); 

#endif 