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
#include <sys/wait.h>
#include <signal.h>
#include <iostream>

#define UDPPORT "33527"  // My USC ID is 1046533527, whose last three digits is 527 
#define TCPPORT "34527"
#define AUDPPORT "30527"
#define BUDPPORT "31527"
#define CUDPPORT "32527"

#define BACKLOG 10   //set the maximum pending connection queue number
#define MAXDATASIZE 1000

void sigchld_handler(int s); //This function sigchld_handler() is from Beej's tutorial
int getTCPsockandbind(struct addrinfo *servinfo);
int getUDPsockandbind(struct addrinfo *servinfo);
void interpUDPmsg(std::string &mapbuff, std::string &mapinfo, int *receivedA, int *receivedB, char *msgA, char *msgB);
char mapsearchresult(char msgA, char msgB,char mapid, int srcVtxid, int dstVtxid);

int main(void)
{
    int TCPfath_fd,TCPchld_fd, UDP_fd;
    struct addrinfo hints, *servinfo, *servinfoA, *servinfoB, *servinfoC;
    struct sigaction sa;

    int rv;

    char buf[MAXDATASIZE];int numbytes;
    std::string mapbuff, mapinfo;
    char mapid; int srcVtxid,dstVtxid,filesize;
    float Tt,Tp;
    int path[11]; float mindistan;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo("127.0.0.1", TCPPORT, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    TCPfath_fd = getTCPsockandbind(servinfo);
    freeaddrinfo(servinfo);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", UDPPORT, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    UDP_fd = getUDPsockandbind(servinfo);
    freeaddrinfo(servinfo);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", AUDPPORT, &hints, &servinfoA)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", BUDPPORT, &hints, &servinfoB)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", CUDPPORT, &hints, &servinfoC)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    if (listen(TCPfath_fd, BACKLOG) == -1) 
    {
        perror("listen");
        exit(1);
    }

    // The following lines of sigaction() thing is from Beej's Tutorial
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
    {
        perror("sigaction");
        exit(1);
    }

    std::cout << "The AWS is up and ranning" << "\n" << std::endl;
    
    while(1)
    {
        TCPchld_fd = accept(TCPfath_fd,(struct sockaddr *)NULL,NULL);
        if (TCPchld_fd == -1) 
        {
            perror("accept");
            continue;
        }

        if (!fork()) 
        { // this is the child process
            close(TCPfath_fd);
            if ((numbytes = recv(TCPchld_fd, buf, MAXDATASIZE-1, 0)) == -1) 
            {
                perror("recvTCP");
                exit(1);
            }

            //decode the massage received from the client
            mapid = buf[0];
            memcpy(&srcVtxid,buf+1,sizeof(int));
            memcpy(&dstVtxid,buf+1+sizeof(int),sizeof(int));
            memcpy(&filesize,buf+1+2*sizeof(int),sizeof(int));

            std::cout << "The AWS has recieved map ID " << mapid << ", start vertex " << srcVtxid << ", destination vertex " << dstVtxid
             << " and file size " << filesize << " from the client using TCP over port " << TCPPORT << "\n" << std::endl;

            if ((numbytes = sendto(UDP_fd, buf, numbytes, 0, servinfoA->ai_addr, servinfoA->ai_addrlen)) == -1) 
            {
                perror("talker: sendtoA");
                exit(1);
            }
            std::cout << "The AWS has sent map ID to server A using UDP over port " << UDPPORT << "\n" << std::endl;

            if ((numbytes = sendto(UDP_fd, buf, numbytes, 0, servinfoB->ai_addr, servinfoB->ai_addrlen)) == -1) 
            {
                perror("talker: sendtoB");
                exit(1);
            }
            std::cout << "The AWS has sent map ID to server B using UDP over port " << UDPPORT << "\n" << std::endl;

            if ((numbytes = recvfrom(UDP_fd, buf, MAXDATASIZE-1 , 0, (struct sockaddr *) NULL, NULL)) == -1) 
            {
                perror("recvfromA/B");
                exit(1);
            }
            mapbuff.assign(buf, numbytes);
            
            int receivedA = 0, receivedB = 0;
            char msgA = '\0', msgB = '\0';

            interpUDPmsg(mapbuff, mapinfo, &receivedA, &receivedB, &msgA, &msgB);
            if(!receivedA || !receivedB)
            {
                if ((numbytes = recvfrom(UDP_fd, buf, MAXDATASIZE-1 , 0, (struct sockaddr *) NULL, NULL)) == -1) 
                {
                    perror("recvfromA/B");
                    exit(1);
                }
                mapbuff.assign(buf, numbytes);
                interpUDPmsg(mapbuff, mapinfo, &receivedA, &receivedB, &msgA, &msgB);
            }

            char matchstate = mapsearchresult(msgA, msgB, mapid, srcVtxid, dstVtxid);

            if (matchstate == 'y') // all map id, source vertex index and destination vertex index are found
            {
                memcpy(buf, &srcVtxid, sizeof(int));
                memcpy(buf+sizeof(int), &dstVtxid, sizeof(int));
                memcpy(buf+2*sizeof(int), &filesize, sizeof(int));
                memcpy(buf+3*sizeof(int), &mapid, sizeof(char));
                mapinfo.insert(0, buf, 3*sizeof(int)+sizeof(char));

                //send required information to serverC
                if ((numbytes = sendto(UDP_fd, mapinfo.data(), mapinfo.length(), 0, servinfoC->ai_addr, servinfoC->ai_addrlen)) == -1) 
                {
                    perror("talker: sendtoC");
                    exit(1);
                }
                std::cout << "The AWS has sent map, source ID, destination ID, propagation speed and transmission speed to" <<
                 "server C using UDP over port " << UDPPORT << "\n" << std::endl;
                //receive calculated results from serverC
                if ((numbytes = recvfrom(UDP_fd, buf, MAXDATASIZE-1 , 0, (struct sockaddr *) NULL, NULL)) == -1) 
                {
                    perror("recvfromC");
                    exit(1);
                }
                memcpy(&mindistan, buf                  , sizeof(float));
                memcpy(&Tt       , buf + sizeof(float)  , sizeof(float));
                memcpy(&Tp       , buf + sizeof(float)*2, sizeof(float));
                memcpy(path      , buf + sizeof(float)*3, sizeof(path));
                //show the result
                std::cout << "The AWS has received results from serverC:\n" <<  "Shortest path: ";
                for (int i =0 ; i< 11; i++)
                {
                    if(path[i] == 2333)
                        break;
                    
                    if (i==0)
                        std::cout << path[i];
                    else
                        std::cout << " -- " << path[i];

                }
                std::cout << "\nShortest Distance: " << mindistan << " km\nTransmission Delay: " << Tt << " s\nPropagation Delay: " << Tp << " s\n" << std::endl;
                //send calculated results to client
                std::string tempstr("y");
                tempstr.append(buf,numbytes);
                if (send(TCPchld_fd, tempstr.data(), tempstr.length(), 0) == -1)
                    perror("sendTCP");
                std::cout << "The AWS has sent calculated results to client using TCP over port " << TCPPORT << "\n" << std::endl;
            }
            else if(matchstate == 'e') // abnormal map1 and map2 state
            {
                perror("storedmapswrong");
            }
            else // map id, source vertex index or destination vertex index is not found
            {
                std::string tempstr("!");
                tempstr.append(1,matchstate);
                if (send(TCPchld_fd, tempstr.data(), 2, 0) == -1)
                    perror("sendTCP");
            }

            close(TCPchld_fd);
            exit(0);
        }

        //close(TCPchld_fd);
    }

    close(TCPfath_fd);
    return 0;
}

