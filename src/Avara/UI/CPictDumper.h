/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.
/*/

#pragma once
#include "CDirectObject.h"

#define		kMaxDumps	64

class CPictDumper : public CDirectObject
{
public:
			short		numDumps;
			PicHandle	allDumps[kMaxDumps];
			Str255		nameHeader;
	
	virtual	void	IPictDumper(StringPtr fileNameStart);
	virtual	void	TakeShot(WindowPtr theWindow);
	virtual	void	CloseDumper(Boolean	doSave);
};