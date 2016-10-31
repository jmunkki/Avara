/*
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: CTCPComm.c
    Created: Sunday, January 28, 1996, 8:51
    Modified: Sunday, January 28, 1996, 19:39
*/

#include "CTCPComm.h"
#include "CTCPConnection.h"
#include "AvaraTCP.h"
#include "CEventHandler.h"
#include "CCompactTagBase.h"

#define	kTCPConnectDialogId	410
#define	kTCPServerDialogId	411
#define	kTCPPopStrings		410

#define	kTCPCommPrefsTag	'tcpT'

#define	kDefaultHostTag		'host'
#define	kDefaultPortTag		'port'

#define	kDefaultServerPortTag 'sprt'

#define	kHotListTag			'hotL'
#define	kMaxHotTags			32
#define	kDefaultTCPPort		19567
#define	kHotPopMenu			19567

typedef struct
{	long	portNumber;
	OSType	addressTag;
}	hotListItem;

enum	{	kClientConnectItem = 1,
			kClientCloseItem,
			kClientAddressTextItem,
			kClientPortTextItem,
			kClientPasswordTextItem,
			kClientHotPopupItem
		};
		
enum	{	kServerStartItem = 1,
			kServerCancelItem,
			kServerPortTextItem,
			kServerPasswordTextItem
		};

void	CTCPComm::ITCPComm(
	short	clientCount,
	short	bufferCount)
{
	OSErr	theErr;

	ICommManager(bufferCount);
	
	prefs = new CTagBase;
	prefs->ITagBase();
	prefs->ConvertFromHandle(gApplication->prefsBase->ReadHandle(kTCPCommPrefsTag));
	isConnected = false;
	myId = -1;
	
	connections = NULL;

	theErr = OpenAvaraTCP();
	if(theErr == noErr)
	{	while(clientCount--)
		{	CTCPConnection	*newConn;
		
			newConn = new CTCPConnection;
			newConn->ITCPConnection(this);
			newConn->next = connections;
			connections = newConn;
		}
	}
}

OSErr	CTCPComm::AllocatePacketBuffers()
{
	TCPPacketInfo *	pp;
	OSErr			theErr;
	short			packetSpace = packetBufCount;

	packetBuffers = (PacketInfo *) NewPtr(sizeof(TCPPacketInfo) * packetSpace);
	theErr = MemError();

	pp = (TCPPacketInfo *)packetBuffers;

	while(packetSpace--)
	{	Enqueue((QElemPtr) pp, &freeQ);
		pp++;
	}

	return theErr;
}


void	CTCPComm::Dispose()
{
	CTCPConnection	*nextConn;
	
	if(prefs)
	{	Handle	compactPrefs;
	
		compactPrefs = prefs->ConvertToHandle();
		gApplication->prefsBase->WriteHandle(kTCPCommPrefsTag, compactPrefs);

		DisposeHandle(compactPrefs);

		prefs->Dispose();
		prefs = NULL;
	}
	
	while(connections)
	{	nextConn = connections->next;
	
		connections->Dispose();
		connections = nextConn;
	}
	
	inherited::Dispose();
}

pascal
Boolean	TCPDialogFilter(
	DialogPtr	theDialog,
	EventRecord	*theEvent,
	short		*itemHit)
{
	Rect	iRect;
	Handle	iHandle;
	short	iType;
	GrafPtr	saved;
	Boolean	didHandle = false;
	short	doHilite = 0;

	GetPort(&saved);
	SetPort(theDialog);

	switch(theEvent->what)
	{	case updateEvt:
			if(theDialog == (DialogPtr)theEvent->message)
			{	GetDItem(theDialog, 1, &iType, &iHandle, &iRect);
				PenSize(3,3);
				InsetRect(&iRect, -4, -4);
				FrameRoundRect(&iRect, 16, 16);
				PenSize(1,1);
			}
			else
			{	gApplication->ExternalEvent(theEvent);
			}
			break;
		case keyDown:
			{	char	theChar;
			
				theChar = theEvent->message;
				if(theChar == 13 || theChar == 3)
				{	*itemHit = 1;
					didHandle = true;
					doHilite = 1;
				}
				else if(theChar == 27 ||
					 (theChar == '.' && (theEvent->modifiers & cmdKey)))
				{	*itemHit = 2;
					doHilite = 2;
					didHandle = true;
				}
				else if(((DialogPeek)theDialog)->editField == kClientPortTextItem-1)
				{	if(!(theChar == 8 || (theChar >= '0' && theChar <= '9')
						|| (theChar > 27 && theChar <= 31)
						|| theChar == 9))
					{	didHandle = true;
					}
				}
			}
			break;
	}
	
	if(doHilite)
	{	ControlHandle	theControl;
		long			finalTick;
	
		GetDItem(theDialog, doHilite, &iType, (Handle *)&theControl, &iRect);
		HiliteControl(theControl, 1);
		Delay(3, &finalTick);
		HiliteControl(theControl, 0);
	}

	SetPort(saved);
	return didHandle;
}

