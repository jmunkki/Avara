/*
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CPlayerManager.c
    Created: Saturday, March 11, 1995, 5:50
    Modified: Sunday, September 15, 1996, 20:51
*/

#include "CPlayerManager.h"
#include "CNetManager.h"
#include "CCommManager.h"
#include "CommDefs.h"

#include "CAbstractPlayer.h"
#include "CWalkerActor.h"
#include "CIncarnator.h"

#include "CRosterWindow.h"
#include "JAMUtil.h"
#include "CEventHandler.h"
#include "CommandList.h"
#include "CInfoPanel.h"
#include "InfoMessages.h"

void	CPlayerManager::IPlayerManager(
	CAvaraGame		*theGame,
	short			id,
	CNetManager		*aNetManager,
	CRosterWindow	*aRoster)
{
	Rect	*mainScreenRect;

	itsGame = theGame;
	itsPlayer = NULL;
	slot = id;

	mainScreenRect = &(*GetMainDevice())->gdRect;
	mouseCenterPosition.h = (mainScreenRect->left + mainScreenRect->right) / 2;
	mouseCenterPosition.v = (mainScreenRect->top + mainScreenRect->bottom) / 2;
	oldMouse = mouseCenterPosition;

	theRoster = aRoster;
	theNetManager = aNetManager;
	levelCRC = 0;
	levelTag = 0;
	position = id;
	itsPlayer = NULL;
	resetMousePeriod = 0;

	mugPict = NULL;
	mugSize = -1;
	mugState = 0;
	
	NetDisconnect();
	isLocalPlayer = false;	
}

void	CPlayerManager::SetPlayer(
	CAbstractPlayer	*thePlayer)
{
	itsPlayer = thePlayer;
}

static
unsigned	long	lastMouseControlTime;
#if 0

void	CPlayerManager::DoMouseControl(
	Point	*deltaMouse,
	Boolean	doCenter)
{
	if(doCenter)
	{	Point			thisMouse;
		unsigned long	thisTime;

		if(itsGame->resetMouse || (resetMousePeriod++ & 2047) == 0)
		{	itsGame->resetMouse = false;
			RawMouse = mouseCenterPosition;
			MTemp = mouseCenterPosition;
			CrsrNewCouple = -1;
			Delay(1, (long *)&thisTime);
			GetMouse(&oldMouse);
			LocalToGlobal(&oldMouse);
		}

		thisTime = TickCount();
		if(thisTime - lastMouseControlTime > 1)
		{	//	thisMouse = Mouse;
			GetMouse(&thisMouse);
			LocalToGlobal(&thisMouse);
			deltaMouse->h = thisMouse.h - oldMouse.h;
			deltaMouse->v = thisMouse.v - oldMouse.v;
			oldMouse = thisMouse;
			
			lastMouseControlTime = thisTime;
		//	RawMouse = mouseCenterPosition;
		//	oldMouse = mouseCenterPosition;
		//	MTemp = mouseCenterPosition;
		//	CrsrNew = CrsrCouple;
		//	CrsrNewCouple = -1;
			
		//	GetMouse(&oldMouse);
		}
		else
		{	deltaMouse->h = 0;
			deltaMouse->v = 0;
		}

		deltaMouse->h <<= itsGame->sensitivity + 1;
		deltaMouse->v <<= itsGame->sensitivity + 1;
	}
	else
	{	Point	newMouse;
	
		oldMouse = mouseCenterPosition;
		GetMouse(&newMouse);
		LocalToGlobal(&newMouse);
		deltaMouse->h = (newMouse.h - oldMouse.h) << (itsGame->sensitivity + 1);
		deltaMouse->v = (newMouse.v - oldMouse.v) << (itsGame->sensitivity + 1);
		deltaMouse->h /= 5;
		deltaMouse->v /= 5;
		oldMouse = newMouse;
	}
	
	if(itsGame->moJoOptions & kFlipAxis)
	{	deltaMouse->v = -deltaMouse->v;
	}
}
#else
#include "CursorDevices.h"