//This function sigchld_handler() is from Beej's tutorial
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int getTCPsockandbind(struct addrinfo *servinfo)
{
    int sockfd;
    struct addrinfo *p;
    int reuseaddr=1;

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) 
        {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    if (p == NULL) 
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    return sockfd;
}

int getUDPsockandbind(struct addrinfo *servinfo)
{
    int sockfd;
    struct addrinfo *p;
    int reuseaddr=1;
    
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            perror("listener: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) 
        {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }
    if (p == NULL) 
    {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit (2);
    }
    return sockfd;
}

void interpUDPmsg(std::string &mapbuff, std::string &mapinfo, int *receivedA, int *receivedB, char *msgA, char *msgB)
{
    int pos;
    std::string markA("serverA");
    std::string markB("serverB");

    pos = mapbuff.find(markA);
    if( pos != -1 )
    {
        *receivedA = 1;
        if ( mapbuff[pos+markA.length()] != '!')
        {
            *msgA = 'y';
            mapinfo = mapbuff.substr(pos+markA.length()+1,mapbuff.find(markB,pos));
        }
        else
            *msgA = mapbuff[pos+markA.length()+1];
    }
    
    pos = mapbuff.find(markB);
    if( pos != -1 )
    {
        *receivedB= 1;
        if ( mapbuff[pos+markB.length()] != '!')
        {
            *msgB = 'y';
            mapinfo = mapbuff.substr(pos+markB.length()+1,mapbuff.find(markA,pos));
        }
        else
            *msgB = mapbuff[pos+markA.length()+1];
    }
}

char mapsearchresult(char msgA, char msgB,char mapid, int srcVtxid, int dstVtxid)
{
    if (msgA=='y' || msgB=='y')
    {
        if(msgA=='y')
        {
            std::cout << "The AWS has received map information from server A" << "\n" << std::endl;
            std::cout << "The source and destination vertex are in the graph" << "\n" << std::endl;
        }
        else
        {
            std::cout << "The AWS has received map information from server B" << "\n" << std::endl;
            std::cout << "The source and destination vertex are in the graph" << "\n" << std::endl;
        }

        return 'y';
    }
    else if (msgA=='m' && msgB=='m')
    {
        std::cout << "Map " << mapid << " is not found in both serverA and serverB" << "\n" << std::endl;
        std::cout << "Sending error to client using TCP over port " << TCPPORT << std::endl;
        return 'm';
    }
    else
    {
        if (msgA=='s' || msgB=='s')
        {
            if(msgA=='s')
            {
                std::cout << "The AWS has received map information from server A" << "\n" << std::endl;
                std::cout << "Source vertex " << srcVtxid << " not found in the graph, sending error to client using TCP over port " 
                << TCPPORT << "\n" << std::endl;
            }
            else
            {
                std::cout << "The AWS has received map information from server B" << "\n" << std::endl;
                std::cout << "Source vertex " << srcVtxid << " not found in the graph, sending error to client using TCP over port " 
                << TCPPORT << "\n" << std::endl;
            }
            return 's';
        }
        else if (msgA=='d' || msgB=='d')
        {
            if(msgA=='d')
            {
                std::cout << "The AWS has received map information from server A" << "\n" << std::endl;
                std::cout << "Destination vertex " << dstVtxid << " not found in the graph, sending error to client using TCP over port " 
                << TCPPORT << "\n" << std::endl;
            }
            else
            {
                std::cout << "The AWS has received map information from server B" << "\n" << std::endl;
                std::cout << "Destination vertex " << dstVtxid << " not found in the graph, sending error to client using TCP over port " 
                << TCPPORT << "\n" << std::endl;
            }
            return 'd';
        }
        else if (msgA=='b' || msgB=='b')
        {
            if(msgA=='b')
            {
                std::cout << "The AWS has received map information from server A" << "\n" << std::endl;
                std::cout << "Both sourse vertex " << srcVtxid << " and destination vertex " << dstVtxid << 
                " not found in the graph, sending error to client using TCP over port " << TCPPORT << "\n" << std::endl;
            }
            else
            {
                std::cout << "The AWS has received map information from server B" << "\n" << std::endl;
                std::cout << "Both sourse vertex " << srcVtxid << " and destination vertex " << dstVtxid << 
                " not found in the graph, sending error to client using TCP over port " << TCPPORT << "\n" << std::endl;
            }
            return 'b';
        }
        return 'e';
    }
}