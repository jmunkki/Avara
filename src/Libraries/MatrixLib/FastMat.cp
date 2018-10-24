/*/
    Copyright ©1992-1995, Juri Munkki
    All rights reserved.

    File: FastMat.c
    Created: Saturday, October 17, 1992, 1:45
    Modified: Sunday, November 19, 1995, 8:25
/*/

#define FASTMAT
#include "FastMat.h"
#include <Gestalt.h>
#include "PowerPlugs.h"

asm	void OneMatrix(Matrix	*theMatrix)
{
    MOVE.L   4(A7),A0
    MOVEM.L	 D3-D4,-(A7)
    LEA      64(A0),A0
    MOVEQ.L  #1,D0
    SWAP.W   D0
    MOVEQ.L  #0,D1
    MOVEQ.L  #0,D2
    MOVEQ.L  #0,D3
    MOVEQ.L  #0,D4
    MOVE.L   D0,-(A0)
    MOVEM.L  D0-D4,-(A0)
    MOVEM.L  D0-D4,-(A0)
    MOVEM.L  D0-D4,-(A0)
    MOVEM.L	 (A7)+,D3-D4
	rts
}

Fixed	VectorLength(
long	n,
Fixed	*v)
{
	long	acc[2] = {0,0};
	
	while(n--)
	{	FSquareAccumulate(*v++,acc);
	}
	
	return FSqroot(acc);
}

Fixed	NormalizeVector(
long	n,
Fixed	*v)
{
	Fixed	l;
	
	l = VectorLength(n, v);
	if(l)
	while(n--)
	{	*v = FDivNZ(*v, l);
		v++;
	}
	
	return l;
}

void	InitMatrix()
{
	long	theProcessor;

#ifndef FASTMAT020
	Gestalt(gestaltProcessorType,&theProcessor);
	if(		theProcessor == gestalt68000
		||	theProcessor == gestalt68010)
	{	
		VectorMatrixProduct = VectorMatrixProduct68000;
		VectorMatrix33Product = VectorMatrixProduct68000;
		VectorMatrix34Product = VectorMatrixProduct68000;
		FlaggedVectorMatrix34Product = VectorMatrixProduct68000;

		FMul = FMul68000;
		FDiv = FDiv68000;
		FMulDiv = FMulDiv68000;
		FMulDivV = FMulDiv68000;
		FSquareAccumulate = FSquareAccumulate68000;
		FSquareSubtract = FSquareSubtract68000;
		FRandom = FRandom68000;
	}
	else
	if(		theProcessor == gestalt68020
		||	theProcessor == gestalt68030
		||	theProcessor == gestalt68040)
	{	
		VectorMatrixProduct = VectorMatrixProduct68020;
		VectorMatrix33Product = VectorMatrix33Product020;
		VectorMatrix34Product = VectorMatrix34Product020;
		FlaggedVectorMatrix34Product = FlaggedVectorMatrix34Product020;

		FMul = FMul68020;
		FDiv = FDiv68020;
		FMulDiv = FMulDiv68020;
		FMulDivV = FMulDiv68020V;
		FSquareAccumulate = FSquareAccumulate68020;
		FSquareSubtract = FSquareSubtract68020;
		FRandom = FRandom68000;
	}
	else
	{
		VectorMatrixProduct = VectorMatrixProductC;
		VectorMatrix33Product = VectorMatrixProductC;
		VectorMatrix34Product = VectorMatrixProductC;
		FlaggedVectorMatrix34Product = VectorMatrixProductC;

		FMul = FMulC;
		FDiv = FDivC;
		FMulDiv = FMulDivC;
		FMulDivV = FMulDivC;
		FRandom = FRandomC;
	}
#endif
	
	FlaggedVectorMatrix34Product = (VMProduct *)GetPowerRoutine(
										FlaggedVectorMatrix34Product020,
										FASTMATPOWERPLUGID, kPowerFlaggedVMProduct);

	InitTrigTables();
	FRandSeed = TickCount();
}

void	CloseMatrix()
{
	CloseTrigTables();
}