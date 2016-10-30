/*/
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: CUDPConnection.c
    Created: Monday, January 29, 1996, 13:36
    Modified: Wednesday, August 14, 1996, 17:47
/*/

#include "CUDPConnection.h"
#include "CUDPComm.h"
#include "CommDefs.h"

#define	kInitialRetransmitTime		480			//	2	seconds
#define	kInitialRoundTripTime 		240			//	<1 second
#define	kMaxAllowedRetransmitTime	960			//	4 seconds
#define	kAckRetransmitBase			10
#define	kULPTimeOut					(7324*4)	//	30*4 seconds
#define	kMaxTransmitQueueLength		128			//	128 packets going out...
#define	kMaxReceiveQueueLength		32			//	32 packets...arbitrary guess

#define	RTTSMOOTHFACTOR			6
#define	DEVIATIONSMOOTHFACTOR	4
#define	PESSIMISTSMOOTHFACTOR	5

#if	DEBUG_AVARA
static	StringPtr	errorKillReason = "\pNo reason";
#define	ERRORKILLDEBUG(str)	errorKillReason = str;
#else
#define	ERRORKILLDEBUG(str)
#endif

#if	DEBUG_AVARA
void	CUDPConnection::DebugEvent(
	char	eType,
	long	pNumber)
{
	Str32	pString;
	char	pCode[4];

#if 0	
	if(eType == 'R' || eType == 'Q' || eType == 'D' || eType == 'A')
		return;

	if(eType != ' ' && eType != '>' && eType != '/')
		return;
#endif
	pCode[0] = eType;
	NumToString(pNumber, pString);
	if(pString[0] == 1)
	{	pCode[1] = ' ';
		pCode[2] = ' ';
		pCode[3] = pString[1];
	}
	else
	if(pString[0] == 2)
	{	pCode[1] = ' ';
		pCode[2] = pString[1];
		pCode[3] = pString[2];
	}
	else
	{	pCode[3] = pString[pString[0]--];
		pCode[2] = pString[pString[0]--];
		pCode[1] = pString[pString[0]--];
	}
	
	d[(kDebugBufferSize-1) & dp++] = *(OSType *)pCode;
}
#endif

void	CUDPConnection::IUDPConnection(
	CUDPComm	*theOwner)
{
	OSErr		err=noErr;
	short		i;

	killed = false;
	itsOwner = theOwner;
	myId = -1;	//	Unknown
	
	ipAddr = 0;
	port = 0;
	
	for(i=0;i<kQueueCount;i++)
	{	queues[i].qFlags = 0;
		queues[i].qHead = 0;
		queues[i].qTail = 0;
	}
	
	serialNumber = 0;	
	receiveSerial = 0;
	maxValid = -kSerialNumberStepSize;

	retransmitTime = kInitialRetransmitTime;
	roundTripTime = kInitialRoundTripTime;
	pessimistTime = roundTripTime;
	optimistTime = roundTripTime;
	realRoundTrip = roundTripTime;
	deviation = roundTripTime;
	haveToSendAck = false;
	nextAckTime = 0;
	
	nextWriteTime = 0;
	validTime = 0;
	
	cramData = 0;
	seed = 0;

	next = NULL;
	
	ackBase = 0;
	offsetBufferBusy = NULL;
	ackBitmap = 0;

	busyQLen = 0;
	quota = 0;
	
	routingMask = 0;
}

void	CUDPConnection::FlushQueues()
{
	QElemPtr	elem;
	QHdr		*head;
	short		i;

	ackBase = 0;
	offsetBufferBusy = NULL;

	ackBitmap = 0;

	for(i=0;i<kQueueCount;i++)
	{	head = &queues[i];
		do
		{	elem = head->qHead;
			if(elem)
			{	short	theErr;
			
				theErr = Dequeue(elem, head);
				if(theErr == noErr)
				{	itsOwner->ReleasePacket((PacketInfo *)elem);
				}
			}
		} while(elem);
	}
	
	busyQLen = 0;
}

