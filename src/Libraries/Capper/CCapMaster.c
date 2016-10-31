/*
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CCapMaster.c
    Created: Friday, November 11, 1994, 21:02
    Modified: Thursday, January 25, 1996, 5:18
*/

#include "CCapMaster.h"
#include "CCapKey.h"
#include "CCapFunc.h"
#include "CharWordLongPointer.h"
#include <Script.h>
#include <OSUtils.h>
#include <LowMem.h>

//#define	KbdType (*(char *)0x21E)	/*[GLOBAL VAR]  keyboard model number [byte]*/

#define	KbdDebugOffset	0

void	CCapMaster::ICapMaster(
	Handle	CapDefs,
	short	funcResId)
{
	short	i;
	
	for(i=0;i<NUMCAPS;i++)
	{	caps[i] = new CCapKey;
		caps[i]->ICapKey(i, this);
	}
	
	AddFuncs(funcResId);

	currentFunc = NULL;
	currentCap = NULL;
	itsWindow = NULL;
	isActive = false;
}

void	CCapMaster::AddFuncs(
	short	funcResId)
{
	Handle	funcRes;
	short	i;

	numFuncs = 0;
	numPositions = 0;
	
	funcRes = GetResource('KFUN', funcResId);
	if(funcRes)
	{	charWordLongP	aPtr;
	
		HLock(funcRes);
		aPtr.c = *funcRes;
		
		funcsPerColumn = *aPtr.w++;
		if(funcsPerColumn > MAXFUNCSPERKEY+1)
		{	funcsPerColumn = MAXFUNCSPERKEY+1;
		}
		
		numFuncs = *aPtr.w++;
		if(numFuncs > MAXFUNCS)
			numFuncs = MAXFUNCS;

		for(i=0;i<MAXFUNCS;i++)
		{	funcs[i] = NULL;
		}

		for(i=0;i<numFuncs;i++)
		{	unsigned char *name;
			unsigned char *help;
			short		icon;
			short		thePosition;
			long		mask;
		
			name = aPtr.str;
			aPtr.c += name[0] - (name[0] & 1) + 2;
			help = aPtr.str;
			aPtr.c += help[0] - (help[0] & 1) + 2;
			thePosition = *aPtr.w++;
			icon = *aPtr.w++;
			mask = *aPtr.l++;

			order[thePosition] = i;

			if(name[0])
			{	if(thePosition > numPositions)
					numPositions = thePosition;
				funcs[i] = new CCapFunc;
				funcs[i]->ICapFunc(i, name, help, icon, mask);
			}
		}
		
		HUnlock(funcRes);
	}
	
	ReleaseResource(funcRes);

}

void	CCapMaster::Dispose(void)
{
	short	i;
	
	for(i=0;i<numFuncs;i++)
	{	if(funcs[i])	
			funcs[i]->Dispose();
	}
	
	for(i=0;i<NUMCAPS;i++)
	{	caps[i]->Dispose();
	}

	if(itsWindow)
		DisposeWindow(itsWindow);
	
	delete this;

}


