/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CMissile.c
    Created: Sunday, December 18, 1994, 3:29
    Modified: Wednesday, March 8, 1995, 6:46
/*/

#include "CMissile.h"

void	CMissile::IAbstractMissile(CDepot *theDepot)
{
	inherited::IAbstractMissile(theDepot);

	partCount = 1;
	LoadPart(0, kMissileBSP);
}
