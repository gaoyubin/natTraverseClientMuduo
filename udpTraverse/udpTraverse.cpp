
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

#include "../stun/stun.h"
#include "../stun/udp.h"
#include "../stun/MyStun.hpp"
using namespace std;

bool  g_isTraversed=false;
Addr  g_trAddr;
void* recvUdpBuf(void *arg){

    string uAddr,peerAddr;

    uAddr=*(string*)arg;


    struct sockaddr_in uAddrIn;

    int sockfd;
    uint16_t  uPort,peerPort;
    char uIP[32]={0},peerIP[32]={0};
    sscanf(uAddr.c_str(),"%[^:]:%u",uIP,&uPort);

    uAddrIn.sin_family = AF_INET;
    uAddrIn.sin_port = htons(uPort);
    uAddrIn.sin_addr.s_addr = inet_addr(uIP);

    cout<<uIP<<"|"<<uPort<<endl;


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
        return NULL;
    }




    if (sockfd == -1) {
        perror("socket() failed:");
        //return -1;
    }
    //设置recvfrom函数为超时函数
    struct timeval tv_out;
    tv_out.tv_sec = 10;//等待10秒
    tv_out.tv_usec = 0;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));

    while (1){
        struct sockaddr_in peerAddrIn;//暂存接受数据包的来源，在recvfrom中使用
        socklen_t peerAddrLen = sizeof(peerAddrIn);//在recvfrom中使用
        char inBuf[1000];
        memset(inBuf,'\0',sizeof(inBuf));
        int len = recvfrom(sockfd, inBuf, sizeof(inBuf), 0,(struct sockaddr*)&peerAddrIn,&peerAddrLen);

        if (len == -1) {
            perror("recvfrom() failed:");
            return NULL;
        }else{

            if(strcmp(inBuf,"OK")){

            }
            else{

            }


            printf("this socket addr %s %d successful\n",inet_ntoa(peerAddrIn.sin_addr),ntohs(peerAddrIn.sin_port));

            g_isTraversed=true;
            printf("recBuf fn rec %s\n",inBuf);

            //break;

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

void*  udpTraverse(void*arg) {

    int  epfd, nfds;

    struct epoll_event ev;
    struct epoll_event events[MAX_EPOLL_SIZE];
    int opt = 1;;
    int ret = 0;



    vector<TrAddrInfo> trAddrInfoVect=*(vector<TrAddrInfo>*)arg;


    struct sockaddr_in uAddrIn;
    struct sockaddr_in peerAddrIn;
    struct sockaddr_in sourceAddrIn;

    int sockfd;


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
    } else {
        printf("socket OK\n");
    }

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
    } else {
        printf("IP bind OK\n");
    }

    epfd = epoll_create(MAX_EPOLL_SIZE);
    if (epfd == -1) {
        perror("");
    }
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sockfd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        fprintf(stderr, "epoll set insertion error: fd=%dn", sockfd);
        exit(1);
    } else {
        printf("ep add OK\n");
    }

    int cnt=0;
    while (cnt<=10) {
        cnt++;
        nfds = epoll_wait(epfd, events, sizeof(events) / sizeof(epoll_event), 900);
        if (nfds == 0) {
            //perror("epoll_wait");
            //break;
            for(int i=1;i<trAddrInfoVect.size();++i){
                string outStr="traverse";
                string peerIP=trAddrInfoVect[i].addr.ip;
                uint16_t  peerPort=trAddrInfoVect[i].addr.port;
                int curOffset=trAddrInfoVect[i].len==0?0:trAddrInfoVect[i].cur++;
                peerAddrIn.sin_port = htons(peerPort+curOffset);
                peerAddrIn.sin_addr.s_addr = inet_addr(peerIP.c_str());
                int len = sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&peerAddrIn,sizeof(peerAddrIn));
                if (len == -1) {
                    perror("while sending package to C2 , sendto() failed:");
                    close(sockfd);
                    return NULL;
                }
                printf("send to %s:%d---%s\n",peerIP.c_str(),peerPort+curOffset,outStr.c_str());

                peerAddrIn.sin_port = htons(peerPort-curOffset);
                len = sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&peerAddrIn,sizeof(peerAddrIn));
                if (len == -1) {
                    perror("while sending package to C2 , sendto() failed:");
                    close(sockfd);
                    return NULL;
                }
                printf("send to %s:%d---%s\n",peerIP.c_str(),peerPort-curOffset,outStr.c_str());
            }

        }
        else{
            char inBuf[200];
            memset(inBuf,0,sizeof(inBuf));
            socklen_t sourceAddrLen=sizeof(struct sockaddr);
            int len = recvfrom(sockfd, inBuf, sizeof(inBuf), 0,(struct sockaddr*)&sourceAddrIn,&sourceAddrLen);
            printf("traverse succes peerAddr is %s:%d\n",inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port));

            if(len<=0){
                perror("recvfrom");

            }
            else{
                //printf("recv from %s:%d------%s\n",inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port),inBuf);
                if(inet_addr(trAddrInfoVect[1].addr.ip.c_str())==sourceAddrIn.sin_addr.s_addr &&
                    htons(trAddrInfoVect[1].addr.port)==sourceAddrIn.sin_port){



                    g_trAddr=trAddrInfoVect[1].addr;
                    g_isTraversed=true;

                    string outStr="OK";
                    sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&sourceAddrIn,sizeof(sourceAddrIn));
                    break;
                }
                //printf("recv from %s:%d------%s\n",inet_ntoa(sourceAddrIn.sin_addr),ntohs(sourceAddrIn.sin_port),inBuf);
                //g_isTraversed=true;
                else if(trAddrInfoVect.size()>=3 &&
                        inet_addr(trAddrInfoVect[2].addr.ip.c_str())==sourceAddrIn.sin_addr.s_addr &&
                        htons(trAddrInfoVect[2].addr.port)==sourceAddrIn.sin_port){

                    g_trAddr=trAddrInfoVect[2].addr;
                    g_isTraversed=true;
                    string outStr="OK";
                    sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&sourceAddrIn,sizeof(sourceAddrIn));
                    continue;
                }

            }

        }


    }
    cout<<"g_isTraversed"<<" "<<g_isTraversed<<endl;
    close(sockfd);
    return NULL;

}

