/*/
    Copyright ©1992-1995, Juri Munkki
    All rights reserved.

    File: Mat68000.c
    Created: Saturday, October 17, 1992, 1:03
    Modified: Wednesday, November 1, 1995, 13:54
/*/

#include "FastMat.h"

#pragma options(!mc68020,!assign_registers)

/*
**	vd[i][j] = sum(k=0 to 3, vs[i][k] * m[k][j])
**	where	i=0 to n-1
**			j=0 to 3
**
**	This is effectively a vector to matrix multiplication
**	if n is 1 and a matrix with matrix multiplication when
**	n is 4.
*/
asm
void	VectorMatrixProduct68000(
	long	n,
	Vector	*vs,
	Vector	*vd,
	Matrix	*m)
{
	fralloc

		movem.l	D3-D7/A2-A4,-(sp)
		subq.l	#1,n
		bmi		@end
		
		move.l	m,A0
		move.l	vs,A1
		move.l	vd,A2
		
@onevector:
		moveq.l	#3,D7		/*	Loop counter for 4 dbra loops. (j)	*/
		move.l	A0,A3		/*	Matrix into A3						*/
@onenumber:
		clr.l	D0			/*	Accumulate into D0					*/
		move.l	A1,A4
		moveq.l	#3,D6		/*	Loop counter for 4 dbra loops. (i)	*/
@onecomponent:
		move.l	(A3),D1		/*	Read matrix item [i][j] into D1		*/
		beq.s	@zero_matrix_item
		bpl.s	@positive_m
		neg.l	D1
		move.l	(A4)+,D2
		beq.s	@zero_result
		bpl.s	@negative_result
		neg.l	D2
		bra.s	@positive_result
@zero_matrix_item:
		addq.l	#4,A4
		bra.s	@zero_result
@positive_m:
		move.l	(A4)+,D2
		beq.s	@zero_result
		bpl.s	@positive_result
		
		neg.l	D2
@negative_result:
		move.w	D1,D3
		mulu.w	D2,D3		/*	D3 = Lo * Lo						*/
		
		swap	D1
		move.w	D1,D4
		mulu.w	D2,D4		/*	D4 = Hi * Lo						*/
		
		swap	D2
		move.w	D1,D5
		mulu.w	D2,D5		/*	D5 = Hi * Hi						*/
		
		swap	D1
		mulu.w	D2,D1		/*	D1 = Lo * Hi						*/
		add.l	D4,D1
		
		move	D5,D3
		swap	D3
		add.l	D3,D1
		sub.l	D1,D0		/*	Accumulate							*/
		bra.s	@nextstep

@positive_result:
		move.w	D1,D3
		mulu.w	D2,D3		/*	D3 = Lo * Lo						*/
		
		swap	D1
		move.w	D1,D4
		mulu.w	D2,D4		/*	D4 = Hi * Lo						*/
		
		swap	D2
		move.w	D1,D5
		mulu.w	D2,D5		/*	D5 = Hi * Hi						*/
		
		swap	D1
		mulu.w	D2,D1		/*	D1 = Lo * Hi						*/
		add.l	D4,D1
		
		move	D5,D3
		swap	D3
		add.l	D3,D1
		add.l	D1,D0		/*	Accumulate							*/
@zero_result:
@nextstep:
		add.w	#16,A3
		dbra	D6,@onecomponent
		
		move.l	D0,(A2)+	/*	Store into vd.						*/
		sub.w	#4*16-4,A3	/*	Next matrix column					*/
		
		dbra	D7,@onenumber
		add.w	#16,A1
		subq.l	#1,n
		bpl		@onevector
		
@end:
		movem.l	(sp)+,D3-D7/A2-A4

	frfree
	rts
}

asm
Fixed	FMul68000(
	Fixed	a,
	Fixed	b)
{
	fralloc
	
		movem.l D3-D5,-(sp)
		clr.l	D0
		move.l	a,D1
		beq.s	@zero_result
		bpl.s	@positive_m
		neg.l	D1
		move.l	b,D2
		beq.s	@zero_result
		bpl.s	@negative_result
		neg.l	D2
		bra.s	@positive_result
@positive_m:
		move.l	b,D2
		beq.s	@zero_result
		bpl.s	@positive_result
		
		neg.l	D2
@negative_result:
		move.w	D1,D3
		mulu.w	D2,D3		/*	D3 = Lo * Lo						*/
		
		swap	D1
		move.w	D1,D4
		mulu.w	D2,D4		/*	D4 = Hi * Lo						*/
		
		swap	D2
		move.w	D1,D5
		mulu.w	D2,D5		/*	D5 = Hi * Hi						*/
		
		swap	D1
		mulu.w	D2,D1		/*	D1 = Lo * Hi						*/
		add.l	D4,D1
		
		move	D5,D3
		swap	D3
		add.l	D3,D1
		sub.l	D1,D0		/*	Accumulate							*/
		bra.s	@end

@positive_result:
		move.w	D1,D3
		mulu.w	D2,D3		/*	D3 = Lo * Lo						*/
		
		swap	D1
		move.w	D1,D4
		mulu.w	D2,D4		/*	D4 = Hi * Lo						*/
		
		swap	D2
		move.w	D1,D5
		mulu.w	D2,D5		/*	D5 = Hi * Hi						*/
		
		swap	D1
		mulu.w	D2,D1		/*	D1 = Lo * Hi						*/
		add.l	D4,D1
		
		move	D5,D3
		swap	D3
		add.l	D3,D1
		move.l	D1,D0		/*	Accumulate							*/
@zero_result:
@end:
		movem.l	(sp)+,D3-D5		
	
	frfree
	rts
}

