/*/
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CPPCConnection.h
    Created: Tuesday, February 21, 1995, 21:12
    Modified: Thursday, January 25, 1996, 7:37
/*/

#pragma once
#include "CCommChannel.h"
#include "PPCStuff.h"

class	CCommServer;
class	CPPCConnection;

/*
**	This class helps the PPC server to make
**	connections to individual clients. Each
**	client requires its own connection.
**
**	This class is not used outside the server
**	class.
*/
class	CPPCConnection : public CCommChannel
{
public:
			CPPCConnection	*next;
			CCommServer		*itsServer;

			InformEnvelope	informE;

	virtual	void			IPPCConnection(CCommServer *theServer, short id);
	virtual	void			Inform();
	virtual	void			EndService();
	virtual	void			InformComplete();

	virtual	void			ReadErr();

	virtual	PacketInfo		*GetPacket();
	virtual	void			ReleasePacket(PacketInfo *thePacket);
	virtual	void			DispatchPacket(PacketInfo *thePacket);
	virtual	short			GetKillReceivers();
};