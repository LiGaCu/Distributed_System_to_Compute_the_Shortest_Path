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

#define AWS_PORT "34527"
#define MAXDATASIZE 1000

int main(int argc, char *argv[])
{
    int TCPfd;
    struct addrinfo hints, *servinfo, *p;

    int rv;

    char buf[MAXDATASIZE];int numbytes;
    char mapid; int srcVtxid,dstVtxid,filesize;
    float Tt,Tp;
    int path[11]; float mindistan;

    if (argc != 5) 
    {
        fprintf(stderr,"usage: [map id]  [src Vertex] [dst Vertex] [filesize]\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("127.0.0.1", AWS_PORT, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    //The following socket connection codes are from Beej's tutorial
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((TCPfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            perror("client: socket");
            continue;
        }

        if (connect(TCPfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(TCPfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) 
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    freeaddrinfo(servinfo); // all done with this structure

    std::cout << "The client is up and running" << "\n" << std::endl;

    //encode the input massage and prepare it for sending
    mapid = argv[1][0];
    srcVtxid = atoi(argv[2]);
    dstVtxid = atoi(argv[3]);
    filesize = atoi(argv[4]);

    buf[0] = mapid;
    memcpy(buf+1, &srcVtxid, sizeof(int));
    memcpy(buf+1+sizeof(int), &dstVtxid, sizeof(int));
    memcpy(buf+1+2*sizeof(int), &filesize, sizeof(int));

    //send the encoded massage to AWS
    if (send(TCPfd, buf, 1+3*sizeof(int), 0) == -1)
    {
        perror("send");
        exit(1);
    }

    //The following massage on the screen means the end of phase 1.
    std::cout << "The client has sent query to AWS using TCP:\n* start vertex " << srcVtxid << ";\n* destination vertex " << dstVtxid << ";\n* map " << mapid
     << ";\n* file size " << filesize << "\n" << std::endl;

    if ((numbytes = recv(TCPfd, buf, MAXDATASIZE-1, 0)) == -1) 
    {
        perror("recv");
        exit(1);
    }
    if(buf[0] == 'y')
    {
        memcpy(&mindistan, buf + 1                  , sizeof(float));
        memcpy(&Tt       , buf + 1 + sizeof(float)  , sizeof(float));
        memcpy(&Tp       , buf + 1 + sizeof(float)*2, sizeof(float));
        memcpy(path      , buf + 1 + sizeof(float)*3, sizeof(path));
        std::cout << "The Client has received results from aws:" <<  std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        std::cout << "Source \tDestination \tMin Length \tTt \t\tTp \t\tDelay" <<  std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        std::cout << srcVtxid << " \t" << dstVtxid << " \t\t" << mindistan << " \t" << Tt << " \t" << Tp << " \t" << (Tt+Tp) << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        std::cout << "Shortest path: ";
        for (int i =0 ; i< 11; i++)
        {
            if(path[i] == 2333)
                break;
            
            if (i==0)
                std::cout << path[i];
            else
                std::cout << " -- " << path[i];

        }
        std::cout << std::endl;
    }
    else if(buf[0] == '!')
    {
        if(buf[1] == 'm')
        {
            std::cout << "No map id " << mapid << " found" << std::endl;
        }
        else if(buf[1] == 's')
        {
            std::cout << "No Vertex id " << srcVtxid << " found" << std::endl;
        }
        else if(buf[1] == 'd')
        {
            std::cout << "No Vertex id " << dstVtxid << " found" << std::endl;
        }
        else if(buf[1] == 'b')
        {
            std::cout << "Both Vertex id " << srcVtxid << " and Vertex id " << dstVtxid << " not found" << std::endl;
        }
    }

    close(TCPfd);
    return 0;
}