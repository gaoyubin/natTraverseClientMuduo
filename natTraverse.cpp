#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<errno.h>


#include "json/json.h"
#include "string"
#include "iostream"
#include "./stun/client.hpp"
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sstream>

#include <semaphore.h>

#include "udpTraverse/udpTraverse.hpp"
#include "udpTurn/udpTurn.hpp"
#include "stun/MyStun.hpp"
#include "heartBeat/heartBeat.hpp"
#include "tcpTurn/tcpTurn.hpp"
#include "natTraverse.hpp"
using namespace std;

#define MAX_IFS 32


//char g_centreSvrIP[30] = "218.192.170.178";
//int  g_centreSvrPort=2007;
//Addr g_stunSvrAddr;
//NatType g_natType;
//string g_netmask;


map<string,ClientConnInfo> g_cliConnInfoMap;
LocalNetInfo g_localNetInfo;
Account g_account;
SvrAddrInfo g_svrAddrInfo;
vector<string>g_userList;
int g_sockfd;
string g_response;
sem_t g_sem;

//pthread_mutex_t listMutex=PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t  listCond=PTHREAD_COND_INITIALIZER;
//
//pthread_mutex_t seeMutex=PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t  seeCond=PTHREAD_COND_INITIALIZER;

int g_udpTurnSvrPort; // in turn

//extern StunAddress4 g_reflexStunAddr4h;
/*********************/
//vector<AddrPair> addrPairVect;
//AddrTriple g_addrTriple;
//StunAddress4

//vector<tuple<StunAddress4,int,int>>g_addrTupleVect;

//vector<TrAddrInfo>g_traverseAddrInfoVect;
//typedef enum{
//    TraverseMethodCone,
//    TraverseMethodOpen,
//    TraverseMethodSymInc,
//    TraverseMethodSymRandom,
//    TraverseMethodTurn,
//
//}TraverseMethod;


int sendCodec(int sockfd,string sendStr){
    char*sendArr=new char[sendStr.size()+sizeof(int32_t)];
    int32_t len=sendStr.size();
    int32_t be32=htonl(len);
    *(int32_t *)sendArr=be32;
    memcpy(sendArr+sizeof(be32),sendStr.c_str(),sendStr.size());
    send(sockfd,sendArr,sendStr.size()+sizeof(int32_t),0);
}
void sendRespondCmd(int sockfd,string answerCmd,string state){
    Json::Value outVal;
    outVal["cmd"]="respond";
    outVal["answerCmd"]=answerCmd;
    outVal["state"]=state;


    sendCodec(sockfd,outVal.toStyledString());
}