void	CPlayerManager::DoMouseControl(
	Point	*deltaMouse,
	Boolean	doCenter)
{
	if(doCenter)
	{	Point			thisMouse;
		unsigned long	thisTime;

		thisTime = TickCount();
		if(thisTime - lastMouseControlTime > 1)
		{	CursorDevicePtr		curs;
			Point				centerDelta;
			static long			historical;
		
			GetMouse(&thisMouse);
			LocalToGlobal(&thisMouse);
			deltaMouse->h = thisMouse.h - oldMouse.h;
			deltaMouse->v = thisMouse.v - oldMouse.v;
			oldMouse = thisMouse;
			
			historical = (historical >> 1) + (((deltaMouse->h >= 0 ? deltaMouse->h : -deltaMouse->h)
										     + (deltaMouse->v >= 0 ? deltaMouse->v : -deltaMouse->v)) << 6);
			
			centerDelta.h = mouseCenterPosition.h - thisMouse.h;
			if(centerDelta.h < 0)
				centerDelta.h = -centerDelta.h;
				
			centerDelta.v = mouseCenterPosition.v - thisMouse.v;
			if(centerDelta.v < 0)
				centerDelta.v = -centerDelta.v;
			
			curs = 0;
			if(//	   deltaMouse->h == 0
				//&& deltaMouse->v == 0
				//&&
				 !(centerDelta.h < 180 && centerDelta.v < 180)
				&& (historical == 0) // || centerDelta.h > (mouseCenterPosition.h >> 1) || centerDelta.v > (mouseCenterPosition.v >> 1))
				&& noErr == CursorDeviceNextDevice(&curs))
			{
				historical = 0x100000;
				CursorDeviceMoveTo(curs, mouseCenterPosition.h, mouseCenterPosition.v);
				oldMouse = mouseCenterPosition;
				//	GetMouse(&oldMouse);
			}

			lastMouseControlTime = thisTime;
			
		}
		else
		{	deltaMouse->h = 0;
			deltaMouse->v = 0;
		}

		deltaMouse->h <<= itsGame->sensitivity + 1;
		deltaMouse->v <<= itsGame->sensitivity + 1;
	}
	else
	{	Point	newMouse;
	
		oldMouse = mouseCenterPosition;
		GetMouse(&newMouse);
		LocalToGlobal(&newMouse);
		deltaMouse->h = (newMouse.h - oldMouse.h) << (itsGame->sensitivity + 1);
		deltaMouse->v = (newMouse.v - oldMouse.v) << (itsGame->sensitivity + 1);
		deltaMouse->h /= 5;
		deltaMouse->v /= 5;
		oldMouse = newMouse;
	}
	
	if(itsGame->moJoOptions & kFlipAxis)
	{	deltaMouse->v = -deltaMouse->v;
	}
}

#endif
Boolean	CPlayerManager::TestHeldKey(
	short	funcCode)
{
	long			m0, m1;
	KeyMap			keyMap;
	char			*keyMapP;
	long			*mapPtr, *keyFun;
	short			i,j;
	Handle			mapRes;

	m0 = 0x80000000L >> funcCode;
	m1 = 0x80000000L >> (funcCode - 32);

	mapRes = itsGame->mapRes;
	mapPtr = (long *)*mapRes;
	keyMapP = (Ptr)keyMap;
	GetKeys(keyMap);
	
	for(i=0;i<128;i+=8)
	{	char	bits;
	
		bits = *keyMapP++;
		if(bits)
		{	j = 8;
			while(j-- && bits)
			{	if(bits < 0)
				{	keyFun = &mapPtr[2*(i+j)];
					if((keyFun[0] & m0) || (keyFun[1] & m1))
						return true;
				}
				bits += bits;
			}
		}
	}
	
	return false;
}

#ifdef USES_KEYMAP_INSTEAD_OF_EVENTS
void	CPlayerManager::AnalyzeKeys(
	EventRecord 	*theEvent,
	FunctionTable	*d)
{
	KeyMap			keyMap;
	char			*keyMapP;
	long			*mapPtr, *keyFun;
	short			i,j;
	Handle			mapRes;

	mapRes = itsGame->mapRes;
	HLock(mapRes);
	mapPtr = (long *)*mapRes;
	
	d->down[0] = d->down[1] = 0;
	d->up[0] = d->up[1] = 0;
	d->rept[0] = d->rept[1] = 0;
	d->held[0] = d->held[1] = 0;
	
	switch(theEvent->what)
	{
		case autoKey:
			keyFun = &mapPtr[(theEvent->message & 0xFF00) >> 7];
			d->rept[0] = keyFun[0];
			d->rept[1] = keyFun[1];
			break;

		case keyDown:
			keyFun = &mapPtr[(theEvent->message & 0xFF00) >> 7];
			d->down[0] = keyFun[0];
			d->down[1] = keyFun[1];
			break;

		case keyUp:
			keyFun = &mapPtr[(theEvent->message & 0xFF00) >> 7];
			d->up[0] = keyFun[0];
			d->up[1] = keyFun[1];
			break;
	}
	
	keyMapP = (Ptr)keyMap;
	GetKeys(keyMap);
	
	for(i=0;i<128;i+=8)
	{	char	bits;
	
		bits = *keyMapP++;
		if(bits)
		{	j = 8;
			while(j-- && bits)
			{	if(bits < 0)
				{	keyFun = &mapPtr[2*(i+j)];
					d->held[0] |= keyFun[0];
					d->held[1] |= keyFun[1];
				}
				bits += bits;
			}
		}
	}

	for(i=0;i<2;i++)
	{	d->down[i] |= d->held[i] & (~oldHeld[i]);
		d->up[i] |= (~d->held[i]) & oldHeld[i];
	}
	
	oldHeld[0] = d->held[0];
	oldHeld[1] = d->held[1];

	HUnlock(mapRes);

	d->buttonStatus = 0;
	if(!(theEvent->modifiers & btnState))	d->buttonStatus |= kbuIsDown;
	if(theEvent->what == mouseDown)			d->buttonStatus |= kbuWentDown;
	else
	if(theEvent->what == mouseUp)			d->buttonStatus |= kbuWentUp;

	DoMouseControl(&d->mouseDelta, !(itsGame->moJoOptions & kJoystickMode));
	
}
#endif

