#include "wrappers.h"

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "malloc error: %s\n", strerror(errno));    
    }
    return ptr;
}

int xfork(void) {
    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork error: %s\n", strerror(errno));
    }
    return pid;
}

/* Implemented safe child process termination */
void xexecve(const char *pathname, char *const argv[], char *const envp[]) {
    if (execve(pathname, argv, envp) == -1) {
        fprintf(stderr, "execve error: %s\n", strerror(errno));
        /* _exit requires process termination */
        _exit(EXIT_FAILURE);
    }   
}