asm
Fixed	FDiv68000(
	Fixed	a,
	Fixed	b)
{
	fralloc

		move.l	#0x1FFFF,D0
		move.l	b,D2
		beq.s	@divide_by_zero
		bpl.s	@positive_b
		neg.l	D2
		move.l	a,D1
		bpl.s	@negative_result
		neg.l	D1
		bra.s	@positive_result
		
@positive_b:
		move.l	a,D1
		beq.s	@divide_by_zero
		bpl.s	@positive_result
		neg.l	D1
		
@negative_result:
		cmp.l	D2,D1
		bcs.s	@nsecondstage
@nfirststage:
		add.l	D2,D2
		lsr.l	#1,D0
		cmp.l	D2,D1
		bcc.s	@nfirststage
		
@nsecondstage:
		add.l	D1,D1
		sub.l	D2,D1
		bcs.s	@nnosub
		add.l	D0,D0
		bcc.s	@nsecondstage
		bra.s	@ndone
@nnosub:
		add.l	D2,D1
		roxl.l	#1,D0
		bcc.s	@nsecondstage
@ndone:
		addq.l	#1,D0
		bra.s	@alldone

@divide_by_zero:
		moveq.l	#-1,D0
		bra.s	@alldone

@positive_result:
		cmp.l	D2,D1
		bcs.s	@psecondstage
@pfirststage:
		add.l	D2,D2
		lsr.l	#1,D0
		cmp.l	D2,D1
		bcc.s	@pfirststage

@psecondstage:
		add.l	D1,D1
		sub.l	D2,D1
		bcs.s	@pnosub
		add.l	D0,D0
		bcc.s	@psecondstage
		bra.s	@pdone
@pnosub:
		add.l	D2,D1
		roxl.l	#1,D0
		bcc.s	@psecondstage

@pdone:
		moveq.l	#-1,D1
		eor.l	D1,D0
@alldone:
	frfree
	rts
}

asm
Fixed	FMulDiv68000(
	Fixed	a,
	Fixed	b,
	Fixed	c)
{
	fralloc
		movem.l	D3-D5,-(sp)
		move.l	a,D0
		bpl.s	@positive_a
		neg.l	c
		neg.l	D0
@positive_a:
		move.l	b,D1
		bpl.s	@positive_b
		neg.l	D1
		neg.l	c
@positive_b:
		move.w	D1,D2
		mulu.w	D0,D2		//	D2 = Lo * Lo

		move.w	D1,D3
		swap	D0
		mulu.w	D0,D3		//	D3 = Hi * Lo
		
		swap	D1
		move.w	D1,D4
		mulu.w	D0,D4		//	D4 = Hi * Hi
		
		swap	D0
		mulu.w	D0,D1		//	D1 = Lo * Hi
		
		clr.l	D5
		move.w	D3,D5		
		swap	D5
		clr.w	D3
		swap	D3
		add.l	D5,D2		//	64 bit addition Hi*Hi:Lo*Lo += Hi*Lo
		addx.l	D3,D4
		
		clr.l	D5
		move.w	D1,D5
		swap	D5
		clr.w	D1
		swap	D1
		add.l	D5,D2		//	64 bit addition Hi*Hi:Lo*Lo+Hi*Lo += Lo*Hi
		addx.l	D1,D4		//	Result is now in D4:D2
		
		add.l	D2,D2
		addx.l	D4,D4
	
		move.l	c,D1
		bpl.s	@positive_c
		neg.l	D1
@positive_c:
		moveq.l	#1,D0
		bra.s	@divloop
@divok:
		lsl.l	#1,D0
		bcs.s	@divdone
		add.l	D2,D2
		addx.l	D4,D4
@divloop:
		sub.l	D1,D4
		bcc.s	@divok

		addx.l	D0,D0
		bcs.s	@divdone

		add.l	D1,D4
		add.l	D2,D2
		addx.l	D4,D4
		bra.s	@divloop

@divdone:
		move.l	c,D1
		bpl.s	@positive_result
		
		addq.l	#1,D0
		bra.s	@done
@positive_result:
		eor.l	#-1,D0
@done:
		movem.l	(sp)+,D3-D5
	
	frfree
	rts
}

