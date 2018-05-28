
#include<stdio.h>
#include<stdlib.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include<pthread.h>
#include<arpa/inet.h>


#include <string>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "udpTraverse.hpp"

#include <sys/epoll.h>
#include <vector>
#include <assert.h>
#include <json/json.h>

#include "../stun/stun.h"
#include "../stun/udp.h"
#include "../stun/MyStun.hpp"
#include "../udpTurn/udpTurn.hpp"

using namespace std;

//bool  g_isTraversed=false;
//Addr  g_trAddr;

extern pthread_mutex_t recvMutex;
extern pthread_cond_t  recvCond;
void* recvUdpBuf(void *arg){

    //pthread_detach(pthread_self());
//    cout<<pthread_self()<<endl;
    printf("%x\n",pthread_self());

    int recvfd=*(int*)arg;
#if 0
    Addr peerAddr;

    pthread_mutex_lock(&recvMutex);
    peerAddr=*(Addr*)arg;
    delete arg;
    *(Addr*)arg=Addr();
    pthread_cond_signal(&recvCond);
    pthread_mutex_unlock(&recvMutex);

    struct sockaddr_in uAddrIn;

    int recvfd;


    uAddrIn.sin_family = AF_INET;
    uAddrIn.sin_port = htons(7777);
    uAddrIn.sin_addr.s_addr = inet_addr("192.168.0.8");

    //cout<<uAddr.ip<<"|"<<uAddr.port<<endl;
    printf("tid=%x,%s:%d\n",pthread_self(),peerAddr.ip.c_str(),peerAddr.port);

    //建立与服务器通信的socket和与客户端通信的socket
    recvfd = socket(AF_INET,SOCK_DGRAM,0);
    if (recvfd == -1) {
        perror("socket() failed:");
        return NULL;
    }
    int reuse = 1;
    int ret = setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }

    ret = setsockopt(recvfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }
    if (bind(recvfd,(struct sockaddr*)&uAddrIn,sizeof(uAddrIn)) == -1) {
        perror("traverse udp bind() failed:");
        return NULL;
    }
    if (recvfd == -1) {
        perror("socket() failed:");
        //return -1;
    }

    struct sockaddr_in peerAddrIn;
    peerAddrIn.sin_family = AF_INET;
    peerAddrIn.sin_port = htons(peerAddr.port);
    peerAddrIn.sin_addr.s_addr = inet_addr(peerAddr.ip.c_str());
    if(peerAddr.port!=5555)
    {
        if (connect(recvfd, (struct sockaddr *) &peerAddrIn, sizeof(struct sockaddr)) == -1) {
            perror("chid connect");
            exit(1);
        } else {
            perror("");
        }
    }


#endif

    //设置recvfrom函数为超时函数
//    struct timeval tv_out;
//    tv_out.tv_sec = 10;//等待10秒
//    tv_out.tv_usec = 0;
//    setsockopt(recvfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));

    while (1){
        struct sockaddr_in sourceAddrIn;//暂存接受数据包的来源，在recvfrom中使用
        socklen_t peerAddrLen = sizeof(sourceAddrIn);//在recvfrom中使用
        char inBuf[1000];
        memset(inBuf,'\0',sizeof(inBuf));
        int len = recvfrom(recvfd, inBuf, sizeof(inBuf), 0,(struct sockaddr*)&sourceAddrIn,&peerAddrLen);
        //sleep(1);
        if (len == -1) {
            perror("recvfrom() failed:");
            return NULL;
        }else{
            //printf("%s\n",inBuf);
            //cout<<"sdfs"<<endl;

            printf("thread %x ,recv%s %d [%s]\n",
                   pthread_self(),
                   //peerAddr.ip.c_str(),
                   //peerAddr.port,
                   inet_ntoa(sourceAddrIn.sin_addr),
                   ntohs(sourceAddrIn.sin_port),
                   inBuf);

        }

    }

}