void	CUDPConnection::SendQueuePacket(
	UDPPacketInfo	*thePacket,
	short			theDistribution)
{
	//	Put the packet into the busy outgoing queue. It will be moved
	//	into the transmit queue very soon.
	thePacket = (UDPPacketInfo *)itsOwner->DuplicatePacket((PacketInfo *)thePacket);

	if(thePacket)
	{	thePacket->packet.distribution = theDistribution;
	
		Enqueue((QElemPtr) thePacket, &queues[kBusyQ]);
		busyQLen++;
#if	DEBUG_AVARA
		if(thePacket->packet.command == kpKeyAndMouse)
		{	DebugEvent('>', thePacket->packet.p3);
		}
#endif
	}
	else
	{
#if	DEBUG_AVARA
		ERRORKILLDEBUG("\PDuplicatePacket failed")
#endif
		ErrorKill();
	}
}

void	CUDPConnection::RoutePacket(
	UDPPacketInfo	*thePacket)
{
	short	extendedRouting = routingMask | (1 << myId);
	
	SendQueuePacket(thePacket, thePacket->packet.distribution & extendedRouting);
	thePacket->packet.distribution &= ~extendedRouting;
}

void	CUDPConnection::ProcessBusyQueue(
	long	curTime)
{
	UDPPacketInfo	*thePacket = NULL;

	while(thePacket = (UDPPacketInfo *)queues[kBusyQ].qHead)
	{	if(noErr == Dequeue((QElemPtr)thePacket, &queues[kBusyQ]))
		{	thePacket->birthDate = curTime;
			thePacket->nextSendTime = curTime;	
			thePacket->serialNumber = serialNumber;
			serialNumber += kSerialNumberStepSize;
			Enqueue((QElemPtr) thePacket, &queues[kTransmitQ]);
		}
		else
		{	//	Debugger();
			break;
		}
	}

	busyQLen = 0;
}

UDPPacketInfo *CUDPConnection::FindBestPacket(	
		long	curTime,
		long	cramTime,
		long	urgencyAdjust)
{
	UDPPacketInfo	*thePacket = NULL;
	UDPPacketInfo	*nextPacket;
	UDPPacketInfo	*bestPacket;
	OSErr			theErr;
	long			bestSendTime, theSendTime;
	long			oldestBirth;
	short			transmitQueueLength = 0;

	bestPacket = (UDPPacketInfo *)queues[kTransmitQ].qHead;
	oldestBirth = curTime;

	if(bestPacket)
	{	while(bestPacket && (bestPacket->serialNumber - maxValid > kSerialNumberStepSize*kMaxReceiveQueueLength))
		{	transmitQueueLength++;
			bestPacket = (UDPPacketInfo *)bestPacket->packet.qLink;
		}
		
		if(bestPacket->birthDate != bestPacket->nextSendTime)
			oldestBirth = bestPacket->birthDate;
	}
	
	if(bestPacket)
	{	bestSendTime = bestPacket->nextSendTime;

		if(bestPacket->packet.flags & kpUrgentFlag)
			bestSendTime -= urgencyAdjust;

		transmitQueueLength++;

		thePacket = (UDPPacketInfo *) bestPacket->packet.qLink;
		while(thePacket)
		{	transmitQueueLength++;

			nextPacket = (UDPPacketInfo *) thePacket->packet.qLink;

			if(thePacket->serialNumber - maxValid <= kSerialNumberStepSize*kMaxReceiveQueueLength)
			{
				theSendTime = thePacket->nextSendTime;

				if(thePacket->birthDate - oldestBirth < 0 && thePacket->birthDate != theSendTime)
				{	oldestBirth = thePacket->birthDate;
				}
	
				if(thePacket->packet.flags & kpUrgentFlag)
					theSendTime -= urgencyAdjust;
	
				if(bestSendTime - theSendTime > 0)
				{	bestSendTime = theSendTime;
					bestPacket = thePacket;
				}
			}
			
			thePacket = nextPacket;
		}

		thePacket = NULL;	//	It already is NULL from the loop.

		if(bestPacket->serialNumber - maxValid > 0)
		{	if(bestSendTime - cramTime - curTime <= 0)
			{	theErr = Dequeue((QElemPtr)bestPacket, &queues[kTransmitQ]);
		
				if(theErr == noErr)
				{	//	An error could have occured if the packet had been
					//	taken while we were looking at the queue. I
					//	don't think it will ever happen, but it pays
					//	to be careful.
					
					thePacket = bestPacket;
				}
#if	DEBUG_AVARA
				else
				{
					DebugStr("\pDequeue failed in FindBestPacket.;g");
				}
#endif
			}
		}
		else
		{	ValidatePacket(bestPacket, curTime);
		}
	}

	if(curTime - oldestBirth > kULPTimeOut && curTime - validTime > kULPTimeOut)
	{
#if	DEBUG_AVARA
		ERRORKILLDEBUG("\PBirthdate too old")
#endif
		ErrorKill();
	}

	if(	transmitQueueLength > kMaxTransmitQueueLength)
	{
#if	DEBUG_AVARA
		ERRORKILLDEBUG("\PTransmit Queue Overflow")
#endif
		ErrorKill();
	}

	return thePacket;
}

