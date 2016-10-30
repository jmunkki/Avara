/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CGameWind.c
    Created: Wednesday, November 16, 1994, 2:05
    Modified: Monday, June 12, 1995, 6:46
/*/

#include "CGameWind.h"
#include "Palettes.h"
#include "PAKit.h"
#include "CAvaraApp.h"
#include "CAvaraGame.h"
#include "AvaraDefines.h"

void	CGameWind::IWindowCommander(
	CCommander			*theCommander,
	CWindowCommander	**theList)
{
	inherited::IWindowCommander(theCommander, theList);

	wantsOnWindowsMenu = false;
	hasGrowBox = true;
	itsWindow = GetNewCWindow(128, 0, FrontWindow());

	
	AddToList();
}

void	CGameWind::PositionWindow()
{
	Rect				gdr, globRect;
	CWindowCommander	*infoPanel;
	
	gdr = (*GetMainDevice())->gdRect;
	
	infoPanel = (CWindowCommander *)((CAvaraApp *)gApplication)->theInfoPanel;
	MoveWindow(itsWindow,
				(gdr.right + gdr.left - itsWindow->portRect.right) >> 1,
				gdr.bottom - itsWindow->portRect.bottom - 16 -
				infoPanel->itsWindow->portRect.bottom, false);
	GetGlobalWindowRect(&globRect);

	if(TryWindowRect(&globRect) == false)
	{	MoveWindow(itsWindow, globRect.left, 40, false);
	}
	RestoreFromPreferences(kGameWindowRectTag, kGameWindowVisTag, true, false);

}

void	CGameWind::DrawContents()
{
	CAvaraGame	*aGame;
	RgnHandle	oldClip;
	Rect		growBoxRect;

	aGame = ((CAvaraApp *)itsCommander)->itsGame;
	
	aGame->DrawGameWindowContents();
	
	if(isActive)
	{
		oldClip = NewRgn();
		GetClip(oldClip);
	
		growBoxRect = itsWindow->portRect;
		growBoxRect.left = growBoxRect.right - 15;
		growBoxRect.top = growBoxRect.bottom - 15;

		ClipRect(&growBoxRect);
		DrawGrowIcon(itsWindow);
		SetClip(oldClip);

		DisposeRgn(oldClip);
	}
}

Boolean	CGameWind::DoGrow(
	EventRecord	*theEvent)
{
	Point		newSize;
	Rect		grower;
	int			index;
	Point		oldSize;
	
	oldSize.h = itsWindow->portRect.right - itsWindow->portRect.left;
	oldSize.v = itsWindow->portRect.bottom - itsWindow->portRect.top;

	SetPort(itsWindow);
	SetRect(&grower,256,120,4096,DEFAULTMAXHEIGHT);
	*(long *)(& newSize) = GrowWindow(itsWindow,theEvent->where,&grower);
	
	SizeWindow(itsWindow,newSize.h,newSize.v, 0);
	SetPort(itsWindow);
	InvalRect(&itsWindow->portRect);

	return oldSize.h != newSize.h || oldSize.v != newSize.v;
}

CloseRequestResult	CGameWind::CloseRequest(
	Boolean isQuitting)
{

	if(isQuitting)
	{	SaveIntoPreferences(kGameWindowRectTag, kGameWindowVisTag);
	}

	HideWindow(itsWindow);
	
	return kDidHideDontDispose;
}
