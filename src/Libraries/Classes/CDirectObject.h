/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CDirectObject.h
    Created: Friday, March 11, 1994, 17:14
    Modified: Tuesday, February 20, 1996, 7:20
/*/

#pragma once

class	CDirectObject
#ifdef THINK_C
	: direct
#endif
{
public:
#ifdef THINK_C
				void *	operator	new(long objSize);
				void	operator	delete(void *objStorage);
#endif
	virtual		void				Dispose();
};

void	AllocateQuickDirectStorage(long numBytes);
void	DeallocateQuickDirectStorage();
void	RestoreQuickBlock(long len, Ptr mem);
Ptr		AquireQuickBlock(long len);
