/*
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CGlowActors.c
    Created: Wednesday, March 15, 1995, 7:06
    Modified: Tuesday, July 30, 1996, 8:40
*/

#include "CGlowActors.h"
#include "CSmartPart.h"

void	CGlowActors::IAbstractActor()
{
	inherited::IAbstractActor();
	canGlow = true;
	glow = 0;
}
void	CGlowActors::BeginScript()
{
	inherited::BeginScript();

	if(shields >= 0)
	{	ProgramLongVar(iHitSound, ReadLongVar(iShieldHitSoundDefault));
	}

	ProgramLongVar(iCanGlow, shields >= 0);
}

CAbstractActor *CGlowActors::EndScript()
{
	glow = 0;
	canGlow = ReadLongVar(iCanGlow);

	return inherited::EndScript();
}

void	CGlowActors::WasHit(
	RayHitRecord	*theHit,
	Fixed 			hitEnergy)
{
	inherited::WasHit(theHit, hitEnergy);

	if(canGlow)
	{	glow += hitEnergy+hitEnergy;
		if(glow > FIX3(1800)) glow = FIX3(1800);
		isActive |= kIsGlowing;
	}
}

void	CGlowActors::FrameAction()
{	
	CSmartPart	**thePart;

	inherited::FrameAction();

	if(glow)
	{	Fixed	doubleGlow;
	
		glow -= (glow>>1)+1;
		if(glow <= 0)
		{	glow = 0;
			isActive &= ~kIsGlowing;
		}
	
		doubleGlow = glow+glow;
		for(thePart = partList; *thePart; thePart++)
		{	(*thePart)->extraAmbient = doubleGlow;
		}
	}
	
	FRandSeed += location[0] + location[1] + location[2];	
}