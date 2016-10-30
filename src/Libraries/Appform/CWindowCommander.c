/*
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CWindowCommander.c
    Created: Wednesday, November 16, 1994, 1:17
    Modified: Wednesday, April 24, 1996, 1:05
*/

#include "CWindowCommander.h"
#include "CEventHandler.h"
#include "CommandList.h"
#include "CCompactTagBase.h"

void	CWindowCommander::IWindowCommander(
	CCommander			*theCommander,
	CWindowCommander	**theList)
{
	inherited::ICommander(theCommander);
	
	wantsOnWindowsMenu = true;
	windowList = theList;
	itsWindow = NULL;
	canClose = true;
	hasGrowBox = false;
	isActive = false;
	nextWindow = NULL;
}

void	CWindowCommander::AddToList()
{
	nextWindow = *windowList;
	*windowList = this;
}

void	CWindowCommander::Close()
{	
	if(itsWindow)
	{
		DisposeWindow(itsWindow);
		itsWindow = NULL;
		
		if(*windowList == this)
		{	*windowList = nextWindow;
		}
		else
		{	CWindowCommander	*wp;

			wp = *windowList;
			while(wp != NULL)
			{	if(wp->nextWindow == this)
				{	wp->nextWindow = nextWindow;
					wp = NULL;
				}
				else
				{	wp = wp->nextWindow;
				}
			}
		}
	}
}

CloseRequestResult	CWindowCommander::CloseRequest(Boolean isQuitting)
{
	Dispose();

	return kDidDispose;
}

void	CWindowCommander::Select()
{
	SelectWindow(itsWindow);
	ShowWindow(itsWindow);
}

void	CWindowCommander::Dispose()
{
	Close();
	
	inherited::Dispose();
}

void	CWindowCommander::RawClick(
	WindowPtr	theWind,
	short		partCode,
	EventRecord	*theEvent)
{
	if(theWind == itsWindow)
	{	switch(partCode)
		{	
			case inDrag:
				{	Rect	dragRect;
					GrafPtr	wMgrPort;
				
					GetWMgrPort(&wMgrPort);
					dragRect = wMgrPort->portRect;
					InsetRect(&dragRect, 4, 4);
					DragWindow(theWind,theEvent->where,&dragRect);
				}
				break;

			case inContent:
				if(itsWindow == FrontWindow())
				{	Point	where;
				
					SetPort(itsWindow);
					where = theEvent->where;
					GlobalToLocal(&where);
					ContentClick(theEvent, where);
				}
				else
				{	SelectWindow(itsWindow);
				}
				break;

			case inGrow:
				DoGrow(theEvent);
				break;

			case inGoAway:
				if(TrackGoAway(theWind,theEvent->where))
				{	CloseRequest(false);
				}
				break;

			case inZoomIn:
			case inZoomOut:
				DoZoom(partCode, theEvent);
				break;
		}
	}
	else
	if(nextWindow)
	{	nextWindow->RawClick(theWind, partCode, theEvent);
	}
}

Boolean	CWindowCommander::DoGrow(
	EventRecord	*theEvent)
{
	Point		newSize;
	Rect		grower;
	int			index;
	Point		oldSize;
	
	oldSize.h = itsWindow->portRect.right - itsWindow->portRect.left;
	oldSize.v = itsWindow->portRect.bottom - itsWindow->portRect.top;

	SetPort(itsWindow);
	SetRect(&grower,128,128,4096,4096);
	*(long *)(& newSize) = GrowWindow(itsWindow,theEvent->where,&grower);
	SizeWindow(itsWindow,newSize.h,newSize.v,0);
	InvalRect(&itsWindow->portRect);
	
	return oldSize.h != newSize.h || oldSize.v != newSize.v;
}

void	CWindowCommander::ContentClick(
	EventRecord	*theEvent,
	Point		where)
{
	
}

Boolean	CWindowCommander::DoZoom(
	short		partCode,
	EventRecord	*theEvent)
{
	if(TrackBox(itsWindow,theEvent->where,partCode))
	{	SetPort(itsWindow);
		EraseRect(&itsWindow->portRect);
		ZoomWindow(itsWindow,partCode,0);
		
		return true;
	}
	else
	{	return false;
	}
}

void	CWindowCommander::ToggleZoomState()
{
	WindowPeek	itsPeek;
	WStateData	*stateData;
	Rect		globRect;
	short		partCode;
	
	SetPort(itsWindow);
	itsPeek = (WindowPeek) itsWindow;
	stateData = (WStateData *) *(itsPeek->dataHandle);
	globRect = itsWindow->portRect;
	LocalToGlobal((Point *)&globRect);
	LocalToGlobal(1+(Point *)&globRect);

	if(stateData->stdState.top == globRect.top &&
		stateData->stdState.bottom == globRect.bottom &&
		stateData->stdState.left == globRect.left &&
		stateData->stdState.right == globRect.right)
			partCode = inZoomIn;
	else
			partCode = inZoomOut;
	
	EraseRect(&itsWindow->portRect);
	ZoomWindow(itsWindow, partCode, 0);
}

