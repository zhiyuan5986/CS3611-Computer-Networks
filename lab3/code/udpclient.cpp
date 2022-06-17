/***
 * udpclient.cpp: Chatting room with multiple users: client-only model with UDP protocol 
 * ***/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <map>


#define SERVERPORT 3490 // the port users will be connecting to
#define PORT "3490"
#define MAXDATASIZE 100 // max number of bytes we can get at once 
#define BOARDCASTADDR "10.255.255.255" //broadcast addr, from Mininet

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    std::map<std::string, std::string> host{{"h1", "10.0.0.1"}, {"h2", "10.0.0.2"},{"h3", "10.0.0.3"},{"h4", "10.0.0.4"}};
    std::map<std::string, std::string> IP{{"10.0.0.1", "h1"}, {"10.0.0.2", "h2"},{"10.0.0.3", "h3"},{"10.0.0.4", "h4"}};
    
    //1. make connection to boardcast address.

    int sockfd, listener;
    struct sockaddr_in their_addr; // connector's address information
    struct hostent *he;
    int nbytes;
    int broadcast = 1;
    //char broadcast = '1'; // if that doesn't work, try this如果上一行不管用，就用这一行

    struct sockaddr_storage server_addr; //store the addr of sender
    char buf[MAXDATASIZE];
    struct addrinfo hints, *p, *servinfo;

    char s[INET6_ADDRSTRLEN], rv_addr[INET6_ADDRSTRLEN], local_addr[INET6_ADDRSTRLEN];
    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    // struct timeval tv;
	fd_set readfds;
	int rv, select_rv, fdmax; 
    socklen_t addr_len;
    
    if ((he=gethostbyname(BOARDCASTADDR)) == NULL) {  // get the host info
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // this call is what allows broadcast packets to be sent: 
    //This part is the most important for BOARDCAST!!!
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    their_addr.sin_family = AF_INET;     // host byte order
    their_addr.sin_port = htons(SERVERPORT); // short, network byte order
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
    addr_len = sizeof their_addr;

    // get us a socket
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((listener = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            perror("setsockopt");
            exit(2);
        }

        //bind to the IP of host
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

    //get the server's IP
	inet_ntop(their_addr.sin_family, get_in_addr((struct sockaddr *)&their_addr),
                                s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure


    //2. This part is to get IP of myself. From: https://blog.csdn.net/zspzwal/article/details/51276929

	int local_sockfd;
	struct ifconf ifconf;
	struct ifreq *ifreq;
	char buf1[1024];
    char* local_ip;
	//初始化ifconf
	ifconf.ifc_len = 1024;
	ifconf.ifc_buf = buf1;
 
	if((local_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		exit(1);
	}
 
	//获取所有接口信息
	ioctl(local_sockfd, SIOCGIFCONF, &ifconf);
 
	//逐个获取IP地址
	ifreq = (struct ifreq*)buf1;
	for(int i = (ifconf.ifc_len/sizeof(struct ifreq)); i>0; i--)
	{
		// printf("name = [%s] : ",ifreq->ifr_name);
		local_ip=inet_ntoa( ((struct sockaddr_in *)&(ifreq->ifr_addr))->sin_addr);//local_ip is used to get the IP address of myself
		ifreq++;
    }
    strcpy(local_addr, local_ip);// copy local_ip to local_addr
    close(local_sockfd);

    //3. use a loop to listen.

    for(;;){
        FD_ZERO(&readfds);
        fdmax = 0;

        FD_SET(0, &readfds);//own descriptor
        FD_SET(listener, &readfds);

        if (fdmax < listener){
            fdmax = listener;
        }
        select_rv = select(fdmax+1, &readfds, NULL, NULL, NULL);
        if(select_rv == -1){
            perror("select");
            break;
        }else{
            if(select_rv == 0){
                continue;
            }else{
                //get data
                if(FD_ISSET(listener, &readfds)){
                    bzero(buf, MAXDATASIZE);//clear buffer

                    if((nbytes = recvfrom(listener, buf, MAXDATASIZE, 0, (struct sockaddr *)&server_addr, &addr_len)) > 0){
                        // printf("%d",nbytes);
                        inet_ntop(server_addr.ss_family,
                                get_in_addr((struct sockaddr *)&server_addr),
                                rv_addr, sizeof rv_addr);
                        
                        if(strcmp(rv_addr, local_addr)){ //if (rv_addr != local_addr), we print message in the terminal
                            buf[nbytes] = '\0';
                            printf("%s From %s (%d bytes)\n", buf,IP[rv_addr].data(),nbytes);
                        }
                    }else{
                        if(nbytes < 0){
                            perror("recvfrom");
                        }else{
                            printf("server exited, connection finished!\n");
                        }
                        break; 
                    }
                }
                if(FD_ISSET(0, &readfds)){
                    bzero(buf, MAXDATASIZE);
                    fgets(buf, MAXDATASIZE, stdin);
                    if ((nbytes=sendto(sockfd, buf, strlen(buf), 0,
                            (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
                        perror("sendto");
                        exit(1);
                    }
                    printf(" sent %d bytes to %s\n", nbytes, inet_ntoa(their_addr.sin_addr));
                }
            }
        }
    }
    close(listener);
    close(sockfd);

    return 0;
}