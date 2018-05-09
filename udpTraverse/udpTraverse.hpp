//
// Created by tl on 18-4-17.
//

#ifndef NATTRAVERSECLIENTMUDUO_UDPTRAVERSE_HPP
#define NATTRAVERSECLIENTMUDUO_UDPTRAVERSE_HPP

#include <iostream>
#include "../stun/stun.h"
#include <vector>
#include <map>

using namespace std;

class Addr{
public:
    Addr(string ip="0.0.0.0",int port=0):ip(ip),port(port){
        //cout<<"Addr constr"<<endl;
    };

    friend ostream&operator<<(ostream& os,const Addr&addr){

        os<<addr.ip<<":"<<addr.port;
        return os;

    }
    bool operator==(Addr b){
        if(ip==b.ip && port==b.port)
            return true;
        else
            return false;
    }
    bool operator!=(Addr b){
        if(*this==b)
            return false;
        else
            return true;
    }

    string ip;
    uint16_t  port;
};

struct  TrAddrInfo{
    TrAddrInfo(Addr  a=Addr(),int c=0,int l=0,string s=string("traverse")):addr(a),outStr(s),step(c),len(l){}
    string outStr="traverse";
    Addr addr;
    int step;
    int len;
};

struct ClientConnInfo{
    vector<Component>udpCompVect;
    vector<Component>tcpCompVect;
};
struct  Component{
    Component(string uHostIP="0.0.0.0",int uHostPort=0,
              string pReflexIP="0.0.0.0",int pReflexPort=0,
              string uReflexIP="0.0.0.0",int uReflexPort=0):
            uHostAddr(uHostIP,uHostPort),
            peerReflexAddr(pReflexIP,pReflexPort),
            uReflexAddr(uReflexIP,uReflexPort){

    }
    Addr uHostAddr;
    Addr peerReflexAddr;
    Addr uReflexAddr;
    int len=0;
    int step=0;
    vector<TrAddrInfo>trAddrInfoVect;
    pthread_t traverseTid;
    pthread_t heartBeatTid;
    //int udpTurnId;


};


//struct  AddrTriple{
//    AddrTriple(string u="0.0.0.0:0",string p1="0.0.0.0:0",string p2="0.0.0.0:0"):uAddr(u),peerAddr1(p1),peerAddr2(p2){}
//    string uAddr;
//    string peerAddr1;
//    string peerAddr2;
//
//};
void*udpTraverse(void*arg);
void* recvUdpBuf(void *arg);
extern char g_centreSvrIP[];
//extern bool  g_isTraversed;
extern vector<Component> g_compVect;
#endif //NATTRAVERSECLIENTMUDUO_UDPTRAVERSE_HPP
