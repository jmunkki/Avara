/*
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: AddFPolySegment.c
    Created: Friday, July 1, 1994, 7:15
    Modified: Wednesday, February 7, 1996, 22:15
*/

#include "PAKit.h"

#pragma options(mc68020,!assign_registers)

extern	PolyWorld		*thePolyWorld;

PolyEdgePtr	AddFPolySegment(
	Fixed	ax,
	Fixed	ay,
	Fixed	bx,
	Fixed	by)
{
asm	{
		movem.l	A2/D2-D7,-(sp)
		clr.l	D0								;	Default result is NULL
		
		move.l	thePolyWorld,A0
		move.l	PolyWorld.xOffset(A0),D7
		clr.w	D7	//	Clear lower bits.

		move.l	OFFSET(PolyWorld,bounds.right)(A0),D2
		move.l	D7,D1
		add.l	ax,D7
		clr.w	D2
		add.l	bx,D1
		cmp.l	D2,D7
		blt.s	@noXClip						; Line is on left side of clip bound
		cmp.l	D2,D1
		bge		@noEdge							; Line is totally on right side of clip bound
@noXClip
		move.l	ay,D2
		move.l	by,D3
		
		cmp.l	D2,D3
		bge.s	@noSwap
		exg		D7,D1
		exg		D2,D3	; D3 = YMax.long, D2 = YMin.long
@noSwap
		move.l	D3,D6
		sub.l	D2,D6	;	D6 = delta Y

		move.l	D2,D4		;	D4 = YMin.long

		subq.l	#1,D3
		subq.l	#1,D4
		swap.w	D3
		swap.w	D4
		addq.w	#1,D3
		ble		@noEdge						; Line is above viewport

		addq.w	#1,D4
		cmp.w	PolyWorld.height(A0),D4
		bge		@noEdge						; Line is below viewport

		cmp.w	D3,D4
		beq		@noEdge
		
		sub.l	D7,D1							; D1 = delta x

		move.l	PolyWorld.newEdge(A0),A1

		swap.w	D1								; Get ready for a long division
		move.w	D1,D5

		cmp.l	PolyWorld.endEdge(A0),A1
		bcc		@noEdge

		ext.l	D5
		clr.w	D1								; delta x is now in D5:D1
		
		divs.l	D6,D5:D1						; D1 = slope of this line
		bvs.s	@didOverflow
@noOverflow

		move.l	D1,PolyEdge.dx(A1)				; Store slope

		tst.w	D4								; See if we have to clip the top
		bpl.s	@unitAdjust

		muls.l	D2,D5:D1						; Clip top
		clr.w	D4
		move.w	D5,D1
		swap.w	D1
		sub.l	D1,D7
		bra.s	@endClips

@didOverflow
		clr.l	D1								;	One in a million overflow
		bra.s	@noOverflow						;	Slope was zeroed...this seems to work.

@unitAdjust
		clr.l	D5
		sub.w	D2,D5
		muls.l	D5,D5:D1
		move.w	D5,D1
		swap.w	D1
		add.l	D1,D7							; Adjust PolyEdge.dx for first pixel

@endClips
		sub.l	PolyEdge.dx(A1),D7
		move.l	D7,PolyEdge.x(A1)

		move.l	PolyWorld.onLists(A0),A2
		move.l	(A2,D4.w*4),D7
		move.l	A1,(A2,D4.w*4)
		move.l	D7,PolyEdge.nextOn(A1)

		cmp.w	PolyWorld.height(A0),D3
		bge.s	@noOffList

		move.l	PolyWorld.offLists(A0),A2
		move.l	-4(A2,D3.w*4),D7
		move.l	A1,-4(A2,D3.w*4)
		move.l	D7,PolyEdge.nextOff(A1)
@noOffList
		move.w	PolyWorld.currentColor(A0),PolyEdge.color(A1)
		move.w	PolyWorld.polyCount(A0),PolyEdge.polyID(A1)
		move.l	A1,D0							;	Return pointer to edge record
		add.w	#sizeof(PolyEdge),A1
		move.l	A1,PolyWorld.newEdge(A0)

@noEdge
		movem.l	(sp)+,A2/D2-D7
	}
}

void	ReplicatePolySegment(
	PolyEdgePtr		theEdge)
{
asm	{
		move.l	theEdge,D0
		beq.s	@noEdgeToCopy
		
		move.l	thePolyWorld,A0
		move.l	PolyWorld.newEdge(A0),A1
		cmp.l	PolyWorld.endEdge(A0),A1
		bcc		@noEdgeToCopy

		move.w	PolyWorld.currentColor(A0), PolyEdge.color(A1)
		move.w	PolyWorld.polyCount(A0),PolyEdge.polyID(A1)

		exg		D0,A0
		
		move.l	PolyEdge.dx(A0),PolyEdge.dx(A1)
		move.l	PolyEdge.x(A0),PolyEdge.x(A1)
		move.l	PolyEdge.nextOn(A0),PolyEdge.nextOn(A1)
		move.l	A1,PolyEdge.nextOn(A0)

		move.l	PolyEdge.nextOff(A0),PolyEdge.nextOff(A1)
		move.l	A1,PolyEdge.nextOff(A0)
		exg		D0,A0

		add.w	#sizeof(PolyEdge),A1
		move.l	A1,PolyWorld.newEdge(A0)
@noEdgeToCopy
	}
}