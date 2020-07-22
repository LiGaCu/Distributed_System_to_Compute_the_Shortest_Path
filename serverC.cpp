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
#include <string>
#include <sstream>

#define CUDPPORT "32527"
#define AWSUDPPORT "33527"

#define MAXDATASIZE 1000

#define maxnode_num 11
#define inf_dis 100000000
#define defaultidx 12

void searchpath(std::istringstream &tokenStream,int srcVtxid,int dstVtxid, int *path, float *mindistan);

//The following Digikstra() and recoverPath() funtions are from https://www.cnblogs.com/simuhunluo/p/7469495.html
void Dijkstra(int node_num, int srcidx, float *dist, int *prev, float distans[maxnode_num][maxnode_num]);
void recoverPath(int *prev, int srcidx, int dstidx, int *indextable, int *path);

int getUDPsockandbind(struct addrinfo *servinfo);

int main(void)
{
    int selffd;
    struct addrinfo hints, *servinfo;
    int rv;

    char buf[MAXDATASIZE];int numbytes;
    std::string stringbuff;
    int srcVtxid, dstVtxid, filesize; char mapid;
    float Pspeed; int Tspeed;
    float Tt,Tp;
    int path[11]; float mindistan = inf_dis;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo("127.0.0.1", CUDPPORT, &hints, &servinfo)) != 0) 
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

    std::cout << "The server C is up and running using UDP on port " << CUDPPORT << "\n" << std::endl;

    while(1)
    {
        if ((numbytes = recvfrom(selffd, buf, MAXDATASIZE-1 , 0, (struct sockaddr *) NULL, NULL)) == -1) 
        {
            perror("recvfrom");
            exit(1);
        }
        memcpy(&srcVtxid, buf, sizeof(int));
        memcpy(&dstVtxid, buf+sizeof(int), sizeof(int));
        memcpy(&filesize, buf+2*sizeof(int), sizeof(int));
        memcpy(&mapid, buf+3*sizeof(int), sizeof(char));

        stringbuff.assign(buf + 3*sizeof(int) + sizeof(char), numbytes - 3*sizeof(int) - sizeof(char));

        std::istringstream tokenStream (stringbuff);
        std::string Pspeedbuf, Tspeedbuf;
        std::getline(tokenStream,Pspeedbuf);
        std::getline(tokenStream,Tspeedbuf);
        Pspeed = atof(Pspeedbuf.c_str());
        Tspeed = atoi(Tspeedbuf.c_str());

        std::cout << "The Server C has received data for calculation:\n" << "* Propagation Speed: " << Pspeed << " km/s;\n* Transmission Speed: " << Tspeed 
        << " KB/s;\n* Map ID: " << mapid << ";\n* Source ID: " << srcVtxid << ";\n* Destination ID: " << dstVtxid << ";\n" << std::endl;

        // find the shortest path and its length and store them in path and mindistan respectively.
        searchpath(tokenStream, srcVtxid, dstVtxid, path, &mindistan); 

        Tt = (float) filesize/ (float) Tspeed;
        Tp = mindistan/Pspeed;

        std::cout << "The Server C has finished the calculation:\n" <<  "Shortest path: ";
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

        memcpy(buf                  , &mindistan, sizeof(float));
        memcpy(buf + sizeof(float)  , &Tt       , sizeof(float));
        memcpy(buf + sizeof(float)*2, &Tp       , sizeof(float));
        memcpy(buf + sizeof(float)*3, path      , sizeof(path));
        if ((numbytes = sendto(selffd, buf, sizeof(float)*3 + sizeof(path), 0, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) 
        {
            perror("talker: sendto");
            exit(1);
        }
        std::cout << "The Server C has finished sending the output to AWS\n" << std::endl ;
    }

    close(selffd);
    freeaddrinfo(servinfo);
    return 0;
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

void searchpath(std::istringstream &tokenStream,int srcVtxid,int dstVtxid, int *path, float *mindistan)
{
    std::string line;
    int lspacepos, rspacepos, fstdgt, secdgt;
    float distan;

    float dist[maxnode_num];
    int prev[maxnode_num];
    float distans[maxnode_num][maxnode_num];
    int node_num = 0, link_num = 0;

    int indextable[maxnode_num];
    int firstidx, secondidx, srcidx = maxnode_num, dstidx = maxnode_num;

    for(int i=1; i<maxnode_num; i++)
        for(int j=1; j<maxnode_num; j++)
            distans[i][j] = inf_dis;
    
    for(int i=1; i<maxnode_num; i++)
        indextable[i] = defaultidx;

    while ( getline (tokenStream,line) )
    {
        lspacepos = line.find(" ");
        rspacepos = line.find(" ",lspacepos+1);
        if(lspacepos != -1 && rspacepos != -1)
        {
            link_num++;
            fstdgt = atoi(line.substr(0,lspacepos).c_str());
            secdgt = atoi(line.substr(lspacepos+1,rspacepos-lspacepos-1).c_str());
            distan = atof(line.substr(rspacepos+1).c_str());

            for(int i=1;i<maxnode_num;i++)
            {
                if(indextable[i] == fstdgt)
                {
                    firstidx = i;
                    break;
                }
                else if(indextable[i] == defaultidx)
                {
                    indextable[i] = fstdgt;
                    firstidx = i;
                    node_num++;
                    break;
                }
            }
            for(int i=1;i<maxnode_num;i++)
            {
                if(indextable[i] == secdgt)
                {
                    secondidx = i;
                    break;
                }
                else if(indextable[i] == defaultidx)
                {
                    indextable[i] = secdgt;
                    secondidx = i;
                    node_num++;
                    break;
                }
            }
            if (distans[firstidx][secondidx] > distan)
            {
                distans[firstidx][secondidx] = distan;
                distans[secondidx][firstidx] = distan;
            }
        }
    }

    for(int i=1; i<maxnode_num; i++)
        dist[i] = inf_dis;

    for(int i=1;i<maxnode_num;i++)
    {
        if(indextable[i] == srcVtxid) {
            srcidx = i; 
            break;
        }
    }
    for(int i=1;i<maxnode_num;i++)
    {
        if(indextable[i] == dstVtxid) {
            dstidx = i;
            break;
        }
    }

    Dijkstra(node_num, srcidx, dist, prev, distans);
    *mindistan = dist[dstidx];
    recoverPath(prev, srcidx, dstidx, indextable, path);

}

//The following Digikstra() and recoverPath() funtion are from https://www.cnblogs.com/simuhunluo/p/7469495.html
void Dijkstra(int node_num, int srcidx, float *dist, int *prev, float distans[maxnode_num][maxnode_num])
{
    bool s[maxnode_num];    
    for(int i=1; i<=node_num; ++i)
    {
        dist[i] = distans[srcidx][i];
        s[i] = 0;     
        if(dist[i] == inf_dis)
            prev[i] = 0;
        else
            prev[i] = srcidx;
    }
    dist[srcidx] = 0;
    s[srcidx] = 1;

    for(int i=2; i<=node_num; ++i)
    {
        int tmp = inf_dis;
        int u = srcidx;
        
        for(int j=1; j<=node_num; ++j)
            if((!s[j]) && dist[j]<tmp)
            {
                u = j;            
                tmp = dist[j];
            }
        s[u] = 1;    

        for(int j=1; j<=node_num; ++j)
            if((!s[j]) && distans[u][j]<inf_dis)
            {
                float newdist = dist[u] + distans[u][j];
                if(newdist < dist[j])
                {
                    dist[j] = newdist;
                    prev[j] = u;
                }
            }
    }

}

void recoverPath(int *prev, int srcidx, int dstidx, int *indextable, int *path)
{
    int que[maxnode_num];
    int tot = 1;
    que[tot] = dstidx;
    tot++;
    int tmp = prev[dstidx];
    while(tmp != srcidx)
    {
        que[tot] = tmp;
        tot++;
        tmp = prev[tmp];
    }
    que[tot] = srcidx;
    int j = 0;
    for(int i=tot; i>=1; --i)
    {
        path[j] = indextable[que[i]];
        j++;
    }
    path[j]=2333;
}