void	FrameFunctionToPacket(FrameFunction	*ff, PacketInfo *outPacket, short slot);

void	FrameFunctionToPacket(
	FrameFunction	*ff,
	PacketInfo		*outPacket,
	short			slot)
{
	FunctionTable	*ft;
	long			*pd;
	short			*spd;

	ft = &ff->ft;

	outPacket->p1 = ft->buttonStatus | kDataFlagMask;
	outPacket->p2 = ff->ft.msgChar;
	outPacket->p3 = ff->validFrame;

	pd = (long *)outPacket->dataBuffer;

	if(ft->down[0])
	{	outPacket->p1 &= ~kNoDownData;
		*pd++ =	ft->down[0];
	}
	if(ft->up[0])
	{	outPacket->p1 &= ~kNoUpData;
		*pd++ =	ft->up[0];
	}
	if(ft->held[0])
	{	outPacket->p1 &= ~kNoHeldData;
		*pd++ =	ft->held[0];
	}
	spd = (short *)pd;

	if(ft->mouseDelta.v)
	{	outPacket->p1 &= ~kNoMouseV;
		*spd++ =	ft->mouseDelta.v;
	}

	if(ft->mouseDelta.h)
	{	outPacket->p1 &= ~kNoMouseH;
		*spd++ =	ft->mouseDelta.h;
	}

	outPacket->dataLen = ((char *)spd) - outPacket->dataBuffer;
}

#define	AVARA_EVENT_MASK	(mDownMask+mUpMask+keyDownMask+keyUpMask+autoKeyMask)

void	CPlayerManager::EventHandler()
{
	EventRecord 	theEvent;
	short			i,j;
	CCommManager	*theComm;
	PacketInfo		*outPacket;
	FrameFunction	*ff;
	long			upModifiers, downModifiers;

	long			*mapPtr, *keyFun;
	Handle			mapRes;

	mapRes = itsGame->mapRes;
	HLock(mapRes);
	mapPtr = (long *)*mapRes;

	itsGame->topSentFrame++;
	ff = &frameFuncs[(FUNCTIONBUFFERS - 1) & itsGame->topSentFrame];

	ff->validFrame = itsGame->topSentFrame;

	if(itsGame->allowBackgroundProcessing && (itsGame->topSentFrame & 7) == 0)
		GetNextEvent(0, &theEvent);
	else
		SystemTask();

	ff->ft.down[0] = 0;
	ff->ft.down[1] = 0;
	ff->ft.up[0] = 0;
	ff->ft.up[1] = 0;
	
	if(keyboardActive)
	{	ff->ft.held[0] = 0;
		ff->ft.held[1] = 0;
	}
	else
	{	ff->ft.held[0] = oldHeld[0];
		ff->ft.held[1] = oldHeld[1];
	}

	ff->ft.buttonStatus = 0;

	do
	{			long	key[2];
				long	*modKey;
				short	downBits;
				short	upBits;
	
		GetOSEvent(AVARA_EVENT_MASK, &theEvent);

		keyFun = &mapPtr[(theEvent.message & 0xFF00) >> 7];
		key[0] = keyFun[0];
		key[1] = keyFun[1];

		downBits = ~oldModifiers & theEvent.modifiers;
		upBits = oldModifiers & ~theEvent.modifiers;
		oldModifiers = theEvent.modifiers;

		modKey = &mapPtr[0x37 * 2];
		for(i=cmdKey;i<=controlKey;i+=i)
		{	if(downBits & i)
			{	if(!keyboardActive)
				{	ff->ft.down[0] |= modKey[0];	ff->ft.down[1] |= modKey[1];
					ff->ft.held[0] |= modKey[0];	ff->ft.held[1] |= modKey[1];
				}

				if(TESTFUNC(kfuTypeText, modKey))
				{	keyboardActive = !keyboardActive;
					ff->ft.down[0] |= modKey[0];	ff->ft.down[1] |= modKey[1];
					ff->ft.held[0] |= modKey[0];	ff->ft.held[1] |= modKey[1];
				}

				oldHeld[0] |= modKey[0];	oldHeld[1] |= modKey[1];
			}

			if(upBits & i)
			{	if(!keyboardActive)
				{	ff->ft.up[0] |= modKey[0];		ff->ft.up[1] |= modKey[1];
					ff->ft.held[0] &= ~modKey[0];	ff->ft.up[1] &= ~modKey[1];
				}

				oldHeld[0] &= ~modKey[0];		oldHeld[1] &= ~modKey[1];
			}
			
			modKey += 2;
		}

		switch(theEvent.what)
		{	case autoKey:
				if(!(key[0] || key[1]) || keyboardActive)
				{	inputBuffer[bufferEnd++] = theEvent.message;
					bufferEnd &= INPUTBUFFERMASK;
				}
				break;
	
			case keyDown:
				if(!keyboardActive)
				{	ff->ft.down[0] |= key[0];	ff->ft.down[1] |= key[1];
					ff->ft.held[0] |= key[0];	ff->ft.held[1] |= key[1];
				}

				oldHeld[0] |= key[0];	oldHeld[1] |= key[1];

				if(keyboardActive)
				{	if(TESTFUNC(kfuTypeText, keyFun))
					{	keyboardActive = false;
						ff->ft.down[0] |= key[0];	ff->ft.down[1] |= key[1];
						ff->ft.held[0] |= key[0];	ff->ft.held[1] |= key[1];
					}
					else
					{	inputBuffer[bufferEnd++] = theEvent.message;
						bufferEnd &= INPUTBUFFERMASK;
					}
				}
				else
				{	if(!(key[0] || key[1]))
					{	inputBuffer[bufferEnd++] = theEvent.message;
						bufferEnd &= INPUTBUFFERMASK;
					}

					if(TESTFUNC(kfuTypeText, keyFun))
					{	keyboardActive = true;
						ff->ft.down[0] |= key[0];	ff->ft.down[1] |= key[1];
					}
				}
				break;

			case keyUp:
				if(!keyboardActive)
				{	ff->ft.up[0] |= key[0];		ff->ft.up[1] |= key[1];
					ff->ft.held[0] &= ~key[0];	ff->ft.up[1] &= ~key[1];
				}

				oldHeld[0] &= ~key[0];		oldHeld[1] &= ~key[1];
				break;

			case mouseDown:
				ff->ft.buttonStatus |= kbuWentDown;
				break;

			case mouseUp:
				ff->ft.buttonStatus |= kbuWentUp;
				break;
		}
	} while(theEvent.what);
	
	if(bufferEnd != bufferStart)
	{	ff->ft.msgChar = inputBuffer[bufferStart++];
		bufferStart &= INPUTBUFFERMASK;
	}
	else
	{	ff->ft.msgChar = 0;
	}

	DoMouseControl(&ff->ft.mouseDelta, !(itsGame->moJoOptions & kJoystickMode));

	if(!(theEvent.modifiers & btnState))
		ff->ft.buttonStatus |= kbuIsDown;

	theComm = theNetManager->itsCommManager;
	
	outPacket = theComm->GetPacket();
	if(outPacket)
	{	long	*pd;
		
		outPacket->command = kpKeyAndMouse;
		outPacket->distribution = theNetManager->activePlayersDistribution;
		
		FrameFunctionToPacket(ff, outPacket, slot);

		if(ff->ft.msgChar)
		{	short	othersDistribution;
			
			othersDistribution = kdEveryone & ~theNetManager->activePlayersDistribution;
			theNetManager->FlushMessageBuffer();
			theComm->SendPacket(othersDistribution, kpRosterMessage, 0,0,0, 1, &ff->ft.msgChar);
		}
		
		theNetManager->FastTrackDeliver(outPacket);
		outPacket->flags |= kpUrgentFlag;
		theComm->WriteAndSignPacket(outPacket);
	}
}

