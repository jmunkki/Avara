/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CPlacedActors.c
    Created: Tuesday, November 22, 1994, 5:46
    Modified: Sunday, January 21, 1996, 5:11
/*/

#include "CPlacedActors.h"
#include "CSmartPart.h"

void	CPlacedActors::IAbstractActor()
{
	location[0] = 0;
	location[1] = 0;
	location[2] = 0;
	
	inherited::IAbstractActor();
}

void	CPlacedActors::BeginScript()
{
	inherited::BeginScript();

	ProgramFixedVar(iHeight, location[1]);
}

CAbstractActor *CPlacedActors::EndScript()
{
	inherited::EndScript();

	heading = GetLastArcDirection();
	GetLastArcLocation(location);
	location[1] = ReadFixedVar(iHeight) + ReadFixedVar(iBaseHeight);

	return this;
}