#if 0
void*  udpTraverse(void*arg) {
    string uAddr,peerAddr;
    AddrPair udpTraverseArg=*(AddrPair*)arg;
    uAddr=udpTraverseArg.uAddr;
    peerAddr=udpTraverseArg.peerAddr;




    struct sockaddr_in uAddrIn;
    struct sockaddr_in peerAddrIn;
    int sockfd;
    uint16_t  uPort,peerPort;
    char uIP[32]={0},peerIP[32]={0};
    sscanf(uAddr.c_str(),"%[^:]:%d",uIP,&uPort);

	uAddrIn.sin_family = AF_INET;
	uAddrIn.sin_port = htons(uPort);
	uAddrIn.sin_addr.s_addr = inet_addr(uIP);



	//初始化服务器S信息
    sscanf(peerAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);
	peerAddrIn.sin_family = AF_INET;
	peerAddrIn.sin_port = htons(peerPort);
	peerAddrIn.sin_addr.s_addr = inet_addr(peerIP);

    cout<<uIP<<"|"<<uPort<<endl;
    cout<<peerIP<<"|"<<peerPort<<endl;

    //return NULL;
	//建立与服务器通信的socket和与客户端通信的socket
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (sockfd == -1) {
		perror("socket() failed:");
		return NULL;
	}
    int reuse = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }
    if (bind(sockfd,(struct sockaddr*)&uAddrIn,sizeof(uAddrIn)) == -1) {
        perror("traverse udp bind() failed:");
        return false;
    }
//    pthread_t  ptd;
//    pthread_create(&ptd,NULL,recvUdpBuf,&sockfd);
	for (int i = 0; i < 100; ++i)
	{
        /*
        if(g_isTraversed==true){
            cout<<"traverse success"<<endl;

            close(sockfd);
            //modify by gaoyubin
            return NULL;
            //return false;
        }*/

        usleep(10*1000);
        string outStr="traverse";
		int len = sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&peerAddrIn,sizeof(peerAddrIn));
		if (len == -1) {
			perror("while sending package to C2 , sendto() failed:");
            close(sockfd);
			return NULL;
		}
        cout<<outStr<<endl;

	}
    close(sockfd);
    return NULL;

}
#endif

#define MAX_EPOLL_SIZE  32

struct TrResAddr{

    Addr peerReflexAddr;
    Addr uReflexAddr;
};
//trAddrInfoVect the first member is the localAddr,the last member is turnSvr,
void*  traverseUDP(void *arg) {

    printf("traverseUDP func\n");
    int  epfd, nfds;

    struct epoll_event ev;
    struct epoll_event events[MAX_EPOLL_SIZE];
    int opt = 1;;
    int ret = 0;

    printf("arg addr=%x\n",arg);
    vector<TrAddrInfo> trAddrInfoVect=((UDPComponent*)arg)->trAddrInfoVect;//*(vector<TrAddrInfo>*)arg;

    UDPComponent*udpComponentPtr=(UDPComponent*)arg;
    map<int,TrResAddr>trResAddrMap;
    map<Addr,int>addrHeightMap;
    for(int i=1;i<trAddrInfoVect.size()-1;i++)
    {
        addrHeightMap[trAddrInfoVect[i].addr]=i;
    }
    struct sockaddr_in uAddrIn;
    struct sockaddr_in peerAddrIn;
    struct sockaddr_in sourceAddrIn;

    int sockfd;
    //int*traverseFlagPtr=new int(0);
    //Addr* traverseAddrPtr=new Addr;

    uAddrIn.sin_family = AF_INET;
    uAddrIn.sin_port = htons(trAddrInfoVect[0].addr.port);
    uAddrIn.sin_addr.s_addr = inet_addr(trAddrInfoVect[0].addr.ip.c_str());

    //初始化服务器S信息

    peerAddrIn.sin_family = AF_INET;
    sourceAddrIn.sin_family = AF_INET;


    cout<<trAddrInfoVect[0].addr.ip.c_str()<<"|"<<trAddrInfoVect[0].addr.port<<endl;
    //cout<<peerIP<<"|"<<peerPort<<endl;

    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
//    else {
//        printf("socket OK\n");
//    }

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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

    epfd = epoll_create(MAX_EPOLL_SIZE);
    if (epfd == -1) {
        perror("");
    }
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sockfd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        fprintf(stderr, "epoll set insertion error: fd=%dn", sockfd);
        exit(1);
    }