/*
bool getSymIncRange(StunAddress4 &stunSvrAddr4,StunAddress4 &peerAddr4,Addr localAddr,Addr &forecastAddr,int len){


    UInt16 port=rand()%3000+4000;
    cout<<"port: "<<port<<endl;
    bool verbose=false;

    Socket myFd1 = openPort(port,ntohl(inet_addr(localAddr.ip.c_str())),verbose);
    Socket myFd2 = openPort(localAddr.port,ntohl(inet_addr(localAddr.ip.c_str())),verbose);
    Socket myFd3 = openPort(localAddr.port+1,ntohl(inet_addr(localAddr.ip.c_str())),verbose);



    if ( ( myFd1 == INVALID_SOCKET) || ( myFd2 == INVALID_SOCKET)  )
    {
        cerr << "Some problem opening port/interface to send on" << endl;
        return false;
    }



    StunAtrString username;
    StunAtrString password;

    username.sizeValue = 0;
    password.sizeValue = 0;

    int count=0;


    while ( count < 5 )
    {
        struct timeval tv;
        fd_set fdSet;
#ifdef WIN32
        unsigned int fdSetSize;
#else
        int fdSetSize;
#endif
        FD_ZERO(&fdSet); fdSetSize=0;
        FD_SET(myFd1,&fdSet); fdSetSize = (myFd1+1>fdSetSize) ? myFd1+1 : fdSetSize;
        FD_SET(myFd2,&fdSet); fdSetSize = (myFd2+1>fdSetSize) ? myFd2+1 : fdSetSize;
        FD_SET(myFd3,&fdSet); fdSetSize = (myFd3+1>fdSetSize) ? myFd3+1 : fdSetSize;
        tv.tv_sec=0;
        tv.tv_usec=150*1000; // 150 ms
        if ( count == 0 ) tv.tv_usec=0;

        int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
        int e = getErrno();
        if ( err == SOCKET_ERROR )
        {
            // error occured
            cerr << "Error " << e << " " << strerror(e) << " in select" << endl;
            break;
        }
        else if ( err == 0 )
        {
            count++;
            for(int i=0;i<2;i++)
                stunSendTest( myFd1, stunSvrAddr4, username, password, 7 ,verbose );
            for(int i=0;i<2;i++)
                stunSendTest( myFd2, peerAddr4, username, password, 8 ,verbose );
            for(int i=0;i<2;i++)
                stunSendTest( myFd2, peerAddr4, username, password, 8 ,verbose );
            for(int i=0;i<2;i++)
                stunSendTest( myFd3, stunSvrAddr4, username, password, 9 ,verbose );
        }
        else
        {
            if (verbose) clog << "-----------------------------------------" << endl;
            assert( err>0 );
            // data is avialbe on some fd

            for ( int i=0; i<3; i++)
            {
                Socket myFd;
                if ( i==0 )
                {
                    myFd=myFd1;
                }
                else if(i==1)
                {
                    myFd=myFd2;
                }
                else{
                    myFd=myFd3;
                }


                if ( myFd!=INVALID_SOCKET )
                {
                    if ( FD_ISSET(myFd,&fdSet) )
                    {
                        char msg[STUN_MAX_MESSAGE_SIZE];
                        int msgLen = sizeof(msg);

                        StunAddress4 from;

                        getMessage( myFd,
                                    msg,
                                    &msgLen,
                                    &from.addr,
                                    &from.port,verbose );

                        StunMessage resp;
                        memset(&resp, 0, sizeof(StunMessage));

                        stunParseMessage( msg,msgLen, resp,verbose );

                        if ( verbose )
                        {
                            clog << "Received message of type " << resp.msgHdr.msgType
                                 << "  id=" << (int)(resp.msgHdr.id.octet[0]) << endl;
                        }

                        switch( resp.msgHdr.id.octet[0] ){
                            case 7:
                                    struct in_addr mapInAddr;
                                    //在stunParseMessage的时候，已经转化为主机序了了．但是inet_ntoa需要网络序，
                                    // 所以映射地址要想htonl
                                    mapInAddr.s_addr=htonl(resp.mappedAddress.ipv4.addr);
                                    startAddr.ip=inet_ntoa(mapInAddr);
                                    startAddr.port=resp.mappedAddress.ipv4.port;
                                    cout<<"recv 7"<<endl;
                                    break;
                            case 9:

                                mapInAddr.s_addr=htonl(resp.mappedAddress.ipv4.addr);
                                endAddr.ip=inet_ntoa(mapInAddr);
                                endAddr.port=resp.mappedAddress.ipv4.port;
                                cout<<"recv 9"<<endl;
                                break;
                            default:
                                cout<<"recv invalid"<<endl;

                        }
                        if(startAddr!=Addr() && endAddr!=Addr()){
                            return true;
                        }

                    }
                }
            }
        }
    }
    close(myFd1);
    close(myFd2);
    close(myFd3);
    return false;
}
*/
