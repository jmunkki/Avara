/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CCapFunc.h
    Created: Friday, November 11, 1994, 21:24
    Modified: Friday, December 1, 1995, 22:20
/*/

#pragma once

#define HILITESTUFF		BitClr((Ptr)0x938 /* HiliteMode */, 7 /* hiliteBit */)

class	CCapMaster;

class	CCapFunc : direct
{
public:
			short		id;
			Str63		name;
			Str255		help;
			short		icon;
			CIconHandle	iconHandle;
			long		mask;
	
	virtual	void		ICapFunc(short theId, StringPtr theName, StringPtr theHelp,
									short theIcon, long theMask);
	virtual	void		DrawIcon(short x, short y);
	virtual	void		DrawFull(short x, short y);
	virtual	RgnHandle	GetFullRegion(short x, short y);
	virtual	void		Dispose();
	virtual	Boolean		DragFull(short x, short y, Point where, CCapMaster *master, Rect *from);
	virtual	Boolean		DoDrag(	CCapMaster *master, RgnHandle fromRegion,
								RgnHandle dragRegion, Point where);

};