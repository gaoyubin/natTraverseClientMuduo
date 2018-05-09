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

#include "udpTraverse/udpTraverse.hpp"
#include "udpTurn/udpTurn.hpp"
#include "stun/MyStun.hpp"
#include "heartBeat/heartBeat.hpp"

using namespace std;

#define MAX_IFS 32

string uname;
char g_centreSvrIP[30] = "218.192.170.178";
int  g_centreSvrPort=2007;
Addr g_stunSvrAddr;
//NatType g_natType;
//string g_netmask;
class SvrAddrInfo{
    Addr stunSvrAddr;
    Addr udpTurnSvrAddr;
    Addr tcpTurnSvrAddr;
    Addr centreSvrAddr;

};
class LocalNetInfo{
public:
    MyNatType natType;
    string netMask;
    string hostIP;
};

map<string,ClientConnInfo>cliConnInfoMap;
LocalNetInfo g_localNetInfo;

//int g_hostPort;       // in detect
//string g_reflexAddr;  // in detect can get
//string g_peerAddr;    // in traverse or turn

vector<Component> g_compVect(2);

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

        printf("recv data of my world is :%s\n",inBuf);
        string inStr(inBuf);
        Json::Reader reader;
        Json::Value inVal;
        if(reader.parse(inStr,inVal)){
            if(!inVal["cmd"].isNull()){
                if(inVal["cmd"]=="respond"){
                    cout<<"respond"<<inVal["answerCmd"]<<":"<<inVal["state"]<<endl;
                    if(inVal["answerCmd"]=="list"){
                        string userList=inVal["extraInfo"].asString();
                        istringstream  iss(userList);
                        string s;
                        while(iss>>s){
                            cout<<s<<endl;
                        }

                    }
                }
                else if(inVal["cmd"]=="detect"){

                    char hostIP[32];
                    char netMask[32];
                    char netInterface[32];


                    getIpInterface(hostIP,netMask,netInterface);

                    cout<<netMask<<endl;
                    cout<<hostIP<<endl;
                    cout<<netInterface<<endl;


                    g_compVect[0].uHostAddr.port=rand()%2000+1024;
                    g_compVect[0].uHostAddr.ip=hostIP;
                    g_compVect[1].uHostAddr.port=g_compVect[0].uHostAddr.port+1;
                    g_compVect[1].uHostAddr.ip=hostIP;
                    StunAddress4 dest1;
                    if(!inVal["stunSvrIP"].isNull()){
                        g_stunSvrAddr.ip=inVal["stunSvrIP"].asString();
                    }
                    else{
                        g_stunSvrAddr.ip="217.10.68.152";
                    }
                    dest1.addr=ntohl(inet_addr(g_stunSvrAddr.ip.c_str()));
                    dest1.port=3478;
                    StunAddress4 sAddr;
                    sAddr.port=g_compVect[0].uHostAddr.port;

                    //int symIncDiff=0;
                    MyNatType natType=detectNatType(dest1,false,g_compVect);

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
                    outVal["uname"]=uname;
                    outVal["natType"]=natType;
                    outVal["netMask"]=netMask;
                    //for test
                    outVal["comp"]["cnt"]=2;

                    outVal["comp"]["0"]["hostAddr"]=g_compVect[0].uHostAddr.ip\
                    +":"+to_string(g_compVect[0].uHostAddr.port);


                    outVal["comp"]["0"]["reflexAddr"]=g_compVect[0].uReflexAddr.ip\
                    +":"+to_string(g_compVect[0].uReflexAddr.port);


                    outVal["comp"]["1"]["hostAddr"]=g_compVect[1].uHostAddr.ip\
                    +":"+to_string(g_compVect[1].uHostAddr.port);

                    outVal["comp"]["1"]["reflexAddr"]=g_compVect[1].uReflexAddr.ip\
                    +":"+to_string(g_compVect[1].uReflexAddr.port);

                    string sendStr=outVal.toStyledString();

                    sendCodec(sockfd,sendStr);
                    cout<<"send"<<sendStr.c_str()<<endl;

                }
                else if(inVal["cmd"]=="traverse"){
                    uint16_t  peerPort;
                    char peerIP[32]={0};
                    string peerAddr;
                    //for test
                    for(int i=0;i<inVal["comp"]["cnt"].asInt()-1;++i){
                        int symIncIndex=-1;
                        g_compVect[i].trAddrInfoVect.clear();
                        g_compVect[i].trAddrInfoVect.push_back(TrAddrInfo(Addr(g_compVect[i].uHostAddr.ip,g_compVect[i].uHostAddr.port),0,0));
                        if(inVal["comp"][to_string(i)]["checklist"]["index"].isNull()){
                            symIncIndex=inVal["comp"][to_string(i)]["checklist"]["index"].asInt();
                        }
                        int checkListCnt=inVal["comp"][to_string(i)]["checklist"]["cnt"].asInt();
                        for(int j=0;j<checkListCnt;++j){
                            int step=0,len=0;
                            string id="traverse";


                            memset(peerIP,0,sizeof(peerIP));
                            if(j==checkListCnt-1){
                                id=inVal["comp"][to_string(i)]["checklist"]["id"].asString();
                            }

                            peerAddr=inVal["comp"][to_string(i)]["checklist"][to_string(j)].asString();
                            sscanf(peerAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);
                            if(j==symIncIndex) {
                                step = inVal["comp"][to_string(i)]["checklist"]["step"].asInt();
                                len = inVal["comp"][to_string(i)]["checklist"]["len"].asInt();
                                //peerPort-=step;
                            }
                            g_compVect[i].trAddrInfoVect.push_back(TrAddrInfo(Addr(peerIP,peerPort),step,len,id));
                        }

                        pthread_create(&g_compVect[i].traverseTid,NULL,udpTraverse,(void*)&g_compVect[i].trAddrInfoVect);


                    }
                    void*peerAddrPtr;
                    pthread_join(g_compVect[0].traverseTid,&peerAddrPtr);

                    g_compVect[0].peerReflexAddr=*(Addr*)peerAddrPtr;
                    cout<<g_compVect[0].uHostAddr<<"-->"<<g_compVect[0].peerReflexAddr<<endl;

                    pthread_create(&g_compVect[0].heartBeatTid,NULL,sendHeartBeat,&g_compVect[0]);
//                    pthread_join(g_compVect[1].tid,&peerAddrPtr);
//                    g_compVect[1].peerReflexAddr=*(Addr*)peerAddrPtr;
//                    cout<<g_compVect[1].peerReflexAddr<<endl;
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
                    pthread_create(&udpTraverseTid,NULL,udpTraverse,(void*)&g_traverseAddrInfoVect);

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

                    g_compVect[0].uHostAddr.port=rand()%2000+1024;
                    g_compVect[1].uHostAddr.port=g_compVect[0].uHostAddr.port+1;


                    StunAddress4 stunSvrAddr3478;
                    stunSvrAddr3478.port=3478;
                    stunSvrAddr3478.addr=ntohl(inet_addr(g_stunSvrAddr.ip.c_str()));

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

                    if(getSymIncPort(stunSvrAddr3478, peerAddr4Vect, g_compVect)==true){
                        Json::Value outVal;
                        outVal["cmd"]="respond";
                        outVal["answerCmd"]="detect";

                        //todo
                        outVal["netmask"]=g_localNetInfo.netMask;
                        outVal["natType"]=g_localNetInfo.natType;

                        outVal["comp"]["cnt"]=(int)g_compVect.size();
                        for(int i=0;i<g_compVect.size();++i){
//                             printf("%s:%d,step=%d,len=%d\n",
//                             g_compVect[i].uReflexAddr.ip.c_str(),
//                             g_compVect[i].uReflexAddr.port,
//                             g_compVect[i].step,
//                             g_compVect[i].len);

                             outVal["comp"][to_string(i)]["hostAddr"]=g_compVect[i].uHostAddr.ip\
                                +":"+to_string(g_compVect[i].uHostAddr.port);


                             outVal["comp"][to_string(i)]["reflexAddr"]=g_compVect[i].uReflexAddr.ip\
                                +":"+to_string(g_compVect[i].uReflexAddr.port);

                             outVal["comp"][to_string(i)]["len"]=g_compVect[i].len;
                             outVal["comp"][to_string(i)]["step"]=g_compVect[i].step;

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


int  onLogin(int sockfd,string& uname,string pwd="123456",string cs="s"){
    Json::Value value;
    value["cmd"]="login";
    value["c/s"]=cs;
    value["uname"]=uname;
    value["pwd"]=pwd;
    string outStr=value.toStyledString();
    cout<<"send"<<outStr.c_str()<<endl;


    //cout<<len<<endl;
    sendCodec(sockfd,outStr);
}
int onSee(){

}


//void* threadFun(void *agr){
//    while(1){
//        sleep(1);
//        cout<<"threadFun"<<endl;
//    }
//}


pthread_mutex_t recvMutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  recvCond=PTHREAD_COND_INITIALIZER;
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

    //udpTraverse(argv[1],argv[2]);

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
//    pthread_create(&udpTraverseTid,NULL,udpTraverse,(void*)&udpTraverseArg);
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
//    g_compVect[0].uHostAddr.port=rand()%2000+2233;
//    g_compVect[1].uHostAddr.port=g_compVect[0].uHostAddr.port+1;
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
//    if(getSymIncPort(stunSvrAddr3478,peerAddr4,g_compVect)==true){
//        for(int i=0;i<g_compVect.size();++i){
//            printf("%s:%d,step=%d,len=%d\n",
//                   g_compVect[i].uReflexAddr.ip.c_str(),
//                   g_compVect[i].uReflexAddr.port,
//                   g_compVect[i].step,
//                   g_compVect[i].len);
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
//    pthread_create(&sendHeartBeatTid,NULL,sendHeartBeat,&comp);


    Addr peerAddr;
    struct sockaddr_in uAddrIn;
    int recvfd;

    uAddrIn.sin_family = AF_INET;
    uAddrIn.sin_port = htons(7777);
    uAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);

    //建立与服务器通信的socket和与客户端通信的socket
    recvfd = socket(AF_INET,SOCK_DGRAM,0);
    if (recvfd == -1) {
        perror("socket() failed:");
        return NULL;
    }

    if (bind(recvfd,(struct sockaddr*)&uAddrIn,sizeof(uAddrIn)) == -1) {
        perror("traverse udp bind() failed:");
        return NULL;
    }
    if (recvfd == -1) {
        perror("socket() failed:");
        //return -1;
    }

    for(int i=0;i<10;i++){
        pthread_t  recvUdpBufTid;
        pthread_create(&recvUdpBufTid,NULL,recvUdpBuf,&recvfd);

    }

    while(1);
//    /**************************************************************************/

    //初始化随机种子
    srand(getpid());

    int sockfd;
    char sendbuffer[200];
    char recvbuffer[200];
    //  char buffer[1024];
    struct sockaddr_in svrAddrIn;


    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error:%s\a\n", strerror(errno));
        exit(1);
    }

    bzero(&svrAddrIn, sizeof(svrAddrIn));
    svrAddrIn.sin_family = AF_INET;
    svrAddrIn.sin_port = htons(g_centreSvrPort);
    svrAddrIn.sin_addr.s_addr = inet_addr(g_centreSvrIP);
    memset(&svrAddrIn.sin_zero, 0, 8);

    if(argc>2){
        svrAddrIn.sin_port = htons(atoi(argv[2]));
        svrAddrIn.sin_addr.s_addr = inet_addr(argv[1]);
    }
    if (connect(sockfd, (struct sockaddr *) (&svrAddrIn), sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "ConnatTraverseClientMuduonect error:%s\n", strerror(errno));
        exit(1);
    }
    pthread_t recvPtd;
    pthread_create(&recvPtd, NULL, recvTcpBuf, &sockfd);

    if(argc<4){
        printf("Please input your uname:\n");
        cin >> uname;
    }
    else{
        uname=argv[3];
    }

    string outStr;


    onLogin(sockfd, uname);
    if(argc>4){
        Json::Value outVal;
        outVal["uname"] = uname;
        outVal["cmd"] = "see";
        outVal["peerName"] = argv[4];
        outVal["peerPwd"] = "123456";
        outStr = outVal.toStyledString();
        cout << "send " << outStr.c_str() << endl;
        sendCodec(sockfd, outStr);
    }

    while (1) {
        printf("Please input your cmd:\n");
        string input;
        cin >> input;


        if (input == "see") {
            printf("Please input your peer:\n");
            string peerName;
            cin >> peerName;
            Json::Value outVal;
            outVal["uname"] = uname;
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
        sendCodec(sockfd, outStr);

    }
}