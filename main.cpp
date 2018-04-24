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
#include "stun/stun.h"
#include "udpTraverse/udpTraverse.hpp"
#include "udpTurn/udpTurn.hpp"

using namespace std;

#define MAX_IFS 32

string uname;
//char g_svrIP[30] = "218.192.170.178";
char g_svrIP[30] = "192.168.1.101";
int g_hostPort;       // in detect
string g_reflexAddr;  // in detect can get
string g_peerAddr;    // in traverse or turn
int g_udpTurnSvrPort; // in turn
int g_tcpTurnSvrPort = 2007;

int getIpInterface(char *localIp,char *netMask,char*netInterface){
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];
    int SockFD;

    SockFD = socket(AF_INET, SOCK_DGRAM, 0);
    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if (ioctl(SockFD, SIOCGIFCONF, &ifc) < 0) {
        printf("ioctl(SIOCGIFCONF): %m\n");
        return 0;

    }

    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++) {
        string tmp(inet_ntoa( ( (struct sockaddr_in *)&ifr->ifr_addr)->sin_addr));
        //cout<<tmp.size()<<endl;
        //cout<<tmp.c_str()<<endl;
        //cout<<sizeof(tmp.c_str())<<endl;
        if(localIp!=NULL)
            snprintf(localIp,tmp.size()+1, "%s", tmp.c_str());
        //localIp=inet_ntoa( ( (struct sockaddr_in *)  &ifr->ifr_addr)->sin_addr);
        if (ifr->ifr_addr.sa_family == AF_INET && strcmp(localIp,"127.0.0.1")!=0 ) {

            strncpy(ifreq.ifr_name, ifr->ifr_name,sizeof(ifreq.ifr_name));
            if(netInterface!=NULL)
                strncpy(netInterface, ifr->ifr_name,sizeof(ifreq.ifr_name));
            if (ioctl (SockFD, SIOCGIFNETMASK, &ifreq) < 0) {
                printf("SIOCGIFHWADDR(%s): %m\n", ifreq.ifr_name);
                return 0;
            }
            tmp=(inet_ntoa( ( (struct sockaddr_in*)&ifreq.ifr_ifru)->sin_addr));
            //cout<<tmp.size()<<endl;
            if(netMask!=NULL)
                snprintf(netMask,tmp.size()+1, "%s",tmp.c_str());
            break;


        }

    }
    return 0;


}
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

extern StunAddress4 g_reflexStunAddr4;

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
                }
                else if(inVal["cmd"]=="detect"){

                    char hostIP[32];
                    char netMask[32];
                    char netInterface[32];

                    getIpInterface(hostIP,netMask,netInterface);
                    cout<<netMask<<endl;
                    cout<<hostIP<<endl;
                    cout<<netInterface<<endl;

                    srand(getpid());
                    g_hostPort=rand()%2000+1024;
                    char* myArgv[]={"client","stun.ekiga.net","-p",""};
                    //if(inVal.isNull())
                    char natSvrIP[20];
                    if(!inVal["stunSvrIP"].isNull()){

                        sprintf(natSvrIP,"%s",inVal["stunSvrIP"].asString().c_str());
                        myArgv[1]=natSvrIP;
                    }

                    char portStr[10];
                    sprintf(portStr,"%d",g_hostPort);
                    myArgv[3]=portStr;
                    NatType natType=(NatType)stunDetect(4,myArgv);
                    string hostAddr;

                    hostAddr=hostIP;
                    hostAddr+=":"+to_string(g_hostPort);
                    cout<<hostAddr<<endl;


                    int tmpInt=ntohl(g_reflexStunAddr4.addr);
                    g_reflexAddr=inet_ntoa(*  (struct in_addr *)&tmpInt);

                    g_reflexAddr+=":"+to_string(g_reflexStunAddr4.port);//放在string里面，不用考虑字节序问题

                    cout<<g_reflexStunAddr4.port<<endl;


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

                    string uAddr="0.0.0.0";
                    uAddr+=":"+to_string(g_hostPort);

                    string peerAddr=inVal["peerAddr"].asString();
                    if(udpTraverse(uAddr,peerAddr)==true){
                        g_peerAddr=peerAddr;
                        sendRespondCmd(sockfd,"traverse","OK");

                    }
                    else{
                        sendRespondCmd(sockfd,"traverse","FAIL");
                    }

                }
                else if(inVal["cmd"]=="udpTurn"){
                    string id=inVal["id"].asString();
                    g_udpTurnSvrPort=inVal["port"].asInt();

                    if(udpTurn("0.0.0.0",g_hostPort,g_svrIP,g_udpTurnSvrPort,id)==true){
                        g_peerAddr=g_svrIP;
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
    /**************************************************************************/

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
    svrAddrIn.sin_port = htons(g_tcpTurnSvrPort);
    svrAddrIn.sin_addr.s_addr = inet_addr(g_svrIP);
    memset(&svrAddrIn.sin_zero, 0, 8);
    if (connect(sockfd, (struct sockaddr *) (&svrAddrIn), sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Connect error:%s\n", strerror(errno));
        exit(1);
    }
    pthread_t recvPtd;
    pthread_create(&recvPtd, NULL, recvTcpBuf, &sockfd);


    printf("Please input your uname:\n");
    string outStr;
    cin >> uname;

    onLogin(sockfd, uname);

    while (1) {
        printf("Please input your cmd:\n");
        string input;
        cin >> input;


        if (input == "see") {
            printf("Please input your peer:\n");
            string peerName;
            cin >> peerName;
            Json::Value value;
            value["uname"] = uname;
            value["cmd"] = "see";
            value["peerName"] = peerName;
            value["peerPwd"] = "123456";
            outStr = value.toStyledString();
        } else {
            outStr=input;
            cout << "this cmd is invalid" << endl;
        }

        cout << "send " << outStr.c_str() << endl;
        sendCodec(sockfd, outStr);

    }
}