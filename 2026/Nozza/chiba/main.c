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