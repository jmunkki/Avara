/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: MatTrig68000.c
    Created: Saturday, July 30, 1994, 22:50
    Modified: Friday, February 2, 1996, 10:26
/*/

#include <FixMath.h>
#include "FastMat.h"

#define	ARCUSTABLEBITS	9
#define	ARCUSTABLESIZE	(1+(1<<ARCUSTABLEBITS))
#define	TRIGTABLESIZE	4096L	/*	Don't change this number!	*/

static	short			*trigTable = 0;
static	unsigned short	*arcTanTable = 0;
static	unsigned short	*arcTanOneTable = 0;

#define	WRITETABLESno

void	InitTrigTables()
{
	Handle	trigResTable;
	Handle	arcTanResTable;
	
	long	quarterPi = 0xC910L;
	long	i;
	
	trigResTable = GetResource('TRIG',128);
	arcTanResTable = GetResource('TRIG',129);
	
	if(trigResTable)
	{	HLock(trigResTable);
		trigTable = (short *)*trigResTable;
	}
	else
	{	trigTable = (short *)NewPtr(sizeof(short) * TRIGTABLESIZE);
		for(i = 0; i<TRIGTABLESIZE; i++)
		{	trigTable[i] = FracSin((quarterPi * i * 2)/TRIGTABLESIZE) >> 14;
		}

#ifdef WRITETABLES
		trigResTable = NewHandle(GetPtrSize((Ptr)trigTable));
		BlockMove(trigTable, *trigResTable, GetHandleSize(trigResTable));
		AddResource(trigResTable, 'TRIG', 128, "\PSine Cosine");
		SetResAttrs(trigResTable, GetResAttrs(trigResTable) | resPreload | resLocked);
		ChangedResource(trigResTable);
		WriteResource(trigResTable);
#endif
	}
	
	if(arcTanResTable)
	{	HLock(arcTanResTable);
		arcTanTable = (unsigned short *)*arcTanResTable;
		arcTanOneTable = arcTanTable + ARCUSTABLESIZE;
	}
	else
	{
		arcTanTable = (unsigned short *)NewPtr(sizeof(unsigned short) * ARCUSTABLESIZE * 2);
		arcTanOneTable = arcTanTable + ARCUSTABLESIZE;
	
		for(i = 0; i<ARCUSTABLESIZE; i++)
		{	arcTanTable[i] = FixATan2(ARCUSTABLESIZE-1, i);
			arcTanOneTable[i] = FRadToOne(arcTanTable[i]);
		}

#ifdef	WRITETABLES
		arcTanResTable = NewHandle(GetPtrSize((Ptr)arcTanTable));
		BlockMove(arcTanTable, *arcTanResTable, GetHandleSize(arcTanResTable));
		AddResource(arcTanResTable, 'TRIG', 129, "\PArcTan2");
		SetResAttrs(arcTanResTable, GetResAttrs(arcTanResTable) | resPreload | resLocked);
		ChangedResource(arcTanResTable);
		WriteResource(arcTanResTable);
#endif

	}
}

void	CloseTrigTables()
{
	if(trigTable)
	{	DisposePtr((Ptr)trigTable);
	}
}

asm
Fixed	FOneSin(
	Fixed	a)
{
		move.l	trigTable,A0
		move.l	4(a7),D1
		move.w	D1,D2
		asl.w	#2,D1
		bcc.s	@firstQuadr
		neg.w	D1
		beq.s	@resultOne
@firstQuadr:
		lsr.w	#4,D1
		add.w	D1,D1
		clr.l	D0
		move.w	0(A0,D1.w),D0
		bra.s	@notOne
@resultOne:
		moveq.l	#1,D0
		swap.w	D0
@notOne:
		add.w	D2,D2
		bcc.s	@firstHalf
		neg.l	D0
@firstHalf:

	rts
}

asm
Fixed	FOneCos(
	Fixed	a)
{
		move.l	trigTable,A0
		move.l	4(a7),D1
		add.w	#16384,D1
		move.w	D1,D2
		asl.w	#2,D1
		bcc.s	@firstQuadr
		neg.w	D1
		beq.s	@resultOne
@firstQuadr:
		lsr.w	#4,D1
		add.w	D1,D1
		clr.l	D0
		move.w	0(A0,D1.w),D0
		bra.s	@notOne
@resultOne:
		moveq.l	#1,D0
		swap.w	D0
@notOne:
		add.w	D2,D2
		bcc.s	@firstHalf
		neg.l	D0
@firstHalf:
	rts
}

