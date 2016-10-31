/*
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: CTCPComm.h
    Created: Sunday, January 28, 1996, 8:30
    Modified: Sunday, January 28, 1996, 19:25
*/

#pragma once
#include "CCommManager.h"
#include "MacTCP.h"

class	CTCPComm : public CCommManager
{
public:
			Boolean				isConnected;
	class	CTagBase			*prefs;
	class	CTCPConnection		*connections;

	virtual	void		ITCPComm(short clientCount, short bufferCount);
	virtual	OSErr		AllocatePacketBuffers();
	virtual	void		Dispose();
	
	virtual	void		ContactServer(ip_addr addr, short port, StringPtr password);
	virtual	void		Connect();	//	Client
	virtual	void		StartServing();
	
	virtual	void		DoHotPop(DialogPtr theDialog, Rect *popRect);
};