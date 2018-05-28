//
// Created by tl on 18-5-9.
//

#ifndef NATTRAVERSECLIENTMUDUO_MAIN_HPP
#define NATTRAVERSECLIENTMUDUO_MAIN_HPP

#include <string>
#include <iostream>
#include <vector>
#include <map>


using namespace std;


class Addr{
public:
    //Addr(uint32_t ip,int port):inetport(port)
    Addr(string ip="0.0.0.0",int port=0):ip(ip),port(port){
        //cout<<"Addr constr"<<endl;
    };

    friend ostream&operator<<(ostream& os,const Addr&addr){
        //os<<":";
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
    bool operator<(Addr b)const{
        if(this->ip < b.ip)
            return  true;
        else if(this->ip == b.ip){
            if(this->port < b.port)
                return true;
            else
                return false;
        }
        else
            return false;

    }
    bool operator<(Addr b){
        return static_cast<const Addr>(*this)<b;

    }


    string ip;
    uint16_t  port;
};
struct SvrAddrInfo{
    Addr stunSvrAddr;
    Addr udpTurnSvrAddr;
    Addr tcpTurnSvrAddr;
    Addr centreSvrAddr;

};
enum MyNatType{
    NatTypeFail = 0,
    NatTypeBlocked,

    NatTypeOpen,
    NatTypeCone,
    NatTypeSymInc,
    NatTypeSymOneToOne,
    NatTypeSymRandom,

    NatTypeUnknown
    //StunTypeConeNat,
    //StunTypeRestrictedNat,
    //StunTypePortRestrictedNat,
    //StunTypeSymNat,


} ;

struct LocalNetInfo{
public:
    MyNatType natType;
    string netMask;
    string hostIP;
};
struct Account{
    string name;
    string password;
    string role;
    friend ostream& operator<<(ostream&os,const Account& account){
        os<<"name="<<account.name<<endl;
        os<<"password="<<account.password<<endl;
        os<<"role="<<account.role<<endl;
        return os;
    }

};
//class TrAddrInfo;
class  TrAddrInfo{
public:
    TrAddrInfo(Addr  a=Addr(),int c=0,int l=0,string s=string("traverse")):addr(a),step(c),len(l),outStr(s){}
    string outStr="traverse";
    Addr addr;
    int step;
    int len;
};
struct  UDPComponent{
    UDPComponent(string uHostIP="0.0.0.0",int uHostPort=0,
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


struct  TCPComponent{
//    TCPComponet(){
//
//
//    }
    Addr turnCliAddr;
    Addr turnSvrAddr;
    Addr liveCliAddr;
    Addr liveSvrAddr;
    string  tcpID;
    pthread_t proxyTid;
    //int udpTurnId;


};
struct ClientConnInfo{
    vector<UDPComponent>udpComps;
    //vector<TCPComponent>tcpComps;
    TCPComponent tcpComp;
};


extern Account g_account;
extern map<string,ClientConnInfo>g_cliConnInfoMap;
bool initTraverse();
bool  onLogin(Account& account,Addr&centreSvrAddr,map<string,ClientConnInfo>* &cliMap );
bool onList(vector<string>&);
bool onSee(string peerName,string peerPwd="123456");

#endif //NATTRAVERSECLIENTMUDUO_MAIN_HPP
