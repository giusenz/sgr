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

#define SOFTFLOWD_TARGET_EXPORT "127.0.0.1:9995"
#define NETFLOW_VERSION         "5"

int drop_privileges(const char *username);

void sigproc(int sig);
volatile sig_atomic_t running_flag = 1;

void print_usage(void);

int main(int argc, char *argv[]) {
    char *device    = NULL;
    char *pcap_path = NULL;
    u_char c;
    
    while((c = getopt(argc, argv, "hi:r:")) != '?') {
        if((c == 255) || (c == (u_char)-1)) break;
        
        switch(c) {
            case 'h':
                print_usage();
                return (EXIT_SUCCESS);
                break;
            case 'i':
                device = strdup(optarg);
                break;
            case 'r':
                pcap_path = strdup(optarg);
                break;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Input error: unexpected arguments detected.\n");
        print_usage();
        if (device)    free(device);
        if (pcap_path) free(pcap_path);
        return (EXIT_FAILURE);
    }

    if (device && pcap_path) {
        fprintf(stderr, "Input error: -i and -r are mutually exclusive. Choose a single source.\n");
        print_usage();
        return (EXIT_FAILURE);
    }

    if (!device && !pcap_path) {
        fprintf(stderr, "Input error: choose a source (-i or -r)\n");
        print_usage();
        return (EXIT_FAILURE);
    }
    
    signal(SIGINT, sigproc);
    signal(SIGTERM, sigproc);
    
    int pid = xfork();

    if (pid == -1) {
        return (EXIT_FAILURE);
    }

    if (pid == 0) {
        char* cargv[20];
        int i = 0;
        cargv[i++] = "softflowd";
        cargv[i++] = "-d";
        cargv[i++] = "-n";
        cargv[i++] = SOFTFLOWD_TARGET_EXPORT;
        cargv[i++] = "-v";
        cargv[i++] = NETFLOW_VERSION;

        if (device) {
            cargv[i++] = "-i";
            cargv[i++] = device;
            cargv[i++] = "-t";
            cargv[i++] = "maxlife=60s";
            cargv[i++] = "-t";
            cargv[i++] = "udp=30s";
            cargv[i++] = "-t";
            cargv[i++] = "tcp=300s";
        }

        if (pcap_path) {
            cargv[i++] = "-r";
            cargv[i++] = pcap_path;
            cargv[i++] = "-m";
            cargv[i++] = "1";
            cargv[i++] = "-t";
            cargv[i++] = "maxlife=1s";
            cargv[i++] = "-t";
            cargv[i++] = "expint=1s";
        }

        cargv[i] = NULL;
    
        xexecvp("softflowd", cargv);
    }            

    if (device)    free(device); 
    if (pcap_path) free(pcap_path);
    
    int status;
    /* Waiting is not blocked */
    if (waitpid(pid, &status, WNOHANG) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
            fprintf(stderr, "Probe boot failed\n");
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

void print_usage(void) {
    printf("Usage: chiba [-h] [-r <path>] [-i <device>]\n");
    printf("-h               [Print help]\n");
    printf("-r <path>        [Static PCAP file path]\n");
    printf("-i <device>      [Live network interface name]\n");
}