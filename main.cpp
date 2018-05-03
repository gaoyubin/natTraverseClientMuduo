#if 0
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>
#include "json/json.h"

using namespace muduo;
using namespace muduo::net;

class ChargenClient : boost::noncopyable
{
 public:
  ChargenClient(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      client_(loop, listenAddr, "ChargenClient")
  {
    client_.setConnectionCallback(
        boost::bind(&ChargenClient::onConnection, this, _1));
    client_.setMessageCallback(
        boost::bind(&ChargenClient::onMessage, this, _1, _2, _3));
    // client_.enableRetry();
  }

  void connect()
  {
    client_.connect();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    if (!conn->connected())
      loop_->quit();
      Json::Value value;
      value['login']=1234456;
      value['c/s']="s";
      string out=value.toStyledString();
    conn->send(out);
      std::cout<<out<<std::endl;
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
  {
    buf->retrieveAll();
  }

  EventLoop* loop_;
  TcpClient client_;
};

int main(int argc, char* argv[])
{
     LOG_INFO << "pid = " << getpid();

    EventLoop loop;
    //InetAddress serverAddr(argv[1], 2019);
    InetAddress serverAddr("192.168.1.103", 2007);
    ChargenClient chargenClient(&loop, serverAddr);
    chargenClient.connect();
    loop.loop();

}
#endif

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

using namespace std;

#define MAX_IFS 32

string uname;
//char g_centreSvrIP[30] = "218.192.170.178";
char g_centreSvrIP[30] = "192.168.1.101";
int  g_centreSvrPort=2007;
int g_hostPort;       // in detect
string g_reflexAddr;  // in detect can get
string g_peerAddr;    // in traverse or turn

vector<AddrPair> g_trAddrPairVect;

int g_udpTurnSvrPort; // in turn

extern StunAddress4 g_reflexStunAddr4h;
/*********************/
//vector<AddrPair> addrPairVect;
//AddrTriple g_addrTriple;
//StunAddress4

//vector<tuple<StunAddress4,int,int>>g_addrTupleVect;

vector<TrAddrInfo>g_traverseAddrInfoVect;
typedef enum{
    TraverseMethodCone,
    TraverseMethodOpen,
    TraverseMethodSymInc,
    TraverseMethodSymRandom,
    TraverseMethodTurn,

}TraverseMethod;

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


                    g_hostPort=rand()%2000+1024;

                    StunAddress4 dest1;
                    if(!inVal["stunSvrIP"].isNull()){
                        dest1.addr=ntohl(inet_addr(inVal["stunSvrIP"].asString().c_str()));
                    }
                    else{
                        dest1.addr=ntohl(inet_addr("217.10.68.152"));
                    }
                    dest1.port=3478;
                    StunAddress4 sAddr;
                    sAddr.port=g_hostPort;

                    MyNatType natType=detectNatType(dest1,false,&sAddr);
                    cout<<getNatTypeStr(natType)<<endl;

                    string hostAddr;

                    hostAddr=hostIP;
                    hostAddr+=":"+to_string(g_hostPort);
                    cout<<hostAddr<<endl;


                    int tmpInt=ntohl(g_reflexStunAddr4h.addr);
                    g_reflexAddr=inet_ntoa(*  (struct in_addr *)&tmpInt);

                    g_reflexAddr+=":"+to_string(g_reflexStunAddr4h.port);//放在string里面，不用考虑字节序问题

                    cout<<g_reflexStunAddr4h.port<<endl;


                    Json::Value outVal;
                    outVal["cmd"]="respond";
                    outVal["answerCmd"]="detect";
                    outVal["uname"]=uname;
                    outVal["natType"]=natType;
                    outVal["hostAddr"]=hostAddr;
                    outVal["netMask"]=netMask;
                    outVal["reflexAddr"]=g_reflexAddr;


                    string sendStr=outVal.toStyledString();
                    sendCodec(sockfd,sendStr);
                    cout<<"send"<<sendStr.c_str()<<endl;

                }
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
                else if(inVal["cmd"]=="getPort"){

                }
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

int main(int argc,char *argv[]) {



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
//    if(getSymIncRange(stunSvrAddr4,peerAddr4,localAddr,startAddr,endAddr)==true){
//        cout<<startAddr.ip<<":"<<startAddr.port<<endl;
//        cout<<endAddr.ip<<":"<<endAddr.port<<endl;
//    }
//    else{
//        cout<<"get fail"<<endl;
//    }

    srand(getpid());
    StunAddress4 dest1;
    //dest1.addr=ntohl(inet_addr("217.10.68.152"));
    dest1.addr=ntohl(inet_addr("218.192.170.178"));
    dest1.port=3478;
//
    StunAddress4 sAddr;
    sAddr.port=rand()%2000+22330;
    cout<<"port: "<<sAddr.port<<endl;
    MyNatType natType=detectNatType(dest1,false,&sAddr);
    cout<<getNatTypeStr(natType)<<endl;
    /**************************************************************************/

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