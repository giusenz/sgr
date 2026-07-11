#include "wrappers.h"
#include "exporter.h"
#include "ring_buffer.h"
#include "collector.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>

#define SOFTFLOWD_PATH "/usr/local/sbin/softflowd"

int drop_privileges(const char *username);

void sigproc(int sig);
volatile sig_atomic_t running_flag = 1;

void print_help(void);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return (EXIT_FAILURE);
    }
    
    signal(SIGINT, sigproc);
    signal(SIGTERM, sigproc);
    
    int pid = xfork();
    
    if (pid == -1) {
        return (EXIT_FAILURE);
    }

    if (pid == 0) {
        argv[0] = "softflowd";
        xexecve(SOFTFLOWD_PATH, argv, NULL);
    }            
    
    int status;
    /* Waiting is not blocked */
    if (waitpid(pid, &status, WNOHANG) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
            fprintf(stderr, "softflowd boot failed\n");
            return (EXIT_FAILURE);
        }
    } 

    int sockfd = init_collector_socket(PORT);
    if (sockfd == -1) {
        fprintf(stderr, "failed to initialize collector socket\n");
        goto err_child;
    }

    drop_privileges("nobody");
        
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {   
        fprintf(stderr, "failed with curl_global_init\n");
        goto err_socket;
    }

    rbuffer *rb = xmalloc(sizeof(rbuffer));
    if (rb == NULL) {
        fprintf(stderr, "failed to allocate memory for ring buffer wrapper\n");
        goto err_curl;
    }
    if (ring_buffer_init(rb) != 0) {
        fprintf(stderr, "failed to initialize ring buffer\n");
        free(rb);
        goto err_curl;
    }
        
    ebuffer *eb = xmalloc(sizeof(ebuffer));
    if (eb == NULL) {
        fprintf(stderr, "failed to allocate memory for export buffer wrapper\n");
        goto err_rb;
    }
    if (export_buffer_init(eb) != 0) {
        fprintf(stderr, "failed to initialize export buffer\n");
        free(eb);
        goto err_rb;
    }

    ct_data ctd;
    ctd.sockfd = sockfd;
    ctd.buffer = rb;
        
    pthread_t collector_thread;
    if (pthread_create(&collector_thread, NULL, collector_thread_routine, &ctd) != 0) {
        fprintf(stderr, "failed to create collector thread\n");
        goto err_rb;
    }

    if (export_routine(rb, eb) != 0) {
        running_flag = 0;
    }

    if (pthread_join(collector_thread, NULL) != 0) {
        fprintf(stderr, "pthread_join failed\n");
        goto err_eb;
    } 
    
    /* LIFO cleanup */
    export_buffer_destroy(eb);
    ring_buffer_destroy(rb);
    curl_global_cleanup();
    close(sockfd);
    
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);

    return (EXIT_SUCCESS);

    /* LIFO cascade cleanup */
    err_eb:
        export_buffer_destroy(eb);
    err_rb:
        ring_buffer_destroy(rb);
    err_curl:
        curl_global_cleanup();
    err_socket:
        close(sockfd);
    err_child:
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
    
    return (EXIT_FAILURE);
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
    (void)sig;

    static int called = 0;
    if (called) return; else called = 1;
    running_flag = 0;
}

void print_help(void) {
    printf("Usage: ./chiba [OPTTIONS]\n");
    printf("\nThis program acts as a wrapper for softflowd. All standard softflowd flags are supported.\n");
    printf("For a complete list of supported flow export options, please run:\n");
    printf("$ softflowd -h \n$ man softflowd\n");
}