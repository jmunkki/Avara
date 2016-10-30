/*/
    Copyright ©1992-1995, Juri Munkki
    All rights reserved.

    File: PADrivers.c
    Created: Thursday, November 26, 1992, 13:27
    Modified: Friday, March 17, 1995, 12:47
/*/

#include "PAKit.h"
#pragma options(!profile)

#define SLOWBITMAPPIXELDRIVERx

static	long	bitmapMasks[64];
static	long	twoBitMasks[32];
static	long	fourBitMasks[16];

void	True16PixelDriver(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
asm	{
		movem.l	D3/D5-D6/A2-A4,-(sp)

		move.l	diffp,A0
		move.l	baseAddr,A2
		
		clr.l	D5
		move.w	rowBytes,D5
		move.w	rightEdge,D6
		move.w	rowCount,D0
		subq.w	#1,D0

		move.l	colorTable,A1
		move.l	(A1),A1

@mainLoop
		move.w	(A0)+,D1		;	This is x1
		cmp.w	D6,D1
		bge.s	@nextRow

		lea		(A2,D1.w*2),A3	;	This is the address of the pixels
@x1ok
		move.w	(A0)+,D2		;	This is the color index
		bmi.s	@mainLoop		;	Negative colors are not drawn
		move.l	(A1,D2.w*4),D2	;	Load color word
		
		btst.l	#0,D1
		beq.s	@evenAddr
		move.w	D2,(A3)+
		addq.w	#1,D1
@evenAddr
		move.w	(A0)+,D3		;	This is x2
		sub.w	D3,D1
		beq.s	@done
		neg.w	D1
		asr.w	#1,D1
		beq.s	@almostDone
		subq.l	#1,D1
@pixLoop
		move.l	D2,(A3)+
		dbra	D1,@pixLoop
@almostDone
		btst.l	#0,D3
		beq.s	@done
		move.w	D2,(A3)+
		
@done	move.w	D3,D1
		cmp.w	D3,D6
		bne		@x1ok

@nextRow
		add.l	D5,A2			;	Add rowbytes to base address
		dbra	D0,@mainLoop
@stop
		movem.l	(sp)+,D3/D5-D6/A2-A4
	}
}


void	True24PixelDriver(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
asm	{
		movem.l	D3-D7/A2-A4,-(sp)

		move.l	diffp,A0
		move.l	baseAddr,A2

		moveq.l	#-1,D0
		clr.l	D5
		move.w	rowBytes,D5
		move.w	rightEdge,D6
		move.w	rowCount,D7
		subq.w	#1,D7

		move.l	colorTable,A1
		move.l	(A1),A1

@mainLoop
		move.w	(A0)+,D1		;	This is x1
		cmp.w	D6,D1
		bge.s	@nextRow

		lea		(A2,D1.w*4),A3	;	This is the address of the pixels
@x1ok
		move.w	(A0)+,D2		;	This is the color index
		bmi.s	@mainLoop		;	Negative colors are not drawn
		move.l	(A1,D2.w*4),D2	;	Load color longword
		
		move.w	(A0)+,D3		;	This is x2
		sub.w	D3,D1
//		beq.s	@done			;	Not necessary since empty spans are not allowed
		eor.w	D0,D1

@pixLoop
		move.l	D2,(A3)+
		dbra	D1,@pixLoop
		
@done	move.w	D3,D1
		cmp.w	D3,D6
		bne		@x1ok

@nextRow
		add.l	D5,A2			;	Add rowbytes to base address
		dbra	D7,@mainLoop
@stop
		movem.l	(sp)+,D3-D7/A2-A4
	}
}

