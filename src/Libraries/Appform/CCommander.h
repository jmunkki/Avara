/*
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CCommander.h
    Created: Wednesday, November 16, 1994, 0:56
    Modified: Friday, May 12, 1995, 15:30
*/

#pragma once
#include "CDirectObject.h"

class	CEventHandler;
class	CCommander;

class	CCommander : public CDirectObject
{
public:
	class	CCommander	*itsCommander;
	
	virtual	void		ICommander(CCommander *theCommander);
	virtual	void		AdjustMenus(CEventHandler *master);
	virtual	void		DoCommand(long theCommand);
	virtual	Boolean		DoEvent(CEventHandler *master, EventRecord *theEvent);
	virtual	void		BecomeTarget();
	virtual	void		StopBeingTarget();
	
	virtual	void		Dispose();

	virtual	void		OSErrorNotify(short id);
};

#ifndef	MAINCOMMANDER
extern	CCommander	*theTarget;
#endif
