//
// Created by tl on 18-4-20.
//

#include "udpTurn.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;

bool operator==(struct sockaddr_in a,struct sockaddr_in b){
    if(a.sin_addr.s_addr==b.sin_addr.s_addr && a.sin_port==b.sin_port)
        return true;
    else
        return false;
}

bool udpTurn(string cliIP,int cliPort,string svrIP,int svrPort,string &id) {
    int sockfd;
    struct sockaddr_in svrAddrIn;
    struct sockaddr_in cliAddrIn;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket create error\n");
        exit(1);
    }

    svrAddrIn.sin_family = AF_INET;
    svrAddrIn.sin_port = htons(svrPort);
    svrAddrIn.sin_addr.s_addr = inet_addr(svrIP.c_str());
    if (svrAddrIn.sin_addr.s_addr == INADDR_NONE) {
        printf("Incorrect ip address!\n");
        close(sockfd);
        exit(1);
    }

    cliAddrIn.sin_family=AF_INET;
    cliAddrIn.sin_port=htons(cliPort);
    cliAddrIn.sin_addr.s_addr=inet_addr(cliIP.c_str());

    int reuse = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr* )&cliAddrIn, sizeof(cliAddrIn)) < 0)
    {
        perror("udpTurn bind");
        exit(EXIT_FAILURE);
    }

    int nbytes=0;
    nbytes = sendto(sockfd , id.c_str(), id.size(), 0, (struct sockaddr *)&svrAddrIn, sizeof(svrAddrIn));
    //nbytes = sendto(sockfd , id.c_str(), id.size(), 0, (struct sockaddr *)&svrAddrIn, sizeof(svrAddrIn));

    if (nbytes < 0)
    {
        perror("sendto");
        close(sockfd);
        return false;

    }
    struct sockaddr_in peerAddrIn;
    socklen_t peerLen = sizeof(peerAddrIn);
    char inBuf[30];
    memset(inBuf,0,sizeof(inBuf));
    nbytes = recvfrom(sockfd, inBuf, sizeof(inBuf), 0, (struct sockaddr *) &peerAddrIn, &peerLen);
    if(nbytes<0){
        perror("recvform");
    }
    if(peerAddrIn==svrAddrIn && string(inBuf)=="OK"){
        close(sockfd);
        return true;
    }
    else{
        close(sockfd);
        return false;
    }




}