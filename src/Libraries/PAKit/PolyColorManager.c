/*
    Copyright ©1992-1995, Juri Munkki
    All rights reserved.

    File: PolyColorManager.c
    Created: Sunday, December 6, 1992, 6:44
    Modified: Tuesday, May 23, 1995, 18:03
*/

#include "PolyColor.h"
#include "RamFiles.h"

PolyColorTable	*thePolyColorTable;

void	InitPolyColorTable(
	PolyColorTable	*polyc)
{
	int		i;

	for(i=0;i<64;i++)
	{	polyc->hashTable[i] = 65535;
	}
	
	polyc->seed = 0;
	polyc->polyColorCount = 0;
	polyc->logicalColorEntryHandleSize = 0;
	polyc->realColorEntryHandleSize = 0;
	polyc->colorEntryHandle = (PolyColorEntry **)NewHandle(0);
	
	SetPolyColorTable(polyc);
	FindPolyColor(0x000000);
	FindPolyColor(0xFFFFFF);
}

void	ClosePolyColorTable(
	PolyColorTable *polyc)
{
	DisposHandle((Handle)polyc->colorEntryHandle);
}

void	SetPolyColorTable(
	PolyColorTable	*polyc)
{
	thePolyColorTable = polyc;
}

PolyColorTable	*GetPolyColorTable()
{
	return	thePolyColorTable;
}

unsigned short	FindPolyColor(
	long	theColor)
{
	PolyColorTable		*polyc = thePolyColorTable;
	unsigned short		hashValue,hashIndex;
	PolyColorEntry		*tablePtr,*entryPtr;
	
	hashValue =	3 & (theColor >> 6);
	hashValue = (hashValue << 2) + (3 & (theColor >> 14));
	hashValue = (hashValue << 2) + (3 & (theColor >> 22));
	
	hashIndex = polyc->hashTable[hashValue];

	tablePtr = *polyc->colorEntryHandle;
	while(hashIndex != 65535)
	{	entryPtr = tablePtr+hashIndex;
		if(entryPtr->color == theColor)
		{	return hashIndex;
		}
		else
		{	hashIndex = entryPtr->nextColor;
		}
	}
	
	if(polyc->polyColorCount > 32768)
		return 0;

	if(IncreaseByClump(	(Handle)polyc->colorEntryHandle,
						&polyc->realColorEntryHandleSize,
						&polyc->logicalColorEntryHandleSize,
						sizeof(PolyColorEntry),
						sizeof(PolyColorEntry) * POLYCOLORCLUMPSIZE) == noErr)
	{
		entryPtr = polyc->polyColorCount + *polyc->colorEntryHandle;
		entryPtr->color = theColor;
		entryPtr->priority = 0;
		entryPtr->nextColor = polyc->hashTable[hashValue];
		polyc->hashTable[hashValue] = polyc->polyColorCount;
		
		polyc->seed++;

		return polyc->polyColorCount++;
	}
	else
	{	return 0;
	}
}

void	IncreasePolyColorPriority(
	unsigned short	index,
	short			increase)
{
	PolyColorTable	*polyc = thePolyColorTable;
	
	if(index < polyc->polyColorCount)
	{	(*polyc->colorEntryHandle)[index].priority += increase;
	}
}

Handle	ConvertToColorTable()
{
	PolyColorTable	*polyc = thePolyColorTable;
	Handle	theClut;
	char	emptryString[] = {0};
	
	theClut = NewHandle(polyc->polyColorCount*sizeof(ColorSpec)+sizeof(long)+sizeof(short)*2);
	if(theClut)
	{	ColorTable		*colorTab;
		ColorSpec		*theSpec;
		PolyColorEntry	*theEntry;
		short			i;
	
		HLock(theClut);
		colorTab = (ColorTable *)*theClut;
		
		colorTab->ctSeed = 0;
		colorTab->ctFlags = 0;
		colorTab->ctSize = polyc->polyColorCount-1;
		
		theSpec = colorTab->ctTable;
		theSpec->value = 0;
		theSpec->rgb.red = 0xFFFF;
		theSpec->rgb.green = 0xFFFF;
		theSpec->rgb.blue = 0xFFFF;
		theSpec++;
		
		theEntry = *polyc->colorEntryHandle;
		
		for(i=1;i<polyc->polyColorCount-1;i++)
		{	while(theEntry->color == 0xFFFFFF || theEntry->color == 0)
				theEntry++;
			
			theSpec->value = i;
			theSpec->rgb.red = theEntry->color >> 16;
			theSpec->rgb.red += theSpec->rgb.red << 8;

			theSpec->rgb.green = (theEntry->color >> 8) & 0xFF;
			theSpec->rgb.green += theSpec->rgb.green << 8;

			theSpec->rgb.blue = theEntry->color & 0xFF;
			theSpec->rgb.blue += theSpec->rgb.blue << 8;
			theSpec++;
			theEntry++;
		}

		theSpec->value = polyc->polyColorCount-1;
		theSpec->rgb.red = 0x0000;
		theSpec->rgb.green = 0x0000;
		theSpec->rgb.blue = 0x0000;
		HUnlock(theClut);
	}
	
	return theClut;
}

long	RGBToLongColor(
	RGBColor	*theColor)
{	
	return ((theColor->red & 0xFF00L) << 8)
			+ (theColor->green & 0xFF00L)
			+ ((theColor->blue >> 8) & 0xFF);
}