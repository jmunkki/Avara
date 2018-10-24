/*/
    Copyright ©1992-1995, Juri Munkki
    All rights reserved.

    File: Mat68020.c
    Created: Saturday, October 17, 1992, 0:50
    Modified: Monday, November 20, 1995, 10:35
/*/

#include "FastMat.h"

#pragma options(mc68020,!assign_registers)

/*
**	vd[i][j] = sum(k=0 to 3, vs[i][k] * m[k][j])
**	where	i=0 to n-1
**			j=0 to 3
**
**	This is effectively a vector to matrix multiplication.
*/
asm
void	VectorMatrixProduct68020(
	long	n,
	Vector	*vs,
	Vector	*vd,
	Matrix	*m)
{
	short	temp;

	machine 68020
	fralloc
		move.l	n,D0
		beq.s	@end
		movem.l	D3-D7/A2-A4,-(sp)

		move.l	m,A0
		move.l	vs,A1
		move.l	vd,A2

		lsl.l	#4,D0
		lea		(A1,D0.L),A4
@onevector:
		movem.l	(A1)+,D0-D3
		
		move.w	#3,temp		/*	Loop counter for dbra 4 loops.		*/
		move.l	A0,A3		/*	Matrix into A3						*/
@onenumber:
		move.l	(A3)+,D7	/*	Read matrix item [0][j] into D7		*/
		muls.l	D0,D6:D7	/*	Do a long multiply with vd[i][0].	*/
		
		move.l	12(A3),D5	/*	Read item [1][j] into D6.			*/
		muls.l	D1,D4:D5
		add.l	D5,D7		/*	Accumulate to D6:D7					*/
		
		move.l	28(A3),D5
		addx.l	D4,D6
		muls.l	D2,D4:D5
		add.l	D5,D7		/*	Accumulate to D6:D7					*/

		move.l	44(A3),D5
		addx.l	D4,D6
		muls.l	D3,D4:D5
		add.l	D5,D7		/*	Accumulate to D6:D7					*/
		addx.l	D4,D6
		
		move.w	D6,D7
		swap	D7

		move.l	D7,(A2)+	/*	Store into vs[i][j]					*/
		
		subq.w	#1,temp
		bpl.s	@onenumber
		
		cmp.l	A1,A4
		bne.s	@onevector

		movem.l	(sp)+,D3-D7/A2-A4
@end:
	frfree
	rts
}

asm
Fixed	FMul68020(
	Fixed	a,
	Fixed	b)
{
	machine	68020
	fralloc
	move.l	a,D0
	muls.l	b,D1:D0
	move.w	D1,D0
	swap	D0
	frfree
	rts
}

asm
Fixed	FDiv68020(
	Fixed	a,
	Fixed	b)
{
	machine	68020
	fralloc
	move.l	b,D0
	beq.s	@end
	move.l	a,D1
	swap    D1
	move.l  D1,D0
	clr.w	D0
	ext.l	D1
	divs.l	b,D1:D0
@end:
	frfree
	rts
}

asm
Fixed	FMulDiv68020(
	Fixed	a,
	Fixed	b,
	Fixed	c)
{
	machine	68020
	fralloc

	move.l	a,D0
	move.l	c,D2
	beq.s	@infinity
	muls.l	b,D1:D0
	divs.l	D2,D1:D0
@infinity:
	frfree
	rts
}

/*
**	Here's a special version for the perspective transformation.
**	It alleviates the need for viewing pyramid clipping by detecting
**	overflow. In cases of overflow it returns 0x3FFF FFFF when
**	a * b >= 0 and -0x3FFF FFFF when a * b < 0.
*/
asm
Fixed	FMulDiv68020V(
	Fixed	a,
	Fixed	b,
	Fixed	c)
{
	machine	68020
	fralloc

	move.l	a,D0
	move.l	c,D2
	beq.s	@infinity
	muls.l	b,D1:D0
	divs.l	D2,D1:D0
	bvc.s	@end		//	Test for overflow
	moveq.l	#-1,D0
	lsr.l	#2,D0		//	D0 = 0x3FFF FFFF
	tst.l	D1
	bpl.s	@infinity
	neg.l	D0
@end:
@infinity:
	frfree
	rts
}

asm
void	FSquareAccumulate68020(
	Fixed	n,
	long	*acc)
{
	machine	68020
	fralloc

		move.l	acc,A0
		move.l	(A0)+,D2
		move.l	n,D1
		muls.l	D1,D0:D1
		add.l	D1,(A0)
		addx.l	D0,D2
		move.l	D2,-(A0)		

	frfree
	rts
}

asm
void	FSquareSubtract68020(
	Fixed	n,
	long	*acc)
{
	machine	68020
	fralloc

		move.l	acc,A0
		move.l	(A0)+,D2
		move.l	n,D1
		muls.l	D1,D0:D1
		sub.l	D1,(A0)
		subx.l	D0,D2
		move.l	D2,-(A0)		

	frfree
	rts
}
