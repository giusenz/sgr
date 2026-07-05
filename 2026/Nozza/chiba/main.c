#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>

#include "exporter.h"
#include "ring_buffer.h"
#include "collector.h"

#define SOFTFLOWD_PATH "/usr/local/sbin/softflowd"

int drop_privileges(const char *username);

void sigproc(int sig);
volatile sig_atomic_t running_flag = 1;

int xfork(void);
void xexecve(const char *pathname, char *const argv[], char *const envp[]);
void print_help(void);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        exit(EXIT_FAILURE);
    }
    
    signal(SIGINT, sigproc);
    signal(SIGTERM, sigproc);
    
    if (xfork() == 0) {
        argv[0] = "softflowd";
        xexecve(SOFTFLOWD_PATH, argv, NULL);            
    } else {
        int sockfd = init_collector_socket(PORT);

        if (sockfd == -1) {
            perror("failed to initialize collector socket");
            exit(EXIT_FAILURE);
        }

        drop_privileges("nobody");
        
        rbuffer *rb = xmalloc(sizeof(rbuffer));
        if (ring_buffer_init(rb) == -1) {
            perror("failed to initialize bounded buffer");
            exit(EXIT_FAILURE);
        }
        
        ebuffer *eb = xmalloc(sizeof(ebuffer));
        export_buffer_init(eb);

        ct_data ctd;
        ctd.sockfd = sockfd;
        ctd.buffer = rb;
        
        pthread_t collector_thread;
        if (pthread_create(&collector_thread, NULL, collector_thread_routine, &ctd) != 0) {
            perror("failed to create collector thread");
            exit(EXIT_FAILURE);
        }
        
        /* EXPORT CONTIGUOUS BUFFER LIFETIME */
        /* BATCH EXTRACTION */
        /* SEND WITH LIBCURL */

        if (pthread_join(collector_thread, NULL) != 0) {
            perror("pthread_join failed");
        } 
        
        ring_buffer_destroy(rb);

        export_buffer_destroy(eb);

        wait(NULL);
    }

    return 0;
}

int drop_privileges(const char *username) {
    struct passwd *pw = NULL;
    
    /* root has group id 0 and user id 0 */
    if (getgid() && getuid()) {
        fprintf(stderr, "privileges are not dropped as we're not superuser\n");
        return -1;
    }

    pw = getpwnam(username);
    
    if (pw == NULL) {
        username = "nobody";
        pw = getpwnam(username);
    }
    
    if(pw != NULL) {
        if(setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0) {
            fprintf(stderr, "unable to drop privileges [%s]\n", strerror(errno));
            return -1;
        } else {
            fprintf(stderr, "user changed to %s\n", username);
        }
    } else {
        fprintf(stderr, "unable to locate user %s\n", username);
        return -1;
    }

    umask(0);
    return 0;
}

void sigproc(int sig) {
  static int called = 0;
  if (called) return; else called = 1;
  running_flag = 0;
}

int xfork(void) {
    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return pid;
}

void xexecve(const char *pathname, char *const argv[], char *const envp[]) {
    if (execve(pathname, argv, envp) == -1) {
        fprintf(stderr, "execve error: %s\n", strerror(errno));
    }   
}

void print_help(void) {
    printf("Usage: ./chiba [OPTTIONS]\n");
    printf("\nThis program acts as a wrapper for softflowd. All standard softflowd flags are supported.\n");
    printf("For a complete list of supported flow export options, please run:\n");
    printf("$ softflowd -h \n$ man softflowd\n");
}