void	CCapMaster::GetKeyCaps()
{
	#define SHAPEMAXPTS 20  /* hopefully, fewer than 20 points per shape */

	typedef struct
	{
		char modifierMask;
		char keyCode;
		short deltaV;
		short deltaH;
	} KeyEntryRec;
	
	Handle	specialTable;
	short	modifiers = 0;
	short	kchrNum;
	short	kcapNum;
	Rect	tempRect;
	Rect	totalRect;
	long	rectCount = 0;
	Handle	kchrResHandle;

	specialTable = GetResource('NMAP', NAMEMAPID);
	kchrNum = GetScript(GetEnvirons(smKeyScript), smScriptKeys);
	kcapNum = KbdDebugOffset + LMGetKbdType();  /* KCAP ID of current keyboard */
	lastKeyboardType = kcapNum;
	lastKchrNum = kchrNum;

	kchrResHandle = RGetResource('KCHR', kchrNum);
	if (ResError() == noErr && kchrResHandle != nil)
	{
		Ptr			kcapPtr;
		Handle		kcapResHandle;
		long		state = 0;

		kcapResHandle = RGetResource('KCAP', kcapNum);
		if (ResError() == noErr && kcapResHandle != nil)
		{
			short		mainIndex, shapeIndex, keyIndex, shapeTotal, shapeCount;
			short		modifiedKeyCode;
			RgnHandle	keyshapeRgnHandle;

			keyshapeRgnHandle = NewRgn();
			
			HLock(kcapResHandle);
			kcapPtr = *kcapResHandle;
			kcapPtr += sizeof(Rect);		/* skip boundary from origin */
			kcapPtr += sizeof(Rect);		/* skip textedit area */
			
			/* loop through main array */
			mainIndex = *((short *) kcapPtr);
			kcapPtr += sizeof(short);

			while(mainIndex-- > 0)
			{
				Point		shapePoint[SHAPEMAXPTS];

				/* loop through shape array - build array of points for this shape */
				shapeIndex = *((short *) kcapPtr);
				kcapPtr += sizeof(short);

				shapeTotal = (shapeIndex < SHAPEMAXPTS ? shapeIndex + 1 : SHAPEMAXPTS);
				for (shapeCount=0; shapeIndex>-1; shapeIndex--, shapeCount++)
				{
					shapePoint[shapeCount] = *((Point *) kcapPtr);
					kcapPtr += sizeof(Point);
				}
				
				/* start drawing keys of this shape from 0,0 */
				MoveTo(0,0);
				
				/* loop through key array */
				keyIndex = *((short *) kcapPtr);
				kcapPtr += sizeof(short);
				while(keyIndex-- >= 0)
				{	char		theChar;
					char		theKeyCode;
					short		specialKey;
					KeyEntryRec	thisKeyEntryRec;
					Point		currPoint, penPoint;

					/* get modifier mask, keyCode, and offset from previous key */					
					thisKeyEntryRec = *((KeyEntryRec *) kcapPtr);
					kcapPtr += sizeof(KeyEntryRec);
					
					/* move the pen to the start of the key */
					Move(thisKeyEntryRec.deltaH, thisKeyEntryRec.deltaV);
					
					/* draw the key, composed of one or more rects */
					SetPt(&currPoint, 0, 0);
					OpenRgn();
					
					for (shapeCount=0, shapeIndex=shapeTotal; shapeIndex; 
								shapeIndex--, shapeCount++)
					{
						Point	swapPoint;
					
						//	set the rect, then reverse coordinates if necessary
						//	to ensure it is not empty
						
						SetRect(&tempRect, currPoint.h, currPoint.v, 
							shapePoint[shapeCount].h, shapePoint[shapeCount].v);
							
						if (tempRect.top > tempRect.bottom)
						{	swapPoint.v = tempRect.top;
							tempRect.top = tempRect.bottom;
							tempRect.bottom = swapPoint.v;
						}
						
						if (tempRect.left > tempRect.right)
						{	swapPoint.h = tempRect.left;
							tempRect.left = tempRect.right;
							tempRect.right = swapPoint.h;
						}
						
						/* move the rect to the pen location and add it to the region */
						currPoint = shapePoint[shapeCount];
						GetPen(&penPoint);
						OffsetRect(&tempRect, penPoint.h, penPoint.v);	
						FrameRect(&tempRect);

						if(rectCount++)
							UnionRect(&tempRect, &totalRect, &totalRect);
						else
							totalRect = tempRect;
					}
					
					/* draw the key frame */
					CloseRgn(keyshapeRgnHandle);
					
					/* convert the keyCode to a character code */
					/* mask out high bit of keyCode and add masked modifiers;
					   KeyTrans stroke bit taken from modifier parameter */
					
					modifiers = 0;
					if (thisKeyEntryRec.keyCode & 0x80) 
						modifiers &= (((short) thisKeyEntryRec.modifierMask) << 8);
					else
						modifiers |= (((short) thisKeyEntryRec.modifierMask) << 8);
					
					theKeyCode = thisKeyEntryRec.keyCode & 0x007F;
									
					specialKey = (*specialTable)[theKeyCode];
					
					if(specialKey == 0)
					{
						modifiedKeyCode = theKeyCode | (modifiers & 0xFF80);
						theChar = KeyTrans(*kchrResHandle, 0x80 | modifiedKeyCode, (void *)&state);
						caps[theKeyCode]->SetKeyChar(theChar);
					}
					else
					{	Str255	tempS;
					
						GetIndString(tempS, KEYSTRINGLIST, specialKey);
						caps[theKeyCode]->SetKeyString(tempS);
					}

					if(theKeyCode < 0x3C || theKeyCode > 0x3E)
					{	caps[theKeyCode]->AddKeyArea(
													(tempRect.left + tempRect.right) / 2,
													(tempRect.top + tempRect.bottom) / 2,
													keyshapeRgnHandle);
					}
					
					/* clear region and reposition pen for next key */
					SetEmptyRgn(keyshapeRgnHandle);
					MoveTo(penPoint.h, penPoint.v);
				}
			}
			HUnlock(kcapResHandle);
			DisposeRgn(keyshapeRgnHandle);
			
			/* release the KCAP and KCHR unless they're System resources */
			if (HomeResFile(kcapResHandle) > 1) ReleaseResource(kcapResHandle);
		}	
		if (HomeResFile(kchrResHandle) > 1) ReleaseResource(kchrResHandle);
	}
	
	{	short	i;
		Point	offset;
		Point	newWindowSize;
		short	x;
		
		offset.h = PANELMARGIN - totalRect.left;
		offset.v = PANELMARGIN - totalRect.top;
	
		for(i=0;i<NUMCAPS;i++)
		{	caps[i]->OffsetKey(offset.h, offset.v);
		}
		
		OffsetRect(&totalRect, offset.h, offset.v);
		keysRect = totalRect;

		infoRect.left = MAINMARGIN;
		infoRect.right = infoRect.left + INFOPANELWIDTH;
		infoRect.top = keysRect.bottom + MAINMARGIN;
		infoRect.bottom =	MAXFUNCSPERKEY < numFuncs ?
							MAXFUNCSPERKEY : numFuncs;
		infoRect.bottom = infoRect.top + infoRect.bottom * INFOLINEHEIGHT + FIRSTLINESPACE
							+ 1 * PANELMARGIN;

		funcRect.left = infoRect.right + MAINMARGIN;
		funcRect.top = infoRect.top;
		funcRect.right = keysRect.right - MAINMARGIN + PANELMARGIN;
		funcRect.bottom = infoRect.bottom;
		
		x = (numPositions + funcsPerColumn - 1) / funcsPerColumn;
		x = x * FUNCMAXWIDTH + 2 * PANELMARGIN;
		if(funcRect.right - funcRect.left < x)
		{	funcRect.right = funcRect.left + x;
		}
		
		newWindowSize.v = infoRect.bottom > funcRect.bottom ?
						  infoRect.bottom : funcRect.bottom;
		newWindowSize.h = funcRect.right;
		newWindowSize.h += MAINMARGIN;
		newWindowSize.v += MAINMARGIN;
		
		SizeWindow(	itsWindow,
					newWindowSize.h, newWindowSize.v, true);
	}
	
	ReleaseResource(specialTable);
}