void	BytePixelDriver(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
asm	{
		movem.l	D3/D5-D7/A2-A4,-(sp)

		move.l	diffp,A0
		move.l	baseAddr,A2

		clr.l	D5
		move.w	rowBytes,D5
		move.w	rightEdge,D6
		move.w	rowCount,D7
		subq.w	#1,D7

		move.l	colorTable,A1
		move.l	(A1),A1
		lea		4(A1),A4

@mainLoop
		move.w	(A0)+,D1		;	This is x1
		cmp.w	D6,D1
		blt.s	@notDoneYet

		exg		A1,A4			;	Swap pattern rows
		add.l	D5,A2			;	Add rowBytes
		dbra	D7,@mainLoop
		bra		@stop

@notDoneYet
		lea		(A2,D1.w),A3	;	This is the address of the pixels
@x1ok
		move.w	(A0)+,D2		;	This is the color index
		bmi.s	@mainLoop		;	Negative colors are not drawn
		move.l	(A1,D2.w*8),D2	;	Load color longword

		moveq.l	#3,D0
		move.w	(A0)+,D3		;	This is x2		
		and.w	D1,D0
		sub.w	D3,D1

		jmp		@firstTable(PC,D0.w*8)
@writeThree
		move.b	D2,(A3)+
@writeWord
		move.w	D2,(A3)+
		addq.w	#1,D1
		beq.s	@done
@firstTable
		addq.w	#8,D1
		bmi.s	@moreThanN
		jmp		@skipper(PC,D1.w*8)
		
		addq.w	#2,D1
		bmi.s	@writeThree
		beq.s	@writeTwoByteDone
		bpl.s	@writeFirstDone

		addq.w	#1,D1
		bmi.s	@writeWord
		beq.s	@writeByteDone
		dc.w	0

		move.b	D2,(A3)+
		addq.w	#1,D1
		beq.s	@done

		addq.w	#8,D1
		bmi.s	@moreThanN
		jmp		@skipper(PC,D1.w*8)

@moreThanN
		move.l	D2,(A3)+
		move.l	D2,(A3)+
		addq.w	#8,D1
		bmi.s	@moreThanN
		jmp		@skipper(PC,D1.w*8)

@skipper
		move.l	D2,(A3)+		;	To copy Eight bytes
		move.w	D3,D1
		move.l	D2,(A3)+
		bra.s	@doneGood

		move.l	D2,(A3)+		;	To copy Seven bytes
		move.w	D2,(A3)+		
		bra.s	@writeByteDone
		dc.w	0

		move.l	D2,(A3)+		;	To copy six bytes
		move.w	D3,D1
		move.w	D2,(A3)+
		bra.s	@doneGood

		move.l	D2,(A3)+		;	To copy five bytes
		bra.s	@writeByteDone
		dc.l	0

		move.l	D2,(A3)+		;	To copy four bytes
		move.w	D3,D1
		bra.s	@doneGood
		dc.w	0

		move.w	D2,(A3)+		;	To copy three bytes
		bra.s	@writeByteDone
		dc.l	0

		move.w	D3,D1
		move.w	D2,(A3)+		;	To copy two bytes
		bra.s	@doneGood
@writeTwoByteDone
		move.b	D2,(A3)+

@writeByteDone
		lsr.l	#8,D2
@writeFirstDone
		move.b	D2,(A3)+		;	To copy one byte		
@done	move.w	D3,D1
@doneGood
		cmp.w	D3,D6
		bne		@x1ok

@nextRow
		exg		A1,A4
		add.l	D5,A2			;	Add rowbytes to base address

		dbra	D7,@mainLoop
@stop
		movem.l	(sp)+,D3/D5-D7/A2-A4
	}
}

#if 0

