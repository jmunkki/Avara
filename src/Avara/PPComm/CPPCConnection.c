/*
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CPPCConnection.c
    Created: Tuesday, February 21, 1995, 21:13
    Modified: Tuesday, February 20, 1996, 9:15
*/

#include "CPPCConnection.h"
#include "CCommServer.h"
#include "PPCToolbox.h"

/*
**	Objectsof this class do not need packet
**	buffers, because they borrow them from
**	the server object. Initialize accordingly.
*/
void	CPPCConnection::IPPCConnection(
	CCommServer	*theServer,
	short		id)
{
	ICommChannel(0);

	itsServer = theServer;
	myId = id;
	next = NULL;
}

/*
**	Helper function for completed "inform".
**
**	An inform is like an open door waiting
**	for someone to walk in and use the server.
*/
pascal
void	PPCConnectionInformFilter(
	InformEnvelope	*informP)
{
	((CPPCConnection *)informP->t)->InformComplete();
}

/*
**	An accept completes. See if we got a connection
**	or need to re-open the door with another inform.
*/
pascal
void	PPCConnectionAcceptFilter(
	InformEnvelope	*informP)
{
	CPPCConnection	*theConn;

	theConn = (CPPCConnection *)informP->t;
	
	if(informP->r.ioResult == noErr)
	{
		theConn->isConnected = true;
		theConn->AsyncRead();
	}
	else
	{	if(!theConn->isClosing)
			theConn->Inform();
	}
}	

/*
**	"Open the door" for clients to start using
**	the server.
*/
void	CPPCConnection::Inform()
{
	OSErr			iErr;

	isConnected = false;
	informE.r.ioCompletion = (PPCCompProcPtr)PPCConnectionInformFilter;
	informE.r.portRefNum = itsServer->itsRefNum;
	informE.r.portName = NULL;
	informE.r.locationName = NULL;
	informE.r.userName = NULL;
	informE.r.autoAccept = false;
	informE.t = this;
	
	iErr = PPCInform(&informE.r, true);
}

/*
**	When someone "walks in", we have to
**	accept the client for a connection.
*/
void	CPPCConnection::InformComplete()
{
	if(informE.r.ioResult == noErr)
	{	isConnected = true;
		sessionRef = informE.r.sessRefNum;
	
		informE.r.ioCompletion = (void *)PPCConnectionAcceptFilter;
		PPCAccept((PPCAcceptPBPtr)&informE.r, true);
	}
}

/*
**	Close connection to server, if necessary.
*/
void	CPPCConnection::EndService()
{
	PPCEndPBRec		pb;
	OSErr			iErr;

	if(isConnected)
	{	pb.ioCompletion = NULL;
		pb.sessRefNum = informE.r.sessRefNum;
		iErr = PPCEnd(&pb, false);
		writeCount = 0;
		writtenCount = 0;
	}
}

/*
**	Borrow a packet from the server, since we have none.
*/
PacketInfo *	CPPCConnection::GetPacket()
{
	return itsServer->GetPacket();
}

/*
**	Release the packet to the server, since we shouldn't have any.
*/
void	CPPCConnection::ReleasePacket(
	PacketInfo *thePacket)
{
	itsServer->ReleasePacket(thePacket);
}

/*
**	Let the server do all the packet dispatching.
**	Let's sign the packet just in case, since we know where it's coming.
*/
void	CPPCConnection::DispatchPacket(
	PacketInfo	*thePacket)
{
	thePacket->sender = myId;
	itsServer->DispatchPacket(thePacket);
}

/*
**	There was a read problem. We need to close the connection
**	and "reopen the door".
*/
void	CPPCConnection::ReadErr()
{
	inherited::ReadErr();
	if(!isClosing)
	{	Inform();
	}
}

short	CPPCConnection::GetKillReceivers()
{
	return 1;
}