void	CTCPComm::DoHotPop(
	DialogPtr	theDialog,
	Rect		*popRect)
{
	MenuHandle	hotPopper;
	char		emptyPascal = 0;
	Str255		tempString;
	Point		popPosition;
	long		popResult;
	
	hotPopper = NewMenu(kHotPopMenu, (StringPtr)&emptyPascal);

	GetIndString(tempString, kTCPPopStrings, 1);
	AppendMenu(hotPopper, tempString);
	GetIndString(tempString, kTCPPopStrings, 2);
	AppendMenu(hotPopper, tempString);
	GetIndString(tempString, kTCPPopStrings, 3);
	AppendMenu(hotPopper, tempString);
	
	InsertMenu(hotPopper, hierMenu);
	
	popPosition.h = popRect->left;
	popPosition.v = popRect->top;
	LocalToGlobal(&popPosition);
	popResult = PopUpMenuSelect(hotPopper, popPosition.v, popPosition.h, 1);
	
	DeleteMenu(kHotPopMenu);
	DisposeMenu(hotPopper);
}

void	CTCPComm::ContactServer(
	ip_addr		addr,
	short		port,
	StringPtr	password)
{
	OSErr	theErr;

	theErr = connections->ContactServer(addr, port, password);
}

void	CTCPComm::Connect()
{
	DialogPtr		clientDialog;
	short			itemHit;
	ModalFilterUPP	myFilter;
	Str255			tempString;
	Str255			hostName;
	Rect			iRect,popRect;
	Handle			iHandle;
	Handle			hostHandle;
	Handle			portHandle;
	Handle			passHandle;
	short			iType;
	long			thePortNumber;
	
	if(connections)
	{	myFilter = NewModalFilterProc(TCPDialogFilter);
	
		clientDialog = GetNewDialog(kTCPConnectDialogId, 0, (WindowPtr)-1);
		SetPort(clientDialog);
	
		GetDItem(clientDialog, kClientHotPopupItem, &iType, &iHandle, &popRect);
		GetDItem(clientDialog, kClientAddressTextItem, &iType, &hostHandle, &iRect);
		GetDItem(clientDialog, kClientPortTextItem, &iType, &portHandle, &iRect);
		GetDItem(clientDialog, kClientPasswordTextItem, &iType, &passHandle, &iRect);
	
		prefs->ReadString(kDefaultHostTag, tempString);
		if(tempString[0])
		{	SetIText(hostHandle, tempString);
		}
		
		NumToString(prefs->ReadLong(kDefaultPortTag, kDefaultTCPPort), tempString);
		SetIText(portHandle, tempString);
		
		do
		{	ModalDialog(myFilter, &itemHit);
			switch(itemHit)
			{	case kClientConnectItem:
				case kClientCloseItem:
	
				case kClientAddressTextItem:
				case kClientPortTextItem:
				case kClientPasswordTextItem:
					break;
				case kClientHotPopupItem:
					DoHotPop(clientDialog, &popRect);
					break;
			}
		} while(itemHit != kClientConnectItem && itemHit != kClientCloseItem);
		
		GetIText(hostHandle, hostName);
		prefs->WriteString(kDefaultHostTag, hostName);
		
		GetIText(portHandle, tempString);
		if(tempString[0])
		{	StringToNum(tempString, &thePortNumber);
		}
		else
		{	thePortNumber = kDefaultTCPPort;
		}
		
		prefs->WriteLong(kDefaultPortTag, thePortNumber);
	
		if(itemHit == kClientConnectItem)
		{	ip_addr		addr;
			OSErr		theErr;
		
			theErr = PascalStringToAddress(hostName, &addr);
			if(theErr == noErr)
			{	GetIText(passHandle, tempString);
				if(tempString[0] == 0)
				{	tempString[0] = 1;
					tempString[1] = '?';
				}
				ContactServer(addr, thePortNumber, tempString);
			}
		}
	
		DisposeDialog(clientDialog);
		DisposeRoutineDescriptor(myFilter);
	}
	else
	{	SysBeep(10);
	}
}

void	CTCPComm::StartServing()
{
	DialogPtr		serverDialog;
	short			itemHit;
	ModalFilterUPP	myFilter;
	Str255			tempString;
	Rect			iRect,popRect;
	Handle			portHandle;
	short			iType;
	long			thePortNumber;
	
	myFilter = NewModalFilterProc(TCPDialogFilter);

	serverDialog = GetNewDialog(kTCPServerDialogId, 0, (WindowPtr)-1);
	SetPort(serverDialog);

	GetDItem(serverDialog, kServerPortTextItem, &iType, &portHandle, &iRect);

	NumToString(prefs->ReadLong(kDefaultServerPortTag, kDefaultTCPPort), tempString);
	SetIText(portHandle, tempString);
	
	do
	{	ModalDialog(myFilter, &itemHit);

#ifdef NEEDSWITCH
		switch(itemHit)
		{	case kServerStartItem:
			case kServerCancelItem:
			case kServerPortTextItem:
			case kServerPasswordTextItem:
				break;
		}
#endif

	} while(itemHit != kClientConnectItem && itemHit != kClientCloseItem);
	
	GetIText(portHandle, tempString);
	if(tempString[0])
	{	StringToNum(tempString, &thePortNumber);
	}
	else
	{	thePortNumber = kDefaultTCPPort;
	}
	
	prefs->WriteLong(kDefaultPortTag, thePortNumber);

	DisposeDialog(serverDialog);
	DisposeRoutineDescriptor(myFilter);

}