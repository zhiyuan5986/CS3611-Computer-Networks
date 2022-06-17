# 本次实验的过程和经验
4.21 上午
* 在此前的思考中，对于如何区分缓冲区输出和来自其他client的输出，发现有一个`FD_ISSET`可以使用，来进行非阻塞的输入

```cpp
for(i = 0; i <= fdmax; i++) {
    if (FD_ISSET(i, &read_fds)) { //i is in read_fds
        /***do something***/
    } // END got new incoming connection
    if (FD_ISSET(0, &readfds)) { //there are input in the keyboard
        /***do something***/
    }
}
```


4.21 下午
* 发现有一个问题，对于原来的selectserver，无法确定收到的消息来自哪一个client。所以，现在的思路是，不如每次建立完连接，发送完消息后，就关闭连接。

4.21 晚上
* 写出了第一版的程序，但是在连接过程中出了一点问题，第153行的`recv`函数返回值为0。
```cpp
/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <map>
#include <iostream>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 100 // max number of bytes we can get at once 
void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    std::map<std::string, std::string> host{{"h1", "10.0.0.1"}, {"h2", "10.0.0.2"},{"h3", "10.0.0.3"},{"h4", "10.0.0.4"}};
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	char buf[MAXDATASIZE];
    struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

    struct timeval tv;
    fd_set master;
	fd_set readfds;
	int select_rv, fdmax; 
    socklen_t nbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
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

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}


	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
        FD_ZERO(&readfds);

        FD_SET(0, &readfds);//own descriptor
        FD_SET(sockfd, &readfds);

        if (fdmax < sockfd){
            fdmax = sockfd;
        }
        // tv.tv_sec = 2;
        // tv.tv_usec = 500000;
        select_rv = select(fdmax+1, &readfds, NULL, NULL, NULL);

        if(select_rv == -1){
            perror("select");
            break;
        }
        else {
            if(select_rv == 0){
                continue;
            }
            else {
                //get data
                if(FD_ISSET(sockfd, &readfds)){
                    bzero(buf, MAXDATASIZE);//clear buffer
                    if(nbytes = recv(sockfd, buf, MAXDATASIZE, 0) > 0){
                        printf("Receive Message：%s\n", buf);
                    }else{
                        if(nbytes < 0){
                            printf("Receive message failed!\n");
                        }else{ //nbytes == 0: 为什么会导致nbytes==0呢? send端的connection为何关闭了呢？
                            printf("server exited, connection finished!\n");
                        }
                        break;
                    }
                }
                //client input and send data
                if(FD_ISSET(0, &readfds)){ 
                    //send data
                    std::string WhichToSend;
                    std::string WhatToSend;
                    std::string head;
                    std::cin >> head;
                    std::cin >> WhichToSend;
                    getline(std::cin,WhatToSend);

                    struct addrinfo hints1, *servinfo1, *p1;
                    char s1[INET6_ADDRSTRLEN];
                    memset(&hints1, 0, sizeof hints1);
	                hints1.ai_family = AF_UNSPEC;
	                hints1.ai_socktype = SOCK_STREAM;
                    int rv1 = getaddrinfo(host[WhichToSend.substr(0,2)].data(), PORT, &hints1, &servinfo1);
                    for(p1 = servinfo1; p1 != NULL; p1 = p1->ai_next) {
		                if ((new_fd = socket(p1->ai_family, p1->ai_socktype,
				            p1->ai_protocol)) == -1) {
			                    perror("client: socket");
			                    continue;
		                }

		                if (connect(new_fd, p1->ai_addr, p1->ai_addrlen) == -1) {
                            close(new_fd);
			                perror("client: connect");
			                continue;
		                }

		                break;
	                }

	                if (p1 == NULL) {
		                fprintf(stderr, "client: failed to connect\n");
		                return 2;
	                }

                    //get the server's IP
	                inet_ntop(p1->ai_family, get_in_addr((struct sockaddr *)p1->ai_addr),
			                s1, sizeof s1);
	                printf("client: connecting to %s\n", s1);
	                freeaddrinfo(servinfo1); 
                    
                     if (send(new_fd, (WhatToSend.substr(1,WhatToSend.length())).data(), WhatToSend.length()-1, 0) == -1) {
                         perror("send");
                     }
                    
                }
            }
        }
	}

	return 0;
}
```
![结果图](./fig1.png "结果图")
考虑了一下，结合selectserver.cpp文件，发现似乎是在处理`recv`的接收端时，没有区分清楚应该从哪个socket接收消息。准备第二天再干。

4.21 晚
发现应该是没有对新连接的线路进行accept。

4.21 晚
添加了`accept`后，可以正常接收消息了，现在的任务是添加一个读取send端IP地址的部分。

4.22 上午
基本搞定第一题，可以实现要求，但是似乎由于mininet不太稳定，有些时候会出现连接不上的情况：
![](./fig2.png "结果图")

4.22 晚上
上面一条提到的链接不稳定问题实际上是vmware挂载Ubuntu20.04出现的不稳定问题，换成双系统下的Ubuntu20.04后就可以正常执行了。
![](./fig3.png "结果图")
