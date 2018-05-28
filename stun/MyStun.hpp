//
// Created by tl on 18-4-25.
//

#ifndef NATTRAVERSECLIENTMUDUO_MYSTUNNATTYPE_HPP
#define NATTRAVERSECLIENTMUDUO_MYSTUNNATTYPE_HPP


#include "stun.h"
#include <iostream>
#include <vector>
#include "../natTraverse.hpp"


using namespace std;






#define NatType_To_Str(s)                 #s



MyNatType
detectNatType(StunAddress4 &dest1,
              bool verbose,
              vector<UDPComponent>&compVect // NIC to use

);
string  getNatTypeStr(MyNatType natType);
int getIpInterface(char *localIp,char *netMask,char*netInterface);
void stunSendTest( Socket myFd, StunAddress4& dest,
              const StunAtrString& username, const StunAtrString& password,
              int testNum, bool verbose );
bool getSymIncPort(StunAddress4 &stunSvrAddr3478, vector<StunAddress4> &peerAddr4Vect, vector<UDPComponent> &compVect);

#endif //NATTRAVERSECLIENTMUDUO_MYSTUNNATTYPE_HPP