void	BytePixelDriverOld(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
asm	{
		movem.l	D3/D5-D7/A2-A4,-(sp)

		move.l	diffp,A0
		move.l	baseAddr,A2

		clr.l	D5
		move.w	rowBytes,D5
		move.w	rightEdge,D6
		move.w	rowCount,D7
		subq.w	#1,D7

		move.l	colorTable,A1
		move.l	(A1),A1
		lea		4(A1),A4

@mainLoop
		move.w	(A0)+,D1		;	This is x1
		cmp.w	D6,D1
		blt.s	@notDoneYet

		exg		A1,A4			;	Swap pattern rows
		add.l	D5,A2			;	Add rowBytes
		dbra	D7,@mainLoop
		bra		@stop

@notDoneYet
		lea		(A2,D1.w),A3	;	This is the address of the pixels
@x1ok
		move.w	(A0)+,D2		;	This is the color index
		bmi.s	@mainLoop		;	Negative colors are not drawn
		move.l	(A1,D2.w*8),D2	;	Load color longword

		moveq.l	#3,D0
		move.w	(A0)+,D3		;	This is x2		
		and.w	D1,D0
		sub.w	D3,D1

		jmp		@firstTable(PC,D0.w*8)
@writeThree
		move.b	D2,(A3)+
@writeWord
		move.w	D2,(A3)+
		addq.w	#1,D1
		beq.s	@done
@firstTable
		addq.w	#8,D1
		bmi.s	@moreThanN
		jmp		@skipper(PC,D1.w*8)
		
		addq.w	#2,D1
		bmi.s	@writeThree
		beq.s	@writeTwoByteDone
		bpl.s	@writeFirstDone

		addq.w	#1,D1
		bmi.s	@writeWord
		beq.s	@writeByteDone
		dc.w	0

		move.b	D2,(A3)+
		addq.w	#1,D1
		beq.s	@done

		addq.w	#8,D1
		bmi.s	@moreThanN
		jmp		@skipper(PC,D1.w*8)

@moreThanN
		move.l	D2,(A3)+
		move.l	D2,(A3)+
		addq.w	#8,D1
		bmi.s	@moreThanN
		jmp		@skipper(PC,D1.w*8)

@skipper
		move.l	D2,(A3)+		;	To copy Eight bytes
		move.w	D3,D1
		move.l	D2,(A3)+
		bra.s	@doneGood

		move.l	D2,(A3)+		;	To copy Seven bytes
		move.w	D2,(A3)+		
		bra.s	@writeByteDone
		dc.w	0

		move.l	D2,(A3)+		;	To copy six bytes
		move.w	D3,D1
		move.w	D2,(A3)+
		bra.s	@doneGood

		move.l	D2,(A3)+		;	To copy five bytes
		bra.s	@writeByteDone
		dc.l	0

		move.l	D2,(A3)+		;	To copy four bytes
		move.w	D3,D1
		bra.s	@doneGood
		dc.w	0

		move.w	D2,(A3)+		;	To copy three bytes
		bra.s	@writeByteDone
		dc.l	0

		move.w	D3,D1
		move.w	D2,(A3)+		;	To copy two bytes
		bra.s	@doneGood
@writeTwoByteDone
		move.b	D2,(A3)+

@writeByteDone
		ror.l	#8,D2
@writeFirstDone
		move.b	D2,(A3)+		;	To copy one byte		
@done	move.w	D3,D1
@doneGood
		cmp.w	D3,D6
		bne		@x1ok

@nextRow
		exg		A1,A4
		add.l	D5,A2			;	Add rowbytes to base address

		dbra	D7,@mainLoop
@stop
		movem.l	(sp)+,D3/D5-D7/A2-A4
	}
}

static	long	minNewTime = 0x7fffFFFF;
static	long	maxNewTime;
static	long	minOldTime = 0x7fffFFFF;
static	long	maxOldTime;
static	long	oldTimer;
static	long	newTimer;
static	long	callTimes;

#define	THEDELAY	-60000000L

void	BytePixelDriver(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
	TMTask	miniTask;
	short	i;
	long	deltaTime;
	

	miniTask.tmAddr = NULL;
	miniTask.tmWakeUp = 0;
	miniTask.tmReserved = 0;
	InsXTime((QElemPtr)&miniTask);
	FlushDataCache();
	FlushInstructionCache();
	PrimeTime((QElemPtr)&miniTask, THEDELAY);
	BytePixelDriverOld(diffp, colorTable, rowBytes, baseAddr, rowCount, rightEdge);
	RmvTime((QElemPtr)&miniTask);


	if(rowCount > 100)
	{
		deltaTime = miniTask.tmCount - THEDELAY;

		oldTimer += deltaTime;

		if(deltaTime < minOldTime)
		{	minOldTime = deltaTime;
		}

		if(deltaTime > maxOldTime)
		{	maxOldTime = deltaTime;
		}
	}
	
	miniTask.tmAddr = NULL;
	miniTask.tmWakeUp = 0;
	miniTask.tmReserved = 0;
	InsXTime((QElemPtr)&miniTask);
	FlushDataCache();
	FlushInstructionCache();
	PrimeTime((QElemPtr)&miniTask, THEDELAY);
	BytePixelDriverNew(diffp, colorTable, rowBytes, baseAddr, rowCount, rightEdge);
	RmvTime((QElemPtr)&miniTask);

	if(rowCount > 100)
	{
		deltaTime = miniTask.tmCount - THEDELAY;

		callTimes++;
		newTimer += deltaTime;

		if(deltaTime < minNewTime)
		{	minNewTime = deltaTime;
		}

		if(deltaTime > maxNewTime)
		{	maxNewTime = deltaTime;
		}
	}

}
#endif

#define	MASKTABLE		bitmapMasks
#define	MASKOFFSET		64
#define	BITSINLONG		32
#define	RIGHTSHIFT		3

