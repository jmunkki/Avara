/*
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: CAsyncBeeper.h
    Created: Tuesday, July 23, 1996, 2:28
    Modified: Tuesday, July 23, 1996, 2:56
*/

#pragma once
#include "CDirectObject.h"
#include "Sound.h"

class	CAsyncBeeper : public CDirectObject
{
public:

			SndChannelPtr		beepChannel;
			Handle				beepSound;

	virtual	void				IAsyncBeeper();
	virtual	void				BeepDoneCheck();
	virtual	void				AsyncBeep(Handle theSoundRes);
	virtual	void				PlayNamedBeep(StringPtr beepName);
	virtual	void				Dispose();

};

#ifdef MAIN_ASYNCBEEPER
		CAsyncBeeper	*gAsyncBeeper = NULL;
#else
extern	CAsyncBeeper	*gAsyncBeeper;
#endif