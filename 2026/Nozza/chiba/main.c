#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "ring_buffer.h"
#include "collector.h"

#define SOFTFLOWD_PATH "/usr/local/sbin/softflowd"

volatile sig_atomic_t running_flag = 1;

int xfork(void);
void sigproc(int sig);
void xexecve(const char *pathname, char *const argv[], char *const envp[]);
void print_help(void);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        exit(EXIT_FAILURE);
    }

    if (xfork() == 0) {
        argv[0] = "softflowd";
        xexecve(SOFTFLOWD_PATH, argv, NULL);            
    } else {
        int sockfd = init_collector_socket(PORT);
        if (sockfd == -1) {
            perror("failed to initialize collector socket");
            exit(EXIT_FAILURE);
        }
        
        rbuffer *rb = xmalloc(sizeof(rbuffer));
        if (ring_buffer_init(rb) == -1) {
            perror("failed to initialize bounded buffer");
            exit(EXIT_FAILURE);
        }
        
        struct collector_thread_data ctd;
        ctd.sockfd = sockfd;
        ctd.buffer = rb;
        
        pthread_t collector_thread;
        if (pthread_create(&collector_thread, NULL, collector_thread, &ctd) != 0) {
            perror("failed to create collector thread");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

int xfork(void) {
    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    return pid;
}

void sigproc(int sig) {
  static int called = 0;

  fprintf(stderr, "Leaving...\n");
  if (called) return; else called = 1;

  running_flag = 0;
}

void xexecve(const char *pathname, char *const argv[], char *const envp[]) {
    if (execve(pathname, argv, envp) == -1) {
        fprintf(stderr, "execve error: %d\n", errno);
    }   
}

void print_help(void) {
    printf("Usage: ./chiba [OPTTIONS]\n");
    printf("\nThis program acts as a wrapper for softflowd. All standard softflowd flags are supported.\n");
    printf("For a complete list of supported flow export options, please run:\n");
    printf("$ softflowd -h \n$ man softflowd\n");
}