//
// Created by tl on 18-5-7.
//

#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include "heartBeat.hpp"
#include "../udpTraverse/udpTraverse.hpp"
#include "../natTraverse.hpp"
//
//#define DEF 65535*65535
//
//typedef void (*pFunction)(int);
//
//int testChangeSignature(int a, int b, int i) {
//    int maxvalue=DEF;
//    void f(int);
//    pFunction pf1 = &f;
//    return a + i;
//
//}

//only send never recv
void* sendUDPHeartBeat(void *arg){

    //cout << testChangeSignature(2, 0, 1) << endl;
    printf("sendUDPHeartBeat func\n");
    UDPComponent comp=*(UDPComponent*)arg;
    struct sockaddr_in uAddrIn;
    
    struct sockaddr_in peerAddrIn;

    int sockfd;


    uAddrIn.sin_family = AF_INET;
    uAddrIn.sin_port = htons(comp.uHostAddr.port);
    uAddrIn.sin_addr.s_addr = inet_addr(comp.uHostAddr.ip.c_str());

    //初始化服务器peer信息
    peerAddrIn.sin_family = AF_INET;

    cout<<comp.uHostAddr.ip.c_str()<<"|"<<comp.uHostAddr.port<<endl;


    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
//    else {
//        printf("socket OK\n");
//    }
    int opt=1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret) {
        exit(1);
    }
    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if (ret) {
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr *) &uAddrIn, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }
//    else {
//        printf("IP bind OK\n");
//    }


    struct sockaddr_in unRecvAddrIn;
    unRecvAddrIn.sin_family=AF_INET;
    unRecvAddrIn.sin_port=htons(rand()%3000+2000);
    unRecvAddrIn.sin_addr.s_addr=inet_addr("14.215.177.39");

    if (connect(sockfd, (struct sockaddr *) &unRecvAddrIn, sizeof(struct sockaddr)) == -1) {
        perror("child connect");
        exit(1);
    } else {
        perror("");
    }

    peerAddrIn.sin_family = AF_INET;
    peerAddrIn.sin_port = htons(comp.peerReflexAddr.port);
    peerAddrIn.sin_addr.s_addr = inet_addr(comp.peerReflexAddr.ip.c_str());
    printf("thread %x send heartBeat to %s:%d\n",pthread_self(),comp.peerReflexAddr.ip.c_str(),comp.peerReflexAddr.port);
    while(1){

        string sendStr="";
        int len = sendto(sockfd,sendStr.c_str(),sendStr.size(),0,(struct sockaddr*)&peerAddrIn,sizeof(peerAddrIn));
        //usleep(300*1000);

        sleep(3);
        if (len == -1) {
            perror("while sending package to C2 , sendto() failed:");
            close(sockfd);
            return NULL;
        }

    }


}