void	CCapMaster::DrawInfo(void)
{
	short	oldSize;
	short	oldFont;
	short	oldFace;
	short	oldMode;
	short	i;
	short	x,y;

	SetPort(itsWindow);
	oldSize = itsWindow->txSize;	TextSize(12);
	oldFont = itsWindow->txFont;	TextFont(1);
	oldFace = itsWindow->txFace;	TextFace(underline);
	oldMode = itsWindow->txMode;	TextMode(srcOr);

	x = infoRect.left + PANELMARGIN;
	y = infoRect.top + FIRSTLINESPACE;

	if(currentCap)
	{	MoveTo(x, y - 6);
		DrawString((StringPtr)currentCap->label);
		TextFace(0);
		DrawChar(':');
		
		for(i=0;i<currentCap->funcCount;i++)
		{	CCapFunc	*theFunc;
		
			theFunc = funcs[currentCap->funcList[i]];
			if(theFunc)
			{	theFunc->DrawFull(x,y);
				y += INFOLINEHEIGHT;
			}
		}
	}
	else
	{	Rect	tempFrame;

		tempFrame = infoRect;
		InsetRect(&tempFrame, PANELMARGIN, PANELMARGIN);
		TextSize(9);
		TextFace(0);

		if(currentFunc)
		{	currentFunc->DrawFull(x, y - FUNCHELPOFFSET);
			tempFrame.top += FUNCHELPMARGIN;
			TextBox(currentFunc->help+1, currentFunc->help[0], &tempFrame, teJustLeft);
		}
		else
		{	Handle	theHelpText;
		
			theHelpText = GetResource('TEXT', MASTERHELP);
			HLock(theHelpText);
			TextBox(*theHelpText, GetHandleSize(theHelpText), &tempFrame, teJustLeft);
			
			HUnlock(theHelpText);
			ReleaseResource(theHelpText);
		}
	}

	TextSize(oldSize);
	TextFont(oldFont);
	TextFace(oldFace);
	TextMode(oldMode);
}
void	CCapMaster::DrawFuncs(void)
{
	short		x,y;
	short		i,j;
	CCapFunc	*theFunc;
	
	SetPort(itsWindow);
	
	x = funcRect.left + PANELMARGIN;
	y = funcRect.top + PANELMARGIN;
	
	j = 1;
	for(i=0;i<numFuncs;i++)
	{	theFunc = funcs[order[i]];
	
		if(theFunc)
		{	theFunc->DrawFull(x,y);
		}
		y += INFOLINEHEIGHT;
		
		if(y + INFOLINEHEIGHT > funcRect.bottom || j++ >= funcsPerColumn)
		{	y = funcRect.top + PANELMARGIN;
			x += FUNCMAXWIDTH;
			j = 1;
		}
	}
}

