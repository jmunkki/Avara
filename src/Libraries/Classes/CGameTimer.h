/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CGameTimer.h
    Created: Sunday, November 20, 1994, 0:21
    Modified: Tuesday, June 18, 1996, 21:40
/*/

#pragma once
#include "CDirectObject.h"
#include "CExitHandler.h"

typedef	struct
{
	TMTask	task;

	long	stepTime;
	long	stepCount;
	long	currentStep;

}	GameTimerRecord;


class	CGameTimer : public CDirectObject
{
public:
			GameTimerRecord	timer;
			ExitRecord		theExit;

	virtual	void			IGameTimer();
	virtual	void			Dispose();
	virtual	void			SetFrameTiming(long frameTime);
	virtual	void			ResetTiming();

	virtual	void			StopTimer();
	virtual	void			ResumeTimer();

	virtual	long			GetStepCount();
};