void	CPlayerManager::ResendFrame(
	long	theFrame,
	short	requesterId,
	short	commandCode)
{
	CCommManager	*theComm;
	PacketInfo		*outPacket;
	FrameFunction	*ff;

	theComm = theNetManager->itsCommManager;

	ff = &frameFuncs[(FUNCTIONBUFFERS - 1) & theFrame];
	
	if(ff->validFrame == theFrame)
	{	outPacket = theComm->GetPacket();
		if(outPacket)
		{	long	*pd;
		
			outPacket->command = commandCode;
			outPacket->distribution = 1 << requesterId;

			FrameFunctionToPacket(ff, outPacket, slot);

			outPacket->flags |= kpUrgentFlag;
			theComm->WriteAndSignPacket(outPacket);
		}
	}
	else	//	Ask me later packet
	{	theComm->SendUrgentPacket(1 << requesterId, kpAskLater, 0, 0, theFrame, 0,0);
	}

}

void	CPlayerManager::ResumeGame()
{
	short	i;
	
	for(i=0;i<FUNCTIONBUFFERS;i++)
	{	frameFuncs[i].validFrame = -1;
	}

	bufferStart = 0;
	bufferEnd = 0;
	keyboardActive = false;

	oldHeld[0] = oldHeld[1] = 0;
	oldModifiers = 0;
	isLocalPlayer = (theNetManager->itsCommManager->myId == slot);
}

void	CPlayerManager::ProtocolHandler(
	struct PacketInfo *thePacket)
{
	long			*pd;
	short			*spd;
	FrameFunction	*ff;
	long			frameNumber;
	char			p1;
	
	p1 = thePacket->p1;
	frameNumber = thePacket->p3;
	
	pd = (long *)thePacket->dataBuffer;
	ff = &frameFuncs[(FUNCTIONBUFFERS - 1) & frameNumber];
	ff->validFrame = frameNumber;
	