void	CCapMaster::DrawKeys(void)
{
	short	i;
	
	SetPort(itsWindow);
	for(i=0;i<NUMCAPS;i++)
	{	caps[i]->DrawKey();
	}
}

void	CCapMaster::DrawAll(void)
{
	short	temp;
	
	temp =	infoRect.bottom > funcRect.bottom ?
		  	infoRect.bottom : funcRect.bottom;

	MoveTo(infoRect.left - PANELMARGIN, infoRect.top - PANELMARGIN);
	LineTo(funcRect.right + PANELMARGIN, infoRect.top - PANELMARGIN);
	MoveTo(infoRect.right + PANELMARGIN, infoRect.top - PANELMARGIN);
	LineTo(infoRect.right + PANELMARGIN, temp + PANELMARGIN);

	DrawInfo();
	DrawKeys();
	DrawFuncs();
}

void	CCapMaster::CreateWindow(
	WindowPtr	behind,
	Boolean		doShow)
{
	itsWindow = GetNewCWindow(WINDRESID, 0, behind);
	SetPort(itsWindow);
	GetKeyCaps();
	if(doShow)	ShowWindow(itsWindow);
}

void	CCapMaster::UpdateWindow()
{
	SetPort(itsWindow);
	BeginUpdate(itsWindow);
	EraseRect(&itsWindow->portRect);
	DrawAll();
	EndUpdate(itsWindow);
}

void	CCapMaster::DoFuncAreaMouseDown(
	EventRecord	*theEvent,
	Point		where)
{
	short		x,y;
	short		i,j;
	CCapFunc	*theFunc;

	x = funcRect.left + PANELMARGIN;
	y = funcRect.top + PANELMARGIN;
	
	for(j=1,i=0;i<numFuncs;i++)
	{	theFunc = funcs[order[i]];
		
		if(theFunc && theFunc->DragFull(x,y, where, this, &funcRect))
		{	return;
		}

		y += INFOLINEHEIGHT;
		
		if(y + INFOLINEHEIGHT > funcRect.bottom || j++ >= funcsPerColumn)
		{	y = funcRect.top + PANELMARGIN;
			x += FUNCMAXWIDTH;
			j = 1;
		}
	}
	
	if(currentFunc || currentCap)
	{	InvalRect(&infoRect);
		currentFunc = 0;
		currentCap = 0;
	}
}

void	CCapMaster::DoInfoAreaMouseDown(
	EventRecord	*theEvent,
	Point		where)
{
	short	i;
	short	x,y;

	if(currentCap)
	{	x = infoRect.left + PANELMARGIN;
		y = infoRect.top + FIRSTLINESPACE;
	
		MoveTo(x, infoRect.top + FIRSTLINESPACE - 6);
		DrawString((StringPtr)currentCap->label);
		DrawChar(':');
		
		for(i=0;i<currentCap->funcCount;i++)
		{	RgnHandle	dragRegion;
		
			dragRegion = funcs[currentCap->funcList[i]]->GetFullRegion(x,y);
			if(PtInRgn(where, dragRegion))
			{	currentCap->DragFunc(funcs[currentCap->funcList[i]], where, dragRegion);
				i = currentCap->funcCount;
			}
			
			DisposeRgn(dragRegion);
			y += INFOLINEHEIGHT;
		}
	}
	else
	{	if(currentFunc)
		{	x = infoRect.left + PANELMARGIN;
			y = infoRect.top + FIRSTLINESPACE - FUNCHELPOFFSET;
			currentFunc->DragFull(x,y, where, this, &funcRect);
		}
	}
}

