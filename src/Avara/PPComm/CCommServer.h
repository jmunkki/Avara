/*
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CCommServer.h
    Created: Tuesday, February 21, 1995, 21:08
    Modified: Wednesday, August 14, 1996, 19:03
*/

#pragma once
#include "CAbstractCommServer.h"

class	CPPCConnection;

class	CCommServer : public CAbstractCommServer
{
public:
			short			itsRefNum;
			CPPCConnection	*connectionList;

	virtual	void			ICommServer(short clientCount, short bufferCount);
	virtual	void			Dispose();
	virtual	void			StartServing();
	virtual	void			StopServing();
	virtual	void			DispatchPacket(PacketInfo *thePacket);
	virtual	void			DisconnectSlot(short slotId);
	virtual	long			GetMaxRoundTrip(short distribution);
};