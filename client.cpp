#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>

#define SCHEDULER_TCP_PORT "34937"
#define HOSTIPADDR "127.0.0.1"

using namespace std;

void *get_in_addr(struct sockaddr* sa) {
    // ipv4
    if (sa -> sa_family == AF_UNIX) {
        return &(((struct sockaddr_in*)sa) -> sin_addr);
    }
    // ipv6
    return &(((struct sockaddr_in6*)sa) -> sin6_addr);
}

// modified from beej's client.c
int main(int argc, char* argv[]) {
    int sockfd, status, numbytes;
    struct addrinfo client, *res, *p;
    char s[INET6_ADDRSTRLEN];
    if (argc != 2) {
        fprintf(stderr, "Incorrent command argument\n");
        exit(1);
    }

    //fprintf(stdout, "client server: location: %s\n", argv[1]);
    
    memset(&client, 0, sizeof(client));
    client.ai_family = AF_UNSPEC;
    client.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(HOSTIPADDR, SCHEDULER_TCP_PORT, &client, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    for (p = res; p != NULL; p = p -> ai_next) { 
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));
    fprintf(stdout, "The client is up and running\n");

    freeaddrinfo(res);

    if ((numbytes = send(sockfd, argv[1], strlen(argv[1]), 0)) == -1) {
        perror("send loc to scheduler");
        exit(1);
    }
    fprintf(stdout, "The client has sent query to Scheduler using TCP: client location %s\n", argv[1]);

    int arrangement = 0;
    if ((numbytes = recv(sockfd, &arrangement, sizeof(int), 0)) == -1) {
        perror("recv results from scheduler");
        exit(1);
    }

    if (arrangement == -1) {
        fprintf(stdout, "The client has received results from the scheduler: assigned to NONE\n");
        fprintf(stdout, "Location %s not found", argv[1]);
    } else if (arrangement == 0){
        fprintf(stdout, "The client has received results from the scheduler: assigned to Hospital A\n");
    } else if (arrangement == 1){
        fprintf(stdout, "The client has received results from the scheduler: assigned to Hospital B\n");
    } else if (arrangement == 2){
        fprintf(stdout, "The client has received results from the scheduler: assigned to Hospital C\n");
    } else {
        fprintf(stderr, "wrong arrangement\n");
    }
    
    close(sockfd);
    return 0;
}