UDPPacketInfo *
	CUDPConnection::GetOutPacket(
		long	curTime,
		long	cramTime,
		long	urgencyAdjust)
{
	UDPPacketInfo	*thePacket = NULL;
	UDPPacketInfo	*nextPacket;
	UDPPacketInfo	*bestPacket;
	OSErr			theErr;
	long			bestSendTime, theSendTime;

	if(port)
	{	ProcessBusyQueue(curTime);
		thePacket = FindBestPacket(curTime, cramTime, urgencyAdjust);
	}
	
	if(thePacket == NULL)
	{	//	We don't have packets to send, but we might want to send a
		//	special ACK packet instead when the other end is sending
		//	data that we already got and it looks like we are not missing
		//	any packets. (The receive queue would have them.)
	
		if(haveToSendAck && nextAckTime - curTime <= 0)
		{	//	This is a pointer value of -1. No "useful" data is actually
			//	sent, but the acknowledge part of the UDP packet is sent.
			
			thePacket = kPleaseSendAcknowledge;
		}
	}

	if(thePacket)
	{	haveToSendAck = false;
		nextAckTime = curTime + kAckRetransmitBase + retransmitTime;
		nextWriteTime = curTime + retransmitTime;

#if	DEBUG_AVARA
		if(thePacket == kPleaseSendAcknowledge)
		{	DebugEvent('A',-11);
		}
		else
		{	DebugEvent('S', thePacket->serialNumber);
		}
#endif
	}
	
	return	thePacket;
}