void * recvTcpBuf (void *arg){
    int sockfd=*((int*)arg);


    while(1){
        printf("while(1) !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        char inBuf[1000]={0};
        int recvByte;
        int32_t  be32;
        recvByte=recv(sockfd,&be32,sizeof(be32),0);
        if(recvByte==0){
            cout<<"client close"<<endl;
            break;
        }
        int32_t  len=ntohl(be32);
        recvByte=recv(sockfd,inBuf,len,0);

        printf("-----------------------------------------------------recv data of my world is :%s\n",inBuf);
        string inStr(inBuf);
        Json::Reader reader;
        Json::Value inVal;
        if(reader.parse(inStr,inVal)){
            if(!inVal["cmd"].isNull()){
                if(inVal["cmd"]=="respond"){
                    cout<<"respond"<<inVal["answerCmd"]<<":"<<inVal["state"]<<endl;
                    if(inVal["answerCmd"]=="list"){

//                        pthread_mutex_lock(&listMutex);
                        string userStrWithSpace=inVal["extraInfo"].asString();
                        g_userList.clear();
                        istringstream  iss(userStrWithSpace);
                        string userName;
                        while(iss>>userName){
                            g_userList.push_back(userName);
                        }
                        sem_post(&g_sem);
//                        pthread_cond_signal(&listCond);
//                        pthread_mutex_unlock(&listMutex);

                    }
                    else if(inVal["answerCmd"]=="login"){
                        g_response=inVal["state"].asString();
                        sem_post(&g_sem);
                    }
                }
                else if(inVal["cmd"]=="detect"){
//                    printf("recv detect\n");
//                    printf("now continue\n");
//                    continue;

                    char hostIP[32];
                    char netMask[32];
                    char netInterface[32];

                    string peerName=inVal["peerName"].asString();
                    g_cliConnInfoMap.insert(pair<string,ClientConnInfo>(peerName,ClientConnInfo()));
                    g_cliConnInfoMap[peerName].udpComps.resize(2);

                    getIpInterface(hostIP,netMask,netInterface);

                    cout<<netMask<<endl;
                    cout<<hostIP<<endl;
                    cout<<netInterface<<endl;


                    g_cliConnInfoMap[peerName].udpComps[0].uHostAddr.port=(rand()%2000+1024)&~1;
                    g_cliConnInfoMap[peerName].udpComps[0].uHostAddr.ip=hostIP;
                    g_cliConnInfoMap[peerName].udpComps[1].uHostAddr.port=g_cliConnInfoMap[peerName].udpComps[0].uHostAddr.port+1;
                    g_cliConnInfoMap[peerName].udpComps[1].uHostAddr.ip=hostIP;
                    StunAddress4 dest1;
                    if(!inVal["stunSvrIP"].isNull()){
                        g_svrAddrInfo.stunSvrAddr.ip=inVal["stunSvrIP"].asString();
                    }
                    else{
                        g_svrAddrInfo.stunSvrAddr.ip="217.10.68.152";
                    }
                    dest1.addr=ntohl(inet_addr(g_svrAddrInfo.stunSvrAddr.ip.c_str()));
                    dest1.port=3478;
                    StunAddress4 sAddr;
                    sAddr.port=g_cliConnInfoMap[peerName].udpComps[0].uHostAddr.port;

                    //int symIncDiff=0;
                    MyNatType natType=detectNatType(dest1,false,g_cliConnInfoMap[peerName].udpComps);

                    //fill the LocalNetInfo
                    g_localNetInfo.natType=natType;
                    g_localNetInfo.netMask=netMask;
                    g_localNetInfo.hostIP=hostIP;


                    cout<<getNatTypeStr(natType)<<endl;

//                    string hostAddr;

//                    hostAddr=hostIP;
//                    hostAddr+=":"+to_string(g_hostPort);
//                    cout<<hostAddr<<endl;
//
//
//                    int tmpInt=ntohl(g_reflexStunAddr4h.addr);
//                    g_reflexAddr=inet_ntoa(*  (struct in_addr *)&tmpInt);
//
//                    g_reflexAddr+=":"+to_string(g_reflexStunAddr4h.port);//放在string里面，不用考虑字节序问题
//
//                    cout<<g_reflexStunAddr4h.port<<endl;


                    Json::Value outVal;
                    outVal["cmd"]="respond";
                    outVal["answerCmd"]="detect";
                    outVal["peerName"]=peerName;
                    outVal["natType"]=natType;
                    outVal["netMask"]=netMask;
                    //for test
                    outVal["comp"]["cnt"]=2;

                    outVal["comp"]["0"]["hostAddr"]=g_cliConnInfoMap[peerName].udpComps[0].uHostAddr.ip\
                    +":"+to_string(g_cliConnInfoMap[peerName].udpComps[0].uHostAddr.port);


                    outVal["comp"]["0"]["reflexAddr"]=g_cliConnInfoMap[peerName].udpComps[0].uReflexAddr.ip\
                    +":"+to_string(g_cliConnInfoMap[peerName].udpComps[0].uReflexAddr.port);


                    outVal["comp"]["1"]["hostAddr"]=g_cliConnInfoMap[peerName].udpComps[1].uHostAddr.ip\
                    +":"+to_string(g_cliConnInfoMap[peerName].udpComps[1].uHostAddr.port);

                    outVal["comp"]["1"]["reflexAddr"]=g_cliConnInfoMap[peerName].udpComps[1].uReflexAddr.ip\
                    +":"+to_string(g_cliConnInfoMap[peerName].udpComps[1].uReflexAddr.port);

                    string sendStr=outVal.toStyledString();

                    sendCodec(sockfd,sendStr);
                    cout<<"send"<<sendStr.c_str()<<endl;

                }
                else if(inVal["cmd"]=="traverse"){
                    string peerName=inVal["peerName"].asString();
                    uint16_t  peerPort;
                    char peerIP[32]={0};
                    string peerAddr;
                    //for test
                    for(int i=0;i<inVal["comp"]["cnt"].asInt();++i){
                        int symIncIndex=-1;
                        g_cliConnInfoMap[peerName].udpComps[i].trAddrInfoVect.clear();
                        g_cliConnInfoMap[peerName].udpComps[i].trAddrInfoVect.push_back(TrAddrInfo(Addr(
                                g_cliConnInfoMap[peerName].udpComps[i].uHostAddr.ip,
                                g_cliConnInfoMap[peerName].udpComps[i].uHostAddr.port),0,0));
                        if(!inVal["comp"][to_string(i)]["checklist"]["index"].isNull()){
                            symIncIndex=inVal["comp"][to_string(i)]["checklist"]["index"].asInt();
                        }
                        int checkListCnt=inVal["comp"][to_string(i)]["checklist"]["cnt"].asInt();
                        for(int j=0;j<checkListCnt;++j){
                            int step=0,len=0;

                            string trOutStr="traverse";

                            memset(peerIP,0,sizeof(peerIP));
                            if(j==checkListCnt-1){

                                trOutStr=inVal["comp"][to_string(i)]["checklist"]["id"].asString();

                            }

                            peerAddr=inVal["comp"][to_string(i)]["checklist"][to_string(j)].asString();
                            sscanf(peerAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);
                            if(j==symIncIndex) {
                                step = inVal["comp"][to_string(i)]["checklist"]["step"].asInt();
                                len = inVal["comp"][to_string(i)]["checklist"]["len"].asInt();
                                //peerPort-=step;
                            }
                            g_cliConnInfoMap[peerName].udpComps[i].trAddrInfoVect.push_back(TrAddrInfo(Addr(peerIP,peerPort),step,len,trOutStr));
                        }

//                        if(inVal["comp"][to_string(i)]["checklist"]["natType"].isNull()!=true){
//                            for(int k=0;k<200;++k)
//                            {
//                                g_cliConnInfoMap[peerName].udpComps[i].trAddrInfoVect[0].addr.port+=1;
//                                //printf("g_clionConnInfoMap addr = %x\n",(void *) &g_cliConnInfoMap[peerName].udpComps[i]);
//                                pthread_create(&g_cliConnInfoMap[peerName].udpComps[i].traverseTid,
//                                               NULL,
//                                               traverseUDP,
//                                               (void *) &g_cliConnInfoMap[peerName].udpComps[i]);
//
//
//                            }
//                        }
//                        else{
//                            pthread_create(&g_cliConnInfoMap[peerName].udpComps[i].traverseTid,
//                                           NULL,
//                                           traverseUDP,
//                                           (void *) &g_cliConnInfoMap[peerName].udpComps[i]);
//
//                        }
                        pthread_create(&g_cliConnInfoMap[peerName].udpComps[i].traverseTid,
                                           NULL,
                                           traverseUDP,
                                           (void *) &g_cliConnInfoMap[peerName].udpComps[i]);



                    }
                    //*************************tcp turn
                    g_cliConnInfoMap[peerName].tcpComp.tcpID=inVal["tcpID"].asString();
                    char turnSvrIP[32]="";
                    sscanf(inVal["tcpTurnSvrAddr"].asString().c_str(),"%[^:]:%d",
                           turnSvrIP,
                           &g_cliConnInfoMap[peerName].tcpComp.turnSvrAddr.port
                    );
                    g_cliConnInfoMap[peerName].tcpComp.turnSvrAddr.ip=turnSvrIP;

                    //tcpComp.turnSvrAddr.ip="192.168.1.101";
                    g_cliConnInfoMap[peerName].tcpComp.turnCliAddr.port=rand()%4000+3000;
                    g_cliConnInfoMap[peerName].tcpComp.liveCliAddr.port=rand()%2000+1000;
                    g_cliConnInfoMap[peerName].tcpComp.liveSvrAddr.port=8554;
                    pthread_create(&g_cliConnInfoMap[peerName].tcpComp.proxyTid,NULL,turnTCP,&g_cliConnInfoMap[peerName].tcpComp);
                    //*******************************************

                    //void*peerAddrPtr;
                    for(int i=0;i<inVal["comp"]["cnt"].asInt();++i){

                        pthread_join(g_cliConnInfoMap[peerName].udpComps[i].traverseTid,NULL);

                        printf("uHostAddr=%s:%d,uReflexAddr=%s:%d,peerReflexAddr=%s:%d\n",
                               g_cliConnInfoMap[peerName].udpComps[i].uHostAddr.ip.c_str(),
                               g_cliConnInfoMap[peerName].udpComps[i].uHostAddr.port,
                               g_cliConnInfoMap[peerName].udpComps[i].uReflexAddr.ip.c_str(),
                               g_cliConnInfoMap[peerName].udpComps[i].uReflexAddr.port,
                               g_cliConnInfoMap[peerName].udpComps[i].peerReflexAddr.ip.c_str(),
                               g_cliConnInfoMap[peerName].udpComps[i].peerReflexAddr.port
                        );
                        //cout<<g_cliConnInfoMap[peerName].udpComps[i].uHostAddr<<"-->"<<g_cliConnInfoMap[peerName].udpComps[i].peerReflexAddr<<endl;
                        //for test
                        pthread_create(&g_cliConnInfoMap[peerName].udpComps[i].heartBeatTid, NULL, sendUDPHeartBeat,
                                       &g_cliConnInfoMap[peerName].udpComps[i]);
                    }
                    sem_post(&g_sem);




//                    pthread_join(g_cliConnInfoMap[1].tid,&peerAddrPtr);
//                    g_cliConnInfoMap[1].peerReflexAddr=*(Addr*)peerAddrPtr;
//                    cout<<g_cliConnInfoMap[1].peerReflexAddr<<endl;
                }
#if 0
                else if(inVal["cmd"]=="traverse"){
                    //no use this,use the host ip
                    //string uAddr=inVal["uAddr"].asString();

//                    string uAddr="0.0.0.0";
//                    uAddr+=":"+to_string(g_hostPort);
//
//                    string peerAddr=inVal["peerAddr"].asString();
//                    if( udpTraverse(uAddr,peerAddr)==true ){
//                        g_peerAddr=peerAddr;
//                        sendRespondCmd(sockfd,"traverse","OK");
//
//                    }
//                    else{
//                        sendRespondCmd(sockfd,"traverse","FAIL");
//                    }

                    int sizeOfCheckList=inVal["cnt"].asInt();
//                    string uAddr="0.0.0.0";
//                    uAddr+=":";
//                    uAddr+=to_string(g_hostPort);
                    //**************************************************
                    //delete by gaoyubin
//                    pthread_t  recvUdpBufTid;
//                    pthread_create(&recvUdpBufTid,NULL,recvUdpBuf,&uAddr);
                    g_traverseAddrInfoVect.clear();
                    g_traverseAddrInfoVect.push_back(TrAddrInfo(Addr("0.0.0.0",g_hostPort),0,0));

                    for(int i=0;i<sizeOfCheckList-1;++i){
                        TraverseMethod traverseMethod=(TraverseMethod)inVal["checklist"][to_string(i)]["method"].asInt();
                        string peerAddr=inVal["checklist"][to_string(i)]["peerAddr"].asString();
                        if(traverseMethod==TraverseMethodCone)
                        {

                            uint16_t  peerPort;
                            char peerIP[32]={0};

                            sscanf(peerAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);
                            g_traverseAddrInfoVect.push_back(TrAddrInfo(Addr(peerIP,peerPort),0,0));

                        }
                        else if(traverseMethod==TraverseMethodSymRandom){
                            uint16_t  peerPort;
                            char peerIP[32]={0};

                            sscanf(peerAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);

                            g_traverseAddrInfoVect.push_back(TrAddrInfo(Addr(peerIP,peerPort),1000,0));
                        }


                    }

                    pthread_t  udpTraverseTid;
                    pthread_create(&udpTraverseTid,NULL,traverseUDP,(void*)&g_traverseAddrInfoVect);

                    pthread_join(udpTraverseTid,NULL);

                    if(g_isTraversed==true){
                        cout<<"traverse success"<<endl;
                    }
                    else{
                        cout<<"traverse fail"<<endl;

                    }

                }
#endif
                else if(inVal["cmd"]=="getPort"){
                    string peerName=inVal["peerName"].asString();
                    g_cliConnInfoMap[peerName].udpComps[0].uHostAddr.port=(rand()%2000+1024)&~1;
                    g_cliConnInfoMap[peerName].udpComps[1].uHostAddr.port=g_cliConnInfoMap[peerName].udpComps[0].uHostAddr.port+1;


                    StunAddress4 stunSvrAddr3478;
                    stunSvrAddr3478.port=3478;
                    stunSvrAddr3478.addr=ntohl(inet_addr(g_svrAddrInfo.stunSvrAddr.ip.c_str()));

                    vector<StunAddress4>peerAddr4Vect;
                    for(int i=0;i<inVal["comp"]["cnt"].asInt();++i){
                        char peerIP[32];
                        int peerPort;
                        StunAddress4 peerAddr4;
                        string peerAddr=inVal["comp"][to_string(i)].asString();
                        sscanf(peerAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);
                        cout<<"the peerAddr="<<peerAddr.c_str()<<endl;

                        peerAddr4.addr=ntohl(inet_addr(peerIP));
                        peerAddr4.port=peerPort;
                        peerAddr4Vect.push_back(peerAddr4);
                    }

                    if(getSymIncPort(stunSvrAddr3478, peerAddr4Vect, g_cliConnInfoMap[peerName].udpComps)==true){
                        Json::Value outVal;
                        outVal["cmd"]="respond";
                        outVal["answerCmd"]="detect";
                        outVal["peerName"]=peerName;
                        //todo
                        outVal["netmask"]=g_localNetInfo.netMask;
                        outVal["natType"]=g_localNetInfo.natType;

                        outVal["comp"]["cnt"]=(int)g_cliConnInfoMap[peerName].udpComps.size();
                        for(int i=0;i<outVal["comp"]["cnt"].asInt();++i){
//                             printf("%s:%d,step=%d,len=%d\n",
//                             g_cliConnInfoMap[i].uReflexAddr.ip.c_str(),
//                             g_cliConnInfoMap[i].uReflexAddr.port,
//                             g_cliConnInfoMap[i].step,
//                             g_cliConnInfoMap[i].len);

                             outVal["comp"][to_string(i)]["hostAddr"]=g_cliConnInfoMap[peerName].udpComps[i].uHostAddr.ip\
                                +":"+to_string(g_cliConnInfoMap[peerName].udpComps[i].uHostAddr.port);


                             outVal["comp"][to_string(i)]["reflexAddr"]=g_cliConnInfoMap[peerName].udpComps[i].uReflexAddr.ip\
                                +":"+to_string(g_cliConnInfoMap[peerName].udpComps[i].uReflexAddr.port);

                             outVal["comp"][to_string(i)]["len"]=g_cliConnInfoMap[peerName].udpComps[i].len;
                             outVal["comp"][to_string(i)]["step"]=g_cliConnInfoMap[peerName].udpComps[i].step;

                         }
                        string sendStr=outVal.toStyledString();

                        sendCodec(sockfd,sendStr);
                        cout<<"send"<<sendStr.c_str()<<endl;

                    }
                    else{
                        cout<<"getPort fail"<<endl;
                    }
                }
#if 0
                else if(inVal["cmd"]=="udpTurn"){
                    string id=inVal["id"].asString();
                    g_udpTurnSvrPort=inVal["port"].asInt();

                    if(udpTurn("0.0.0.0",g_hostPort,g_centreSvrIP,g_udpTurnSvrPort,id)==true){
                        g_peerAddr=g_centreSvrIP;
                        g_peerAddr+=":"+ to_string(g_udpTurnSvrPort);
                        cout<<"g_peerAddr"<<" "<<g_peerAddr<<endl;
                        sendRespondCmd(sockfd,"udpTurn","OK");
                        cout<<"turn ok"<<endl;
                        //string outStr="hello world";
                        //sendto(sockfd ,outStr.c_str() , outStr.size(), 0, (struct sockaddr *)&svrAddrIn, sizeof(svrAddrIn));
                    }else{
                        cout<<"turn fail"<<endl;
                    }

                }
                else if(inVal["cmd"]=="start"){
                    cout<<"start to live555"<<endl;
                    struct sockaddr_in peerAddrIn;
                    struct sockaddr_in hostAddrIn;
                    char peerIP[32]={0};
                    int peerPort=0;
                    sscanf(g_peerAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);
                    peerAddrIn.sin_family=AF_INET;
                    peerAddrIn.sin_addr.s_addr=inet_addr(peerIP);
                    peerAddrIn.sin_port=htons(peerPort);

                    hostAddrIn.sin_family = AF_INET;
                    hostAddrIn.sin_port = htons(g_hostPort);
                    hostAddrIn.sin_addr.s_addr = INADDR_ANY;

                    cout<<"|"<<g_hostPort<<endl;
                    cout<<peerIP<<"|"<<peerPort<<endl;

                    //建立与服务器通信的socket和与客户端通信的socket
                    int udpSockfd;
                    udpSockfd = socket(AF_INET,SOCK_DGRAM,0);
                    if (udpSockfd == -1) {
                        perror("socket() failed:");
                        return false;
                    }
                    /****************************/
                    int reuse = 1;
                    int ret = setsockopt(udpSockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
                    if (ret) {
                        exit(1);
                    }

                    ret = setsockopt(udpSockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
                    if (ret) {
                        exit(1);
                    }
                    /****************************/
                    if (bind(udpSockfd,(struct sockaddr*)&hostAddrIn,sizeof(hostAddrIn)) == -1) {
                        perror("bind() failed:");
                        return false;
                    }

                    string outStr="hello world gaoyubin";
                    sendto(udpSockfd ,outStr.c_str() , outStr.size(), 0, (struct sockaddr *)&peerAddrIn, sizeof(peerAddrIn));
                    close(udpSockfd);
                }
#endif

            }
        }





    }
    cout<<"break out the recvTcpBuf fn---------------------------"<<endl;

}



int getLocalnetMask(const char *eth_inf, size_t len,char *netMask)
{
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd)
    {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, eth_inf, len);
    ifr.ifr_name[len - 1] = 0;

    // if error: No such device
    if (ioctl(sd, SIOCGIFNETMASK, &ifr) < 0)
    {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));


    //int ipLen=sizeof(inet_ntoa(sin.sin_addr));
    //cout<<len<<endl;
    snprintf(netMask,16, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}

// 获取本机ip
int getLocalIp(const char *eth_inf, size_t len,char *ip)
{
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd)
    {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, eth_inf, len);
    ifr.ifr_name[len - 1] = 0;

    // if error: No such device
    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0)
    {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));


    //int ipLen=sizeof(inet_ntoa(sin.sin_addr));
    //cout<<len<<endl;
    snprintf(ip,16, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}

