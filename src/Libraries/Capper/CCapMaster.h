/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CCapMaster.h
    Created: Friday, November 11, 1994, 21:01
    Modified: Tuesday, December 19, 1995, 19:22
/*/

#pragma once			

#define	NUMCAPS			128
#define	MAXFUNCS		64
#define	FUNCLONGS		((MAXFUNCS+31)/32)
#define	WINDRESID		8000
#define	KEYSTRINGLIST	8000
#define	NAMEMAPID		8000
#define	MASTERHELP		8000

#define	FUNCMAXWIDTH	(20+100)
#define	INFOLINEHEIGHT	18
#define	FIRSTLINESPACE	18
#define	MAINMARGIN		8
#define	PANELMARGIN		4
#define	INFOPANELWIDTH	FUNCMAXWIDTH+MAINMARGIN+MAINMARGIN
#define	FUNCHELPOFFSET	14
#define	FUNCHELPMARGIN	24

class	CCapMaster : direct
{
public:
	class	CCapKey		*caps[NUMCAPS];
	class	CCapFunc	*funcs[MAXFUNCS];
			short		order[MAXFUNCS];
			CCapKey		*currentCap;
			CCapFunc	*currentFunc;
			
			Boolean		isActive;
			char		lastKeyboardType;
			short		lastKchrNum;
			short		funcsPerColumn;
			short		numFuncs;
			short		numPositions;
			Rect		keysRect;
			Rect		infoRect;
			Rect		funcRect;
			WindowPtr	itsWindow;
	
	virtual	void		ICapMaster(Handle CapDefs, short funcResId);
	virtual	void		AddFuncs(short funcResId);
	virtual	void		GetKeyCaps(void);
	virtual	void		CreateWindow(WindowPtr behind, Boolean doShow);
	virtual	void		UpdateWindow(void);
	virtual	void		DrawKeys(void);
	virtual	void		DrawAll(void);
	virtual	void		DrawInfo(void);
	virtual	void		DrawFuncs(void);
	virtual	void		Dispose(void);
	
	virtual	void		DoFuncAreaMouseDown(EventRecord *theEvent, Point where);
	virtual	void		DoInfoAreaMouseDown(EventRecord *theEvent, Point where);
	virtual	void		DoKeysAreaMouseDown(EventRecord *theEvent, Point where);
	virtual	void		DoMouseDown(EventRecord *theEvent);
	virtual	Boolean		DoEvent(EventRecord *theEvent);
	virtual	void		ChangeLayout(void);
	
	virtual	void		GetCapMap(Handle capMap);
	virtual	void		SetCapMap(Handle capMap);
};