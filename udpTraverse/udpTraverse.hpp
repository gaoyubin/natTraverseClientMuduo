//
// Created by tl on 18-4-17.
//

#ifndef NATTRAVERSECLIENTMUDUO_UDPTRAVERSE_HPP
#define NATTRAVERSECLIENTMUDUO_UDPTRAVERSE_HPP

#include <iostream>
#include "../stun/stun.h"

using namespace std;

struct Addr{
    Addr(string ip="0.0.0.0",int port=0):ip(ip),port(port){
        //cout<<"Addr constr"<<endl;
    };


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
struct  AddrPair{
    AddrPair(string uIP="0.0.0.0:0",int uPort=0,string pIP="0.0.0.0:0",int pPort=0):uAddr(uIP,uPort),peerAddr(pIP,pPort){

    }
    Addr uAddr;
    Addr peerAddr;

};

struct  TrAddrInfo{
    TrAddrInfo(Addr  a,int l=0,int c=0):addr(a),len(l),cur(c){}
    Addr addr;
    int len;
    int cur;
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
bool getSymIncRange(StunAddress4 &stunSvrAddr4,StunAddress4 &peerAddr4,Addr localAddr,Addr &startAddr,Addr &endAddr);

extern char g_centreSvrIP[];
extern bool  g_isTraversed;
#endif //NATTRAVERSECLIENTMUDUO_UDPTRAVERSE_HPP
