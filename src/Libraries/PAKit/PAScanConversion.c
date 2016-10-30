/*
    Copyright ©1992-1996, Juri Munkki
    All rights reserved.

    File: PAScanConversion.c
    Created: Monday, November 23, 1992, 16:43
    Modified: Thursday, February 22, 1996, 5:35
*/

#include "PAKit.h"

#define	DoProfile		0

extern	PolyWorld		*thePolyWorld;

#if DoProfile
void	ScanConvertPolys68KNew(
#else
void	ScanConvertPolys68K(
#endif
	PolyWorld		*polys,
	short			*destBuffer,
	PolyEdgePtr		*thisOn,
	PolyEdgePtr		*thisOff,
	Rect			*theBounds,
	short			bgColor)
{
	Ptr				stackSave;
	Ptr				stackPoint;
	short			i;
	short			*emergencyStop;
	PolyEdgePtr		activeList[ACTIVELISTSIZE];
	PolyEdge		backgroundColor,sentinel, sentinel2;
	Rect			bounds;

	emergencyStop = destBuffer+polys->displayRegionSize-2*ACTIVELISTSIZE;

	bounds = *theBounds;

	i = bounds.bottom - bounds.top;

asm		//	Loop preliminaries
{
		movem.l	D3-D7/A2-A5,-(sp)
		
		move.l	sp, stackSave
		moveq.l	#2,D0
		move.l	sp, D1
		and.l	D1, D0
		sub.l	D0, sp
		move.l	sp, stackPoint

		lea		backgroundColor,A0
		move.w	bgColor,D1
		move.w	D1,PolyEdge.color(A0)
		swap	D1				;	move bgColor to high word.
		clr.w	D1				;	polyId = 0
		clr.w	PolyEdge.polyID(A0)
		move.l	bounds.left,D0
		clr.w	D0
		move.l	D0,PolyEdge.x(A0)
		
		move.w	bounds.right,D5

		lea		activeList,A0	;	Initialize active list.
		lea		sentinel,A1
		move.w	#0x8000,PolyEdge.x(A1)
		move.l	A1,(A0)+		;	Adjust for sentinel item.
		move.l	A0,D6			;	active list table
		move.l	A0,D7			;	Last active edge
		
		lea		sentinel2,A1
		move.w	D5,PolyEdge.x(A1)

		move.l	destBuffer,A5	;	We don't need A5 globals, so let's use A5 as output
@screenLoop
}
asm		//	Add new edges to the active edge list.
{
		addq.l	#4,thisOn
		move.l	thisOn,A2		; Add new entries to active list
		move.l	-(A2),D0		; Read entry into D0
		beq.s	@noActives

@activeInsert
		move.l	D7,A0

@addActive
		move.l	D0,A1
		move.l	D0,(A0)+		; Place pointer in active list
		move.l	PolyEdge.nextOn(A1),D0	; Move to next
		bne.s	@addActive

		move.l	A0,D7
		clr.l	(A2)			; Clear "on" list so that it can be re-used.
@noActives
}

asm		//	Sort the edges according to x with an insertion sort
{
		move.l	D6,A1				;	This is the first item
		cmp.l	D7,A1
		beq.s	@zeroItems

@insertionSort
		move.l	A1,A3				;	Previous position in active list
		move.l	(A1)+,A2			;	Pointer to the item to insert

		move.l	PolyEdge.dx(A2),D0
		add.l	D0,PolyEdge.x(A2)
		move.w	PolyEdge.x(A2),D0
@doInsert
		move.l	-(A3),A4
		cmp.w	PolyEdge.x(A4),D0	;	Compare with key
		bge		@doneInsert

		move.l	A4,4(A3)			;	Swap pointers to items
		move.l	A2,(A3)
		bra.s	@doInsert
		
@doneInsert
		cmp.l	D7,A1
		bcs.s	@insertionSort
		
@zeroItems	
}

asm		//	Now find out what is visible by using a depth stack (actually a linked list)
{
		move.l	stackPoint, sp		//	reset stack. (Used for line coherence list)

		move.l	D1,D0				;	Top color is bgColor, polyId is 0
		cmp.l	D7,D6				;	Compare lastActive with activeList
		beq		@doneLine			;	Are there items to loop through?

		lea		backgroundColor,A1	;	A1 is the top of depth stack list.
		move.w	bounds.left,D2		;	Set current x to left clipping bound

		move.l	D6,A0				;	Start from beginning of active list

@outputLoop
		move.l	(A0)+,A2			;	Read item & move to next one
		move.w	PolyEdge.polyID(A2),D3	;	Read polyID

		move.l	A1,A3				;	Copy stack link pointer	-- Only needed for @findWhere

		cmp.w	D0,D3				;	Check with top of stack polyID
		bcc.s	@pushAtTop

@findWhere
		move.l	PolyEdge.nextOn(A3),A4	;	Move to next item.
		cmp.w	PolyEdge.polyID(A4),D3	;	Compare polyIDs.
		bcc.s	@foundWhere2

		move.l	PolyEdge.nextOn(A4),A3	;	Move to next item.
		cmp.w	PolyEdge.polyID(A3),D3	;	Compare polyIDs.
		blt.s	@findWhere
		bhi.s	@foundWhere

@alreadyThere						;	Item is there, so now remove it
		move.l	PolyEdge.nextOn(A3),D3
		move.l	D3,PolyEdge.nextOn(A4)
		bra.s	@noStackTopChange

@alreadyThere2						;	Item is there, so now remove it
		move.l	PolyEdge.nextOn(A4),D3
		move.l	D3,PolyEdge.nextOn(A3)
		bra.s	@noStackTopChange

@foundWhere2						;	Item isn't on stack. Add it there.
		beq.s	@alreadyThere2
		move.l	A2,PolyEdge.nextOn(A3)
		move.l	A4,PolyEdge.nextOn(A2)
		bra.s	@noStackTopChange

@foundWhere							;	Item isn't on stack. Add it there.
		move.l	A2,PolyEdge.nextOn(A4)
		move.l	A3,PolyEdge.nextOn(A2)
		bra.s	@noStackTopChange

@popFromTop
		move.l	PolyEdge.nextOn(A1),A1
		move.l	A2,-(sp)
		bra.s	@stackTopChange
@rightClip
		move.w	PolyEdge.polyID(A1),D0
		cmp.l	D7,A0
		bne.s	@outputLoop
		bra.s	@doneLine
@pushAtTop
		beq.s	@popFromTop
		move.l	A1,PolyEdge.nextOn(A2)
		move.l	A2,A1

@stackTopChange
		move.l	A1,-(sp)			//	Push entry into coherence list.

		move.w	PolyEdge.x(A2),D0	;	New x is here

		cmp.w	D0,D2				;	Compare new and old x
		bge.s	@emptyRange			;	Old x >= new x

		cmp.w	D5,D0				;	Clip with right clipping bound
		bge.s	@rightClip
//	Output stage
		move.w	D0,D2				;	New x -> current x
		move.l	D0,(A5)+			;	Output color and x

@emptyRange
		move.l	PolyEdge.color(A1),D0	;	Read new color and top polyId

@noStackTopChange
		cmp.l	D7,A0				;	Output loop end
		bne.s	@outputLoop

@doneLine
		move.w	D5,D0
		move.l	D0,(A5)+			;	Output color and x
		
		pea		sentinel2
@outputDone
}
asm		//	Deactivate and remove edges from active list according to "offLists".
{
		move.l	thisOff,A2
		addq.l	#4,thisOff
		move.l	(A2),D0
		beq.s	@noDeactives

@clearActive								;	Zero polyID fields of those
		move.l	D0,A1
		clr.w	PolyEdge.polyID(A1)		;	edges that are no longer
		move.l	PolyEdge.nextOff(A1),D0	;	to be in the active list.
		bne.s	@clearActive
											;	Remove edges with zero polyID
		move.l	D6,A1						;	by shifting pointers.
		move.l	A1,A3						;	A1 is source, A3 is destination of copy.

@removeActive
		move.l	(A1)+,A4
		tst.w	PolyEdge.polyID(A4)
		beq.s	@inactive
		move.l	A4,(A3)+				;	Item is active, copy it.
@inactive
		cmp.l	A1,D7
		bne.s	@removeActive

		move.l	A3,D7
		clr.l	(A2)			; Clear "off" list so that it can be re-used.
}

asm		//	End of main loop
{
		cmp.l	emergencyStop, A5
		bcc		@outOfMem
		subq.w	#1,i
		bne		@screenLoop
		bra		@exitFunction
}
asm
{	//	Coherent case:
@noDeactives
		cmp.l	emergencyStop, A5
		bcc.s	@outOfMem
		subq.w	#1,i
		beq		@exitFunction

		addq.l	#4,thisOn
		move.l	thisOn,A2		; Add new entries to active list
		move.l	-(A2),D0		; Read entry into D0
		bne		@activeInsert
		
		move.l	D6,A1				;	This is the first item
		cmp.l	D7,A1
		beq.s	@coherentZeroItems

@coherentInsertionSort
		move.l	A1,A3				;	Previous position in active list
		move.l	(A1)+,A2			;	Pointer to the item to insert

		move.l	PolyEdge.dx(A2),D0
		add.l	D0,PolyEdge.x(A2)
		move.w	PolyEdge.x(A2),D0
@coherentDoInsert
		move.l	-(A3),A4
		cmp.w	PolyEdge.x(A4),D0	;	Compare with key
		bge		@coherentDoneInsert

		move.l	A4,4(A3)			;	Swap pointers to items
		move.l	A2,(A3)
		bra		@doInsert			//	Coherence was lost!
		
@coherentDoneInsert
		cmp.l	D7,A1
		bcs.s	@coherentInsertionSort

@coherentZeroItems
}

asm		//	Now find out what is visible by the list from the previous line.
{
		move.l	D1,D0				;	Top color is bgColor, polyId is 0
		cmp.l	D7,D6				;	Compare lastActive with activeList
		beq		@doneLine			;	Are there items to loop through?

		move.l	stackPoint, A0		//	-(A0) gets item from list.
		move.w	bounds.left,D2		;	Set current x to left clipping bound
		
		move.l	-(A0),A2			;	Read item & move to next one
		move.w	PolyEdge.x(A2),D0	;	New x is here

		cmp.w	D0,D2				;	Compare new and old x
		bge.s	@coherentEmptyRange	;	Old x >= new x

		cmp.w	D5,D0				;	Clip with right clipping bound
		bge.s	@coherentDone

@coherentOutput
		move.w	D0,D2				;	New x -> current x
		move.l	D0,(A5)+			;	Output color and x
@coherentEmptyRange
		move.l	PolyEdge.color(A2),D0	;	Read new color and top polyId

@coherentOutputLoop
		move.l	-(A0),A2			;	Read item & move to next one
		move.w	PolyEdge.x(A2),D0	;	New x is here

		cmp.w	D0,D2				;	Compare new and old x
		bge.s	@coherentEmptyRange	;	Old x >= new x

		cmp.w	D5,D0				;	Clip with right clipping bound
		blt.s	@coherentOutput
@coherentDone

		move.w	D5,D0
		move.l	D0,(A5)+			;	Output color and x
		bra		@outputDone
}
asm
{
@outOfMem
		subq.w	#1,i
		beq		@exitFunction

		move.l	thisOff,A2
		move.l	thisOn,A0
		
@outOfMemLoop
		clr.l	(A2)+
		clr.l	(A0)+
		move.w	bgColor,(A5)+
		move.w	D5,(A5)+
		subq.w	#1,i
		bne		@outOfMemLoop

@exitFunction
		move.l	stackSave, sp
		movem.l	(sp)+,D3-D7/A2-A5
	}
}

#if DoProfile
void	ScanConvertPolys68KOld(
	PolyWorld		*polys,
	short			*destBuffer,
	PolyEdgePtr		*thisOn,
	PolyEdgePtr		*thisOff,
	Rect			*theBounds,
	short			bgColor)
{
	short			*emergencyStop;
	PolyEdgePtr		activeList[ACTIVELISTSIZE];
	PolyEdge		backgroundColor,sentinel;
	short			i;
	Rect			bounds;

	emergencyStop = destBuffer+polys->displayRegionSize-2*ACTIVELISTSIZE;

	bounds = *theBounds;

	i = bounds.bottom - bounds.top;

asm		//	Loop preliminaries
{
		movem.l	D3-D7/A2-A5,-(sp)
		
		lea		backgroundColor,A0
		move.w	bgColor,D1
		move.w	D1,PolyEdge.color(A0)
		swap	D1				;	move bgColor to high word.
		clr.w	D1				;	polyId = 0
		clr.w	PolyEdge.polyID(A0)
		move.l	bounds.left,D0
		clr.w	D0
		move.l	D0,PolyEdge.x(A0)
		
		move.w	bounds.right,D5

		lea		activeList,A0	;	Initialize active list.
		lea		sentinel,A1
		move.w	#0x8000,PolyEdge.x(A1)
		move.l	A1,(A0)+		;	Adjust for sentinel item.
		move.l	A0,D6			;	active list table
		move.l	A0,D7			;	Last active edge

		move.l	destBuffer,A5	;	We don't need A5 globals, so let's use A5 as output
@screenLoop
}
asm		//	Add new edges to the active edge list.
{
		addq.l	#4,thisOn
		move.l	thisOn,A2		; Add new entries to active list
		move.l	-(A2),D0		; Read entry into D0
		beq.s	@noActives

		move.l	D7,A0

@addActive
		move.l	D0,A1
		move.l	D0,(A0)+		; Place pointer in active list
		move.l	PolyEdge.nextOn(A1),D0	; Move to next
		bne.s	@addActive

		move.l	A0,D7
		clr.l	(A2)			; Clear "on" list so that it can be re-used.
@noActives
}

asm		//	Sort the edges according to x with an insertion sort
{
		move.l	D6,A1				;	This is the first item
		cmp.l	D7,A1
		beq.s	@zeroItems

@insertionSort
		move.l	A1,A3				;	Previous position in active list
		move.l	(A1)+,A2			;	Pointer to the item to insert

		move.l	PolyEdge.dx(A2),D0
		add.l	D0,PolyEdge.x(A2)
		move.w	PolyEdge.x(A2),D0
@doInsert
		move.l	-(A3),A4
		cmp.w	PolyEdge.x(A4),D0	;	Compare with key
		bge		@doneInsert

		move.l	A4,4(A3)			;	Swap pointers to items
		move.l	A2,(A3)
		bra.s	@doInsert
		
@doneInsert
		cmp.l	D7,A1
		bcs.s	@insertionSort
		
@zeroItems	
}

asm		//	Now find out what is visible by using a depth stack (actually a linked list)
{
		move.l	D1,D0				;	Top color is bgColor, polyId is 0
		cmp.l	D7,D6				;	Compare lastActive with activeList
		beq		@doneLine			;	Are there items to loop through?

		lea		backgroundColor,A1	;	A1 is the top of depth stack list.
		move.w	bounds.left,D2		;	Set current x to left clipping bound

		move.l	D6,A0				;	Start from beginning of active list

@outputLoop
		move.l	(A0)+,A2			;	Read item & move to next one
		move.w	PolyEdge.polyID(A2),D3	;	Read polyID

		cmp.w	D0,D3				;	Check with top of stack polyID
		bhi.s	@pushAtTop
		beq.s	@popFromTop

		move.l	A1,A3				;	Copy stack link pointer
@findWhere
		move.l	PolyEdge.nextOn(A3),A4	;	Move to next item.
		cmp.w	PolyEdge.polyID(A4),D3	;	Compare polyIDs.
		bhi.s	@foundWhere2
		beq.s	@alreadyThere2

		move.l	PolyEdge.nextOn(A4),A3	;	Move to next item.
		cmp.w	PolyEdge.polyID(A3),D3	;	Compare polyIDs.
		bhi.s	@foundWhere
		bne.s	@findWhere

@alreadyThere						;	Item is there, so now remove it
		move.l	PolyEdge.nextOn(A3),D3
		move.l	D3,PolyEdge.nextOn(A4)
		bra.s	@noStackTopChange

@alreadyThere2						;	Item is there, so now remove it
		move.l	PolyEdge.nextOn(A4),D3
		move.l	D3,PolyEdge.nextOn(A3)
		bra.s	@noStackTopChange

@foundWhere2						;	Item isn't on stack. Add it there.
		move.l	A2,PolyEdge.nextOn(A3)
		move.l	A4,PolyEdge.nextOn(A2)
		bra.s	@noStackTopChange

@foundWhere							;	Item isn't on stack. Add it there.
		move.l	A2,PolyEdge.nextOn(A4)
		move.l	A3,PolyEdge.nextOn(A2)
		bra.s	@noStackTopChange

@popFromTop
		move.l	PolyEdge.nextOn(A1),A1
		bra.s	@stackTopChange

@pushAtTop
		move.l	A1,PolyEdge.nextOn(A2)
		move.l	A2,A1

@stackTopChange
		move.w	PolyEdge.x(A2),D0	;	New x is here

		cmp.w	D0,D2				;	Compare new and old x
		bge.s	@emptyRange			;	Old x >= new x

		cmp.w	D5,D0				;	Clip with right clipping bound
		bge.s	@doneLine
//	Output stage
		move.w	D0,D2				;	New x -> current x
		move.l	D0,(A5)+			;	Output color and x

@emptyRange
		move.l	PolyEdge.color(A1),D0	;	Read new color and top polyId

@noStackTopChange
		cmp.l	D7,A0				;	Output loop end
		bne.s	@outputLoop

@doneLine
		move.w	D5,D0
		move.l	D0,(A5)+			;	Output color and x
@outputDone
}
asm		//	Deactivate and remove edges from active list according to "offLists".
{
		move.l	thisOff,A2
		addq.l	#4,thisOff
		move.l	(A2),D0
		beq.s	@noDeactives

@clearActive								;	Zero polyID fields of those
		move.l	D0,A1
		clr.w	PolyEdge.polyID(A1)		;	edges that are no longer
		move.l	PolyEdge.nextOff(A1),D0	;	to be in the active list.
		bne.s	@clearActive
											;	Remove edges with zero polyID
		move.l	D6,A1						;	by shifting pointers.
		move.l	A1,A3						;	A1 is source, A3 is destination of copy.

@removeActive
		move.l	(A1)+,A4
		tst.w	PolyEdge.polyID(A4)
		beq.s	@inactive
		move.l	A4,(A3)+				;	Item is active, copy it.
@inactive
		cmp.l	A1,D7
		bne.s	@removeActive

		move.l	A3,D7
		clr.l	(A2)			; Clear "off" list so that it can be re-used.

@noDeactives
}

asm		//	End of main loop
{
		cmp.l	emergencyStop, A5
		bcc.s	@outOfMem
		subq.w	#1,i
		bne		@screenLoop
		bra.s	@exitFunction
@outOfMem
		subq.w	#1,i
		beq		@exitFunction

		move.l	thisOff,A2
		move.l	thisOn,A0
		
@outOfMemLoop
		clr.l	(A2)+
		clr.l	(A0)+
		move.w	bgColor,(A5)+
		move.w	D5,(A5)+
		subq.w	#1,i
		bne		@outOfMemLoop

@exitFunction
		movem.l	(sp)+,D3-D7/A2-A5
	}
}

void	ScanConvertPolysC(
	PolyWorld		*polys,
	short			*destBuffer,
	PolyEdgePtr		*thisOn,
	PolyEdgePtr		*thisOff,
	Rect			*theBounds,
	short			bgColor);

static	long	minNewTime = 0x7fffFFFF;
static	long	maxNewTime;
static	long	minOldTime = 0x7fffFFFF;
static	long	maxOldTime;
static	long	oldTimer;
static	long	newTimer;
static	long	callTimes;

#define	THEDELAY	-60000000L

void	ScanConvertPolys68K(
	PolyWorld		*polys,
	short			*destBuffer,
	PolyEdgePtr		*thisOn,
	PolyEdgePtr		*thisOff,
	Rect			*theBounds,
	short			bgColor)
{
	TMTask	miniTask;
	short	i;
	long	deltaTime;
	short	rowCount = theBounds->bottom - theBounds->top;

	if(rowCount > 100)
		callTimes++;

	if(callTimes & 1)
	{	miniTask.tmAddr = NULL;
		miniTask.tmWakeUp = 0;
		miniTask.tmReserved = 0;
		InsXTime((QElemPtr)&miniTask);
		FlushDataCache();
		FlushInstructionCache();
		PrimeTime((QElemPtr)&miniTask, THEDELAY);
		ScanConvertPolys68KOld(polys, destBuffer, thisOn, thisOff, theBounds, bgColor);
		RmvTime((QElemPtr)&miniTask);
	
		if(rowCount > 100)
		{	deltaTime = miniTask.tmCount - THEDELAY;
	
			oldTimer += deltaTime;
	
			if(deltaTime < minOldTime)
			{	minOldTime = deltaTime;
			}
	
			if(deltaTime > maxOldTime)
			{	maxOldTime = deltaTime;
			}
		}
	}
	else
	{
		miniTask.tmAddr = NULL;
		miniTask.tmWakeUp = 0;
		miniTask.tmReserved = 0;
		InsXTime((QElemPtr)&miniTask);
		FlushDataCache();
		FlushInstructionCache();
		PrimeTime((QElemPtr)&miniTask, THEDELAY);
		ScanConvertPolys68KNew(polys, destBuffer, thisOn, thisOff, theBounds, bgColor);
		RmvTime((QElemPtr)&miniTask);
	
		if(rowCount > 100)
		{
			deltaTime = miniTask.tmCount - THEDELAY;
	
			newTimer += deltaTime;
	
			if(deltaTime < minNewTime)
			{	minNewTime = deltaTime;
			}
	
			if(deltaTime > maxNewTime)
			{	maxNewTime = deltaTime;
			}
		}
	}
}
#endif