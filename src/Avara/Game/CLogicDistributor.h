/*
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: CLogicDistributor.h
    Created: Wednesday, November 22, 1995, 7:48
    Modified: Wednesday, November 22, 1995, 7:57
*/

#pragma once
#include "CLogic.h"

class	CLogicDistributor : public CLogic
{
public:
			short			sendNext;
	
	virtual	CAbstractActor	*EndScript();
	virtual	void			FrameAction();
};