void	CWindowCommander::AdjustMenus(
	CEventHandler	*master)
{
	inherited::AdjustMenus(master);
	
	if(canClose)
		master->EnableCommand(kCloseCmd);

}

void	CWindowCommander::DoCommand(
	long			theCommand)
{
	switch(theCommand)
	{	case kCloseCmd:
			CloseRequest(false);
			break;
		default:
			inherited::DoCommand(theCommand);
	}
}

void	CWindowCommander::DrawContents()
{
	EraseRect(&itsWindow->portRect);
	if(hasGrowBox)
		DrawGrowIcon(itsWindow);
}
void	CWindowCommander::DoUpdate()
{
	if(!EmptyRgn( ((WindowPeek)itsWindow) -> updateRgn))
	{	GrafPtr		saved;
	
		GetPort(&saved);
		SetPort(itsWindow);
		BeginUpdate(itsWindow);
		DrawContents();
		EndUpdate(itsWindow);
		SetPort(saved);
	}
}

void	CWindowCommander::DoActivateEvent()
{
	if(hasGrowBox)
	{	Rect	temp;
	
		SetPort(itsWindow);
		temp = itsWindow->portRect;
		temp.left = temp.right - 15;
		temp.top = temp.bottom - 15;
		InvalRect(&temp);
	}
}

Boolean	CWindowCommander::DoEvent(
	CEventHandler	*master,
	EventRecord		*theEvent)
{
	Boolean	didHandle = false;
	
	switch(theEvent->what)
	{	case updateEvt:
			if(itsWindow == (WindowPtr)theEvent->message)
			{	didHandle = true;
				DoUpdate();
			}
			break;
		case activateEvt:
			if(itsWindow == (WindowPtr)theEvent->message)
			{	didHandle = true;
				isActive = theEvent->modifiers & activeFlag;
				if(isActive)
				{	BecomeTarget();
				}
				
				DoActivateEvent();
			}
			break;			
	}
	
	if(didHandle == false)
	{	if(nextWindow)
			didHandle = nextWindow->DoEvent(master, theEvent);
		else
			inherited::DoEvent(master, theEvent);
	}
	
	return didHandle;
}

