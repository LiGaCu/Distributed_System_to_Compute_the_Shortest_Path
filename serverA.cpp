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
#include <fstream>
#include <string>
#include <ctype.h>

#define AUDPPORT "30527"
#define AWSUDPPORT "33527"

#define MAXDATASIZE 1000

int searchmap(std::string &mapbuff,char mapid, int srcVtxid, int dstVtxid);
int getUDPsockandbind(struct addrinfo *servinfo);

int main(void)
{
    int selffd;
    struct addrinfo hints, *servinfo;
    int rv;

    char buf[MAXDATASIZE];int numbytes;
    char mapid; int srcVtxid,dstVtxid;
    std::string mapbuff; int foundmap=0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", AUDPPORT, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    selffd = getUDPsockandbind(servinfo);
    freeaddrinfo(servinfo);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", AWSUDPPORT, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    std::cout << "The server A is up and running using UDP on port " << AUDPPORT << "\n" << std::endl;

    while(1)
    {
        if ((numbytes = recvfrom(selffd, buf, MAXDATASIZE-1 , 0, (struct sockaddr *) NULL, NULL)) == -1) 
        {
            perror("recvfrom");
            exit(1);
        }

        mapid = buf[0];
        memcpy(&srcVtxid,buf+1,sizeof(int));
        memcpy(&dstVtxid,buf+1+sizeof(int),sizeof(int));

        std::cout << "The server A has received input for finding graph of map " << mapid << "\n" << std::endl;

        foundmap = searchmap(mapbuff, mapid, srcVtxid, dstVtxid);

        if(!foundmap)
            std::cout << "The server A does not have the required graph id " << mapid << "\n" << std::endl;

        if ((numbytes = sendto(selffd, mapbuff.data(), mapbuff.length(), 0, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) 
        {
            perror("talker: sendto");
            exit(1);
        }
        
        if(foundmap)
            std::cout << "The server A has sent Graph to AWS" << "\n" << std::endl;
        else
            std::cout << "The server A has sent \"Graph not found\" to AWS" << "\n" << std::endl;
    }

    close(selffd);
    freeaddrinfo(servinfo);
    return 0;
}

int searchmap(std::string &mapbuff,char mapid, int srcVtxid, int dstVtxid)
{
    std::string line;
    int foundmap = 0, foundsrc = 0, founddst = 0;
    int lspacepos, rspacepos, fstdgt, secdgt;

    std::ifstream map1;
    map1.open("./map1.txt");
 
    if (map1.is_open())
    {
        while ( getline (map1,line) )
        {
            if (foundmap)
            {
                if (line.length() == 1 && isalpha(line.c_str()[0])) // next map begins
                    break;
                line.append("\n");
                mapbuff.append(line);

                lspacepos = -1; rspacepos = -1; 
                lspacepos = line.find(" ");
                rspacepos = line.find(" ",lspacepos+1);
                if(lspacepos != -1 && rspacepos != -1)
                {
                    fstdgt = atoi(line.substr(0,lspacepos).c_str());
                    secdgt = atoi(line.substr(lspacepos+1,rspacepos-lspacepos-1).c_str());
                    if (fstdgt == srcVtxid)
                        foundsrc = 1;
                    else if (fstdgt == dstVtxid)
                        founddst = 1;

                    if (secdgt == srcVtxid)
                        foundsrc = 1;
                    else if (secdgt == dstVtxid)
                        founddst = 1;
                }
            }
            else if (line.compare(&mapid) == 0) // found the required map
            {
                foundmap = 1;
                mapbuff.clear();
                mapbuff.append("serverA\n");
            }
        }
        map1.close();

        if(foundmap == 1 && foundsrc == 1 && founddst ==1)
            return 1;
        else
        {
            mapbuff.clear();
            if (!foundmap)
                mapbuff.append("serverA!m");
            else if (!foundsrc && founddst)
                mapbuff.append("serverA!s");
            else if (foundsrc && !founddst)
                mapbuff.append("serverA!d");
            else if (!foundsrc && !founddst)
                mapbuff.append("serverA!b");

            if (foundmap)
                return 1;
            else
                return 0;
        }
    }
    else
    {
        std::cout << "Unable to open file" << std::endl;
        exit(1);
    }
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
