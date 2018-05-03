//
// Created by tl on 18-4-25.
//
#include <unistd.h>
#include <assert.h>
#include <cstring>
#include <arpa/inet.h>
#include "MyStun.hpp"
#include "stun.h"
#include "udp.h"
#include "../udpTraverse/udpTraverse.hpp"

#include <iostream>
#include <vector>
#include <sys/ioctl.h>
#include <net/if.h>
#include <algorithm>

using namespace std;

//using namespace MyStun;


//add by gaoyubin
StunAddress4 g_reflexStunAddr4h;
string  getNatTypeStr(MyNatType natType){
    switch (natType){
        case NatTypeBlocked:
            return NatType_To_Str(NatTypeBlocked);
        case NatTypeCone:
            return NatType_To_Str(NatTypeCone);
        case NatTypeFail:
            return NatType_To_Str(NatTypeFail);
        case NatTypeOpen:
            return NatType_To_Str(NatTypeOpen);
        case NatTypeSymInc:
            return NatType_To_Str(NatTypeSymInc);
        case NatTypeSymOneToOne:
            return NatType_To_Str(NatTypeSymOneToOne);
        case NatTypeSymRandom:
            return NatType_To_Str(NatTypeSymRandom);
        case NatTypeUnknown:
            return NatType_To_Str(NatTypeUnknown);
    }
}
void
stunSendTest( Socket myFd, StunAddress4& dest,
              const StunAtrString& username, const StunAtrString& password,
              int testNum, bool verbose )
{
    assert( dest.addr != 0 );
    assert( dest.port != 0 );

    bool changePort=false;
    bool changeIP=false;
    bool discard=false;

    switch (testNum)
    {
        case 1:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            break;
        case 2:
            //changePort=true;

            //add by gaoyubin
            changePort=true;
            changeIP=true;
            break;
        case 3:
            changePort=true;
            break;
        case 4:
            changeIP=true;
            break;
        case 5:
            discard=true;
            break;
        default:
            cerr << "Test " << testNum <<" is unkown\n";
            assert(0);
    }

    StunMessage req;
    memset(&req, 0, sizeof(StunMessage));

    stunBuildReqSimple( &req, username,
                        changePort , changeIP ,
                        testNum );

    char buf[STUN_MAX_MESSAGE_SIZE];
    int len = STUN_MAX_MESSAGE_SIZE;

    len = stunEncodeMessage( req, buf, len, password,verbose );

    if ( verbose )
    {
        clog << "About to send msg of len " << len << " to " << dest << endl;
    }

    sendMessage( myFd, buf, len, dest.addr, dest.port, verbose );

    // add some delay so the packets don't get sent too quickly
#ifdef WIN32 // !cj! TODO - should fix this up in windows
    clock_t now = clock();
		 assert( CLOCKS_PER_SEC == 1000 );
		 while ( clock() <= now+10 ) { };
#else
    usleep(10*1000);
#endif

}


bool operator==(StunAddress4& a,StunAddress4& b){
    if(a.addr==b.addr && a.port==b.port)
       return true;
    else
       return false;

}
bool operator<(StunAddress4& a,StunAddress4& b){
    if(a.addr<b.addr)
        return true;
    else if(a.addr==b.addr){
        if(a.port<b.port)
            return true;
        else
            return false;
    }
    else
        return false;

}
#define MAX_IFS 32
int getIpInterface(char *localIp,char *netMask,char*netInterface)
{
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];
    int SockFD;

    SockFD = socket(AF_INET, SOCK_DGRAM, 0);
    if(SockFD==-1){
        perror("socket:");
    }
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
bool checkIsMultiple(int a,int b){
    int x=min(a,b);
    int y=max(a,b);
    if(x==y || 2*x==y || 3*x==y || 4*x==y)
        return true;
    else
        return false;

}
bool checkIsSymInc(vector<StunAddress4>&stunAddrVect,int &symIncDiff){
    if(stunAddrVect.size()<2){
        return false;
    }
    symIncDiff=65535;
    int preDiff=stunAddrVect[1].port-stunAddrVect[0].port;
    for(int i=1;i<stunAddrVect.size();++i){
        int nowDiff=stunAddrVect[i].port-stunAddrVect[i-1].port;
        if(nowDiff>0 && checkIsMultiple(nowDiff,preDiff)){
            cout<<nowDiff<<endl;
            preDiff=nowDiff;
            symIncDiff=min(symIncDiff,nowDiff);

        }
        else{
            return false;
        }
    }
    return true;
}