void	CCapMaster::DoKeysAreaMouseDown(
	EventRecord	*theEvent,
	Point		where)
{
	short	i;
	short	j;
	
	for(i=NUMCAPS-1;i>=0;i--)
	{	CCapKey		*theCap;
		
		theCap = caps[i];
		if(PtInRgn(where, theCap->theKeyRegion))
		{	CCapFunc	*theFunc = NULL;
		
			for(j = theCap->funcCount-1; j>=0;j--)
			{	theFunc = funcs[theCap->funcList[j]];
				if(theFunc)	break;
			}

			if(theFunc)
			{	RgnHandle	dragRegion;
				Rect		dragRect;
			
				dragRegion = NewRgn();
				SetRect(&dragRect, where.h - 9, where.v - 9, where.h + 10, where.v + 10);
				RectRgn(dragRegion, &dragRect);
				theCap->DragFunc(theFunc, where, dragRegion);
				
				DisposeRgn(dragRegion);
			}
			else
			{	currentCap = theCap;
				InvalRect(&infoRect);
			}
			
			break;
		}
	}
}

void	CCapMaster::DoMouseDown(
	EventRecord	*theEvent)
{
	Point	local;
	
	SetPort(itsWindow);
	local = theEvent->where;
	GlobalToLocal(&local);
	
	if(PtInRect(local, &funcRect))
	{	DoFuncAreaMouseDown(theEvent, local);
	}
	else if(PtInRect(local, &infoRect))
	{	DoInfoAreaMouseDown(theEvent, local);
	}
	else if(PtInRect(local, &keysRect))
	{	DoKeysAreaMouseDown(theEvent, local);
	}
}

void	CCapMaster::ChangeLayout()
{
	short	i;
	
	for(i=0;i<NUMCAPS;i++)
	{	caps[i]->ResetCap();
	}
	
	GetKeyCaps();
	
	SetPort(itsWindow);
	InvalRect(&itsWindow->portRect);
}

Boolean	CCapMaster::DoEvent(
	EventRecord	*theEvent)
{
	WindowPtr	theWind;
	short		placeCode;
	Boolean		didHandle = false;
	
	if(	lastKeyboardType != (KbdDebugOffset + LMGetKbdType()) ||
		lastKchrNum != GetScript(GetEnvirons(smKeyScript), smScriptKeys))
	{	ChangeLayout();
	}

	switch(theEvent->what)
	{	case updateEvt:
			if((WindowPtr)theEvent->message == itsWindow)
			{	didHandle = true;
				UpdateWindow();
			}
			break;
		case mouseDown:
			placeCode = FindWindow(theEvent->where,&theWind);
			if(theWind == itsWindow && placeCode == inContent)
			{	didHandle = true;
				DoMouseDown(theEvent);
			}
			break;
		case activateEvt:
			if((WindowPtr)theEvent->message == itsWindow)
			{
				isActive = theEvent->modifiers & activeFlag;
			}
			break;
		case keyDown:
		case autoKey:
		case keyUp:
			{	CCapKey		*pressed;
			
				didHandle = isActive;

				pressed = caps[(theEvent->message >> 8) & 0x7F];
				if(pressed != currentCap)
				{	currentCap = pressed;
					SetPort(itsWindow);
					InvalRect(&infoRect);
				}
			}
			break;
	}
	
	return didHandle;
}

void	CCapMaster::GetCapMap(
	Handle	capMap)
{
	SetHandleSize(capMap, FUNCLONGS * sizeof(long) * NUMCAPS);

	if(!MemError())
	{
		short	i;
		long	*capPtr;
		
		HLock(capMap);
		capPtr = (long *) *capMap;

		for(i=0;i<NUMCAPS;i++)
		{	caps[i]->WriteCaps(capPtr);
			capPtr += FUNCLONGS;
		}
		
		HUnlock(capMap);
	}
}

void	CCapMaster::SetCapMap(
	Handle	capMap)
{
	if(capMap)
	{	
		short	i;
		long	*capPtr;
		
		HLock(capMap);
		capPtr = (long *) *capMap;

		for(i=0;i<NUMCAPS;i++)
		{	caps[i]->ReadCaps(capPtr);
			capPtr += FUNCLONGS;
		}
		
		HUnlock(capMap);
		
		if(itsWindow)
		{	SetPort(itsWindow);
			InvalRect(&itsWindow->portRect);
		}
	}
}