	ff->ft.down[0] =	(p1 & kNoDownData)	? 0 : *pd++;		ff->ft.down[1] = 0;
	ff->ft.up[0] =		(p1 & kNoUpData)	? 0 : *pd++;		ff->ft.up[1] = 0;
	ff->ft.held[0] =	(p1 & kNoHeldData)	? 0 : *pd++;		ff->ft.held[1] = 0;
	
	spd = (short *)pd;

	ff->ft.mouseDelta.v=(p1 & kNoMouseV)	? 0 : *spd++;
	ff->ft.mouseDelta.h=(p1 & kNoMouseH)	? 0 : *spd++;
		
	ff->ft.buttonStatus = p1 & ~kDataFlagMask;
	ff->ft.msgChar = thePacket->p2;
}

void	CPlayerManager::ViewControl()
{
	if(itsPlayer)
		itsPlayer->ControlViewPoint();
}

void	CPlayerManager::Dispose()
{
	inherited::Dispose();
}

void	CPlayerManager::SendResendRequest(
	short	askCount)
{
	if(theNetManager->fastTrack.addr.value || askCount > 0)
	{	theNetManager->playerTable[
			theNetManager->itsCommManager->myId]->
				ResendFrame(itsGame->frameNumber, slot, kpKeyAndMouseRequest);
	}
}

FunctionTable	*CPlayerManager::GetFunctions()
{
	short	i = (FUNCTIONBUFFERS - 1) & itsGame->frameNumber;
	
	if(frameFuncs[i].validFrame != itsGame->frameNumber)
	{	long	quickTick;
		long	firstTime = askAgainTime = TickCount();
		short	askCount = 0;
		long	firstStepCount;

		itsGame->didWait = true;
#if 0
		itsGame->StopTimer();
#endif
		firstStepCount = itsGame->timer.stepCount;

		if(frameFuncs[(FUNCTIONBUFFERS - 1) & (i+1)].validFrame < itsGame->frameNumber)
		{	askAgainTime += 5 + (Random() & 3);
		}

		do
		{	theNetManager->ProcessQueue();

			if(itsGame->canPreSend && itsGame->timer.stepCount - itsGame->nextScheduledFrame >= 0)
			{	itsGame->canPreSend = false;
				theNetManager->FrameAction();
			}

			quickTick = TickCount();
			
			if(quickTick - askAgainTime >= 0)
			{	if(quickTick - firstTime > 120 && 
					TestHeldKey(kfuAbortGame))
				{	itsGame->statusRequest = kAbortStatus;
					break;
				}
			
				askAgainTime = quickTick + 300;	//	Five seconds
				SendResendRequest(askCount++);
				if(askCount == 2)
				{	itsGame->infoPanel->ParamLine(kmWaitingForPlayer, centerAlign, playerName, NULL);

					InitCursor();
					gApplication->SetCommandParams(STATUSSTRINGSLISTID, kmWaitingPlayers, true, 0);
					gApplication->BroadcastCommand(kBusyStartCmd);
				}

//				itsGame->timer.currentStep += 2;
			}

			if(askCount > 1)
			{	gApplication->BroadcastCommand(kBusyTimeCmd);
				if(gApplication->commandResult)
				{	itsGame->statusRequest = kAbortStatus;
					break;
				}
			}

		} while(frameFuncs[i].validFrame != itsGame->frameNumber);
		
		if(askCount > 1)
		{	gApplication->BroadcastCommand(kBusyEndCmd);
			HideCursor();
		}
#if 1
		if(quickTick != firstTime)
		{
			if(quickTick > firstTime+3 || itsGame->longWait)
			{	itsGame->veryLongWait = true;
			}

			itsGame->longWait = true;
		}
#else
		if(itsGame->timer.stepCount != firstStepCount)
		{	itsGame->longWait = true;
		}
#endif
#if 0
		itsGame->ResumeTimer();
#endif
	}
	
	return &frameFuncs[i].ft;
}

void	CPlayerManager::RosterKeyPress(
	unsigned char c)
{
	short	i;
	
	switch(c)
	{	case 7:
			theNetManager->Beep();
			RosterKeyPress('Æ');
			break;			
		case 13:	//	Return
			RosterKeyPress('Â');
			RosterKeyPress(' ');
			break;
		case 9:		//	Tab
			RosterKeyPress(' ');
			RosterKeyPress(' ');
			break;
		case 8:		//	Backspace
			if(message[0]) --message[0];
			break;
		case 27:	//	Escape
			message[0] = 0;
			break;
		default:
			if(c >= 32)
			{	if(message[0] < kMaxMessageChars)
				{	message[++message[0]] = c;
				}
				else
				{	for(i=1;i<kMaxMessageChars;i++)
					{	message[i] = message[i+1];
					}
					message[kMaxMessageChars] = c;
				}
			}
			break;	
	}
}

