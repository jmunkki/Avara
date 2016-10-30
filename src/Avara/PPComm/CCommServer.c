/*/
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CCommServer.c
    Created: Tuesday, February 21, 1995, 21:10
    Modified: Wednesday, August 14, 1996, 19:02
/*/

#include "CCommServer.h"
#include "CPPCConnection.h"
#include "PPCToolbox.h"
#include "GestaltEqu.h"

/*
**	Initialize object for max number of clients and
**	maximum number of packet buffers. Make sure that
**	both are sufficient for your application!
*/
void	CCommServer::ICommServer(
	short	clientCount,
	short	bufferCount)
{
	CPPCConnection	*aConn;
	
	inherited::ICommServer(clientCount, bufferCount);

	genericInfoTextRes = 401;

	connectionList = NULL;
	
	while(clientCount--)
	{	aConn = new CPPCConnection;
		aConn->IPPCConnection(this, clientCount+1);
		aConn->next = connectionList;
		connectionList = aConn;
	}
}

/*
**	Release allocated storage and
**	dispose of CPPCConnections.
*/
void	CCommServer::Dispose()
{
	CPPCConnection	*aConn, *next;
	
	StopServing();

	aConn = connectionList;
	while(aConn)
	{	next = aConn->next;
		aConn->Dispose();
		aConn = next;
	}
	
	inherited::Dispose();
}

/*
**	Start the server.
**
**	Involves PPCToolbox stuff and calling "Inform" for each
**	of the CPPCConnection objects.
**
**	Probably would need more error checking so that if the PPCToolbox
**	can not be used, more errors can be avoided.
*/
void	CCommServer::StartServing()
{
	OSErr			iErr = noErr;
	PPCOpenPBRec	pb;
	PPCPortRec		port;
	LocationNameRec	locationName;
	CPPCConnection	*aConn;
	long			gestResult;
	
	Gestalt(gestaltPPCToolboxAttr, &gestResult);
	if(gestResult & gestaltPPCSupportsOutGoing)
	{	if(gestResult & gestaltPPCSupportsIncoming)
		{	if(!isServing)
			{
				isServing = true;
		
				locationName.locationKindSelector = ppcNBPTypeLocation;
				GetIndString(locationName.u.nbpType, TALKERSTRINGS, 2);
				
				port.nameScript = 0;	//	smRoman, really, but I was too lazy to #include script.h
				GetIndString(port.name, TALKERSTRINGS, 2);
				port.portKindSelector = ppcByString;
				GetIndString(port.u.portTypeStr, TALKERSTRINGS, 1);
			
				pb.ioCompletion = NULL;
				pb.serviceType = ppcServiceRealTime;
				pb.resFlag = 0;
				pb.portName = &port;
				pb.locationName = &locationName;
				pb.networkVisible = true;
				
				iErr = PPCOpen(&pb, false);
				
				itsRefNum = pb.portRefNum;
				
				for(aConn = connectionList; aConn ; aConn = aConn->next)
				{	aConn->Inform();
				}
			}
		}
		else
		{	SetCursor(&qd.arrow);
			Alert(1001, 0);
		}
	}
	else
	{	SetCursor(&qd.arrow);
		Alert(1000, 0);
	}
}

/*
**	Close down service.
*/
void	CCommServer::StopServing()
{
	CPPCConnection	*aConn;
	OSErr			iErr;
	PPCClosePBRec	pb;
	
	if(isServing)
	{	isServing = false;

		for(aConn = connectionList; aConn ; aConn = aConn->next)
		{	aConn->isClosing = true;
			aConn->EndService();
		}
	
		pb.ioCompletion = NULL;
		pb.portRefNum = itsRefNum;
		
		iErr = PPCClose(&pb, false);
	}
}

/*
**	Server version of DispatchPacket. It involves duplicating
**	and sending the packets to the clients that still need it.
*/
void	CCommServer::DispatchPacket(PacketInfo *thePacket)
{
	CPPCConnection	*aConn;

	for(aConn = connectionList; aConn; aConn = aConn->next)
	{	if(aConn->isConnected && ((1 << aConn->myId) & thePacket->distribution))
		{	PacketInfo	*dupPack;
		
			dupPack = DuplicatePacket(thePacket);
			if(dupPack)	aConn->WritePacket(dupPack);
		}
	}

	if(thePacket->distribution & 1)	//	Bit 0 set means server distribution
	{	inherited::DispatchPacket(thePacket);
	}
}

/*
**	A client has disconnected or is in trouble: end service and
**	clean up after it to recover the packet buffers that it has borrowed.
*/
void	CCommServer::DisconnectSlot(
	short	slotId)
{
	CPPCConnection	*aConn;
	
	for(aConn = connectionList; aConn; aConn = aConn->next)
	{	if(aConn->myId == slotId)
		{	aConn->EndService();
			aConn->FlushOutQueue();
		}
	}
}

long	CCommServer::GetMaxRoundTrip(
	short	distribution)
{
	return	30;	//	just guess it to be 30 milliseconds.
}