pascal
Boolean	ConfirmSaveFilter(
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
				if(theChar == 13 || theChar == 3 ||
					((theEvent->modifiers & cmdKey) &&
					 (theChar == 'Y' || theChar == 'y')))
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
				else if((theEvent->modifiers & cmdKey) &&
					 (theChar == 'N' || theChar == 'n'))
				{	*itemHit = 3;
					didHandle = true;
					doHilite = 3;
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
short	CWindowCommander::ConfirmSave()
{
	Handle			title;
	short			dItem;
	short			result;
	CCommander		*theSaved;
	ModalFilterUPP	myFilter;

	myFilter = NewModalFilterProc(ConfirmSaveFilter);

	Select();
	theSaved = gApplication->BeginDialog();
	DoUpdate();

	title = (Handle)((WindowPeek)itsWindow)->titleHandle;
	HLock(title);
	ParamText((StringPtr)*title, 0,0,0);
	
	dItem = Alert(400, myFilter);
	DisposeRoutineDescriptor(myFilter);

	switch(dItem)
	{
		case 1:
			result = kYesCmd;
			break;
		case 2:
			result = kCancelCmd;
			break;
		case 3:
			result = kNoCmd;
			break;
	}

	HUnlock(title);

	gApplication->EndDialog(theSaved);
	return result;
}

CWindowCommander *	CWindowCommander::FindActiveWindow()
{
	if(isActive)		return this;
	else
		if(nextWindow)	return nextWindow->FindActiveWindow();
		else			return NULL;
}

Boolean	CWindowCommander::CompareFSSpecs(
	FSSpec	*a,
	FSSpec	*b)
{
	short		i;
	StringPtr	an, bn;
	
	if(a->vRefNum != b->vRefNum)	return false;
	if(a->parID != b->parID)		return false;
	
	an = a->name;
	bn = b->name;
	i = *an;
	do
	{	if(*an++ != *bn++)	return false;
	} while(i--);
	
	return true;	
}

Boolean	CWindowCommander::AvoidReopen(
	FSSpec	*theFile,
	Boolean	doSelect)
{
	return false;
}

void	CWindowCommander::GetGlobalWindowRect(
	Rect	*theRect)
{
	GrafPtr	saved;
	
	GetPort(&saved);
	SetPort(itsWindow);
	*theRect = itsWindow->portRect;
	LocalToGlobal(&topLeft(*theRect));
	LocalToGlobal(&botRight(*theRect));
	SetPort(saved);
}

Boolean	CWindowCommander::TryWindowRect(
	Rect	*tryRect)
{
	Rect		dragRect;
	Rect		sizeRect;
	RgnHandle	desktop;

	desktop = GetGrayRgn();

	dragRect = *tryRect;
	dragRect.bottom = dragRect.top;
	dragRect.top -= 20;
	
	sizeRect = *tryRect;
	sizeRect.left = sizeRect.right - 15;
	sizeRect.top = sizeRect.bottom - 15;
	
	if(RectInRgn(&dragRect, desktop) && RectInRgn(&sizeRect, desktop))
	{	SizeWindow(itsWindow, tryRect->right - tryRect->left, tryRect->bottom - tryRect->top, true);
		MoveWindow(itsWindow, tryRect->left, tryRect->top, false);
		return true;
	}
	else
		return false;
}

void	CWindowCommander::ReviveWindowRect(
	Rect	*theRect)
{
	short		turn;
	Rect		globRect;
	
	GetGlobalWindowRect(&globRect);
	
	for(turn = 0; turn < 3; turn ++)
	{	Rect	tryRect;
	
		switch(turn)
		{	case 0:
				tryRect = *theRect;
				break;
			case 1:	//	Move window to default position.
				tryRect = *theRect;
				OffsetRect(&tryRect,	globRect.left - tryRect.left,
										globRect.top - tryRect.top);
				break;
			case 2:	//	Resize window to default size.
				tryRect.left = theRect->left;
				tryRect.top = theRect->top;
				tryRect.right = tryRect.left + globRect.right - globRect.left;
				tryRect.bottom = tryRect.top + globRect.bottom - globRect.top;
				break;
		}
		
		if(TryWindowRect(&tryRect))
		{	break;
		}
	}
}

void	CWindowCommander::RestoreFromPreferences(
	OSType	rectTag,
	OSType	visTag,
	Boolean	defaultShow,
	Boolean	forceNewSize)
{
	Rect	oldSize;
	Boolean	doShow;

	GetGlobalWindowRect(&oldSize);
	gApplication->prefsBase->ReadRect(rectTag, &oldSize);
	
	if(forceNewSize)
	{	oldSize.right = oldSize.left + itsWindow->portRect.right - itsWindow->portRect.left;
		oldSize.bottom = oldSize.top + itsWindow->portRect.bottom - itsWindow->portRect.top;
	}

	doShow = visTag && gApplication->prefsBase->ReadShort(visTag, defaultShow);
	ReviveWindowRect(&oldSize);

	if(doShow)
	{	ShowWindow(itsWindow);
	}
}

void	CWindowCommander::SaveIntoPreferences(
	OSType	rectTag,
	OSType	visTag)
{
	Rect	saveRect;

	GetGlobalWindowRect(&saveRect);
	gApplication->prefsBase->WriteRect(rectTag, &saveRect);	
	gApplication->prefsBase->WriteShort(visTag, ((WindowPeek)itsWindow)->visible);
}


void	CWindowCommander::DrawNumber(
	long	theNum,
	short	options)
{
	Str63		theStr;
	
	if(options & kDNumPrependSpace)
	{	NumToString(theNum, theStr+1);
		theStr[0] = theStr[1] + 1;
		theStr[1] = ' ';
	}
	else
	{	NumToString(theNum, theStr);
	}
	
	if(options & kDNumAppendComma)	theStr[++theStr[0]] = ',';
	if(options & kDNumAppendPeriod)	theStr[++theStr[0]] = '.';
	if(options & kDNumAppendSpace)	theStr[++theStr[0]] = ' ';

	if(options & (kDNumAlignRight+kDNumAlignCenter))
	{	short	w;
	
		w = StringWidth(theStr);
		if(options & kDNumAlignCenter)
		{	w >>= 1;
		}
		Move(-w, 0);
	}
	
	DrawString(theStr);
}

RgnHandle	CWindowCommander::SaveClip()
{
	RgnHandle	theRgn;
	
	theRgn = NewRgn();
	CopyRgn(itsWindow->clipRgn, theRgn);
	
	return theRgn;
}

void	CWindowCommander::RestoreClip(
	RgnHandle	theRgn)
{
	SetClip(theRgn);
	DisposeRgn(theRgn);
}