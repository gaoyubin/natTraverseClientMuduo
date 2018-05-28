//
// Created by tl on 18-4-20.
//

#include "udpTurn.hpp"
#include "../udpTraverse/udpTraverse.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <bits/socket.h>


using namespace std;

bool operator==(struct sockaddr_in a,struct sockaddr_in b){
    if(a.sin_addr.s_addr==b.sin_addr.s_addr && a.sin_port==b.sin_port)
        return true;
    else
        return false;
}
bool operator!=(struct sockaddr_in a,struct sockaddr_in b){
    return !(a==b);
}
bool turnUDP(Addr &cliAddr, Addr &svrAddr, string &id) {
    printf("in turnUDP func\n");

    bool mustSend=true;

    int sockfd;
    struct sockaddr_in svrAddrIn;
    struct sockaddr_in cliAddrIn;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket create error\n");
        exit(1);
    }

    svrAddrIn.sin_family = AF_INET;
    svrAddrIn.sin_port = htons(svrAddr.port);
    svrAddrIn.sin_addr.s_addr = inet_addr(svrAddr.ip.c_str());
    if (svrAddrIn.sin_addr.s_addr == INADDR_NONE) {
        printf("Incorrect ip address!\n");
        close(sockfd);
        exit(1);
    }

    cliAddrIn.sin_family=AF_INET;
    cliAddrIn.sin_port=htons(cliAddr.port);
    cliAddrIn.sin_addr.s_addr=inet_addr(cliAddr.ip.c_str());

//    int reuse = 1;
//    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
//    if (ret) {
//        exit(1);
//    }
//
//    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
//    if (ret) {
//        exit(1);
//    }

    if (bind(sockfd, (struct sockaddr* )&cliAddrIn, sizeof(cliAddrIn)) < 0)
    {
        perror("turnUDP bind");
        exit(EXIT_FAILURE);
    }

//    int nbytes=0;
//    nbytes = sendto(sockfd , id.c_str(), id.size(), 0, (struct sockaddr *)&svrAddrIn, sizeof(svrAddrIn));
//    //nbytes = sendto(sockfd , id.c_str(), id.size(), 0, (struct sockaddr *)&svrAddrIn, sizeof(svrAddrIn));
//    printf("turnCli send %d nbytes\n",nbytes);
//    if (nbytes < 0)
//    {
//        perror("sendto");
//        close(sockfd);
//        return false;
//
//    }

    struct timeval tv;
    tv.tv_sec=0;
    tv.tv_usec=500*1000;
    //for test
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    int loopCnt=0;
    //int timeWaitCnt=0;

    string outStr="id:";
    outStr+=id;
    while (loopCnt<50) {
        loopCnt++;
        int nbytes=0;

        if(mustSend==true){
            nbytes = sendto(sockfd , outStr.c_str(), outStr.size(), 0, (struct sockaddr *)&svrAddrIn, sizeof(svrAddrIn));
            printf("turnCli send [ %s ]\n",outStr.c_str());
        }
//        if(outStr=="OK")
//            timeWaitCnt++;
//        if(timeWaitCnt==2){
//            close(sockfd);
//            return true;
//        }
        struct sockaddr_in peerAddrIn;
        socklen_t peerLen = sizeof(peerAddrIn);
        char inBuf[100];
        memset(inBuf, 0, sizeof(inBuf));
        nbytes = recvfrom(sockfd, inBuf, sizeof(inBuf), 0, (struct sockaddr *) &peerAddrIn, &peerLen);

        if (nbytes < 0) {
            if(errno==EWOULDBLOCK){
                if(outStr=="OK"){
                    close(sockfd);
                    return true;

                }
                printf("EWOULDBLOCK\n");
                continue;
            }

            perror("recvform");
        }
        printf("recved from %s:%d,[ %s ]\n", inet_ntoa(peerAddrIn.sin_addr),ntohs(peerAddrIn.sin_port),inBuf);
        if( peerAddrIn != svrAddrIn){
            printf("addr no equal\n");
            continue;
        }
//        else if(string(inBuf) != "OK"){
//            continue;
//        }
        else if (peerAddrIn == svrAddrIn ) {
            if(string(inBuf)=="accept OK"){//当收到accept OK,就表明父亲已经收到，就不再发送，颜面造成服务器中负担
                printf("set mustSend=false\n");
                mustSend=false;
                continue;
            }
            else if(string(inBuf)=="turn OK" ){//当收到turn OK,就进入time_wait阶段，如果在time_wait阶段没有收到turn OK,就表明服务器已经收到OK，客户端必须结束
                printf("inBuf == turn ok\n");
                outStr="OK";
                mustSend=true;
                //timeWaitCnt=0;
            }
            else if(string(inBuf)=="OK"){//这是以防，两个客户端已经建立好映射，但是网络中残留的"OK"被服务器转发到对端
                close(sockfd);
                return true;
            }

        } else {
            close(sockfd);
            return false;
        }
    }
    cout<<"break out the turn"<<endl;

}