//    else {
//        printf("ep add OK\n");
//    }
    //string outStr="traverse";
    int cnt=0;
    while (cnt<=40) {
//        if(trAddrInfoVect.size()<=2)
//            break;
        nfds = epoll_wait(epfd, events, sizeof(events) / sizeof(epoll_event), 40);
        if(nfds==-1){
            perror("");
        }
        else if (nfds == 0) {
            //perror("epoll_wait");
            //break;
            cnt++;
            for(int i=1;i<trAddrInfoVect.size()-1;++i){

                string peerIP=trAddrInfoVect[i].addr.ip;
                int  peerPort=trAddrInfoVect[i].addr.port;

                string outStr;
                Json::Value outVal;
                outVal["cmd"]=trAddrInfoVect[i].outStr;
                outVal["IP"]=peerIP;
                outVal["port"]=peerPort;

                //int curOffset=trAddrInfoVect[i].len==0?0:trAddrInfoVect[i].step++;
                if(trAddrInfoVect[i].step!=0){
                    for(int j=0;j<trAddrInfoVect[i].len/trAddrInfoVect[i].step;++j){
                        peerAddrIn.sin_port = htons(peerPort+j*trAddrInfoVect[i].step);
                        peerAddrIn.sin_addr.s_addr = inet_addr(peerIP.c_str());



                        outVal["port"]=peerPort+j*trAddrInfoVect[i].step;
                        outStr=outVal.toStyledString();

                        int len = sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&peerAddrIn,sizeof(peerAddrIn));
                        if (len == -1) {
                            perror("while sending package to C2 , sendto() failed:");
                            close(sockfd);
                            return NULL;
                        }
                        printf("thread %x send to %s:%d---%s\n",pthread_self(),peerIP.c_str(),peerPort+j*trAddrInfoVect[i].step,outStr.c_str());
                    }
                }
                else{
                    peerAddrIn.sin_port = htons(peerPort);
                    peerAddrIn.sin_addr.s_addr = inet_addr(peerIP.c_str());

                    outStr=outVal.toStyledString();
                    int len = sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&peerAddrIn,sizeof(peerAddrIn));
                    if (len == -1) {
                        perror("while sending package to C2 , sendto() failed:");
                        close(sockfd);
                        return NULL;
                    }
                    printf("thread %x send to %s:%d---%s\n",pthread_self(),peerIP.c_str(),peerPort,outStr.c_str());
                }



//                peerAddrIn.sin_port = htons(peerPort-curOffset);
//                len = sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&peerAddrIn,sizeof(peerAddrIn));
//                if (len == -1) {
//                    perror("while sending package to C2 , sendto() failed:");
//                    close(sockfd);
//                    return NULL;
//                }
//                printf("send to %s:%d---%s\n",peerIP.c_str(),peerPort-curOffset,outStr.c_str());
            }

        }
        else{
            char inBuf[200];
            memset(inBuf,0,sizeof(inBuf));
            socklen_t sourceAddrLen=sizeof(struct sockaddr);
            int len = recvfrom(sockfd, inBuf, sizeof(inBuf), 0,(struct sockaddr*)&sourceAddrIn,&sourceAddrLen);
            printf("-------------------------------------------thread %x traverse succes ,content [ %s ] , peerAddr is %s:%d\n",pthread_self(),inBuf,inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port));
            if(len<=0)
                perror("recvfrom");

#if 0
                //printf("recv from %s:%d------%s\n",inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port),inBuf);
//                if(inet_addr(trAddrInfoVect[1].addr.ip.c_str())==sourceAddrIn.sin_addr.s_addr &&
//                    htons(trAddrInfoVect[1].addr.port)==sourceAddrIn.sin_port){
//
//
//
//                    //g_trAddr=trAddrInfoVect[1].addr;
//                    *traverseFlagPtr=1;
//
//                    string outStr="OK";
//                    sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&sourceAddrIn,sizeof(sourceAddrIn));
//                    //sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&sourceAddrIn,sizeof(sourceAddrIn));
//                    //sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&sourceAddrIn,sizeof(sourceAddrIn));
//                    //break;
//                }
//                //printf("recv from %s:%d------%s\n",inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port),inBuf);
//                //g_isTraversed=true;
//                else if(trAddrInfoVect.size()>=3 &&
//                        inet_addr(trAddrInfoVect[2].addr.ip.c_str())==sourceAddrIn.sin_addr.s_addr &&
//                        htons(trAddrInfoVect[2].addr.port)==sourceAddrIn.sin_port){
//
//                    //g_trAddr=trAddrInfoVect[2].addr;
//                    *traverseFlagPtr=1;
//                    string outStr="OK";
//
//                    continue;
//                }
                if(strcmp(inBuf,"traverse")==0){
                    for(int i=0;i<trAddrInfoVect.size()-1;++i){
                        if(trAddrInfoVect[i].addr.ip==inet_ntoa(sourceAddrIn.sin_addr)){
                            trAddrInfoVect[i].outStr="ask";
                        }
                    }
                    if( *traverseAddrPtr==Addr() ||   (*traverseAddrPtr!=Addr() && inet_addr(trAddrInfoVect[1].addr.ip.c_str())==sourceAddrIn.sin_addr.s_addr) ){
                        *traverseAddrPtr=Addr(inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port));

                    }



                }
                else if(strcmp(inBuf,"ask")==0){
                    if(inet_addr(trAddrInfoVect[1].addr.ip.c_str())==sourceAddrIn.sin_addr.s_addr ){
                       *traverseAddrPtr=Addr(inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port));
                        if(trAddrInfoVect.size()==4)//the host
                            udpComponentPtr->uReflexAddr=udpComponentPtr->peerReflexAddr;
                        break;
                    }
                    else if(*traverseAddrPtr==Addr()){
                        *traverseAddrPtr=Addr(inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port));
                    }
                    //*traverseFlagPtr=1;
                }