/*
**	The following code segment was posted on usenet by Ken Turkowski of
**	Apple Computer. Mr. Turkowski seems to be a real graphics wizard
**	and has provided many other useful insights on the field of computer
**	graphics. Here he shows a way to get a high precision square root
**	with nothing but shifts, addition and subtraction.
**
**	I converted this routine into 68K assembler and made it work on a
**	64 bit operand and return a 32 bit result of a 16:16 fixed point
**	number format. Works really well.
**
**	Original code follows (with comment delimiters changed):
**
**	typedef long Fract;     (* 2.30: 1 sign bit, one integer bit, 30 fractional bits  *)
**
**	Fract
**	FFracSqrt(Fract a)
**	{
**		register unsigned long root, remHi, remLo, testDiv, count;
**
**		root = 0;		(* Clear root *)
**		remHi = 0;		(* Clear high part of partial remainder *)
**		remLo = a;		(* Get argument into low part of partial remainder *)
**		count = 30;		(* Load loop counter *)
**
**		do {
**			remHi = (remHi << 2) | (remLo >> 30); remLo <<= 2;	(* get 2 bits of arg	*)
**			root <<= 1;					(* Get ready for the next bit in the root		*)
**			testDiv = (root << 1) + 1;	(* Test divisor *)
**			if (remHi >= testDiv) {
**				remHi -= testDiv;
**				root += 1;
**			}
**		} while (count-- != 0);
**
**		return(root);
**	}
**
**	Ken Turkowski @ Apple Computer, Inc., Cupertino, CA
**	Internet: turk@apple.com	Applelink: TURK		UUCP: sun!apple!turk
**
**	The following code was written by Juri Munkki as an adaptation
**	of the above algorithm. It is a fairly straightforward implementation.
*/
asm
Fixed	FSqroot(
	long	*ab)
{
		movem.l	D3-D5,-(sp)
		move.l	4(a7),A0
		clr.l	D0			// Result accumulates here.
		clr.l	D1			// Bits shift into this place
		move.l	(A0)+,D2
		move.l	(A0)+,D3
		moveq.l	#31,D4
@looper:
		add.l	D3,D3		//	Do a shift right using addition
		addx.l	D2,D2
		addx.l	D1,D1
		add.l	D3,D3		//	Do a second shift right.
		addx.l	D2,D2
		addx.l	D1,D1
		add.l	D0,D0
		moveq.l	#1,D5
		add.l	D0,D5
		add.l	D0,D5
		cmp.l	D5,D1
		blt.s	@notbig
		addq.l	#1,D0
		sub.l	D5,D1
@notbig:
		dbra	D4,@looper
		movem.l	(sp)+,D3-D5

	rts
}

asm
void	FSquareAccumulate68000(
	Fixed	n,
	long	*acc)
{
	fralloc

		move.l	D3,-(sp)
		move.l	acc,A0
		move.l	n,D0
		beq.s	@zero
		bpl.s	@positive
		neg.l	D0
@positive:
		move.w	D0,D1
		mulu.w	D1,D1	//	lo * lo
		move.w	D0,D2
		swap	D0
		mulu.w	D0,D2	//	lo * hi
		move.w	D2,D3
		swap	D3
		clr.w	D2
		clr.w	D3
		swap	D2
		mulu.w	D0,D0	//	hi * hi
		add.l	D3,D1
		addx.l	D2,D0
		add.l	D3,D1
		addx.l	D2,D0
		move.l	(A0),D2
		add.l	D1,4(A0)
		addx.l	D0,D2
		move.l	D2,(A0)
@zero:
		move.l	(sp)+,D3
	
	frfree
	rts
}

asm
void	FSquareSubtract68000(
	Fixed	n,
	long	*acc)
{
	fralloc
	
		move.l	D3,-(sp)
		move.l	acc,A0
		move.l	n,D0
		beq.s	@zero
		bpl.s	@positive
		neg.l	D0
@positive:
		move.w	D0,D1
		mulu.w	D1,D1	//	lo * lo
		move.w	D0,D2
		swap	D0
		mulu.w	D0,D2	//	lo * hi
		move.w	D2,D3
		swap	D3
		clr.w	D2
		clr.w	D3
		swap	D2
		mulu.w	D0,D0	//	hi * hi
		add.l	D3,D1
		addx.l	D2,D0
		add.l	D3,D1
		addx.l	D2,D0
		move.l	(A0),D2
		sub.l	D1,4(A0)
		subx.l	D0,D2
		move.l	D2,(A0)
@zero:
		move.l	(sp)+,D3
	
	frfree
	rts
}