Fixed	FDegSin(
	Fixed	a)
{
	return	FOneSin(FMul(a, 182));	//	a / 360);
}

Fixed	FDegCos(
	Fixed	a)
{
	return	FOneSin(a / 360+16384);
}

Fixed	FRadSin(
	Fixed	a)
{
	return	FOneSin(FDivNZ(a, FTWOPI));
}

Fixed	FRadCos(
	Fixed	a)
{
	return	FOneSin(FDivNZ(a, FTWOPI)+16384);
}

/*	Uses a Taylor series expansion:	Not accurate
**	Instead of FRadArcSin and FRadArcCos, use FRadArcTan2 when you can!
*/
Fixed	FRadArcSin(
	Fixed	n)
{
	return	n + FMul(n,FMul(n,n))/6;
}

//	Uses the same expansion, but adjusts the result, of course
Fixed	FRadArcCos(
	Fixed	n)
{
	return  FHALFPI - n - FMul(n,FMul(n,n))/6;
}

Fixed	FRadArcTan2(
	Fixed	x,
	Fixed	y)
{
	Fixed	absX,absY;
	Fixed	ratio;
	Fixed	result;

	if(x || y)
	{
		absX = x < 0 ? -x : x;
		absY = y < 0 ? -y : y;
		ratio = absX > absY ? FDivNZ(absY, absX) : FDivNZ(absX, absY);
		ratio += 1<<(15-ARCUSTABLEBITS);
		ratio >>= 16-ARCUSTABLEBITS;
		
		result = arcTanTable[ratio];
		
		if(absY > absX)	result = FHALFPI - result;
		if(x < 0)		result = FONEPI - result;
		if(y < 0)		result = - result;
	}
	else
	{	result = 0;
	}
	
	return result;
}

Fixed	FOneArcTan2(
	Fixed	x,
	Fixed	y)
{
	Fixed	absX,absY;
	Fixed	ratio;
	Fixed	result;
	
	if(x || y)
	{	absX = x < 0 ? -x : x;
		absY = y < 0 ? -y : y;
		ratio = absX > absY ? FDivNZ(absY, absX) : FDivNZ(absX, absY);
		ratio += 1<<(15-ARCUSTABLEBITS);
		ratio >>= 16-ARCUSTABLEBITS;
		
		result = arcTanOneTable[ratio];
		
		if(absY > absX)	result = 0x4000L - result;
		if(x < 0)		result = 0x8000L - result;
		if(y < 0)		result = - result;
	}
	else
		result = 0;
	
	return result;
}

void	FindSpaceAngle(
	Fixed	*delta,
	Fixed	*heading,
	Fixed	*pitch)
{
	Fixed	headingA;
	Fixed	sideLen;
	
	headingA = FOneArcTan2(delta[2], delta[0]);
	
	if((headingA-0x2000) & 0x4000)
		sideLen = FDivNZ(delta[2],FOneCos(headingA));
	else
		sideLen = FDivNZ(delta[0],FOneSin(headingA));

	*heading = headingA;
	*pitch = FOneArcTan2(sideLen, -delta[1]);
}

void	FindSpaceAngleAndNormalize(
	Fixed	*delta,
	Fixed	*heading,
	Fixed	*pitch)
{
	Fixed	headingA;
	Fixed	pitchA;
	Fixed	sideLen;
	Fixed	tempCos, tempSin;
	
	headingA = FOneArcTan2(delta[2], delta[0]);
	
	tempCos = FOneCos(headingA);
	tempSin = FOneSin(headingA);

	if((headingA-0x2000) & 0x4000)
		sideLen = FDivNZ(delta[2],tempCos);
	else
		sideLen = FDivNZ(delta[0],tempSin);


	pitchA = FOneArcTan2(sideLen, delta[1]);

	*heading = headingA;
	*pitch = -pitchA;

	sideLen = FOneCos(pitchA);
	delta[2] = FMul(sideLen, tempCos);
	delta[1] = FOneSin(pitchA);
	delta[0] = FMul(sideLen, tempSin);
}