#endif
            Json::Reader reader;
            Json::Value inVal;
            string inStr=inBuf;
            if(reader.parse(inStr,inVal)){
                Addr sourceAddr=Addr(inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port));
                int height=0;
                if(addrHeightMap.find(sourceAddr)==addrHeightMap.end())
                {
                    printf("cannot find sourceAddr in addrHeightMap\n");
                    for(auto it=addrHeightMap.begin();it!=addrHeightMap.end();++it){
                        if(sourceAddr.ip==it->first.ip){
                            height=it->second;
                            printf("height %d choose the %s:%d\n",height,it->first.ip.c_str(),it->first.port);
                            break;

                        }

                    }
                }
                else{
                    height=addrHeightMap[sourceAddr];
                }

                if(inVal["cmd"].asString()=="traverse"){
                    // the height is equal the index
                    cnt/=2;
                    trAddrInfoVect[height].outStr="ask";
//                    if(height==1)
//                        break;

                }
                else if(inVal["cmd"].asString()=="ask"){
                    trAddrInfoVect[height].outStr="ask";
                    trResAddrMap[height].peerReflexAddr=sourceAddr;
                    trResAddrMap[height].uReflexAddr=Addr(inVal["IP"].asString(),inVal["port"].asInt());
//                    if(height==1){
//                        break;
//                    }
                }
//                else if(inVal["cmd"].asString()=="OK"){//turn success
//                    printf("turn success\n");
//                    trAddrInfoVect.pop_back();
//                }
            }



        }

    }




    close(sockfd);
//    if(*traverseAddrPtr==Addr()){//can not traverse ,so use turn
//        cout<<"traverse fail"<<endl;
//        printf("the client addr is %s:%d\n",trAddrInfoVect[0].addr.ip,trAddrInfoVect[0].addr.port);
//        if(turnUDP(trAddrInfoVect[0].addr, trAddrInfoVect.back().addr, trAddrInfoVect.back().outStr)){
//            *traverseAddrPtr=trAddrInfoVect.back().addr;
//            cout<<"turn success"<<endl;
//        }
//        else{
//            cout<<"turn fail"<<endl;
//        }
//    }
//    else{
//        cout<<"---------------------------------------------------------------------------traverse success"<<endl;
//    }

    if(trResAddrMap.size()==0){
        cout<<"traverse fail"<<endl;
        printf("the client addr is %s:%d\n",trAddrInfoVect[0].addr.ip.c_str(),trAddrInfoVect[0].addr.port);

        //for test
        if(turnUDP(trAddrInfoVect[0].addr, trAddrInfoVect.back().addr, trAddrInfoVect.back().outStr)){
            trResAddrMap[1].peerReflexAddr=trAddrInfoVect.back().addr;
            trResAddrMap[1].uReflexAddr=trAddrInfoVect.back().addr;
            cout<<"turn success"<<endl;
        }
        else{
            cout<<"turn fail"<<endl;
        }
    }
    else{
        cout<<"---------------------------------------------------------------------------traverse success"<<endl;
    }


    //cout<<*traverseAddrPtr<<endl;
    //printf("*traverseAddrPtr=%s:%d\n",traverseAddrPtr->ip.c_str(), traverseAddrPtr->port);
    //cout<<"*traverseAddrPtr="<<*traverseAddrPtr<<endl;
    //cout<<"g_isTraversed"<<" "<<*traverseFlagPtr<<endl;

    if(trResAddrMap.begin()->second.uReflexAddr!=Addr())
        udpComponentPtr->uReflexAddr=trResAddrMap.begin()->second.uReflexAddr;
    udpComponentPtr->peerReflexAddr=trResAddrMap.begin()->second.peerReflexAddr;
    pthread_exit(NULL);


}

