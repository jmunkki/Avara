/*/
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CCommClient.c
    Created: Thursday, February 23, 1995, 20:23
    Modified: Wednesday, August 14, 1996, 19:02
/*/

#include "CCommClient.h"
#include "CPPCConnection.h"
#include "PPCToolbox.h"
#include "GestaltEqu.h"

void	CCommClient::ICommClient(
	short	bufferCount)
{
	ICommChannel(bufferCount);
	
	genericInfoTextRes = 401;
	itsRefNum = 0;
}

void	CCommClient::Dispose()
{	
	Disconnect();
	
	inherited::Dispose();
}

/*
**	Filter out unwanted PPC connections.
*/
pascal
Boolean	PPCClientBrowseFilter(
	LocationNamePtr	theLocation,
	PortInfoPtr		thePortInfo)
{
	Str32	goodName;
	Str32	goodType;
	
	if(theLocation->locationKindSelector)
	{
		GetIndString(goodName, TALKERSTRINGS, 2);
		GetIndString(goodType, TALKERSTRINGS, 1);
		return	thePortInfo->name.portKindSelector == ppcByString &&
				EqualString(goodName, thePortInfo->name.name, true, true) &&
				EqualString(goodType, thePortInfo->name.u.portTypeStr, true, true);
	}
	else
		return false;	//	Only remote servers are allowed.
}

/*
**	Bring up user interface that allows the user to choose
**	a server.
*/
OSErr	CCommClient::Browse()
{
	Str255			prompt;
	LocationNameRec	locationName;
	PortInfoRec		port;
	PPCStartPBRec	pb;
	OSErr			iErr;
	
	GetIndString(prompt, TALKERSTRINGS, 4);
	
	iErr = PPCBrowser(prompt, NULL, false, &locationName, &port, PPCClientBrowseFilter, NULL);

	if(iErr == noErr)
	{	pb.ioCompletion = NULL;
		pb.portRefNum = itsRefNum;
		pb.serviceType = ppcServiceRealTime;
		pb.resFlag = 0;
		pb.portName = &port.name;
		pb.locationName = &locationName;
		pb.userRefNum = 0;
		
		if(port.authRequired)
		{	Str32			userName;
			Str255			prompt;
			Boolean			guestSelected;
			unsigned long	defaultUserRef;
			
			guestSelected = false;
			GetIndString(prompt, TALKERSTRINGS, 5);
			iErr = GetDefaultUser(&defaultUserRef, userName);
			if(iErr)
			{	userName[0] = 0;
			}
			iErr = StartSecureSession(&pb,
					userName, iErr == noErr, false, &guestSelected, prompt);
			userIdentity = pb.userRefNum;
		}
		else
		{	userIdentity = 0;
			iErr = PPCStart(&pb, false);
		}
		
		sessionRef = pb.sessRefNum;
		if(!iErr)	AsyncRead();
	}

	return 	iErr;
}

/*
**	Connect to a server.
**	
**	Calls browse to allow the user to choose
**	a server and then connects to it, if necessary.
*/
void	CCommClient::Connect()
{
	OSErr			iErr = noErr;
	PPCOpenPBRec	pb;
	PPCPortRec		port;
	LocationNameRec	locationName;
	CPPCConnection	*aConn;
	long			gestResult;
	
	Gestalt(gestaltPPCToolboxAttr, &gestResult);
	if(gestResult & gestaltPPCSupportsOutGoing)
	{
		if(!isOpen)
		{
	
			locationName.locationKindSelector = ppcNBPTypeLocation;
			GetIndString(locationName.u.nbpType, TALKERSTRINGS, 3);
			
			port.nameScript = 0;	//	smRoman, really, but I was too lazy to #include script.h
			GetIndString(port.name, TALKERSTRINGS, 3);
			port.portKindSelector = ppcByString;
			GetIndString(port.u.portTypeStr, TALKERSTRINGS, 1);
		
			pb.ioCompletion = NULL;
			pb.serviceType = ppcServiceRealTime;
			pb.resFlag = 0;
			pb.portName = &port;
			pb.locationName = &locationName;
			pb.networkVisible = false;
			
			iErr = PPCOpen(&pb, false);
	
			itsRefNum = pb.portRefNum;
			isOpen = (iErr == noErr);
		}
					
		if(isOpen && !isConnected)
		{	iErr = Browse();
			isConnected = (iErr == noErr);
		}
	}
	else
	{	SetCursor(&qd.arrow);
		Alert(1000, 0);
	}
}

/*
**	Break the connection, if there is one.
*/
void	CCommClient::Disconnect()
{
	OSErr			iErr;
	PPCClosePBRec	pbclose;
	PPCEndPBRec		pbend;
	
	FlushOutQueue();
	isClosing = true;

	if(isConnected)
	{	isConnected = false;
	
		pbend.ioCompletion = NULL;
		pbend.sessRefNum = sessionRef;
		iErr = PPCEnd(&pbend, false);
		if(userIdentity)
		{	iErr = DeleteUserIdentity(userIdentity);
		}
	}

	if(isOpen)
	{
		isOpen = false;
		pbclose.ioCompletion = NULL;
		pbclose.portRefNum = itsRefNum;
		iErr = PPCClose(&pbclose, false);
	}
}

/*
**	Sign a packet and then write it out.
*/
void	CCommClient::WriteAndSignPacket(
	PacketInfo	*thePacket)
{
	thePacket->sender = myId;
	WritePacket(thePacket);
}

/*
**	Send a packet to the server, but dispatch
**	it to self (and remove self from distribution),
**	if possible.
*/
void	CCommClient::WritePacket(
	PacketInfo	*thePacket)
{
	if(thePacket->distribution & (1<<myId))
	{	DispatchPacket(thePacket);
		thePacket->distribution ^= 1<<myId;
	}
	
	if(thePacket->distribution)
	{	inherited::WritePacket(thePacket);
	}
	else
	{	ReleasePacket(thePacket);
	}
}

/*
**	Oops, were we disconnected?
*/
void	CCommClient::DisconnectSlot(
	short	slotId)
{
	if(slotId == myId)
	{	Disconnect();
	}
}
long	CCommClient::GetMaxRoundTrip(
	short	distribution)
{
	return	30;	//	just guess it to be 30 milliseconds.
}