void* sendTCPHeartBeat(void*arg){
    int sockfd=*(int*)arg;
    while(1){
        sleep(3);
        Json::Value outVal;
        outVal["cmd"]="heartBeat";
        string outStr=outVal.toStyledString();
        //cout<<outStr<<endl;
        sendCodec(sockfd,outStr);

    }
}

bool initTraverse(){
    srand(getpid());
    printf("getpid()=%d\n",getpid());
    sem_init(&g_sem,0,0);
}
bool  onLogin(Account& account,Addr&centreSvrAddr,map<string,ClientConnInfo>* &cliMapPtr){
    //  char buffer[1024];
    struct sockaddr_in svrAddrIn;

    if ((g_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error:%s\a\n", strerror(errno));
        exit(1);
    }
    bzero(&svrAddrIn, sizeof(svrAddrIn));
    svrAddrIn.sin_family = AF_INET;
    svrAddrIn.sin_port = htons(centreSvrAddr.port);
    svrAddrIn.sin_addr.s_addr = inet_addr(centreSvrAddr.ip.c_str());
    memset(&svrAddrIn.sin_zero, 0, 8);

    if (connect(g_sockfd, (struct sockaddr *) (&svrAddrIn), sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Connect natTraverseClientMuduo error:%s\n", strerror(errno));
        return false;
        //exit(1);
    }
    pthread_t recvtid;
    pthread_create(&recvtid, NULL, recvTcpBuf, &g_sockfd);

    Json::Value value;
    value["cmd"]="login";
    value["c/s"]=account.role;
    value["uname"]=account.name;
    value["pwd"]=account.password;
    string outStr=value.toStyledString();
    cout<<"send"<<outStr.c_str()<<endl;

    sendCodec(g_sockfd,outStr);


    struct timespec timeout;

    timeout.tv_sec=3+time(NULL);
    timeout.tv_nsec=0;
    if(sem_timedwait(&g_sem,&timeout)==-1){
        perror("sem_timedwait");
        return false;
    }
    else{
        if(g_response=="OK"){

            pthread_t heartBeatTid;
            pthread_create(&heartBeatTid, NULL, sendTCPHeartBeat, &g_sockfd);
            g_account=account;
            if(g_account.role=="s"){
                cliMapPtr=&g_cliConnInfoMap;
            }
            return true;
        }
        else{
            return false;
        }

    }


}
bool onList(vector<string>&vs ){
    if(g_sockfd==-1){
        cout<<"you has no login"<<endl;
        //return vector<string>();
        return false;
    }

    Json::Value outVal;
    outVal["cmd"] ="list";
    string  outStr= outVal.toStyledString();
    cout<<"send"<<outStr.c_str()<<endl;
    sendCodec(g_sockfd, outStr);
    struct timespec timeout;
    timeout.tv_sec=3+time(NULL);
    timeout.tv_nsec=0;
    if(sem_timedwait(&g_sem,&timeout)==-1){
        //cout<<"dfsf"<<endl;
        perror("sem_timedwait!!!!");
        //return vector<string>();
        return false;
    }
    else{
        //return g_userList;
        vs=g_userList;
        return true;
    }



//    pthread_mutex_lock(&listMutex);
//    pthread_cond_wait(&listCond,&listMutex);
//    pthread_mutex_unlock(&listMutex);

    //return g_userList;

}
bool onSee(string peerName,string peerPwd){
    Json::Value outVal;
    outVal["uname"] = g_account.name ;
    outVal["cmd"] = "see";
    outVal["peerName"] = peerName;
    outVal["peerPwd"] =  peerPwd;
    string outStr = outVal.toStyledString();

    cout<<"send"<<outStr.c_str()<<endl;
    sendCodec(g_sockfd, outStr);
    struct timespec timeout;
    timeout.tv_sec=30+time(NULL);
    timeout.tv_nsec=0;
loop:

    if(sem_timedwait(&g_sem,&timeout)==-1){
        if(errno==EINTR){
            goto loop;
        }
        else{

            perror("sem_timedwati");
            return false;
        }
    }
    else{
        return true;
    }

}




#if 1
int main(int argc,char *argv[]) {


//pthread_key_create


//    if(argc<3){
//        printf("too less arg");
//        return 1;
//    }


//    string tmpStr="192.168.1.103:8080";
//    char tmpArr[320];
//    int tmp;
//    sscanf(tmpStr.c_str(),"%[^:]:%d",tmpArr,&tmp);
//    cout<<tmpArr<<endl;
//    cout<<tmp<<endl;

        //traverseUDP(argv[1],argv[2]);

//    StunAddress4 dest1;
//    dest1.addr=ntohl(inet_addr("218.192.170.178"));
//    //dest1.addr=ntohl(inet_addr("217.10.68.152"));
//    dest1.port=3478;natTraverseCentreMuduo  natTraverseClientMu
//    StunAddress4 sAddr;
//    //sAddr.port
//    char chTmp='a';
//    cout<<to_string(chTmp)<<endl;
//    cout<<chTmp<<endl;
//    EmNatType res= detectNatType(dest1, false, NULL);
//    cout<<getNatTypeStr(res)<<endl;


//    int a=1200;
//    string s="12";
//    s+=to_string(a);
//    cout<<s<<endl;

//    pthread_t  ptd;
//    pthread_create(&ptd, NULL, threadFun, NULL);
//    sleep(5);
//    pthread_cancel(ptd);

//    string uAddr="0.0.0.0:3344";
//    string peerAddr="192.168.1.103:8080";
//    pthread_t udpTraverseTid;
//    AddrPair udpTraverseArg(uAddr,peerAddr);
//    pthread_create(&udpTraverseTid,NULL,traverseUDP,(void*)&udpTraverseArg);
//    while (1);

//    int arr[8]={0};
//    arr[8]=12;
//    int *ptr=NULL;
//
//
//
//    *ptr=12;

//    for(int i=0;i<10;i++){
//        srand(1);
//        cout<<rand()<<endl;
//    }
//
//
//    while(1);
//    Addr addr1;
//    //Addr addr2();
//    if(addr1==Addr()){
//        //cout<<addr1.ip<<":"<<addr1.port<<endl;
//        cout<<"hello"<<endl;
//    }

//    srand(getpid());
//    StunAddress4 stunSvrAddr4,peerAddr4;
//    stunSvrAddr4.addr=ntohl(inet_addr("218.192.170.178"));
//    stunSvrAddr4.port=3478;
//
//    peerAddr4.addr=ntohl(inet_addr("218.192.170.178"));;
//    peerAddr4.port=3479;
//
//
//    Addr localAddr,startAddr,endAddr;
//    localAddr.port=3456;
//
//    if(getSymIncPort(stunSvrAddr4,peerAddr4,localAddr,startAddr,endAddr)==true){
//        cout<<startAddr.ip<<":"<<startAddr.port<<endl;
//        cout<<endAddr.ip<<":"<<endAddr.port<<endl;
//    }
//    else{
//        cout<<"get fail"<<endl;
//    }

//    srand(getpid());
//    StunAddress4 dest1;
//    //dest1.addr=ntohl(inet_addr("217.10.68.152"));
//    dest1.addr=ntohl(inet_addr("218.192.170.178"));
//    dest1.port=3478;
////
//    StunAddress4 sAddr;
//    sAddr.port=rand()%2000+22330;
//    cout<<"port: "<<sAddr.port<<endl;
//    int symIncDiff=0;
//    MyNatType natType=detectNatType(dest1,false,&sAddr,symIncDiff);
//    cout<<getNatTypeStr(natType)<<endl;

//    srand(getpid());
//    StunAddress4 stunSvrAddr3478,peerAddr4;
//    g_cliConnInfoMap[0].uHostAddr.port=rand()%2000+2233;
//    g_cliConnInfoMap[1].uHostAddr.port=g_cliConnInfoMap[0].uHostAddr.port+1;
//
//    stunSvrAddr3478.port=3478;
//    stunSvrAddr3478.addr=ntohl(inet_addr("218.192.170.178"));
//    char peerIP[32];
//    int peerPort;
//    string peerAddr="218.192.170.171:4455";
//    sscanf(peerAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);
//    cout<<"the peerAddr="<<peerAddr<<endl;
//
//    peerAddr4.addr=ntohl(inet_addr(peerIP));
//    peerAddr4.port=peerPort;
//    if(getSymIncPort(stunSvrAddr3478,peerAddr4,g_cliConnInfoMap)==true){
//        for(int i=0;i<g_cliConnInfoMap.size();++i){
//            printf("%s:%d,step=%d,len=%d\n",
//                   g_cliConnInfoMap[i].uReflexAddr.ip.c_str(),
//                   g_cliConnInfoMap[i].uReflexAddr.port,
//                   g_cliConnInfoMap[i].step,
//                   g_cliConnInfoMap[i].len);
//        }
//
//    }
//    else{
//        cout<<"getPort fail"<<endl;
//    }
//    while(1){
//        int i=1;
//        int sum;
//        sum+=i;
//        cout<<sum<<endl;
//
//    }

//    int cnt=0;
//    while(cnt<3){
//        cnt++;
//        vector<int>v;
//        cout<<v.size()<<endl;
//        for(int i=0;i<3;i++){
//            v.push_back(i);
//        }
//        cout<<v.size()<<endl;
//    }
//    //填充结构体
//    TrAddrInfo trAddrInfo;
//    trAddrInfo.len=2;
//    trAddrInfo.step=65535;
//    trAddrInfo.outStr="hello";
//
//    //转为string并打印
//    string str=(char*)&trAddrInfo;
//    cout<<str<<endl;
//
//    //将string赋值json的一个属性
//    Json::Value trAddrVal;
//    trAddrVal["AddrInfo"]=str;
//
//    //将trAddrVal转为json,并打印
//    string inStr=trAddrVal.toStyledString();
//    cout<<inStr<<endl;
//
//    //将json解析出
//    Json::Reader reader;
//    Json::Value inVal;
//    if(reader.parse(inStr,inVal)){
//        string trAddrStr=inVal["AddrInfo"].asString();
//        char* chPtr= const_cast<char*>(trAddrStr.c_str());
//        TrAddrInfo* trAddrInfoPtr=(TrAddrInfo*)chPtr;
//        cout<<trAddrInfo.outStr<<endl;
//        cout<<trAddrInfo.len<<endl;
//        cout<<trAddrInfo.step<<endl;
//
//    }
//
//    return 1;

//    //string a=string(localIP) +":"+1;
//    string a="sdf"+1;
//    cout<<a<<endl;
        // to_string(UDP_TURN_PORT)


//    vector<int>v(3);
//    int a=v[1000];
//    cout<<a<<endl;
//    v[0]=0;
//    v[5]=5;
//    v[2]=2;
//    for(int i=0;i<v.size();++i){
//        cout<<v[i]<<endl;
//    }
//    return 0;
//    Addr peer1Addr;
//    peer1Addr.ip="0.0.0.0";
//    peer1Addr.port=8888;
//    pthread_t  recvUdpBufTid;
//    pthread_create(&recvUdpBufTid,NULL,recvUdpBuf,&peer1Addr);



//    for(int i=0;i<10;i++){
//        pthread_t  recvUdpBufTid;
//        Addr* peerAddr=new Addr;
//        peerAddr->ip="192.168.0.8";
//        peerAddr->port=5555+i;
//        pthread_create(&recvUdpBufTid,NULL,recvUdpBuf,peerAddr);
//        pthread_mutex_lock(&recvMutex);
//        while(*peerAddr!=Addr()){
//            pthread_cond_wait(&recvCond,&recvMutex);
//        }
//        pthread_mutex_unlock(&recvMutex);
//    }

//    Component comp;
//    comp.uHostAddr.port=5555;
//    comp.peerReflexAddr.port=8000;
//    pthread_t  sendHeartBeatTid;
//    pthread_create(&sendHeartBeatTid,NULL,sendUDPHeartBeat,&comp);


//    Addr peerAddr;
//    struct sockaddr_in uAddrIn;
//    int recvfd;
//
//    uAddrIn.sin_family = AF_INET;
//    uAddrIn.sin_port = htons(7777);
//    uAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
//
//    //建立与服务器通信的socket和与客户端通信的socket
//    recvfd = socket(AF_INET,SOCK_DGRAM,0);
//    if (recvfd == -1) {
//        perror("socket() failed:");
//        return NULL;
//    }
//
//    if (bind(recvfd,(struct sockaddr*)&uAddrIn,sizeof(uAddrIn)) == -1) {
//        perror("traverse udp bind() failed:");
//        return NULL;
//    }
//    if (recvfd == -1) {
//        perror("socket() failed:");
//        //return -1;
//    }
//
//    for(int i=0;i<10;i++){
//        pthread_t  recvUdpBufTid;
//        pthread_create(&recvUdpBufTid,NULL,recvUdpBuf,&recvfd);
//
//    }

//    srand(getpid());
//    TCPComponent tcpComp;
//    tcpComp.turnID=to_string(2222);
//    tcpComp.turnSvrAddr.port=8888;
//    //tcpComp.turnSvrAddr.ip="192.168.1.101";
//    tcpComp.turnCliAddr.port=rand()%4000+3000;
//    tcpComp.liveCliAddr.port=rand()%2000+1000;
//    tcpComp.liveSvrAddr.port=8554;
//
//    pthread_t tid;
//    pthread_create(&tid,NULL,turnTCP,&tcpComp);

//    string testAddr="192.168.1.101:9898";
//    char testIP[32];
//    int testPort;
//
//
//    sscanf(testAddr.c_str(),"%[^:]:%d",testIP,&testPort);
//    cout<<testIP<<endl;
//    cout<<testPort<<endl;





    //pthread_cond_wait(&listCond,NULL);


//    string testStr("hello");
//    cout<<testStr.size()<<endl;
//    const char* chPtr=testStr.c_str();
//    int a=1;
//
//    while(1);
//    /**************************************************************************/

    //初始化随机种子
    initTraverse();
    Account account;
    Addr centreSvrAddr;

    if(argc>2){
        centreSvrAddr.ip=argv[1];
        centreSvrAddr.port=atoi(argv[2]);
    }
    if(argc<4){
        printf("Please input your uname:\n");
        cin >> g_account.name;
    }
    else{
        account.name=argv[3];
        if(account.name=="w")
            account.role="s";
        else
            account.role="c";

    }
    map<string,ClientConnInfo>* liveSvrCliConnMapPtr;
    if(onLogin(account,centreSvrAddr,liveSvrCliConnMapPtr)==true){
        cout<<"login success"<<endl;
        cout<<g_account<<endl;
    }
    else
        cout<<"login fail"<<endl;

    if(argc>4){
        onSee(argv[4]);
    }

    while (1) {
        string outStr;
        printf("Please input your cmd:\n");
        string input;
        cin >> input;

        if (input == "see") {
            printf("Please input your peer:\n");
            string peerName;
            cin >> peerName;
            Json::Value outVal;
            outVal["uname"] = g_account.name ;
            outVal["cmd"] = "see";
            outVal["peerName"] = peerName;
            outVal["peerPwd"] = "123456";
            outStr = outVal.toStyledString();
        }
        else if(input=="list") {
            Json::Value outVal;
            outVal["cmd"] ="list";
            outStr = outVal.toStyledString();

        }
        else {
            outStr=input;
            cout << "this cmd is invalid" << endl;
            continue;
        }

        cout << "send " << outStr.c_str() << endl;
        sendCodec(g_sockfd, outStr);

    }
}

#endif