static
OSErr	WordWrapString(
	StringPtr	fullLine)
{
	if(StringWidth(fullLine) <= kChatMessageWidth)
	{	return	0;
	}
	else
	{	short	relen;
	
		relen = fullLine[0];
		
		while(relen)
		{	unsigned char	theChar;
		
			theChar = fullLine[relen--];
			if(theChar <= 32 || theChar == 'Ê')	//	option-space
			{	short	width;
			
				width = TextWidth(fullLine, 1, relen);
				if(width < kChatMessageWidth / 2)
				{	return TruncString(kChatMessageWidth, fullLine, smTruncEnd);
				}
				else if(width <= kChatMessageWidth)
				{	fullLine[0] = relen+2;
					fullLine[relen+2] = ' ';
					return 1;
				}
			}
		}
	}
	
	return TruncString(kChatMessageWidth, fullLine, smTruncEnd);
}

void	CPlayerManager::FlushMessageText(
	Boolean	forceAll)
{
	Str255			fullLine;
	short			maxLen;
	short			baseLen;
	GrafPtr			savedPort;
	TextSettings	savedTexs;
	OSErr			err;

	GetPort(&savedPort);
	SetPort(theRoster->itsWindow);
	GetSetTextSettings(&savedTexs, geneva, 0, 9, srcOr);

	do
	{	
		BlockMoveData(playerName, fullLine, playerName[0] + 1);

#if 0
		while(fullLine[0] && fullLine[fullLine[0]] == 32)
		{	fullLine[0]--;
		}
#endif
		TruncString(kChatMessageWidth/3, fullLine, smTruncEnd);
		
		fullLine[++fullLine[0]] = ':';
		fullLine[++fullLine[0]] = ' ';
		
		baseLen = fullLine[0];
		maxLen = lineBuffer[0];

		if(maxLen + baseLen > 250)
		{	maxLen = 250 - baseLen;
		}

		BlockMoveData(lineBuffer+1, fullLine + 1 + baseLen, maxLen);
		fullLine[0] += maxLen;
#if 1
		err = WordWrapString(fullLine);
#else
		err = TruncString(kChatMessageWidth, fullLine, smTruncEnd);
#endif
		
		if(err == 1 || forceAll)
		{	short	outLen;
		

			if(err == 1)
			{	outLen = fullLine[0] - baseLen - 1;
			}
			else
			{	outLen = maxLen;
			}
			
			if(outLen != 0)	theRoster->NewChatLine(fullLine);

			BlockMoveData(lineBuffer + outLen + 1, lineBuffer + 1, lineBuffer[0] - outLen);
			lineBuffer[0] -= outLen;

			if(err == 0)	forceAll = false;
		}
	} while(err == 1 || forceAll);

	isLocalPlayer = (theNetManager->itsCommManager->myId == slot);

	if(isLocalPlayer)
	{	theRoster->SetChatLine(fullLine);
	}
	
	RestoreTextSettings(&savedTexs);
	SetPort(savedPort);
}

void	CPlayerManager::RosterMessageText(
	short	len,
	char	*c)
{
	while(len--)
	{	unsigned char	theChar;
	
		theChar = *c++;
		
		switch(theChar)
		{	case 8:
				if(lineBuffer[0])	lineBuffer[0]--;
				break;
			case 13:
				FlushMessageText(true);
				break;
			case 27:
				lineBuffer[0] = 0;
				break;
			default:
				if(theChar >= 32)
				{	lineBuffer[++lineBuffer[0]] = theChar;
					if(lineBuffer[0] > 220)
					{	FlushMessageText(true);
					}
				}
				break;
		}
	}
	
	FlushMessageText(false);
}

void	CPlayerManager::GameKeyPress(
	char theChar)
{
	theNetManager->ReceiveRosterMessage(slot, 1, &theChar);
}

void	CPlayerManager::NetDisconnect()
{
	if(itsPlayer)
	{	if(theNetManager->isPlaying)
		{	itsPlayer->netDestruct = true;	
		}
		else
		{	itsPlayer->Dispose();
		}
	}

	if(mugPict)
		DisposeHandle(mugPict);

	mugPict = NULL;
	mugSize = -1;
	mugState = 0;
	
	globalLocation.h = 0;
	globalLocation.v = 0;

	message[0] = 0;
	lineBuffer[0] = 0;
	playerName[0] = 0;
	spaceCount = 0;
	playerRegName[0] = 0;
	isRegistered = false;
	loadingStatus = kLNotConnected;
	winFrame = -1;
	theRoster->InvalidateArea(kUserBox, position);
	theRoster->InvalidateArea(kOnePlayerBox, position);
}

void	CPlayerManager::ChangeNameAndLocation(
	StringPtr	theName,
	Point		location)
{
	StringPtr	lastChar;

	if(loadingStatus == kLNotConnected)
	{	loadingStatus = kLConnected;
		theRoster->InvalidateArea(kOnePlayerBox, position);
	}
	
	if(location.h != globalLocation.h || location.v != globalLocation.v)
	{	globalLocation = location;
		theRoster->InvalidateArea(kFullMapBox, position);
		theRoster->InvalidateArea(kMapInfoBox, position);
	} 

	BlockMoveData(theName, playerName, theName[0]+1);

	lastChar = playerName + playerName[0];
	spaceCount = 0;
	while(	lastChar > playerName &&
			*lastChar-- == ' ')
	{	spaceCount++;
	}
	
	playerName[0] -= spaceCount;

	theRoster->InvalidateArea(kUserBoxTopLine, position);
	FlushMessageText(false);
}

