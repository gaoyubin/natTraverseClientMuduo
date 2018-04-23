
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
using namespace std;

bool  g_isTraverse=false;

void* recvUdpBuf(void *arg){

    int sockfd=*(int*)arg;
    if (sockfd == -1) {
        perror("socket() failed:");
        //return -1;
    }
    while (1){
        struct sockaddr_in peerAddr;//暂存接受数据包的来源，在recvfrom中使用
        socklen_t peerAddrLen = sizeof(peerAddr);//在recvfrom中使用
        char inBuf[1000];
        memset(inBuf,'\0',sizeof(inBuf));
        int len = recvfrom(sockfd, inBuf, sizeof(inBuf), 0,(struct sockaddr*)&peerAddr,&peerAddrLen);

        if (len == -1) {
            perror("recvfrom() failed:");
            //return -1;
        }else{
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(struct sockaddr_in);



            printf("this socket addr %s %d successful\n",inet_ntoa(peerAddr.sin_addr),ntohs(peerAddr.sin_port));

            g_isTraverse=true;
            printf("recBuf fn rec %s\n",inBuf);

            //break;

        }

    }

}

bool udpTraverse(string peerAddr,string uAddr) {

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

	//建立与服务器通信的socket和与客户端通信的socket
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (sockfd == -1) {
		perror("socket() failed:");
		return false;
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
    pthread_t  ptd;
    pthread_create(&ptd,NULL,recvUdpBuf,&sockfd);
	for (int i = 0; i < 30; ++i)
	{
        if(g_isTraverse==true){
            cout<<"traverse success"<<endl;

            //close(sockfd);
            //modify by gaoyubin
            return true;
        }

        usleep(500*1000);
        string outStr="traverse";
		int len = sendto(sockfd,outStr.c_str(),outStr.size(),0,(struct sockaddr*)&peerAddrIn,sizeof(peerAddrIn));
		if (len == -1) {
			perror("while sending package to C2 , sendto() failed:");
            close(sockfd);
			return false;
		}
        cout<<outStr<<endl;

	}
    //close(sockfd);
    return false;

}