MyNatType
detectNatType(StunAddress4 &dest1,
              bool verbose,
              StunAddress4 *sAddr // NIC to use
)
{
    assert( dest1.addr != 0 );
    assert( dest1.port != 0 );

    StunAddress4 dest2=dest1;
    dest2.port+=1;

    UInt32 interfaceIp=0;
    UInt16 port=22331;
    if (sAddr)
    {
        interfaceIp = sAddr->addr;
        port=sAddr->port;

    }
    Socket myFd1 = openPort(port++,interfaceIp,verbose);
    Socket myFd2 = openPort(port++,interfaceIp,verbose);

//    Socket myFd3 = openPort(port++,interfaceIp,verbose);
//    Socket myFd4 = openPort(port++,interfaceIp,verbose);


    if ( ( myFd1 == INVALID_SOCKET) || ( myFd2 == INVALID_SOCKET)  )
    {
        cerr << "Some problem opening port/interface to send on" << endl;
        return NatTypeFail;
    }

    assert( myFd1 != INVALID_SOCKET );
    assert( myFd2 != INVALID_SOCKET );




    StunAddress4 test1sourceAddr;
    StunAddress4 test1mappedAddr;

    vector<StunAddress4>stunAddr4Vect(4);
    int sa4VectOffset=7;
    MyNatType natType=NatTypeUnknown;




    StunAtrString username;
    StunAtrString password;

    username.sizeValue = 0;
    password.sizeValue = 0;

    int count=0;

    int validRespCnt=0;
    while ( count <5 || validRespCnt<4)
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
        tv.tv_sec=0;
        tv.tv_usec=150*1000; // 150 ms
        if ( count == 0 ) tv.tv_usec=0;

        int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
        int e = getErrno();
        if ( err == SOCKET_ERROR )
        {
            // error occured
            cerr << "Error " << e << " " << strerror(e) << " in select" << endl;
            natType= NatTypeFail;
            break;
        }
        else if ( err == 0 )
        {
            // timeout occured
            count++;

//            if(bRespNatTest){
//                stunSendTest( myFd1, dest1, username, password, 1 ,verbose );
//            }
//            if(bRespConeTest)
//                stunSendTest( myFd1, dest2, username, password, 6 ,verbose );
            if(1){
                for(int i=0;i<2;i++)
                    stunSendTest( myFd1, dest1, username, password, 7 ,verbose );
                for(int i=0;i<2;i++)
                    stunSendTest( myFd1, dest2, username, password, 8 ,verbose );
                for(int i=0;i<2;i++)
                    stunSendTest( myFd2, dest1, username, password, 9 ,verbose );
                for(int i=0;i<2;i++)
                    stunSendTest( myFd2, dest2, username, password, 10 ,verbose );


            }



        }
        else
        {
            if (verbose) clog << "-----------------------------------------" << endl;
            assert( err>0 );
            // data is avialbe on some fd

            for ( int i=0; i<2; i++)
            {
                Socket myFd;
                if ( i==0 )
                {
                    myFd=myFd1;
                }
                else
                {
                    myFd=myFd2;
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


                        if(stunAddr4Vect[resp.msgHdr.id.octet[0]-sa4VectOffset].addr==0){

                            struct in_addr   stInAddr;//必要的结构体，只要包含前面三个头文件，就不要犯愁这个结构的定义
                            char ipaddr[20]; //保存转换后的地址

                            char *pIp;
                            stInAddr.s_addr=htonl(resp.mappedAddress.ipv4.addr);
                            pIp=inet_ntoa(stInAddr);
                            strcpy(ipaddr,pIp);
                            cout<<ipaddr<<":"<<resp.mappedAddress.ipv4.port<<"------"<<to_string(resp.msgHdr.id.octet[0])<<endl;

                            stunAddr4Vect[resp.msgHdr.id.octet[0]-sa4VectOffset]=resp.mappedAddress.ipv4;
                            validRespCnt++;
                        }
                        if(validRespCnt==4){
                            //add by gaoyubin
                            //stunSendTest( myFd1, dest1, username, password, 7 ,verbose );
                            int symIncDiff=0;
                            g_reflexStunAddr4h=stunAddr4Vect[0];
                            char localIP[32];
                            getIpInterface(localIP,NULL,NULL);
                            if(ntohl(inet_addr(localIP))==stunAddr4Vect[0].addr)
                                natType=NatTypeOpen;
                            if(stunAddr4Vect[0]==stunAddr4Vect[1])
                                natType= NatTypeCone;
                            else if(stunAddr4Vect[0].port==port-1 && stunAddr4Vect[2].port==port)
                                natType= NatTypeSymOneToOne;
//                            else if(stunAddr4Vect[0]<stunAddr4Vect[1] &&
//                                    stunAddr4Vect[1]<stunAddr4Vect[2] &&
//                                    stunAddr4Vect[2]<stunAddr4Vect[3])
                            else if(checkIsSymInc(stunAddr4Vect,symIncDiff)){
                                g_reflexStunAddr4h=stunAddr4Vect[3];
                                g_reflexStunAddr4h.port+=symIncDiff;
                                natType= NatTypeSymInc;

                            }

                            else{
                                sort(stunAddr4Vect.begin(),stunAddr4Vect.end());

                                g_reflexStunAddr4h.port=(stunAddr4Vect[0].port+stunAddr4Vect[3].port)/2;
                                natType= NatTypeSymRandom;
                            }


                            //add by gaoyubin
                            stunSendTest( myFd1, dest1, username, password, 7 ,verbose );
                            close(myFd1);
                            close(myFd2);
                            return natType;

                        }


                    }
                }
            }
        }
    }
    close(myFd1);
    close(myFd2);
    return natType;
}

