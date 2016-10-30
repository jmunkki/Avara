/*/
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CCommChannel.c
    Created: Monday, February 27, 1995, 23:17
    Modified: Thursday, February 22, 1996, 9:10
/*/

#include "CCommChannel.h"
#include "CommDefs.h"

#define	kMaxWriteQueueLength	10

void	CCommChannel::ICommChannel(
	short packetSpace)
{
	CAbstractCommClient::ICommClient(packetSpace);

	sessionRef = 0;
	writePack = NULL;
	writeActive = false;

	writeCount = 0;		//	Used to monitor how many packets have been queued.
	writtenCount = 0;	//	Used to monitor how many queued packets have been sent.
	
	readPack = NULL;
	isClosing = false;
	outQ.qFlags = 0;
	outQ.qHead = 0;
	outQ.qTail = 0;
	isClosing = false;
}

/*
**	For some reason, the outgoing queue has to
**	be emptied without sending the packets.
**
**	Usually called after a connection fails or closes.
*/
void	CCommChannel::FlushOutQueue()
{
	PacketInfo	*thePack;
	
	do
	{	thePack = (PacketInfo *) outQ.qHead;
		if(thePack)
		{	if(Dequeue((QElemPtr) thePack, &outQ))
			{	break;
			}
			else
			{	ReleasePacket(thePack);
			}
		}
	} while(thePack);
}

/*
**	Release used storage.
*/
void	CCommChannel::Dispose()
{
	FlushOutQueue();
	
	inherited::Dispose();
}

/*
**	Helper function that simply calls our method.
*/
pascal
void	CommChannelReadFilter(
	ReadEnvelope	*readP)
{
	((CCommChannel *)readP->t)->ReadComplete();
}

/*
**	Start an asynchronous read operation.
*/
void	CCommChannel::AsyncRead()
{
	readPack = GetPacket();

	if(readPack)
	{	readE.r.ioCompletion = (PPCCompProcPtr)CommChannelReadFilter;
		readE.r.sessRefNum = sessionRef;
		readE.r.bufferLength = PACKETDATABUFFERSIZE;
		readE.r.bufferPtr = readPack->dataBuffer;
		readE.t = this;
		
		PPCRead(&readE.r, true);
	}
}

/*
**	A read operation has completed. Check for errors
**	and handle the packet. Call AsyncRead if necessary
**	so that a read is always pending.
*/
void	CCommChannel::ReadComplete()
{
	PacketInfo	*wasRead;
	
	if(readE.r.ioResult == noErr)
	{	readPack->dataLen = readE.r.actualLength;
		readPack->p3 = readE.r.userData;
		readPack->p2 = ((short *)&readE.r.blockType)[1];
		readPack->p1 = ((char *)&readE.r.blockType)[1];
		readPack->command = ((char *)&readE.r.blockType)[0];

		readPack->sender = ((short *)&readE.r.blockCreator)[0];
		readPack->distribution = ((short *)&readE.r.blockCreator)[1];

		wasRead = readPack;

		AsyncRead();
		
		DispatchPacket(wasRead);
		ReleasePacket(wasRead);
	}
	else
	{	ReadErr();
	}
}

/*
**	Read reported a problem. Handle it (by dispatching an error packet).
*/
void	CCommChannel::ReadErr()
{
	if(!isClosing)
	{	readPack->sender = myId;
		readPack->distribution = kdEveryone;
		readPack->p2 = readE.r.ioResult;
		readPack->p1 = 0;	//	Read error
		readPack->dataLen = 0;
		
		if(readPack->p2 == sessClosedErr)
		{	readPack->command = kpDisconnect;
		}
		else
		{	readPack->command = kpError;
		}
		DispatchPacket(readPack);
	}

	ReleasePacket(readPack);
}

/*
**	Helper function for write completion method.
*/
pascal
void	CommChannelWriteFilter(
	WriteEnvelope	*writeP)
{
	((CCommChannel *)writeP->t)->WriteComplete();
}

/*
**	A write has completed. If successful, see if more
**	data has to be written.
*/
void	CCommChannel::WriteComplete()
{
	ReleasePacket(writePack);

	if(writeE.r.ioResult == noErr)
	{	writtenCount++;
		AsyncWrite();
	}
	else
	{	writeActive = false;
		WriteErr();
	}
}

/*
**	Handle write errors (by dispatching an error packet).
*/
void	CCommChannel::WriteErr()
{
#define	DO_NOT_MESSAGE_WRITE_ERRORS
#ifdef	MESSAGE_WRITE_ERRORS
	PacketInfo	*errPack;
	
	if(!isClosing)
	{	errPack = GetPacket();
		if(errPack)		//	Otherwise we are in deep sh*t
		{	errPack->sender = myId;
			errPack->distribution = kdEveryone;
			errPack->p2 = writeE.r.ioResult;
			errPack->dataLen = 0;
			if(errPack->p2 == sessClosedErr)
			{	errPack->command = kpDisconnect;
			}
			else
			{	errPack->command = kpError;
			}
	
			errPack->p1 = 1;
			DispatchPacket(errPack);
			ReleasePacket(errPack);
		}
	}
#endif
}

/*
**	Write data, if there are packets in the outgoing
**	queue.
*/
void	CCommChannel::AsyncWrite()
{
	writePack = (PacketInfo *)outQ.qHead;

	if(writePack)
	{	Dequeue((QElemPtr) writePack, &outQ);

		writeE.r.userData = writePack->p3;
		((char *)&writeE.r.blockType)[0] = writePack->command;
		((char *)&writeE.r.blockType)[1] = writePack->p1;
		((short *)&writeE.r.blockType)[1] = writePack->p2;

		((short *)&writeE.r.blockCreator)[0] = writePack->sender;
		((short *)&writeE.r.blockCreator)[1] = writePack->distribution;
		writeE.r.ioCompletion = (PPCCompProcPtr)CommChannelWriteFilter;
		writeE.r.sessRefNum = sessionRef;
		writeE.r.bufferLength = writePack->dataLen;
		writeE.r.bufferPtr = writePack->dataBuffer;
		writeE.r.more = false;
		writeE.t = this;
	
		writeActive = true;
		PPCWrite(&writeE.r, true);
	}
	else
	{	writeActive = false;
	}
}

/*
**	Send a packet of data. (Queue it into the outgoing queue
**	and then call AsyncWrite, if no write is currently active.)
*/
void	CCommChannel::WritePacket(PacketInfo *thePacket)
{
	writeCount++;
	
	if(writeCount-writtenCount > kMaxWriteQueueLength)
	{	thePacket->command = kpKillConnection;
		thePacket->distribution = GetKillReceivers();
		thePacket->p1 = myId;
		DispatchPacket(thePacket);
	}
	else
	{	Enqueue((QElemPtr) thePacket, &outQ);
		if(!writeActive)
		{	writeCount = 1;
			writtenCount = 0;
			AsyncWrite();
		}
	}
}

short	CCommChannel::GetKillReceivers()
{
	return 1<<myId;
}