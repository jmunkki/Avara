/*
    Copyright ©1993-1994, Juri Munkki
    All rights reserved.

    File: CBaseObject.c
    Created: Sunday, July 25, 1993, 7:48
    Modified: Friday, October 14, 1994, 14:29
*/

#include "CBaseObject.h"

#ifdef NO_PRECOMPILED_HEADERS
#include <Memory.h>
#include <OSUtils.h>
#endif

#ifdef __cplusplus
static	Size	sObjectSizeTemp;
/*
**	May cause problems with pre-emptive threads.
*/
void *	CBaseObject::operator new(
	size_t	reqSize)
{
	sObjectSizeTemp = (Size)reqSize;
	return (void *)new char[reqSize];
}
void 	CBaseObject::operator delete(
	void	*thePointer,
	size_t	theSize)
{
	delete [] ((char *)thePointer);
}
CBaseObject::CBaseObject(void)
{
	fObjectSize = sObjectSizeTemp;
}
#endif

void	CBaseObject::IBaseObject()
{
	lockCounter = 0;
}

void	CBaseObject::Lock()
{
	if(!lockCounter)
		LockThis();
	lockCounter++;
}

void	CBaseObject::Unlock()
{
	if(lockCounter != 0)
	{	if(--lockCounter == 0)
		{	UnlockThis();
		}
	}
}

void	CBaseObject::LockThis()
{
#ifndef __cplusplus
		HLock((Handle) this);
#endif
}

void	CBaseObject::UnlockThis()
{
#ifndef __cplusplus
		HUnlock((Handle) this);
#endif
}

void	CBaseObject::ForceUnlock()
{
	if(lockCounter > 1) lockCounter = 1;
	Unlock();
}	

void	CBaseObject::Dispose()
{
	delete(this);
}

/*
**	This is a utility method for all
**	my classes. The idea is to make
**	copying Handles that are part of
**	an object easy like this:
**
**		thing = CloneHandle(thing);
**
**	Note that you can't simply call
**	HandToHand(&thing), because thing
**	is an instance variable in a block
**	that might move.
*/
Handle	CBaseObject::CloneHandle(
	Handle theHandle)
{
	char	state;
	
	state = HGetState(theHandle);
	HandToHand(&theHandle);
	if(theHandle)
		HSetState(theHandle, state);
	
	return theHandle;
}

/*
**	Never call this method, but override it,
**	if you are writing a class that should
**	duplicate handles when an object is cloned.
*/
void	CBaseObject::CloneFields()
{

}

/*
**	Call clone to make a deep copy of the object.
*/
CBaseObject*	CBaseObject::Clone()
{
	CBaseObject	*myCopy;
	char		state;
	

#ifdef __cplusplus
	myCopy = (CBaseObject*) new char[fObjectSize];
	
	if (myCopy) {
		BlockMove((Ptr) this, (Ptr) myCopy, fObjectSize);
		myCopy->CloneFields();
	}
#else
	myCopy = this;

	state = HGetState((Handle)this);
	HandToHand((Handle *)&myCopy);
	
	if(myCopy)
	{	HSetState((Handle)myCopy,state);
		myCopy->CloneFields();
	}
#endif	
	return myCopy;
}

Size	CBaseObject::HowMuchMemory()
{
#ifdef __cplusplus
	return fObjectSize;
#else
	return GetHandleSize((Handle)this);
#endif
}