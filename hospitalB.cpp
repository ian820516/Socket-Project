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
#include <string>
#include <iostream>
#include <map>
#include <fstream>
#include <vector>
#include <set>
#include <limits.h>




#define SCHEDULER_UDP_PORT "33937"
#define HOSPITALB_UDP_PORT "31937"
#define HOSPIADDR "127.0.0.1"
#define RESIZE 100000
using namespace std;

extern void ConstructMap();
extern void ShorestPath(int);

typedef pair<int, double> neighber_dis;
typedef vector<neighber_dis> node_graph;
//typedef vector<node_graph> MAP;
map<int, node_graph> map_node;
map<int, int> reindex;
map<int, int> rereindex;
//MAP graph;
set<neighber_dis> node_pair_set;

typedef struct init_msg  {
    int location;
    int capcity;
    int occupancy;
} HospMsg;

typedef struct cal_msg {
    double score;
    double distance;
} CalMSG;

//from beej's server.c
void *get_in_addr(struct sockaddr* sa) {
    // ipv4
    if (sa -> sa_family == AF_UNIX) {
        return &(((struct sockaddr_in*)sa) -> sin_addr);
    }
    // ipv6
    return &(((struct sockaddr_in6*)sa) -> sin6_addr);
}

void MapIndex() {
    map<int, node_graph>::iterator it;
    int i = 0;
    for (it = map_node.begin(); it != map_node.end(); it++){   
        int oldid = it->first;
        reindex[oldid] = i;
        rereindex[i] = oldid;
        i++;
    }
}

double ShorestPath(int source, int dest) {
    vector<double> dist(300, 1e9);
    int u = reindex[source];
    int d = reindex[dest];

    set< pair<double, int> > st;
    st.insert(make_pair(0, u));
    dist[u] = 0;

    while (!st.empty()) {
        pair<int, double> now = *st.begin();
        st.erase(st.begin());

        int v = now.second; // reindex
        int w = now.first;

        node_graph edges = map_node[rereindex[v]];
        map<int, node_graph>::iterator it;
        for (int i = 0; i < edges.size(); i++) {   
            pair<int, double> to = edges[i];
            int first = reindex[to.first];
            if (w + to.second < dist[first]) {
                st.erase(make_pair(dist[first], first));
                dist[first] = w + to.second;
                st.insert(make_pair(dist[first], first));
            }
        }
    }

    
    //cout << "shortest path (" << source << ", " << dest << " ) " << dist[d] << endl;
    
    // map<int, node_graph>::iterator it;
    // it = map_node.find(u);
    // vector<neighber_dis> vec = it->second;
    // for (int i = 0; i < vec.size(); i++){   
    //     cout << "node " << vec[i].first << dist[vec[i].first] <<endl; 
    // }
    return (double)dist[d] / RESIZE ;
}

void ConstructMap() {
    ifstream infile("map.txt");
    int loc1, loc2;
    double distance;
    while (infile >> loc1 >> loc2 >> distance) {
        //cout << "local 1 = " << loc1 << ", local 2 = " << loc2 << ", dis = " << distance << endl;
        neighber_dis temp1 = make_pair(loc2, distance * RESIZE);
        neighber_dis temp2 = make_pair(loc1, distance * RESIZE);

        if (!map_node.count(loc1)) { // contain key       
            // create a vector<pair<int, double>>
            node_graph node;
            map_node[loc1] = node;
            //cout << "creat loc1" << endl;
        }
        
        if (!node_pair_set.count(make_pair(loc1, loc2))) {
            map_node[loc1].push_back(temp1);
            //cout << "push success = <" << map_node[loc1].back().first << ", "<<map_node[loc1].back().second << "> "<< endl;
            node_pair_set.insert(make_pair(loc1, loc2));
        }
      

        if (!map_node.count(loc2)) { // contain key
            node_graph node;
            map_node[loc2] = node;
            //cout << "creat loc2" << endl;
        }

        if (!node_pair_set.count(make_pair(loc2, loc1))) {
            map_node[loc2].push_back(temp2);
            //cout << "push success = <" << map_node[loc2].back().first << ", "<<map_node[loc2].back().second << "> "<< endl;
            node_pair_set.insert(make_pair(loc2, loc1));
        }
    }
    
}


struct addrinfo* GetSched() {

    struct addrinfo sched_hos, *res, *p;
    struct sigaction siga;
    int sockfd, status;

    memset(&sched_hos, 0, sizeof(sched_hos));
    sched_hos.ai_family = AF_UNSPEC;
    sched_hos.ai_socktype = SOCK_DGRAM;
    sched_hos.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(HOSPIADDR, SCHEDULER_UDP_PORT, &sched_hos, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return NULL;
    }