void	BitmapPixelDriver68K(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
#include "PABitmapDriver.body"

#define mot68020

#ifdef SLOWBITMAPPIXELDRIVER

#define	MASKTABLE		bitmapMasks
#define	MASKOFFSET		64
#define	BITSINLONG		32
#define	RIGHTSHIFT		3

void	BitmapPixelDriver020(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
#include "PABitmapDriver.body"
#endif

#define	MASKTABLE		twoBitMasks
#define	MASKOFFSET		32
#define	BITSINLONG		16
#define	RIGHTSHIFT		2

void	TwoBitPixelDriver(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
#include "PABitmapDriver.body"

#define	MASKTABLE		fourBitMasks
#define	MASKOFFSET		16
#define	BITSINLONG		8
#define	RIGHTSHIFT		1

void	FourBitPixelDriver(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
#include "PABitmapDriver.body"

#undef mot68020

#ifndef SLOWBITMAPPIXELDRIVER
/*	Register usage:
**				D0		right edge
**				D1		x1 (most of the time)
**				D2		color index or pattern.
**				D3		x2
**				D4		temp
**				D5		temp
**				D6		32
**				D7		Pattern offset
**				A0		Source data
**				A1		Bitmap table
**				A2		Destination bitmap
**				A3		Destination bits (more specific than A2)
**				A4		Unused
*/
void	BitmapPixelDriver020(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
asm	{
		movem.l	D3-D7/A2-A4,-(sp)

		move.l	diffp,A0
		move.l	colorTable,A1
		move.l	(A1),A1
		move.l	baseAddr,A2
		move.w	rightEdge,D0

		moveq.l	#32,D6

		subq.w	#1,rowCount
		bmi		@stop
		moveq.l	#0,D7			;	This is the pattern offset
@mainLoop
		clr.l	D1
		move.w	(A0)+,D1		;	This is x1
		cmp.w	D0,D1
		bge		@nextRow

@forEachStretch
		move.w	(A0)+,D2		;	This is the color index
		bmi.s	@skipThisStretch

		move.w	(A0),D3			;	This is x2
		lsl.w	#5,D2
		add.w	D7,D2
		move.l	(A1,D2.w),D2	;	This is now the current pattern
		
		move.w	D3,D4
		sub.w	D1,D4			;	Delta x
		cmp.w	D6,D4			;	Is it a small stretch?
		bcc.s	@longStretch	;	Jump if stretch is longer than 32 pixels
		
		rol.l	D3,D2
		bfins	D2,(A2){D1:D4}	;	Copy the bits.
		bra.s	@endStretching

@longStretch					;	Have to write into at least two longwords.
		move.l	D1,D5
		and.w	#~0x1F,D1
		sub.l	D1,D5

		lsr.w	#3,D1			;	Divide by 8
		lea		(A2,D1.w),A3
		
		moveq.l	#32,D1
		sub.l	D5,D1

		bfins	D2,(A3){D5:D1}
		addq.l	#4,A3

		sub.w	D1,D4
		moveq.l	#32,D1
		sub.w	D1,D4
		bmi.s	@postLongStretch

@stretchLoop
		move.l	D2,(A3)+
		sub.w	D1,D4
		bpl.s	@stretchLoop

@postLongStretch
		add.w	D1,D4
		beq.s	@endStretching
		rol.l	D4,D2
		bfins	D2,(A3){0:D4}
		
@endStretching
@skipThisStretch
		move.w	(A0)+,D1
		cmp.w	D0,D1
		bne		@forEachStretch

@nextRow
		addq.l	#4,D7			;	Add to pattern offset
		and.b	#0x1F,D7		;	Keep offset within 32 bytes
		add.w	rowBytes,A2

		subq.w	#1,rowCount
		bpl		@mainLoop
@stop
		movem.l	(sp)+,D3-D7/A2-A4
	}
}
#endif
void	SetMasks()
{
	int		i;
	long	m = 1;
	
	for(i=31;i>=0;i--)
	{	bitmapMasks[i] = ~m;
		bitmapMasks[i+32] = m;
		m += m+1;
	}
	
	m = 3;
	for(i=15;i>=0;i--)
	{	twoBitMasks[i] = ~m;
		twoBitMasks[i+16] = m;
		
		m = (m << 2) + 3;
	}

	m = 15;
	for(i=7;i>=0;i--)
	{	fourBitMasks[i] = ~m;
		fourBitMasks[i+8] = m;
		
		m = (m << 4) + 15;
	}
}