void	CUDPConnection::ValidatePacket(
	UDPPacketInfo	*thePacket,
	long			when)
{
	if(noErr == Dequeue((QElemPtr)thePacket, &queues[kTransmitQ]))
	{	long	roundTrip;
	
		roundTrip = when - thePacket->birthDate;

		if(maxValid == 4)
		{	if(roundTrip < roundTripTime)
			{	roundTripTime = roundTrip;
				pessimistTime = roundTrip;
				optimistTime = roundTrip;
				realRoundTrip = roundTrip;
			}
		}
		else
		{	long	difference;
		
			if(roundTrip > roundTripTime)
			{	pessimistTime = (	(pessimistTime << PESSIMISTSMOOTHFACTOR)
									- pessimistTime + roundTrip)
								>> PESSIMISTSMOOTHFACTOR;
			}
			else
			{	optimistTime = (	(optimistTime << PESSIMISTSMOOTHFACTOR)
									- optimistTime + roundTrip)
								>> PESSIMISTSMOOTHFACTOR;
			}

			roundTripTime = (	(roundTripTime << RTTSMOOTHFACTOR)
								- roundTripTime + roundTrip)
							>> RTTSMOOTHFACTOR;
			
			difference = roundTrip - optimistTime;

			if(difference <= ((optimistTime + deviation) >> 2))
			{
				realRoundTrip = ((realRoundTrip << RTTSMOOTHFACTOR)
									- realRoundTrip + roundTrip)
									>> RTTSMOOTHFACTOR;

			}

			if(difference < 0)	difference = -difference;

			if(difference <= optimistTime)
			{
				difference <<= 1;
				deviation = (	(deviation << DEVIATIONSMOOTHFACTOR)
									- deviation + difference)
								>> DEVIATIONSMOOTHFACTOR;
			}

			retransmitTime =  ((realRoundTrip*2 + roundTripTime) >> 1) + deviation;
			if(retransmitTime < itsOwner->urgentResendTime)
				retransmitTime = itsOwner->urgentResendTime;
			else
			if(retransmitTime > kMaxAllowedRetransmitTime)
				retransmitTime =  kMaxAllowedRetransmitTime;

		}

#if	DEBUG_AVARA
		if(thePacket->packet.command == kpKeyAndMouse)
		{	DebugEvent('/', thePacket->packet.p3);
		}
		DebugEvent('¥', thePacket->serialNumber);
#endif
		itsOwner->ReleasePacket((PacketInfo *)thePacket);
	}
}

void	CUDPConnection::RunValidate()
{
	UDPPacketInfo	*thePacket, *nextPacket;

	thePacket = (UDPPacketInfo *)queues[kTransmitQ].qHead;
	
	while(thePacket)
	{	nextPacket = (UDPPacketInfo *)thePacket->packet.qLink;
	
		if(thePacket->serialNumber - maxValid <= 0)
		{	ValidatePacket(thePacket, validTime);
		}
		
		thePacket = nextPacket;
	}
	
}

static	long	lastAckMap;

char *	CUDPConnection::ValidatePackets(
	char	*validateInfo,
	long	curTime)
{
	short			transmittedSerial;
	short			dummyStackVar;
	UDPPacketInfo	*thePacket, *nextPacket;

	transmittedSerial = *(short *)validateInfo;
	validateInfo += sizeof(short);

	if(transmittedSerial & 1)
	{	long	ackMap;

		transmittedSerial &= ~1;

		ackMap = *(long *)validateInfo;
		lastAckMap = ackMap;

		for(thePacket = (UDPPacketInfo *)queues[kTransmitQ].qHead;
			thePacket;
			thePacket = nextPacket)
		{	nextPacket = (UDPPacketInfo *)thePacket->packet.qLink;

			if(ackMap & (1 << (((thePacket->serialNumber - transmittedSerial) >> 1) - 1)))
			{	ValidatePacket(thePacket, curTime);
#if	DEBUG_AVARA
				DebugEvent('Æ', thePacket->serialNumber);
#endif
			}
		}
		
		validateInfo += sizeof(ackBitmap);
	}

	validTime = curTime;

	if(maxValid - transmittedSerial < 0)
	{	maxValid = transmittedSerial;
		RunValidate();
	}

	return validateInfo;
}

void	CUDPConnection::Dispose()
{
	FlushQueues();

	inherited::Dispose();
}

static	short	receiveSerialStorage[512];