    for (p = res; p != NULL; p = p -> ai_next) { 
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Scheduler_talker: socket");
            continue;
        }
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socker\n");
        return NULL;
    }

    freeaddrinfo(res);
    return p;
}
// modified from beej's listener.c and talker.c
int main(int argc, char* argv[]) {
    int sockfd, status, numbytes;
    struct addrinfo hospB, *res, *p;
    struct sockaddr_storage their_addr, their_udp_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    char client_loc[100];
    
    HospMsg msg;
    CalMSG msgB;

    if (argc != 4) {
        fprintf(stderr, "Incorrent command argument\n");
        exit(1);
    }

    msg.location = atoi(argv[1]);
    msg.capcity = atoi(argv[2]);
    msg.occupancy = atoi(argv[3]);
    //fprintf(stdout, "Hospital server Location = %d, Capcity = %d, Occupancy = %d\n", msg.location, msg.capcity , msg.occupancy);
    
   
    // ShorestPath(0, 2);
    // ShorestPath(2, 18);
    // ShorestPath(18, 8);
    //ShorestPath(255720539, 515154016);
    //ShorestPath(515154016, 545531658);
    //ShorestPath(545531658, 919573377);

    memset(&hospB, 0, sizeof(hospB));
    hospB.ai_family = AF_UNSPEC;
    hospB.ai_socktype = SOCK_DGRAM;
    hospB.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(HOSPIADDR, HOSPITALB_UDP_PORT, &hospB, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    for (p = res; p != NULL; p = p -> ai_next) { 
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "HOSPB: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));
    //fprintf(stdout, "HostB: connecting to %s\n", s);
    freeaddrinfo(res);

    addr_len = sizeof(their_addr);
    
    fprintf(stdout, "Hospital B is up and running using UDP on port %s.\n", HOSPITALB_UDP_PORT);
    fprintf(stdout, "Hospital B has total capacity %d and initial occupancy %d\n", msg.capcity, msg.occupancy);
    struct addrinfo *sched = GetSched();

    if ((numbytes = sendto(sockfd, &msg, sizeof(HospMsg), 0, sched->ai_addr, sched->ai_addrlen)) == -1) {
        perror("HostB: sendto");
        exit(1);
    }

     // Create Map
    ConstructMap();

    // Reindex location
    MapIndex();

    while(true) {

        if ((numbytes = recvfrom(sockfd, client_loc, sizeof(client_loc), 0, (struct sockaddr*)&their_addr, &addr_len)) == -1) {
            perror("Recv from Host B");
            exit(1);
        }

        fprintf(stdout, "Hospital B has received input from client at location %s\n", client_loc);

        double availabilty, score;
        double distance = 0; // init to NONE = -1
        int location = atoi(client_loc);

        availabilty = (double)(msg.capcity - msg.occupancy) / msg.capcity;
        if (availabilty < 0 || availabilty > 1) {
            availabilty = -1; //NONE
        }

        if (!map_node.count(location)) {
            fprintf(stdout, "Hospital B does not have the location %d in map\n", location);
            fprintf(stdout, "Hospital B has sent \"location not found\" to the scheduler\n");
            distance = -1; 
        } 

        fprintf(stdout, "Hospital B has capacity = %d, occupation = %d,", msg.capcity, msg.occupancy);
        if (availabilty == -1) {
            fprintf(stdout, " availabiltiy = NONE\n");
        } else {
            fprintf(stdout, " availabiltiy = %f\n", availabilty);
        }

        if (location != msg.location && distance != -1) {
            distance = ShorestPath(location, msg.location);
        } else {
            distance = -1;
        }
        
        // print disrtance msg
        if (distance == -1) {
            fprintf(stdout, "Hospital B has found the shortest path to client, distance = NONE\n");
            msgB.distance = -1;
        } else {
            fprintf(stdout, "Hospital B has found the shortest path to client, distance = %.6f\n", distance);
            msgB.distance = distance;
        }

        if (distance == -1 || availabilty == -1) {
            score = -1; // NONE
            msgB.score = -1;
            fprintf(stdout, "Hospital B has the score = NONE\n");
        } else {
            score = (double)1 / (distance*(1.1 - availabilty));
            fprintf(stdout, "Hospital B has the score = %.6f\n", score);
            msgB.score = score;
        }
        // send score and distance to scheduler
        
        if ((numbytes = sendto(sockfd, &msgB, sizeof(CalMSG), 0, (struct sockaddr*)&their_addr, addr_len)) == -1) {
            perror("HostB Score and Distance: sendto");
            exit(1);
        }

        if (distance == -1 && score == -1) {
            fprintf(stdout, "Hospital B has sent score = NONE and distance = NONE to the scheduler\n");
        } else if (distance == -1) {
            fprintf(stdout, "Hospital B has sent score = %.6f and distance = NONE to the scheduler\n", score);
        } else if (score == -1) {
            fprintf(stdout, "Hospital B has sent score = NONE and distance = %.6f to the scheduler\n", distance);
        } else {
            fprintf(stdout, "Hospital B has sent score = %.6f and distance = %.6f to the scheduler\n", score, distance);
        }
        
        int assign; 
        if ((numbytes = recvfrom(sockfd, &assign, sizeof(assign), 0, (struct sockaddr*)&their_addr, &addr_len)) == -1) {
            perror("Recv assignment from scheduler ");
            exit(1);
        }
        // do update
        if (assign == 1) {
            msg.occupancy += 1;
            availabilty = (double)(msg.capcity - msg.occupancy) / msg.capcity;
            fprintf(stdout, "Hospital B has been assigned to a client, occupation is update to %d, availabiltiy is updated to %.6f\n", msg.occupancy, availabilty);
        } 

        if ((numbytes = sendto(sockfd, &msg, sizeof(HospMsg), 0, (struct sockaddr*)&their_addr, addr_len)) == -1) {
            perror("HostB Update: sendto");
            exit(1);
        }
        fprintf(stdout, "------------------------------------------------------------------------\n");
    }


    close(sockfd);
    return 0;
}