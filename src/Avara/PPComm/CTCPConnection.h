/*/
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: CTCPConnection.h
    Created: Sunday, January 28, 1996, 11:39
    Modified: Sunday, January 28, 1996, 21:55
/*/

#pragma once
#include "CCommManager.h"
#include "AvaraTCP.h"

#define	TCPSTREAMBUFFERSIZE	8192

class	CTCPConnection;
class	CTCPComm;

class	CTCPConnection : public CCommManager
{
public:
			TCPiopb			param;
			CTCPConnection	*next;
			CTCPComm		*itsOwner;
			TCPNotifyUPP	notify;

			StreamPtr		stream;
			char			streamBuffer[TCPSTREAMBUFFERSIZE];

			ip_addr			remoteAddr;
			short			remotePort;
			
			Boolean			isListening;
			Boolean			isClosing;

			TCPIOCompletionUPP		readCompletionProc;
			TCPIOCompletionUPP		writeCompletionProc;
			TCPIOCompletionUPP		passiveCompletionProc;
	
	virtual	void			ITCPConnection(CTCPComm *theOwner);
	virtual	void			Dispose();
	
	virtual	OSErr			ContactServer(ip_addr addr, short port, StringPtr password);
	
	virtual	OSErr			PassiveOpenAll(short localPort);
	virtual	void			Notify(ushort code, ushort termReason);
	
	virtual	void			ReadComplete(TCPiopbPtr pb);
	virtual	void			WriteComplete(TCPiopbPtr pb);
	virtual	void			PassiveComplete(TCPiopbPtr pb);

	virtual	PacketInfo		*GetPacket();
	virtual	void			ReleasePacket(PacketInfo *thePacket);
	virtual	void			DispatchPacket(PacketInfo *thePacket);
};

