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
//StunAddress4 g_reflexStunAddr4h;
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
        case 12:
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
        else if(nowDiff<0)
            continue;
        else{
            return false;
        }
    }
    return true;
}






MyNatType
detectNatType(StunAddress4 &dest1,
              bool verbose,
              vector<Component>&compVect
)
{
    int symIncDiff=0;
    assert( dest1.addr != 0 );
    assert( dest1.port != 0 );

    StunAddress4 dest2=dest1;
    dest2.port+=1;



    Socket myFd1 = openValidPort(compVect[0].uHostAddr.port,ntohl(inet_addr(compVect[0].uHostAddr.ip.c_str())),verbose);
    Socket myFd2 = openValidPort(compVect[1].uHostAddr.port,ntohl(inet_addr(compVect[1].uHostAddr.ip.c_str())),verbose);

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



                            struct in_addr reflexInAddr;
                            reflexInAddr.s_addr=htonl(stunAddr4Vect[0].addr);
                            g_compVect[0].uReflexAddr.ip=inet_ntoa(reflexInAddr);
                            g_compVect[0].uReflexAddr.port=stunAddr4Vect[0].port;

                            reflexInAddr.s_addr=htonl(stunAddr4Vect[2].addr);
                            g_compVect[1].uReflexAddr.ip=inet_ntoa(reflexInAddr);
                            g_compVect[1].uReflexAddr.port=stunAddr4Vect[2].port;

                            char localIP[32];
                            getIpInterface(localIP,NULL,NULL);
                            if(ntohl(inet_addr(localIP))==stunAddr4Vect[0].addr)
                                natType=NatTypeOpen;
                            if(stunAddr4Vect[0]==stunAddr4Vect[1])
                                natType= NatTypeCone;
                            else if(stunAddr4Vect[0].port==compVect[0].uHostAddr.port && stunAddr4Vect[2].port==compVect[1].uHostAddr.port)
                                natType= NatTypeSymOneToOne;
//                            else if(stunAddr4Vect[0]<stunAddr4Vect[1] &&
//                                    stunAddr4Vect[1]<stunAddr4Vect[2] &&
//                                    stunAddr4Vect[2]<stunAddr4Vect[3])
                            else if(checkIsSymInc(stunAddr4Vect,symIncDiff)){
//                                g_reflexStunAddr4h=stunAddr4Vect[3];
//                                g_reflexStunAddr4h.port+=symIncDiff;
                                  g_compVect[0].uReflexAddr.port=stunAddr4Vect[3].port+symIncDiff;
                                  g_compVect[1].uReflexAddr.port=stunAddr4Vect[3].port+2*symIncDiff;

                                  g_compVect[0].step=symIncDiff;
                                  g_compVect[1].step=symIncDiff;
                                  natType= NatTypeSymInc;


                            }

                            else{
                                sort(stunAddr4Vect.begin(),stunAddr4Vect.end());

                                //g_reflexStunAddr4h.port=(stunAddr4Vect[0].port+stunAddr4Vect[3].port)/2;
                                natType= NatTypeSymRandom;
                            }
                            //for test
                            natType=NatTypeSymRandom;

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
bool getSymIncPort(StunAddress4 &stunSvrAddr3478, vector<StunAddress4> &peerAddr4Vect, vector<Component> &compVect){


    //UInt16 port=rand()%3000+4000;
    //cout<<"port: "<<port<<endl;
    bool verbose=false;
    StunAddress4 stunSvrAddr3479=stunSvrAddr3478;
    stunSvrAddr3479.port++;
    int validRespCnt=0;
    int sa4VectOffset=7;
    Socket myFd1 = openValidPort(compVect[0].uHostAddr.port,ntohl(inet_addr(compVect[0].uHostAddr.ip.c_str())),verbose);
    Socket myFd2 = openValidPort(compVect[1].uHostAddr.port,ntohl(inet_addr(compVect[1].uHostAddr.ip.c_str())),verbose);
    //Socket myFd3 = openPort(localAddr.port+1,ntohl(inet_addr(localAddr.ip.c_str())),verbose);



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

    vector<StunAddress4>stunAddrVect(3);
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
        //FD_SET(myFd3,&fdSet); fdSetSize = (myFd3+1>fdSetSize) ? myFd3+1 : fdSetSize;
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
                stunSendTest( myFd1, stunSvrAddr3478, username, password, 7 ,verbose );
            //for test
            stunSendTest( myFd2, stunSvrAddr3479, username, password, 12 ,verbose );
            for(int i=0;i<2;i++)
                stunSendTest( myFd1, peerAddr4Vect[0], username, password, 10 ,verbose );
            for(int i=0;i<2;i++)
                stunSendTest( myFd1, stunSvrAddr3479, username, password, 8 ,verbose );

            for(int i=0;i<2;i++)
                stunSendTest( myFd2, peerAddr4Vect[1], username, password, 11 ,verbose );

            for(int i=0;i<2;i++)
                stunSendTest( myFd2, stunSvrAddr3478, username, password, 9 ,verbose );

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
                else if(i==1)
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

                        StunAddress4 emptyStunAddr;
                        if(resp.msgHdr.id.octet[0]-sa4VectOffset<stunAddrVect.size() &&  stunAddrVect[resp.msgHdr.id.octet[0]-sa4VectOffset]==emptyStunAddr){
                            cout<<"index="<<resp.msgHdr.id.octet[0]-sa4VectOffset<<endl;
                            stunAddrVect[resp.msgHdr.id.octet[0]-sa4VectOffset]=resp.mappedAddress.ipv4;
                            validRespCnt++;
                        }
                        if(validRespCnt==3){
                            struct in_addr inAddr;
                            cout<<"the 3 receive ip from stun:"<<endl;
                            for(int i=0;i<stunAddrVect.size();++i){
                                inAddr.s_addr=htonl(stunAddrVect[i].addr);
                                cout<<inet_ntoa(inAddr)<<":"<<stunAddrVect[i].port<<endl;
                            }
                            inAddr.s_addr=htonl(stunAddrVect[0].addr);
                            compVect[0].uReflexAddr.ip=inet_ntoa(inAddr);
                            compVect[1].uReflexAddr.ip=inet_ntoa(inAddr);
                            if(stunAddrVect[1].port-stunAddrVect[0].port>0){
                                compVect[0].uReflexAddr.port=stunAddrVect[0].port+compVect[0].step;
                                compVect[0].len=stunAddrVect[1].port-compVect[0].uReflexAddr.port;
                            }
                            else{
                                compVect[0].uReflexAddr.port=2049;
                                compVect[0].len=stunAddrVect[1].port-compVect[0].uReflexAddr.port;
                            }
                            if(stunAddrVect[2].port-stunAddrVect[1].port>0){
                                compVect[1].uReflexAddr.port=stunAddrVect[2].port+compVect[1].step;
                                compVect[1].len=stunAddrVect[2].port-compVect[1].uReflexAddr.port;
                            }
                            else{
                                compVect[1].uReflexAddr.port=2049;
                                compVect[1].len=stunAddrVect[2].port-compVect[1].uReflexAddr.port;
                            }


//                            if(stunAddrVect[1].port-stunAddrVect[0].port==2*compVect[0].step){
//                                compVect[0].len=compVect[0].step;
//                            }
//                            else{
//                                compVect[0].len=stunAddrVect[1].port-stunAddrVect[0].port+compVect[1].step;
//                            }
//                            inAddr.s_addr=htonl(stunAddrVect[1].addr);
//                            compVect[1].uReflexAddr.ip=inet_ntoa(inAddr);
//                            compVect[1].uReflexAddr.port=stunAddrVect[1].port+compVect[1].step;
//                            if(stunAddrVect[2].port-stunAddrVect[1].port==2*compVect[1].step){
//                                compVect[1].len=compVect[1].step;
//                            }
//                            else{
//                                compVect[1].len=stunAddrVect[2].port-stunAddrVect[1].port+compVect[1].step;
//                            }
                            close(myFd1);
                            close(myFd2);

                            return true;
                        }


                    }
                }
            }
        }
    }
    close(myFd1);
    close(myFd2);

    return false;
}


