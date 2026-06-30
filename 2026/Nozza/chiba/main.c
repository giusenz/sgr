#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define SOFTFLOWD_PATH "/usr/local/sbin/softflowd"

#define BUFFER_SIZE 2048

/* Wrappers for fork and execv */
int xfork(void) {
    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    return pid;
}

void xexecve(const char *pathname, char *const argv[], char *const envp[]) {
    if (execve(pathname, argv, envp) == -1) {
        fprintf(stderr, "execve error: %d\n", errno);
    }   
}

/* Help message for the main process*/
void print_help(void) {
    printf("Usage: ./chiba [OPTTIONS]\n");
    printf("\nThis program acts as a wrapper for softflowd. All standard softflowd flags are supported.\n");
    printf("For a complete list of supported flow export options, please run:\n");
    printf("$ softflowd -h \n$ man softflowd\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        exit(EXIT_FAILURE);
    }

    if (xfork() == 0) {
        argv[0] = "softflowd";
        xexecve(SOFTFLOWD_PATH, argv, NULL);
    } else {
        /* COLLECTION FUNCTIONS HERE */
        /* AUXILIARY THREAD STARTS HERE [---> CH LOGIC]*/
        /* ANALYZER HERE */
    }

    return 0;
}

