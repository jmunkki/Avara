/*
    Copyright ©1994, Juri Munkki
    All rights reserved.

    File: CExitHandler.h
    Created: Sunday, November 20, 1994, 0:38
    Modified: Sunday, November 20, 1994, 1:00
*/

#pragma once
#include "CDirectObject.h"

typedef	void	ExitFuncType(long theData);

struct ExitRecord
{
		ExitFuncType	*exitFunc;
		long			theData;
struct	ExitRecord		*nextExit;
};

typedef struct ExitRecord ExitRecord;

class CExitHandler : public CDirectObject
{
public:
	ExitRecord		*firstExit;
	
	virtual	void	IExitHandler();
	virtual	void	AddExit(ExitRecord *theExit);
	virtual	void	RemoveExit(ExitRecord *theExit);
	virtual	void	DoExit();
};

void	StartExitHandler(void);

#ifndef MAINEXITHANDLER
extern	CExitHandler	*gExitHandler;
#else
CExitHandler	*gExitHandler = NULL;
#endif