void	CPlayerManager::SetPosition(
	short		pos)
{
	position = pos;
	theRoster->InvalidateArea(kUserBox, pos);
	theRoster->InvalidateArea(kOnePlayerBox, pos);
}

void	CPlayerManager::LoadStatusChange(
	short	serverCRC,
	OSErr	serverErr,
	OSType	serverTag)
{
	short	oldStatus;
	
	if(		loadingStatus != kLNotConnected
		&&	loadingStatus != kLActive)
	{	oldStatus = loadingStatus;
		
		if(serverErr || levelErr)
		{	if(levelErr == 1)	loadingStatus = kLTrying;
			else
			if(serverErr == 1)
					loadingStatus = kLWaiting;
			else	loadingStatus = kLNotFound;
		}
		else	
		{	if(serverCRC == levelCRC && serverTag == levelTag)
			{	short	i;
			
				loadingStatus = kLLoaded;
#if 0
				oldHeld[0] = oldHeld[1] = 0;
			
				for(i=0;i<FUNCTIONBUFFERS;i++)
				{	frameFuncs[i].validFrame = -1;
				}	
#endif
			}
			else
			{	if(serverTag == levelTag)
				{	loadingStatus = kLMismatch;
				}
			}
		}
		
		if(oldStatus != loadingStatus)
		{	if(loadingStatus != kLConnected)
				winFrame = -1;
			
			theRoster->InvalidateArea(kUserBoxTopLine, position);
		}
	}
}

CAbstractPlayer *CPlayerManager::ChooseActor(
	CAbstractPlayer *actorList,
	short			myTeamColor)
{
	CAbstractPlayer	*newList;
	CAbstractPlayer	**tailList;
	CAbstractPlayer	*nextPlayer;
	short			myTeamMask;

	itsPlayer = NULL;
	playerColor = myTeamColor;
	myTeamMask = 1<<myTeamColor;

	newList = NULL;
	tailList = &newList;
	
	while(actorList)
	{	nextPlayer = (CAbstractPlayer *) actorList->nextActor;
		
		if(actorList->teamColor == myTeamColor)
		{	//	Good enough for me.
		
			itsPlayer = actorList;
			itsPlayer->itsManager = this;
			itsPlayer->PlayerWasMoved();
			itsPlayer->BuildPartProximityList(	itsPlayer->location,
												itsPlayer->proximityRadius,
												kSolidBit);
			itsPlayer->AddToGame();

			myTeamColor = -999;	//	Ignore the rest.
		}
		else
		{	*tailList = actorList;
			tailList = (CAbstractPlayer **) &actorList->nextActor;
		}
		actorList = nextPlayer;
	}
	
	*tailList = NULL;	//	Terminate list.
	actorList = newList;
	
	if(itsPlayer == NULL)
	{	CIncarnator		*spotAvailable;
	
		itsPlayer = new CWalkerActor;
		itsPlayer->IAbstractActor();
		itsPlayer->BeginScript();
		ProgramLongVar(iTeam, myTeamColor);
		FreshCalc();	//	Parser call.
		itsPlayer->EndScript();
		itsPlayer->isInLimbo = true;
		itsPlayer->limboCount = 0;
		itsPlayer->itsManager = this;

		spotAvailable = itsGame->incarnatorList;
		while(spotAvailable)
		{	if(	spotAvailable->enabled &&
				spotAvailable->colorMask & myTeamMask)
			{	itsPlayer->Reincarnate(spotAvailable);
				if(!itsPlayer->isInLimbo)
				{	break;
				}
			}
			spotAvailable = spotAvailable->nextIncarnator;
		}
		
		if(itsPlayer->isInLimbo)
		{	itsPlayer->Dispose();
			itsPlayer = NULL;
		}
		else
		{	itsPlayer->AddToGame();
		}
	}
	
	return actorList;
}

Boolean	CPlayerManager::IncarnateInAnyColor()
{
	short			i;
	CIncarnator		*spotAvailable;

	for(i=1;i<=kMaxTeamColors;i++)
	{	
		itsPlayer = new CWalkerActor;
		itsPlayer->IAbstractActor();
		itsPlayer->BeginScript();
		ProgramLongVar(iTeam, i);
		FreshCalc();	//	Parser call.
		itsPlayer->EndScript();
		itsPlayer->isInLimbo = true;
		itsPlayer->limboCount = 0;
		itsPlayer->itsManager = this;
	
		spotAvailable = itsGame->incarnatorList;
		while(spotAvailable)
		{	if(	spotAvailable->enabled &&
				spotAvailable->colorMask & (1<<i))
			{	itsPlayer->Reincarnate(spotAvailable);
				if(!itsPlayer->isInLimbo)
				{	break;
				}
			}
			spotAvailable = spotAvailable->nextIncarnator;
		}
		
		if(itsPlayer->isInLimbo)
		{	itsPlayer->Dispose();
			itsPlayer = NULL;
		}
		else
		{	itsPlayer->AddToGame();
			playerColor = i;
			return true;
		}
	}
	
	return false;
}