typedef struct
{	short	hi;
	short	lo;
} HiLoLong;

asm
Fixed	FRandom68000()
{
		lea		FRandSeed, A0
		move.w	#0x41A7,D2
		move.w	D2,D0
		mulu.w	struct(HiLoLong.lo)(A0),D2
		move.l	D2,D1
		clr.w	D1
		swap.w	D1
		mulu.w	struct(HiLoLong.hi)(A0),D0
		add.l	D1,D0
		move.l	D0,D1
		add.l	D1,D1
		clr.w	D1
		swap.w	D1
		and.l	#0x0000ffff,D2
		sub.l	#0x7fffFFFF,D2
		and.l	#0x00007fff,D0
		swap.w	D0
		add.l	D1,D0
		add.l	D0,D2
		bpl.s	@positive
		add.l	#0x7fffFFFF,D2
@positive:
		move.l	D2,(A0)
		clr.l	D0
		move.w	D2,D0
	rts
}

asm
Fixed	FRandomBeta()
{
		lea		FRandSeedBeta, A0
		move.w	#0x41A7,D2
		move.w	D2,D0
		mulu.w	struct(HiLoLong.lo)(A0),D2
		move.l	D2,D1
		clr.w	D1
		swap.w	D1
		mulu.w	struct(HiLoLong.hi)(A0),D0
		add.l	D1,D0
		move.l	D0,D1
		add.l	D1,D1
		clr.w	D1
		swap.w	D1
		and.l	#0x0000ffff,D2
		sub.l	#0x7fffFFFF,D2
		and.l	#0x00007fff,D0
		swap.w	D0
		add.l	D1,D0
		add.l	D0,D2
		bpl.s	@positive
		add.l	#0x7fffFFFF,D2
@positive:
		move.l	D2,(A0)
		clr.l	D0
		move.w	D2,D0

	rts
}


/*	FDistanceEstimate
**
**	May underestimate distance by about 15%.
**
**	The method is based on a Graphics Gem, only adapted to 3D.
**
**	Absolute values of the deltas are first calculated, then
**	the deltas are sorted from greatest to smallest (D0, D1, D2).
**	An estimate is calculated with (D0 + D1/2 + D2/4) * (1-1/8-1/256)
*/
asm
Fixed	FDistanceEstimate(
	Fixed	dx,
	Fixed	dy,
	Fixed	dz)
{
	fralloc
		move.l	dx,D0
		bpl.s	@noXNeg
		neg.l	D0
@noXNeg:
		move.l	dy,D1
		bpl.s	@noYNeg
		neg.l	D1
@noYNeg:
		cmp.l	D0,D1
		blt.s	@noXYSwap
		exg.l	D0,D1
@noXYSwap:
		move.l	dz,D2
		bpl.s	@noZNeg
		neg.l	D2
@noZNeg:
		cmp.l	D1,D2
		blt.s	@noYZSwap
		exg.l	D1,D2

		cmp.l	D0,D1
		blt.s	@noXZSwap
		exg.l	D0,D1
@noXZSwap:
@noYZSwap:
		lsr.l	#1,D2
		add.l	D2,D1
		lsr.l	#1,D1
		add.l	D1,D0

		move.l	D0,D1	// scale by (1-1/8-1/256) to keep estimate from being too large.
		lsr.l	#3,D1
		sub.l	D1,D0
		lsr.l	#5,D1
		sub.l	D1,D0

	frfree
	rts
}

/*	FDistanceEstimate
**
**	May overestimate distance by about 15%.
**
**	The method is based on a Graphics Gem, only adapted to 3D.
**
**	Absolute values of the deltas are first calculated, then
**	the deltas are sorted from greatest to smallest (D0, D1, D2).
**	An estimate is calculated with (D0 + D1/2 + D2/4)
*/
asm
Fixed	FDistanceOverEstimate(
	Fixed	dx,
	Fixed	dy,
	Fixed	dz)
{
	fralloc
		move.l	dx,D0
		bpl.s	@noXNeg
		neg.l	D0
@noXNeg:
		move.l	dy,D1
		bpl.s	@noYNeg
		neg.l	D1
@noYNeg:
		cmp.l	D0,D1
		blt.s	@noXYSwap
		exg.l	D0,D1
@noXYSwap:
		move.l	dz,D2
		bpl.s	@noZNeg
		neg.l	D2
@noZNeg:
		cmp.l	D1,D2
		blt.s	@noYZSwap
		exg.l	D1,D2

		cmp.l	D0,D1
		blt.s	@noXZSwap
		exg.l	D0,D1
@noXZSwap:
@noYZSwap:
		lsr.l	#1,D2
		add.l	D2,D1
		lsr.l	#1,D1
		add.l	D1,D0
	
	frfree
	rts
}