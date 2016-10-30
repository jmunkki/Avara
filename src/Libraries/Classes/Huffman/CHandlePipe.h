/*/
    Copyright ©1994, Juri Munkki
    All rights reserved.

    File: CHandlePipe.h
    Created: Tuesday, September 20, 1994, 17:42
    Modified: Monday, October 3, 1994, 20:33
/*/

/*
**	A fairly trivial class that outputs all its input to a handle.
*/

#pragma once
#include "CAbstractPipe.h"

class	CHandlePipe : public CAbstractPipe
{
public:
	Handle		outputHandle;
	
	virtual	OSErr	Open();

	virtual	void	PipeToHandle(Handle output);
	virtual	Handle	GetDataHandle();

	virtual	OSErr	PipeData(Ptr dataPtr, long len);
	virtual	void	Dispose();
};