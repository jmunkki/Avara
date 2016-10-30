/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CExitHandler.c
    Created: Sunday, November 20, 1994, 0:42
    Modified: Sunday, April 28, 1996, 2:20
/*/

#define MAINEXITHANDLER
#include "CExitHandler.h"
#include "LowMem.h"

static	long	oldExit;

static
pascal
void	NewExitHandler()
{
	Ptr		goodA5;
	
	goodA5 = LMGetCurrentA5();

	asm	{	move.l	goodA5,A5
		}

	gExitHandler->DoExit();
	
	asm	{	unlk	A6
			move.l	oldExit,A0
			jmp		(A0)
		}
}

void	CExitHandler::IExitHandler()
{
	oldExit=(long)NGetTrapAddress(0x9F4,ToolTrap);
	NSetTrapAddress((void *)NewExitHandler,0x9F4,ToolTrap);
	
	firstExit = NULL;
}

void	CExitHandler::AddExit(
	ExitRecord	*theExit)
{
	theExit->nextExit = firstExit;
	firstExit = theExit;
}
void	CExitHandler::RemoveExit(
	ExitRecord	*theExit)
{
	ExitRecord	*ep, **px;
	
	ep = firstExit;
	px = &firstExit;
	
	while(ep != NULL)
	{	if(ep == theExit)
		{	*px = theExit->nextExit;
			ep = NULL;
		}
		else
		{	px = &ep->nextExit;
			ep = ep->nextExit;
		}
	}
	
	theExit->exitFunc = NULL;
}
void	CExitHandler::DoExit()
{
	ExitRecord	*nextOne;
	ExitRecord	*newList;
	
	newList = NULL;
	
	while(firstExit)
	{	nextOne = firstExit->nextExit;
		firstExit->nextExit = newList;
		newList = firstExit;
		firstExit = nextOne;
	}
	
	while(newList)
	{	nextOne = newList->nextExit;
		if(newList->exitFunc)
		{	newList->exitFunc(newList->theData);
		}
		newList = nextOne;
	}
}

void	StartExitHandler()
{
	if(!gExitHandler)
	{	gExitHandler = new CExitHandler;
		gExitHandler->IExitHandler();
	}
}