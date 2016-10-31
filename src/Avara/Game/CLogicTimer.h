/*
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CLogicTimer.h
    Created: Wednesday, November 22, 1995, 7:47
    Modified: Sunday, January 21, 1996, 5:06
*/

#pragma once
#include "CLogic.h"

class	CLogicTimer : public CLogic
{
public:
			long			theDelay;
			long			whenFrame;
			
	virtual	void			FrameAction();
	virtual	void			BeginScript();
	virtual	CAbstractActor *EndScript();
};