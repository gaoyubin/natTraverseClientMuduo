//
// Created by tl on 18-5-10.
//

#include "tcpTurn.hpp"
#include "../natTraverse.hpp"
#include "../udpTraverse/udpTraverse.hpp"


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <zconf.h>
#include <cerrno>

#define BUFFER_SIZE 4000
#define MAX_EVENTS 10


void*  turnTCP(void*arg)
{

    TCPComponent tcpComp=*(TCPComponent*)arg;
    int liveCliSockfd;
    int turnCliSockfd;
    int liveConnfd=-1;


    struct sockaddr_in turnCliAddrIn;
    struct sockaddr_in liveCliAddrIn;

    struct sockaddr_in turnSvrAddrIn;
    struct sockaddr_in liveSvrAddrIn;

    printf("turnCli=%s:%d,liveCli=%s:%d\n",
           tcpComp.turnCliAddr.ip.c_str(),
           tcpComp.turnCliAddr.port,
           tcpComp.liveCliAddr.ip.c_str(),
           tcpComp.liveCliAddr.port
    );

    //***************************** live555
    memset(&liveCliAddrIn,0,sizeof(liveCliAddrIn)); // 数据初始化--清零
    liveCliAddrIn.sin_family=AF_INET; // 设置为IP通信
    liveCliAddrIn.sin_addr.s_addr=inet_addr(tcpComp.liveCliAddr.ip.c_str());// 服务器IP地址--允许连接到所有本地地址上
    liveCliAddrIn.sin_port=htons(tcpComp.liveCliAddr.port); // 服务器端口号
    if((liveCliSockfd=socket(PF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket");
        return NULL;
    }
    if (bind(liveCliSockfd,(struct sockaddr *)&liveCliAddrIn,sizeof(struct sockaddr))<0)
    {
        perror("bind");
        return NULL;
    }
    memset(&liveSvrAddrIn,0,sizeof(liveSvrAddrIn)); // 数据初始化--清零
    liveSvrAddrIn.sin_family=AF_INET; // 设置为IP通信
    liveSvrAddrIn.sin_addr.s_addr=inet_addr(tcpComp.liveSvrAddr.ip.c_str());// 服务器IP地址--允许连接到所有本地地址上
    liveSvrAddrIn.sin_port=htons(tcpComp.liveSvrAddr.port); // 服务器端口号

    cout<<"g_account="<<g_account<<endl;
    if(g_account.role=="s"){
        printf("liveCliSockfd connect\n");
        if(connect(liveCliSockfd,(struct sockaddr*)&liveSvrAddrIn,sizeof(struct sockaddr))<0){
            perror("connect to liveSvr fail");

        }
    }
    else{
        listen(liveCliSockfd,3);
        printf("liveCliSockfd listen\n");
    }




    //*********************************turn
    memset(&turnCliAddrIn,0,sizeof(turnCliAddrIn)); // 数据初始化--清零
    turnCliAddrIn.sin_family=AF_INET; // 设置为IP通信
    turnCliAddrIn.sin_addr.s_addr=inet_addr(tcpComp.turnCliAddr.ip.c_str());// 服务器IP地址--允许连接到所有本地地址上
    turnCliAddrIn.sin_port=htons(tcpComp.turnCliAddr.port); // 服务器端口号

    if((turnCliSockfd=socket(PF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket");
        return NULL;
    }
    // 将套接字绑定到服务器的网络地址上
    if (bind(turnCliSockfd,(struct sockaddr *)&turnCliAddrIn,sizeof(struct sockaddr))<0)
    {
        perror("bind");
        return NULL;
    }
    memset(&turnSvrAddrIn,0,sizeof(turnSvrAddrIn)); // 数据初始化--清零
    turnSvrAddrIn.sin_family=AF_INET; // 设置为IP通信
    turnSvrAddrIn.sin_addr.s_addr=inet_addr(tcpComp.turnSvrAddr.ip.c_str());// 服务器IP地址--允许连接到所有本地地址上
    turnSvrAddrIn.sin_port=htons(tcpComp.turnSvrAddr.port); // 服务器端口号

    if(connect(turnCliSockfd,(struct sockaddr*)&turnSvrAddrIn,sizeof(struct sockaddr))<0){
        perror("connect to turnSvr fail\n");
    }
    send(turnCliSockfd,tcpComp.tcpID.c_str(),tcpComp.tcpID.size(),0);





    // 创建一个epoll句柄
    int epoll_fd;
    epoll_fd=epoll_create(MAX_EVENTS);
    if(epoll_fd==-1)
    {
        perror("epoll_create failed");
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev;// epoll事件结构体
    struct epoll_event events[MAX_EVENTS];// 事件监听队列

    ev.events=EPOLLIN;
    ev.data.fd=turnCliSockfd;
    // 向epoll注册server_sockfd监听事件
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,turnCliSockfd,&ev)==-1)
    {
        perror("epll_ctl:server_sockfd register failed");
        exit(EXIT_FAILURE);
    }

    ev.events=EPOLLIN;
    ev.data.fd=liveCliSockfd;
    // 向epoll注册server_sockfd监听事件
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,liveCliSockfd,&ev)==-1)
    {
        perror("epll_ctl:server_sockfd register failed");
        exit(EXIT_FAILURE);
    }

    int nfds;// epoll监听事件发生的个数
    // 循环接受客户端请求

    cout<<"success step in epoll_wait"<<endl;
    while(1)
    {
        // 等待事件发生,timeout单位为毫秒
        nfds=epoll_wait(epoll_fd,events,MAX_EVENTS,1000);
        if(nfds==-1)
        {
            if(errno!=EINTR){
                perror("start epoll_wait failed");
                exit(EXIT_FAILURE);
            }

        }
        if(nfds==0){
            send(turnCliSockfd,"1",1,MSG_OOB);
            continue;
        }
        for(int i=0;i<nfds;i++)
        {


            if(events[i].data.fd==turnCliSockfd)
            {
                char turnbuf[BUFFER_SIZE];
                memset(turnbuf,0,sizeof(turnbuf));
                int turnLen=recv(turnCliSockfd,turnbuf,BUFFER_SIZE,0);
                if(turnLen<0)
                {
                    perror("receive from client failed");
                    exit(EXIT_FAILURE);
                }
                else if(turnLen>0){
                    //printf("receive [ %s ] from turnSvr\n",turnbuf);
                    if(g_account.role=="s")
                        send(liveCliSockfd,turnbuf,turnLen,0);
                    else if(g_account.role=="c" && liveConnfd!=-1){
                        send(liveConnfd,turnbuf,turnLen,0);
                    }
                }
                else{
                    cout<<"turnSvr is closed"<<endl;
                    close(turnCliSockfd);
                    break;
                }
            }
            else if(events[i].data.fd==liveCliSockfd)
            {
                if(g_account.role=="s"){
                    char livebuf[BUFFER_SIZE];
                    memset(livebuf,0,sizeof(livebuf));
                    int liveLen=recv(liveCliSockfd,livebuf,BUFFER_SIZE,0);
                    if(liveLen<0)
                    {

                        //for test
                        //perror("receive from client failed");
                        //exit(EXIT_FAILURE);
                    }
                    else if(liveLen>0){
                        //printf("receive from live:%s\n",livebuf);
                        send(turnCliSockfd,livebuf,liveLen,0);
                    }
                    else{
                        cout<<"liveSvr is closed"<<endl;
                        close(liveCliSockfd);
                        break;
                    }
                }
                else{
                    struct sockaddr in_addr;
                    socklen_t in_len=sizeof(struct sockaddr);


                    liveConnfd = accept(liveCliSockfd,&in_addr,&in_len);
                    if(liveConnfd==-1)
                       perror("liveConnd");
                    else{
                        cout<<"accept success"<<endl;
                        ev.events=EPOLLIN;
                        ev.data.fd=liveConnfd;
                        // 向epoll注册server_sockfd监听事件
                        if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,liveConnfd,&ev)==-1)
                        {
                            perror("epll_ctl:server_sockfd register failed");
                            exit(EXIT_FAILURE);
                        }

                    }


                }

            }
            else if(events[i].data.fd==liveConnfd){
                char livebuf[BUFFER_SIZE];
                memset(livebuf,0,sizeof(livebuf));
                int liveLen=recv(liveConnfd,livebuf,BUFFER_SIZE,0);
                if(liveLen<0)
                {
                    perror("receive from client failed");
                    exit(EXIT_FAILURE);
                }
                else if(liveLen>0){
                    //printf("receive from live:%s\n",livebuf);
                    send(turnCliSockfd,livebuf,liveLen,0);
                }
                else{
                    cout<<"liveSvr is closed"<<endl;
                    close(liveConnfd);
                    break;
                }
            }
        }
    }
    return NULL;
}
