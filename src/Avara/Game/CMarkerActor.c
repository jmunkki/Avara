/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CMarkerActor.c
    Created: Tuesday, November 22, 1994, 2:17
    Modified: Thursday, November 23, 1995, 15:10
/*/

#include "CMarkerActor.h"
#include "CSmartPart.h"

void	CMarkerActor::IAbstractActor()
{
	inherited::IAbstractActor();
	isActive = kIsActive;
	maskBits = kTargetBit + kMarkerBit;
}
CAbstractActor *	CMarkerActor::EndScript()
{
	inherited::EndScript();

	partCount = 1;
	LoadPart(0, 230/*kMarkerBSP*/);
	TranslatePart(partList[0], location[0], location[1], location[2]);
	partList[0]->MoveDone();
//	LinkPartSpheres();
	
	return this;
}

void	CMarkerActor::FrameAction()
{
	inherited::FrameAction();
	
	heading -= FDegToOne(FIX(15));
//	UnlinkLocation();
	partList[0]->Reset();
	InitialRotatePartY(partList[0], heading);
	TranslatePart(partList[0], location[0], location[1], location[2]);
	partList[0]->MoveDone();
//	LinkPartSpheres();

}
