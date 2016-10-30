/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CDirectObject.c
    Created: Friday, March 11, 1994, 17:15
    Modified: Thursday, April 11, 1996, 22:30
/*/

#include "CDirectObject.h"

void	CDirectObject::Dispose()
{
	delete this;
}

#ifdef THINK_C

#define	MEMORY_OFFSET		2
#define	CAREFUL_MODE		1
#define	NASTY_DEBUG_INIT	0
#define	NASTY_VALUE			Random()

#if	CAREFUL_MODE
	#define	DirectNewPtr	NewPtrClear
#else
	#define	DirectNewPtr	NewPtr
#endif

#define	BASIC_BLOCK_BITS	6
#define	BASIC_BLOCK_SIZE	(1L<<BASIC_BLOCK_BITS)
#define	NUM_SIZES			16
#define MAX_BLOCK_SIZE		(BASIC_BLOCK_SIZE * NUM_SIZES)

typedef struct QuickPointerBlock
{
	struct	QuickPointerBlock	*next;
	long						blockLen;
} QuickPointerBlock;

static	Ptr					quickStorageBlock = NULL;
static	long				quickStorageLen = 0;
static	QuickPointerBlock *	firstFreeBlock[NUM_SIZES];

void	AllocateQuickDirectStorage(
	long		numBytes)
{
	short				i;
	
	for(i=0;i<NUM_SIZES;i++)
	{	firstFreeBlock[i] = NULL;
	}
	
	quickStorageBlock = DirectNewPtr(numBytes);
	if(quickStorageBlock)
	{	quickStorageLen = numBytes;
		RestoreQuickBlock(numBytes, quickStorageBlock);
	}
}

void	DeallocateQuickDirectStorage()
{
	if(quickStorageBlock)
	{	DisposePtr(quickStorageBlock);
		quickStorageBlock = NULL;
		quickStorageLen = 0;
	}
}

void	RestoreQuickBlock(
	long	len,
	Ptr		mem)
{
	QuickPointerBlock	*header;
	short				i;
	long				sizeGroup;
	
	header = (QuickPointerBlock *) mem;
	
	sizeGroup = (len >> BASIC_BLOCK_BITS)-1;
	if(sizeGroup >= NUM_SIZES)
	{	sizeGroup = NUM_SIZES-1;
	}

	header->blockLen = len;
	header->next = firstFreeBlock[sizeGroup];
	firstFreeBlock[sizeGroup] = header;
}

Ptr		AquireQuickBlock(
	long	len)
{
	long				sizeGroup;

	if(len > MAX_BLOCK_SIZE-sizeof(long))
	{	return DirectNewPtr(len);
	}
	else
	{	long	roundLen;
		short	i;
	
		QuickPointerBlock	*freeBlock;
		roundLen = (len + BASIC_BLOCK_SIZE + sizeof(long) - 1) & ~(BASIC_BLOCK_SIZE-1);

		for(i=roundLen >> NUM_SIZES-1;i<NUM_SIZES;i++)
		{	freeBlock = firstFreeBlock[i];
			if(	freeBlock &&
				freeBlock->blockLen >= roundLen)
			{	firstFreeBlock[i] = freeBlock->next;
				if(roundLen < freeBlock->blockLen)
				{	RestoreQuickBlock(freeBlock->blockLen - roundLen, roundLen + (Ptr)freeBlock);
				}
				
				*((long *)freeBlock) = roundLen;

#if	NASTY_DEBUG_INIT || CAREFUL_MODE
				{	Ptr	foobie;
				
					foobie = (Ptr)(1+(long *)freeBlock);
					while(len--)
					{
#if CAREFUL_MODE
						*foobie++ = 0x00;
#else
						*foobie++ = NASTY_VALUE;
#endif
					}
				}
#endif
				return (Ptr)(1+(long *)freeBlock);
			}
		}
		
		return DirectNewPtr(len);
	}
}

void	CDirectObject::operator delete(
	void *p)
{
	p = (void *)((char *)p - MEMORY_OFFSET);

	if(!quickStorageBlock ||
		(Ptr)p < quickStorageBlock ||
		(Ptr)p > quickStorageBlock + quickStorageLen)
	{	DisposePtr((Ptr)p);
	}
	else
	{	RestoreQuickBlock( ((long *)p)[-1], ((Ptr)p) - sizeof(long));
	}
}

void *	CDirectObject::operator new(
	long	numBytes)
{
	numBytes += MEMORY_OFFSET;

	if(numBytes > (BASIC_BLOCK_SIZE<<NUM_SIZES) || !quickStorageBlock)
	{	return (void *)(MEMORY_OFFSET + DirectNewPtr(numBytes));
	}
	else
	{	return (void *)(MEMORY_OFFSET + AquireQuickBlock(numBytes));
	}
}

#endif

long	MemoryStatistics()
{
	short				i;
	QuickPointerBlock	*fb;
	long				totFree;
	long				clumpFree[NUM_SIZES];
	long				numClumps[NUM_SIZES];
	
	totFree = 0;
	for(i=0;i<NUM_SIZES;i++)
	{	fb = firstFreeBlock[i];
		clumpFree[i] = 0;
		numClumps[i] = 0;
		while(fb)
		{	clumpFree[i] += fb->blockLen;
			numClumps[i]++;
			fb = fb->next;
		}
		totFree += clumpFree[i];
	}
	
	return totFree;
}

void	__noObject()
{
	Debugger();
}

void	__noMethod()
{
	Debugger();
}