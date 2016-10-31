/*
*/

#include <CBusyWindow.h>
#include "CommandList.h"
#include "CEventHandler.h"

#define	kShowBusyTime		120

void	CBusyWindow::IWindowCommander(
	CCommander			*theCommander,
	CWindowCommander	**theList)
{
	short			i;
	
	inherited::IWindowCommander(theCommander, theList);

	wantsOnWindowsMenu = false;
	hasGrowBox = false;
	itsWindow = GetNewCWindow(kBusyResourceId, 0, FrontWindow());
	SetPort(itsWindow);
	TextFont(0);
	TextSize(12);
	TextMode(srcOr);

	isVisible = false;

//	RestoreFromPreferences('BµsR', 0, false, true);

	textRect = itsWindow->portRect;
	InsetRect(&textRect, 6, 6);
	textRect.bottom -= 24;

	busyLevel = -1;
	needsHilite = NULL;

	abortButton = GetNewControl(kBusyResourceId, itsWindow);
	abortVisible = true;

	AddToList();
}

void	CBusyWindow::DoCommand(
	long	theCommand)
{
	long	*paramTable;

	switch(theCommand)
	{	case kBusyStartCmd:
			paramTable = gApplication->commandParam;
			PushBusyWindow(paramTable[0], paramTable[1], paramTable[2]);
			break;
		case kBusyTimeCmd:
			gApplication->commandResult = IdleBusyWindow();
			break;
		case kBusyEndCmd:
			PopBusyWindow();
			break;
		case kBusyHideCmd:
			DoBusyHide();
			break;
		default:
			inherited::DoCommand(theCommand);
	}
}

void	CBusyWindow::ButtonVisibilityControl()
{
	if(busyStack[busyLevel].canStop)
	{	if(!abortVisible)
		{	abortVisible = true;
			ShowControl(abortButton);
		}
	}
	else
	{	if(abortVisible)
		{	abortVisible = false;
			HideControl(abortButton);
		}
	}
}

void	CBusyWindow::DoBusyHide()
{
	if(isVisible)
	{	HideWindow(itsWindow);
		SendBehind(itsWindow, NULL);
		isVisible = false;
		if(needsHilite)
			HiliteWindow(needsHilite, true);
	}
}

void	CBusyWindow::PushBusyWindow(
	short	listId,
	short	index,
	Boolean	canAbort)
{
	busyLevel++;
	
	if(busyLevel < kBusyStackDepth)
	{	GrafPtr		saved;
		
		GetPort(&saved);
		SetPort(itsWindow);
		busyStack[busyLevel].stringList = listId;
		busyStack[busyLevel].stringIndex = index;
		busyStack[busyLevel].canStop = canAbort;

		ButtonVisibilityControl();

		if(busyLevel == 0)
		{	firstTime = TickCount();
		}

		if(isVisible)
		{	InvalRect(&textRect);
			DoUpdate();
		}

		SetPort(saved);
	}
}

Boolean	CBusyWindow::IdleBusyWindow()
{
	Boolean	doStop = false;

	if(busyLevel >= 0)
	{	if(!isVisible)
		{	if(TickCount() - firstTime > kShowBusyTime)
			{	//	Time to pop up and show up.
				needsHilite = FrontWindow();
				BringToFront(itsWindow);
	
				if(IsWindowHilited(needsHilite))
				{	HiliteWindow(needsHilite, false);
				}
				else
				{	needsHilite = NULL;
				}
				
				HiliteWindow(itsWindow, true);
				ShowWindow(itsWindow);
				isVisible = true;
			}
		}
	
		if(isVisible)
		{	if(abortVisible)
			{	EventRecord		theEvent;
				
				GetOSEvent(mUpMask + mDownMask, &theEvent);
				if(theEvent.what == mouseDown)
				{	GrafPtr		saved;
					ControlHandle	itsControl;
					
					GetPort(&saved);
					SetPort(itsWindow);
					
					GlobalToLocal(&theEvent.where);
					if(FindControl(theEvent.where, itsWindow, &itsControl))
					{	doStop = inButton == TrackControl(itsControl, theEvent.where, NULL);
					}
					
					SetPort(saved);
					
				}
			}
			DoUpdate();
		}
	}

	return doStop;
}

void	CBusyWindow::PopBusyWindow()
{
	busyLevel--;
	
	if(busyLevel < 0)
	{	DoBusyHide();
	}
	else
	{	ButtonVisibilityControl();
		if(isVisible)
		{	GrafPtr		saved;
					
			GetPort(&saved);
			SetPort(itsWindow);

			InvalRect(&textRect);
			DoUpdate();
			
			SetPort(saved);
		}
	}
}

void	CBusyWindow::DrawContents()
{
	Str255	busyMessage;
	
	if(busyLevel >= 0)
	{	DrawControls(itsWindow);

		GetIndString(busyMessage, busyStack[busyLevel].stringList, busyStack[busyLevel].stringIndex);
		TextFont(0);
		TextSize(12);
		TextMode(srcOr);
		
		TextBox(busyMessage + 1, busyMessage[0], &textRect, 1);
	}
}
