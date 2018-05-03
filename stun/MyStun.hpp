//
// Created by tl on 18-4-25.
//

#ifndef NATTRAVERSECLIENTMUDUO_MYSTUNNATTYPE_HPP
#define NATTRAVERSECLIENTMUDUO_MYSTUNNATTYPE_HPP


#include "stun.h"
#include "iostream"
using namespace std;


typedef enum {
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


} MyNatType;

#define NatType_To_Str(s)                 #s


MyNatType
detectNatType(StunAddress4 &dest1,
              bool verbose,
              StunAddress4 *sAddr // NIC to use
);
string  getNatTypeStr(MyNatType natType);
int getIpInterface(char *localIp,char *netMask,char*netInterface);
void
stunSendTest( Socket myFd, StunAddress4& dest,
              const StunAtrString& username, const StunAtrString& password,
              int testNum, bool verbose );
#endif //NATTRAVERSECLIENTMUDUO_MYSTUNNATTYPE_HPP