CAbstractPlayer *CPlayerManager::TakeAnyActor(
	CAbstractPlayer *actorList)
{
	CAbstractPlayer	*nextPlayer;

	nextPlayer = (CAbstractPlayer *) actorList->nextActor;
	
	playerColor = actorList->teamColor;
		
	itsPlayer = actorList;
	itsPlayer->itsManager = this;
	itsPlayer->AddToGame();
	
	return nextPlayer;
}
void	CPlayerManager::SetPlayerStatus(
	short	newStatus,
	long	theWin)
{
	winFrame = theWin;

	if(newStatus != loadingStatus)
	{	if(loadingStatus == kLNotConnected)
		{	theRoster->InvalidateArea(kOnePlayerBox, position);
		}
	
		loadingStatus = newStatus;
		theRoster->InvalidateArea(kUserBoxTopLine, position);
	}
}

void	CPlayerManager::AbortRequest()
{
	theNetManager->activePlayersDistribution &= ~(1<<slot);
	if(isLocalPlayer)
	{	itsGame->statusRequest = kAbortStatus;
	}
}

void	CPlayerManager::DeadOrDone()
{
	theNetManager->deadOrDonePlayers |= 1<<slot;
}

short	CPlayerManager::GetStatusChar()
{
	if(itsPlayer == NULL || (loadingStatus != kLActive && loadingStatus != kLPaused))
	{	return -1;
	}
	else
	{	if(itsPlayer->winFrame >= 0)
		{	return 12;
		}
		else
		{	if(itsPlayer->lives < 10)
			{	return itsPlayer->lives;
			}
			else
			{	return 10;
			}
		}
	}
}

short	CPlayerManager::GetMessageIndicator()
{
	if(itsPlayer && itsPlayer->chatMode && loadingStatus == kLActive)
	{	return 11;
	}
	else
	{	return -1;
	}
}

void	CPlayerManager::StoreMugShot(
	Handle		mugHandle)
{
	if(mugPict)
	{	DisposeHandle(mugPict);
	}
	
	mugPict = mugHandle;

	if(mugHandle)
	{	mugSize = GetHandleSize(mugHandle);
		mugState = mugSize;
	}
	else
	{	mugSize = -2;
		mugState = 0;
	}
}

Handle	CPlayerManager::GetMugShot()
{
	Handle	result = NULL;

	if(mugSize == -1)
	{	if(loadingStatus != kLNotConnected)
		{	mugSize = -2;

			isLocalPlayer = (theNetManager->itsCommManager->myId == slot);
	
			if(isLocalPlayer)
			{	gApplication->BroadcastCommand(kGiveMugShotCmd);
			}
			else
			{	theNetManager->itsCommManager->SendPacket(1L<<slot, kpGetMugShot, 0, 0, 0, 0, NULL);
			}
		}
	}

	if(mugPict && mugState == mugSize)
	{	result = mugPict;
	}

	return result;
}

void	CPlayerManager::GetLoadingStatusString(
	StringPtr	theStr)
{

	if(loadingStatus == kLConnected && winFrame >= 0)
	{	long	timeTemp = FMulDiv(winFrame, itsGame->frameTime, 10);
		char	mins[2];
		char	secs[2];
		char	hundreds[2];

		hundreds[0] = '0' + timeTemp % 10;
		timeTemp /= 10;
		hundreds[1] = '0' + timeTemp % 10;
		timeTemp /= 10;

		secs[0] = '0' + timeTemp % 10;
		timeTemp /= 10;
		secs[1] = '0' + timeTemp % 6;
		timeTemp /= 6;

		NumToString(timeTemp, theStr+1);
		theStr[0] = theStr[1] + 1;
		theStr[1] = '[';
				
		theStr[++theStr[0]] = ':';
		theStr[++theStr[0]] = secs[1];
		theStr[++theStr[0]] = secs[0];
		theStr[++theStr[0]] = '.';
		theStr[++theStr[0]] = hundreds[1];
		theStr[++theStr[0]] = hundreds[0];
		theStr[++theStr[0]] = ']';
	}
	else
	{	GetIndString(theStr, kRosterStringListID, loadingStatus);
	}
}

void	CPlayerManager::SpecialColorControl()
{
	if(isRegistered && itsPlayer)
	{	long		repColor = -1;
		
		switch(spaceCount)
		{	case 2:
				repColor = 0x303030;
				break;
			case 3:
				repColor = 0xe0e0e0;
				break;
		}
		
		if(repColor >= 0)
		{	itsPlayer->SetSpecialColor(repColor);
		}
	}
}