void	CUDPConnection::ReceivedPacket(
	UDPPacketInfo	*thePacket)
{
	UDPPacketInfo	*pack;
	Boolean			changeInReceiveQueue = false;

#if	DEBUG_AVARA
	DebugEvent('R', thePacket->serialNumber);
#endif

//	thePacket->packet.sender = myId;

	haveToSendAck = true;

	if(thePacket->serialNumber - receiveSerial < 0)
	{	//	We already got this one, so just release it.
	
		itsOwner->ReleasePacket((PacketInfo *)thePacket);
	}
	else
	{	if(thePacket->serialNumber == receiveSerial)
		{	//	Packet was next in line to arrive, so it may release others from the received queue
			UDPPacketInfo	*nextPack;

#if	DEBUG_AVARA
			DebugEvent('%', thePacket->serialNumber);
#endif
			receiveSerial = thePacket->serialNumber + kSerialNumberStepSize;
			itsOwner->ReceivedGoodPacket(&thePacket->packet);

			for(	pack = (UDPPacketInfo *) queues[kReceiveQ].qHead;
					pack;pack = nextPack)
			{	nextPack = (UDPPacketInfo *)pack->packet.qLink;
			
				if(receiveSerial - pack->serialNumber >= 0)
				{	OSErr	theErr;
					
					theErr = Dequeue((QElemPtr)pack, &queues[kReceiveQ]);
					if(theErr == noErr)
					{	if(pack->serialNumber == receiveSerial)
						{
#if	DEBUG_AVARA
							DebugEvent('+', pack->serialNumber);
#endif
							receiveSerial = pack->serialNumber + kSerialNumberStepSize;
							itsOwner->ReceivedGoodPacket(&pack->packet);
						}
						else
						{
#if	DEBUG_AVARA
							DebugEvent('-', pack->serialNumber);
#endif
							itsOwner->ReleasePacket(&pack->packet);
						}
						
						changeInReceiveQueue = true;
						nextPack = (UDPPacketInfo *) queues[kReceiveQ].qHead;
					}
#if	DEBUG_AVARA
					else
					{	DebugStr("\pDequeue failed for received packet.");
					}
#endif
				}
			}
		}
		else
		{	//	We're obviously missing a packet somewhere...queue this one for later.
			short	receiveQueueLength = 0;

			for(	pack = (UDPPacketInfo *) queues[kReceiveQ].qHead;
					pack != NULL && pack->serialNumber != thePacket->serialNumber;
					pack = (UDPPacketInfo *)pack->packet.qLink)
			{	receiveQueueLength++;
			}
			
			if(pack)
			{	//	The queue already contains this packet.
#if	DEBUG_AVARA
				DebugEvent('D', pack->serialNumber);
#endif
				itsOwner->ReleasePacket((PacketInfo *)thePacket);
			}
			else
			{
#if	DEBUG_AVARA
				DebugEvent('Q', thePacket->serialNumber);
#endif
				changeInReceiveQueue = true;
				Enqueue((QElemPtr)thePacket, &queues[kReceiveQ]);
			}
		}
	}

	if(changeInReceiveQueue && offsetBufferBusy == NULL)
	{	short	dummyShort;
	
		offsetBufferBusy = &dummyShort;
		if(offsetBufferBusy == &dummyShort)
		{	ackBase = receiveSerial - kSerialNumberStepSize;

			if(queues[kReceiveQ].qHead)
			{
				ackBitmap = 0;
				for(	pack = (UDPPacketInfo *) queues[kReceiveQ].qHead;
						pack != NULL;
						pack = (UDPPacketInfo *)pack->packet.qLink)
				{	ackBitmap |= 1 << (((pack->serialNumber - ackBase) >> 1) - 1);
				}
				ackBase++;	
			}
			
			offsetBufferBusy = NULL;
		}
	}
	
	{	short	*receiveSerials;
		receiveSerials = receiveSerialStorage;
		for(	pack = (UDPPacketInfo *) queues[kReceiveQ].qHead;
				pack != NULL;
				pack = (UDPPacketInfo *)pack->packet.qLink)
		{	*receiveSerials++ = pack->serialNumber;
		}
		
		*receiveSerials++ = -12345;
	}
}

char *	CUDPConnection::WriteAcks(
register	char	*dest)
{
	short		*mainAck;
	
	mainAck = (short *)dest;
	dest += sizeof(short);
	*mainAck = receiveSerial - kSerialNumberStepSize;

	if(offsetBufferBusy == NULL && ackBase & 1)
	{	short	dummyShort;
		char	offset;

		offsetBufferBusy = &dummyShort;
		if(offsetBufferBusy == &dummyShort)
		{	char	*deltas;
			
			*mainAck = ackBase;
			*(long *)dest = ackBitmap;
			dest += sizeof(ackBitmap);
			offsetBufferBusy = NULL;
		}
	}
	
	return dest;
}

