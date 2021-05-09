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

#define HOSPITALA_UDP_PORT "30937"
#define HOSPITALB_UDP_PORT "31937"
#define HOSPITALC_UDP_PORT "32937"

#define SCHEDULER_UDP_PORT "33937"
#define SCHEDULER_TCP_PORT "34937"
#define HOSPIADDR "127.0.0.1"

#define BACKLOG 10

typedef struct init_msg  {
    int location;
    int capcity;
    int occupancy;
} HospMsg;

typedef struct cal_msg {
    double score;
    double distance;
} CalMSG;

extern int SetTCPSched();
extern int SetUDPSched();
extern void sigchle_handler(int);
extern void *get_in_addr(struct sockaddr*);
extern int MakeArrange(CalMSG, CalMSG, CalMSG);
extern void ArrangeHospital(int, int, HospMsg, HospMsg, HospMsg, struct sockaddr_storage, socklen_t, struct sockaddr_storage, socklen_t, struct sockaddr_storage, socklen_t);

int isFull_HospA = 0;
int isFull_HospB = 0;
int isFull_HospC = 0;

//from beej's server.c
void *get_in_addr(struct sockaddr* sa) {
    // ipv4
    if (sa -> sa_family == AF_UNIX) {
        return &(((struct sockaddr_in*)sa) -> sin_addr);
    }
    // ipv6
    return &(((struct sockaddr_in6*)sa) -> sin6_addr);
}

//from beej's server.c
void sigchle_handler(int s){
    int saved_erron = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_erron;
}


int SetTCPSched() {
    int status;
    struct addrinfo sched_client, *res, *p;
    struct sigaction siga;
    int sockfd;
    int yes = 1;
    memset(&sched_client, 0, sizeof(sched_client));
    sched_client.ai_family = AF_UNSPEC;
    sched_client.ai_socktype = SOCK_STREAM;
    sched_client.ai_flags = AI_PASSIVE;

    if (status = getaddrinfo(HOSPIADDR, SCHEDULER_TCP_PORT, &sched_client, &res)) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    for (p = res; p != NULL; p = p -> ai_next) { 
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }


        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    
    freeaddrinfo(res);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    siga.sa_handler = sigchle_handler;
    sigemptyset(&siga.sa_mask);
    siga.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &siga, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    return sockfd;
}

void ArrangeHosptial(int udp_sockfd, int arrangement, HospMsg hosA_buf, HospMsg hosB_buf, HospMsg hosC_buf, 
                    struct sockaddr_storage HospA_addr, socklen_t addrA_len, 
                    struct sockaddr_storage HospB_addr, socklen_t addrB_len, 
                    struct sockaddr_storage HospC_addr, socklen_t addrC_len) {
    int isA, isB, isC;

    if (arrangement == 0) {
        fprintf(stdout, "The scheduler has assigned Hospital A to the client\n");
        isA = 1;
        fprintf(stdout, "The scheduler has sent the result to Hospital A using UDP over port %s\n", SCHEDULER_UDP_PORT);
    } else if (arrangement == 1) {
        fprintf(stdout, "The scheduler has assigned Hopital B to the client\n");
        isB = 1;
        fprintf(stdout, "The scheduler has sent the result to Hospital B using UDP over port %s\n", SCHEDULER_UDP_PORT);
    } else if (arrangement == 2) {
        fprintf(stdout, "The scheduler has assigned Hopital C to the client\n");
        isC = 1;
        fprintf(stdout, "The scheduler has sent the result to Hospital C using UDP over port %s\n", SCHEDULER_UDP_PORT);
    } else {
        fprintf(stdout, "The scheduler has assigned NONE to the client\n");
        fprintf(stdout, "The scheduler has sent the result to NONE using UDP over port %s\n", SCHEDULER_UDP_PORT);
    }

    if (hosA_buf.occupancy < hosA_buf.capcity) {
        if (sendto(udp_sockfd, &isA, sizeof(int), 0, (struct sockaddr*)&HospA_addr, addrA_len) == -1) {
            perror("assign client to Hospital A");
            exit(1);
        }
    }
    if (hosB_buf.occupancy < hosB_buf.capcity) {
        if (sendto(udp_sockfd, &isB, sizeof(int), 0, (struct sockaddr*)&HospB_addr, addrB_len) == -1) {
            perror("assign client to Hospital B");
            exit(1);
        }
    }
    if (hosC_buf.occupancy < hosC_buf.capcity) {
        if (sendto(udp_sockfd, &isC, sizeof(int), 0, (struct sockaddr*)&HospC_addr, addrC_len) == -1) {
            perror("assign client to Hospital C");
            exit(1);
        }
    }
    
}


