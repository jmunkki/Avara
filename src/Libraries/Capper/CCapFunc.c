/*
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CCapFunc.c
    Created: Saturday, November 12, 1994, 0:54
    Modified: Friday, December 1, 1995, 22:47
*/

#include "CCapFunc.h"
#include "CCapMaster.h"
#include "CCapKey.h"

void	CCapFunc::ICapFunc(
	short		theId,
	StringPtr	theName,
	StringPtr	theHelp,
	short		theIcon,
	long		theMask)
{
	short	i;
	short	len;
	
	id = theId;
	
	len = theName[0];
	if(len > 63)	len = 63;
	name[0] = len;

	for(i=1;i<=len;i++)
	{	name[i] = theName[i];
	}

	for(i=0;i<=theHelp[0];i++)
	{	help[i] = theHelp[i];
	}
	
	icon = theIcon;
	iconHandle = GetCIcon(icon);
	mask = theMask;
}

void	CCapFunc::DrawIcon(
	short	x,
	short	y)
{
	Rect	dest;
	
	dest.left = x - 8;
	dest.right = x + 8;
	dest.top = y - 8;
	dest.bottom = y + 8;
	
	PlotCIcon(&dest, iconHandle);
}

void	CCapFunc::DrawFull(
	short	x,
	short	y)
{
	short	oldSize;
	short	oldFont;
	short	oldFace;
	short	oldMode;
	GrafPtr	port;
	
	GetPort(&port);
	DrawIcon(x+9,y+9);
	
	oldSize = port->txSize;	TextSize(9);
	oldFont = port->txFont;	TextFont(geneva);
	oldFace = port->txFace;	TextFace(0);
	oldMode = port->txMode;	TextMode(srcOr);

	MoveTo(x+20, y+13);
	DrawString(name);
	
	TextSize(oldSize);
	TextFont(oldFont);
	TextFace(oldFace);
	TextMode(oldMode);
}

RgnHandle	CCapFunc::GetFullRegion(
	short	x,
	short	y)
{
	short		oldSize;
	short		oldFont;
	short		oldFace;
	GrafPtr		port;
	RgnHandle	theRegion;
	Rect		temp;
	
	GetPort(&port);

	oldSize = port->txSize;	TextSize(9);
	oldFont = port->txFont;	TextFont(geneva);
	oldFace = port->txFace;	TextFace(0);

	theRegion = NewRgn();
	OpenRgn();

	temp.left  = x;
	temp.right = x + 18;
	temp.top = y;
	temp.bottom = y + 18;
	FrameRect(&temp);
	
	temp.left = x + 18;
	temp.right = temp.left + StringWidth(name) + 4;
	temp.top = y + 2;
	temp.bottom = y + 17;
	FrameRect(&temp);

	CloseRgn(theRegion);

	TextSize(oldSize);
	TextFont(oldFont);
	TextFace(oldFace);
	
	return theRegion;
}

void	CCapFunc::Dispose()
{
	DisposeCIcon(iconHandle);

	delete this;
}

static	CCapMaster	*sMaster;
static	RgnHandle	sHiliteArea;

pascal void	DragActionProc()
{
	Point		where;
	RgnHandle	newArea;
	PenState	saved;
	GrafPtr		savedPort;
	
	GetPort(&savedPort);
	SetPort(sMaster->itsWindow);
	GetPenState(&saved);
	GetMouse(&where);
	
	newArea = NewRgn();

	if(sMaster->currentCap && PtInRect(where, &sMaster->infoRect))
	{	RectRgn(newArea, &sMaster->infoRect);
		UnionRgn(sMaster->currentCap->theKeyRegion, newArea, newArea);
	}
	else
	if(PtInRect(where, &sMaster->keysRect))
	{	short	i;
	
		for(i=NUMCAPS-1;i>=0;i--)
		{	if(PtInRgn(where, sMaster->caps[i]->theKeyRegion))
			{	if(sMaster->caps[i] == sMaster->currentCap)
				{	RectRgn(newArea, &sMaster->infoRect);
				}
				
				UnionRgn(sMaster->caps[i]->theKeyRegion, newArea, newArea);
				break;
			}
		}
	}
	
	InsetRgn(newArea, 1,1);
	XorRgn(newArea, sHiliteArea, sHiliteArea);

	PenMode(patXor);
	PenPat(&qd.black);
	FrameRgn(sHiliteArea);
	DisposeRgn(sHiliteArea);

	sHiliteArea = newArea;

	SetPenState(&saved);
	SetPort(savedPort);
}

Boolean	singleClick;

Boolean	CCapFunc::DoDrag(
	CCapMaster	*master,
	RgnHandle	fromRegion,
	RgnHandle	dragRegion,
	Point		where)
{
	unsigned long 	dropPoint;
	Point			landing;
	PenState		saved;
	Boolean			wasMoved;
	
	GetPenState(&saved);
	
	sHiliteArea = NewRgn();

	sMaster = master;
	dropPoint = DragGrayRgn(dragRegion, where,
								&master->itsWindow->portRect,
								&master->itsWindow->portRect,
								noConstraint, DragActionProc);
	landing = *(Point *)&dropPoint;
	landing.h += where.h;
	landing.v += where.v;

	singleClick = (dropPoint == 0);

	PenMode(patXor);
	PenPat(&qd.black);
	FrameRgn(sHiliteArea);
	
	wasMoved = !PtInRgn(landing, fromRegion);

	if(wasMoved)
	{	CCapKey		*dropKey = NULL;
	
		InvalRgn(sHiliteArea);
	
		//	Now find where the key was dropped.
		if(master->currentCap && PtInRect(landing, &master->infoRect))
		{	dropKey = master->currentCap;
		}
		else
		if(PtInRect(landing, &master->keysRect))
		{	short	i;
		
			for(i=NUMCAPS-1;i>=0;i--)
			{	if(PtInRgn(landing, master->caps[i]->theKeyRegion))
				{	dropKey = master->caps[i];
					break;
				}
			}
		}
		
		if(dropKey)
		{	dropKey->AddFunc(this);
			if(dropKey != master->currentCap)
			{	master->currentCap = dropKey;
				InvalRect(&master->infoRect);
			}
		}
	}

	DisposeRgn(sHiliteArea);

	SetPenState(&saved);

	return wasMoved;
}
Boolean	CCapFunc::DragFull(
	short		x,
	short		y,
	Point		where,
	CCapMaster	*master,
	Rect		*from)
{
	RgnHandle	fromRegion;
	RgnHandle	fullRegion;
	Boolean		wasDragged;

	fullRegion = GetFullRegion(x,y);
	
	if(PtInRgn(where, fullRegion))
	{
		wasDragged = true;
		fromRegion = NewRgn();
		RectRgn(fromRegion, from);

		DoDrag(master, fromRegion, fullRegion, where);
		
		if(singleClick)
		{	master->currentCap = NULL;
			master->currentFunc = this;
			InvalRect(&master->infoRect);
		}

		DisposeRgn(fromRegion);
	}
	else
	{	wasDragged = false;
	}
	
	DisposeRgn(fullRegion);
	
	return wasDragged;
}