void	CUDPConnection::MarkOpenConnections(
	CompleteAddress	*table)
{
	short	i;
	
	if(next)	next->MarkOpenConnections(table);

	if(port && myId != 0)
	{	for(i=0;i<itsOwner->maxClients;i++)
		{	if(table->host == ipAddr &&
				table->port == port)
			{	table->host = 0;
				table->port = 0;
				return;
			}
			
			table++;
		}

		port = 0;
		ipAddr = 0;
		myId = -1;
		FlushQueues();
	}
}

void	CUDPConnection::OpenNewConnections(
	CompleteAddress	*table)
{
	short			i;
	CompleteAddress	*origTable = table;
	
	if(port == 0)
	{	for(i=1;i<=itsOwner->maxClients;i++)
		{	if(table->host  && table->port)
			{	myId = i;
				FreshClient(table->host, table->port, 0);
				table->host = 0;
				table->port = 0;
				break;
			}
			
			table++;
		}
	}

	if(next)
		next->OpenNewConnections(origTable);
}

void	CUDPConnection::FreshClient(
	long	remoteHost,
	short	remotePort,
	long	firstReceiveSerial)
{
	FlushQueues();
	serialNumber = 0;	
	receiveSerial = firstReceiveSerial;
	
	maxValid = -kSerialNumberStepSize;
	retransmitTime = kInitialRetransmitTime;
	roundTripTime = kInitialRoundTripTime;
	pessimistTime = roundTripTime;
	optimistTime = roundTripTime;
	realRoundTrip = roundTripTime;
	deviation = roundTripTime;
	
	cramData = 0;

	killed = false;

	quota = 0;
	validTime = itsOwner->GetClock();
	nextWriteTime = validTime + kAckRetransmitBase;
	nextAckTime = validTime + kAckRetransmitBase;
	haveToSendAck = true;

	ipAddr = remoteHost;
	port = remotePort;	
}

Boolean	CUDPConnection::AreYouDone()
{
	if(queues[kReceiveQ].qHead)
	{	return false;
	}
	else
	{	if(next)	return next->AreYouDone();
		else		return true;
	}
}

void	CUDPConnection::CloseSlot(
	short	theId)
{
	if(port && myId == theId)
	{	routingMask = 0;
		myId = -1;
		ipAddr = 0;
		port = 0;
		FlushQueues();
	}
	else
	if(next)	next->CloseSlot(theId);		
}

void	CUDPConnection::ErrorKill()
{
	if(!killed)
	{	killed = true;
		routingMask = 0;

		if(myId == 0)	//	Connection to server fails
			itsOwner->SendPacket(1 << itsOwner->myId, kpPacketProtocolLogout, itsOwner->myId,0,0, 0, NULL);
		else
			itsOwner->SendPacket(kdServerOnly, kpPacketProtocolLogout, myId,0,0, 0,NULL);
	}
}

void	CUDPConnection::ReceiveControlPacket(
	PacketInfo	*thePacket)
{
	switch(thePacket->p1)
	{	case udpCramInfo:
			if(thePacket->sender == myId)
			{	cramData = thePacket->p2;
			}
			else if(next)
			{	next->ReceiveControlPacket(thePacket);
			}
			break;
	}
}

void	CUDPConnection::GetConnectionStatus(
	short				slot,
	UDPConnectionStatus	*parms)
{
	if(slot == myId)
	{	parms->hostIP = ipAddr;
		parms->estimatedRoundTrip = ((realRoundTrip << 9) + 256) / 125;
		parms->averageRoundTrip = ((roundTripTime << 9) + 256) / 125;
		parms->pessimistRoundTrip = ((pessimistTime << 9) + 256) / 125;
		parms->optimistRoundTrip = ((optimistTime << 9) + 256) / 125;
		parms->connectionType = cramData;
	}
	else
	{	if(next)	next->GetConnectionStatus(slot, parms);
	}
}