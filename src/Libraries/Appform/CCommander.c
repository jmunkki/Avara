/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CCommander.c
    Created: Wednesday, November 16, 1994, 0:57
    Modified: Thursday, December 21, 1995, 20:49
/*/

#define	MAINCOMMANDER
#include "CCommander.h"
#include "CEventHandler.h"

CCommander	*theTarget;

void	CCommander::ICommander(
	CCommander		*theCommander)
{	
	itsCommander = theCommander;
}

void	CCommander::AdjustMenus(
	CEventHandler	*master)
{
	if(itsCommander)
		itsCommander->AdjustMenus(master);
}

void	CCommander::DoCommand(
	long			theCommand)
{
	if(itsCommander)
		itsCommander->DoCommand(theCommand);
}

Boolean	CCommander::DoEvent(
	CEventHandler	*master,
	EventRecord		*theEvent)
{
	if(itsCommander)
	{	return itsCommander->DoEvent(master, theEvent);
	}
	else
	{	return false;
	}
}

void	CCommander::StopBeingTarget()
{
	if(theTarget == this) theTarget = NULL;
}

void	CCommander::BecomeTarget()
{
	if(theTarget != this)
	{	if(theTarget)
		{	theTarget->StopBeingTarget();
		}
		
		theTarget = this;
		SetCursor(&qd.arrow);
	}
}

void	CCommander::OSErrorNotify(
	short	id)
{
	Str63	idString;
	
	if(id)
	{	NumToString(id, idString);
		ParamText(idString, 0,0,0);		
		Alert(401, 0);
	}
}

void	CCommander::Dispose()
{
	StopBeingTarget();

	inherited::Dispose();
}