int SetUDPSched() {

    struct addrinfo sched_hos, *res, *p;
    struct sigaction siga;
    int sockfd, status;

    memset(&sched_hos, 0, sizeof(sched_hos));
    sched_hos.ai_family = AF_UNSPEC;
    sched_hos.ai_socktype = SOCK_DGRAM;
    sched_hos.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(HOSPIADDR, SCHEDULER_UDP_PORT, &sched_hos, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    for (p = res; p != NULL; p = p -> ai_next) { 
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Scheduler_talker: socket");
            continue;
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socker\n");
        return 2;
    }

    freeaddrinfo(res);
    return sockfd;
}

int MakeArrange(CalMSG a, CalMSG b, CalMSG c) {
    int result = -1; // for a = 0, b =1, c =2

    result = a.score > b.score ? 0 : 1;
    if (result == -1) { // same score;
        result = a.distance < b.distance ? 0  : 1; 
    }

    if (result == 0) {
        result = -1;
        result = a.score > c.score ? 0 : 2;
        if (result == -1) { // same score;
            result = a.distance < c.distance ? 0 : 2; 
        }
    } else {
        result = -1;
        result = b.score > c.score ? 1 : 2;
        if (result == -1) { // same score;
            result = b.distance < c.distance ? 1 : 2; 
        }
    }
    return result;
}

// modified from beej's server.c
int main() {
  
    int tcp_sockfd, udp_sockfd, nunbytes;
    int newfd;
    //socklen_t sin_size;
    socklen_t addrA_len, addrB_len, addrC_len;
    struct sockaddr_storage HospA_addr, HospB_addr, HospC_addr;
    char s[INET6_ADDRSTRLEN];
    char client_loc[100];
    HospMsg hosA_buf, hosB_buf, hosC_buf;
    CalMSG msgA, msgB, msgC;

    addrA_len = sizeof(HospA_addr);
    addrB_len = sizeof(HospB_addr);
    addrC_len = sizeof(HospC_addr);

    // initilize 
    tcp_sockfd = SetTCPSched();
    udp_sockfd = SetUDPSched();
    
    // fprintf(stdout, "Sock_fd: %d\n", tcp_sockfd);
    // fprintf(stdout, "Sock_fd: %d\n", udp_sockfd);

    fprintf(stdout, "The Scheduler is up and running.\n");

    while (true) {
        
        struct sockaddr_storage their_addr;
        socklen_t sin_size;
        sin_size = sizeof(their_addr);
        newfd = accept(tcp_sockfd, (struct sockaddr*)&their_addr, &sin_size);
        if (newfd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof(s));
        //fprintf(stdout, "server: got connection from %s\n", s);
        //close(tcp_sockfd);


        // receive init info from Hospital A 
        if ((nunbytes = recvfrom(udp_sockfd, &hosA_buf, sizeof(HospMsg), 0, (struct sockaddr*)&HospA_addr, &addrA_len)) == -1) {
            perror("Recv initial info from Host A");
            exit(1);
        }
        fprintf(stdout, "The Scheduler has received information from Hospital A: total capacity is %d and initial occupancy is %d\n", hosA_buf.capcity, hosA_buf.occupancy);
        
        // receive init info from Hospital B
        if ((nunbytes = recvfrom(udp_sockfd, &hosB_buf, sizeof(HospMsg), 0, (struct sockaddr*)&HospB_addr, &addrB_len)) == -1) {
            perror("Recv initial info from Host B");
            exit(1);
        }
        fprintf(stdout, "The Scheduler has received information from Hospital B: total capacity is %d and initial occupancy is %d\n", hosB_buf.capcity, hosB_buf.occupancy);
        
        // receive init info from Hospital C
        if ((nunbytes = recvfrom(udp_sockfd, &hosC_buf, sizeof(HospMsg), 0, (struct sockaddr*)&HospC_addr, &addrC_len)) == -1) {
            perror("Recv initial info from Host C");
            exit(1);
        }
        fprintf(stdout, "The Scheduler has received information from Hospital C: total capacity is %d and initial occupancy is %d\n", hosC_buf.capcity, hosC_buf.occupancy);

        // Set flag to see if any hospital is full
        if (hosA_buf.capcity ==  hosA_buf.occupancy) {
            isFull_HospA = 1;
        }
        if (hosB_buf.capcity ==  hosB_buf.occupancy) {
            isFull_HospB = 1;
        }
        if (hosC_buf.capcity ==  hosC_buf.occupancy) {
            isFull_HospC = 1;
        }

        // receive location info from client
        if (recv(newfd, client_loc, sizeof(client_loc), 0) == -1) {
            perror("Recv from client");
            exit(1);
        }   
        fprintf(stdout, "The Scheduler has received client at location %s from the client using TCP over port %s\n", client_loc, SCHEDULER_TCP_PORT);

        // sending client location to Hospital A
        if (sendto(udp_sockfd, client_loc, sizeof(client_loc), 0, (struct sockaddr*)&HospA_addr, addrA_len) == -1) {
            perror("sendto client location to Hospital A");
            exit(1);

        }
        fprintf(stdout, "The Scheduler has sent client location to Hospital A using UDP over port %s\n", SCHEDULER_UDP_PORT);
        
        // sending client location to Hospital B
        if (sendto(udp_sockfd, client_loc, sizeof(client_loc), 0, (struct sockaddr*)&HospB_addr, addrB_len) == -1) {
            perror("sendto client location to Hospital B");
            exit(1);

        }
        fprintf(stdout, "The Scheduler has sent client location to Hospital B using UDP over port %s\n", SCHEDULER_UDP_PORT);
        
        // sending client location to Hospital C
        if (sendto(udp_sockfd, client_loc, sizeof(client_loc), 0, (struct sockaddr*)&HospC_addr, addrC_len) == -1) {
            perror("sendto client location to Hospital C");
            exit(1);

        }
        fprintf(stdout, "The Scheduler has sent client location tp Hospital C using UDP over port %s\n", SCHEDULER_UDP_PORT);
        


         // recv map info from Hospital A
        if ((nunbytes = recvfrom(udp_sockfd, &msgA, sizeof(CalMSG), 0, (struct sockaddr*)&HospA_addr, &addrA_len)) == -1) {
            perror("Recv score from info from Hospital A");
            exit(1);
        }

        if (msgA.distance == -1 && msgA.score == -1) {
            fprintf(stdout, "The Scheduler has received map information from Hospital A, the score = NONE and the distance = NONE\n");
        } else {
            fprintf(stdout, "The Scheduler has received map information from Hospital A, the score = %.6f and the distance = %.6f\n", msgA.score, msgA.distance);
        }

        // recv map info from Hospital B
        if ((nunbytes = recvfrom(udp_sockfd, &msgB, sizeof(CalMSG), 0, (struct sockaddr*)&HospB_addr, &addrB_len)) == -1) {
            perror("Recv score from info from Hospital B");
            exit(1);
        }

        if (msgB.distance == -1 || msgB.score == -1) {
            fprintf(stdout, "The Scheduler has received map information from Hospital B, the score = NONE and the distance = NONE\n");
        } else {
            fprintf(stdout, "The Scheduler has received map information from Hospital B, the score = %.6f and the distance = %.6f\n", msgB.score, msgB.distance);
        }

        // recv map info from Hospital C
        if ((nunbytes = recvfrom(udp_sockfd, &msgC, sizeof(CalMSG), 0, (struct sockaddr*)&HospC_addr, &addrC_len)) == -1) {
            perror("Recv score from info from Hospital C");
            exit(1);
        }

        if (msgC.distance == -1 || msgC.score == -1) {
            fprintf(stdout, "The Scheduler has received map information from Hospital C, the score = NONE and the distance = NONE\n");
        } else {
            fprintf(stdout, "The Scheduler has received map information from Hospital C, the score = %.6f and the distance = %.6f\n", msgC.score, msgC.distance);
        }

        
        int arrangement = 0; // -1 = NONE, 0 = Hospital A, 1 = Hospital B, 2 = Hospital C
        if (msgA.distance == -1 || (isFull_HospA && isFull_HospB && isFull_HospB)) { // not found location
            arrangement = -1;
        } else {
            arrangement = MakeArrange(msgA, msgB, msgC);
        }
        

        

 

        if (send(newfd, &arrangement, sizeof(int), 0) == -1) {
            perror("send results to client");
            exit(1);
        }


        fprintf(stdout, "The scheduler has sent the result to client using TCP over port %s\n", SCHEDULER_TCP_PORT);
        int isA = 0, isB = 0, isC = 0;

        ArrangeHosptial(udp_sockfd, arrangement, hosA_buf, hosB_buf, hosC_buf, HospA_addr, addrA_len, HospB_addr, addrB_len, HospC_addr, addrC_len);

        

        //hosA_buf.occupancy += 1;
        close(newfd);
        fprintf(stdout, "--------------------------------------------------------------------------------------------\n");
        
    }
 
    close(udp_sockfd);
    return 0;
  

}