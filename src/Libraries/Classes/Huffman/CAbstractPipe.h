/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CAbstractPipe.h
    Created: Tuesday, September 20, 1994, 17:33
    Modified: Thursday, June 1, 1995, 1:31
/*/

#pragma once
#include "CBaseObject.h"

class	CAbstractPipe : public CBaseObject
{
public:
	class	CAbstractPipe		*outputStream;

//	In general, pipe methods are used in this order:
	virtual	OSErr	Open();
	virtual	OSErr	PipeTo(CAbstractPipe *output);
	virtual	OSErr	PipeData(Ptr dataPtr, long len);
	virtual	OSErr	Close();
};