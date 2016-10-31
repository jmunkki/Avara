/*
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: CScaledBSP.c
    Created: Thursday, March 28, 1996, 18:19
    Modified: Thursday, March 28, 1996, 18:21
*/

#include "CScaledBSP.h"

void CScaledBSP::IScaledBSP(
	Fixed			scale,
	short			resId,
	CAbstractActor	*anActor,
	short			aPartCode)
{
	OSErr	iErr;
	Vector	*p;
	short	i;

	itsBSPResource = GetResource(BSPRESTYPE, resId);
	
	iErr = HandToHand(&itsBSPResource);
	if(iErr)
	{	itsBSPResource = NULL;
	}
	else
	{	Fixed		x,y,z;
		ColorRecord	*oldColors;

		header = *(BSPResourceHeader *)*itsBSPResource;

		p = (Vector *)(header.pointOffset + *itsBSPResource);
		for(i=0;i<header.pointCount;i++)
		{	(*p)[0] = FMul((*p)[0], scale);
			(*p)[1] = FMul((*p)[1], scale);
			(*p)[2] = FMul((*p)[2], scale);
			p++;
		}

		header.minBounds.x = FMul(header.minBounds.x, scale);
		header.minBounds.y = FMul(header.minBounds.y, scale);
		header.minBounds.z = FMul(header.minBounds.z, scale);

		header.maxBounds.x = FMul(header.maxBounds.x, scale);
		header.maxBounds.y = FMul(header.maxBounds.y, scale);
		header.maxBounds.z = FMul(header.maxBounds.z, scale);
		
		header.enclosureRadius = FMul(header.enclosureRadius, scale);
		header.enclosurePoint.w = FMul(header.enclosurePoint.w, scale);
		
		*(BSPResourceHeader *)*itsBSPResource = header;
	}
	
	Initialize();
	MoveDone();

	theOwner = anActor;
	partCode = aPartCode;
	
	rSquare[0] = 0;
	rSquare[1] = 0;
	FSquareAccumulate(header.enclosureRadius, rSquare);
}

void CScaledBSP::Dispose()
{
	Handle				handCopy;
	BSPResourceHeader	*bp;
	
	handCopy = itsBSPResource;
	bp = (BSPResourceHeader *)*handCopy;
	bp->refCount = 99;	//	Prevent ReleaseResource call!
	
	inherited::Dispose();
	
	DisposeHandle(handCopy);
}
