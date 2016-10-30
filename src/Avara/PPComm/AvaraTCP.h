/*/
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: AvaraTCP.h
    Created: Sunday, January 28, 1996, 17:22
    Modified: Friday, April 19, 1996, 19:59
/*/

#pragma once
#include <MacTCPLib.h>
#include <DNR.h>
#include "CCommManager.h"

OSErr PascalStringToAddress(StringPtr name, ip_addr *addr);
OSErr AddressToPascalString(ip_addr addr, StringPtr name);

OSErr	OpenAvaraTCP();

typedef struct	
{
	PacketInfo		packet;
	TCPiopb			pb;

}	TCPPacketInfo;

#ifdef MAIN_AVARA_TCP
		short gMacTCP = -1;
#else
extern	short gMacTCP;
#endif