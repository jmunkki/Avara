/*/
    Copyright ©1992-1994, Juri Munkki
    All rights reserved.

    File: CBaseObject.h
    Created: Friday, October 23, 1992, 16:22
    Modified: Tuesday, October 18, 1994, 15:49
/*/

#pragma once

#ifndef __CBASEOBJECT__
#define __CBASEOBJECT__

#include "cplusminusutil.h"

#ifdef	NO_PRECOMPILED_HEADERS
#include <Types.h>
#include <Memory.h>
#endif

class	CBaseObject 
#ifdef THINK_C
	: indirect
#endif
{
protected:
		short	lockCounter;
#ifdef __cplusplus
		Size	fObjectSize;
#endif

public:
#ifdef __cplusplus
			 				CBaseObject(void);
#endif
	virtual void			IBaseObject(void);
	virtual void			Lock(void);
	virtual void			Unlock(void);	
	
	virtual void			LockThis(void);
	virtual void			UnlockThis(void);

	virtual void			ForceUnlock(void);
	virtual void			Dispose(void);
	virtual CBaseObject*	Clone(void);
	virtual void			CloneFields(void);
	virtual Handle			CloneHandle(Handle theHandle);
	
	virtual Size			HowMuchMemory(void);

#ifdef __cplusplus
			void *			operator new(size_t reqSize);
			void 			operator delete(void *thePointer, size_t theSize);
#endif
};

#endif
