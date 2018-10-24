/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: MatMore020.c
    Created: Sunday, July 3, 1994, 14:21
    Modified: Tuesday, November 28, 1995, 1:51
/*/

#include "FastMat.h"

#pragma options(mc68020,!assign_registers)
/*
**	vd[i][j] = sum(k=0 to 3, vs[i][k] * m[k][j])
**	where	i=0 to n-1
**			j=0 to 3
**
**	This is effectively a vector to matrix multiplication.
**	Special case optimized for matrices of the form:
**
**		a	b	c	0
**		d	e	f	0
**		g	h	i	0
**		0	0	0	1
*/
asm
void	VectorMatrix33Product020(
	long	n,
	Vector	*vs,
	Vector	*vd,
	Matrix	*m)
{
	fralloc
	machine 68020

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
		
		move.l	A0,A3		/*	Matrix into A3						*/
		moveq.l	#2,D4		/*	Loop counter for dbra 3 loops.		*/
@onenumber:
		move.l	(A3)+,D7	/*	Read matrix item [0][j] into D7		*/
		muls.l	D0,D6:D7	/*	Do a long multiply with vd[i][0].	*/

		move.l	12(A3),D5	/*	Read item [1][j] into D5.			*/		
		muls.l	D1,D3:D5
		add.l	D5,D7

		move.l	28(A3),D5
		addx.l	D3,D6
		muls.l	D2,D3:D5
		add.l	D5,D7
		addx.l	D3,D6

		move.w	D6,D7
		swap	D7
		move.l	D7,(A2)+	/*	Store into vs[i][j]					*/
		
		dbra	D4,@onenumber

		addq.l	#4,A2		/*	Skip w								*/
		
		cmp.l	A1,A4
		bne.s	@onevector
		
		movem.l	(sp)+,D3-D7/A2-A4
@end:
	frfree
	rts
}

/*
**	vd[i][j] = sum(k=0 to 3, vs[i][k] * m[k][j])
**	where	i=0 to n-1
**			j=0 to 3
**
**	This is effectively a vector to matrix multiplication.
**	Special case optimized for matrices of the form:
**
**		a	b	c	0
**		d	e	f	0
**		g	h	i	0
**		j	k	l	1
*/
asm
void	VectorMatrix34Product020(
	long	n,
	Vector	*vs,
	Vector	*vd,
	Matrix	*m)
{
	fralloc
	machine 68020
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
		
		move.l	A0,A3		/*	Matrix into A3						*/
		moveq.l	#2,D4		/*	Loop counter for dbra 3 loops.		*/
@onenumber:
		move.l	(A3)+,D7	/*	Read matrix item [0][j] into D7		*/
		muls.l	D0,D6:D7	/*	Do a long multiply with vd[i][0].	*/
		move.l	12(A3),D5	/*	Read item [1][j] into D5.			*/		
		muls.l	D1,D3:D5
		add.l	D5,D7		/*	Accumulate to D6:D7					*/
		
		move.l	28(A3),D5
		addx.l	D3,D6
		muls.l	D2,D3:D5
		add.l	D5,D7
		addx.l	D3,D6

		move.w	D6,D7
		swap	D7

		add.l	44(A3),D7
		
		move.l	D7,(A2)+	/*	Store into vs[i][j]					*/
		
		dbra	D4,@onenumber
		
		addq.l	#4,A2		/*	Skip w								*/

		cmp.l	A1,A4
		bne.s	@onevector
		
		movem.l	(sp)+,D3-D7/A2-A4
@end:
	frfree
	rts
}

/*
**	vd[i][j] = sum(k=0 to 3, vs[i][k] * m[k][j])
**	where	i=0 to n-1
**			j=0 to 3
**
**	This is effectively a vector to matrix multiplication.
**	Special case optimized for matrices of the form:
**
**		a	b	c	0
**		d	e	f	0
**		g	h	i	0
**		j	k	l	1
**
**	Only vectors that have a non-zero W component will be
**	transformed. Others will be simply skipped.
*/
asm
void	FlaggedVectorMatrix34Product020(
	long	n,
	Vector	*vs,
	Vector	*vd,
	Matrix	*m)
{
	fralloc
	machine 68020
		move.l	n,D0
		beq.s	@end
		movem.l	D3-D7/A2-A4,-(sp)

		move.l	m,A0
		move.l	vs,A1
		move.l	vd,A2

		lsl.l	#4,D0
		lea		(A1,D0.L),A4
		
		tst.l	12(A1)
		bne.s	@noSkip
@skip:
		moveq.l	#16,D0
		add.l	D0,A1
		add.l	D0,A2
		cmp.l	A1,A4
		beq.s	@done

@onevector:
		tst.l	12(A1)
		beq.s	@skip
@noSkip:
		movem.l	(A1)+,D0-D3
		move.l	A0,A3		/*	Matrix into A3						*/
		moveq.l	#2,D4		/*	Loop counter for dbra 3 loops.		*/
@onenumber:
		move.l	(A3)+,D7	/*	Read matrix item [0][j] into D7		*/
		muls.l	D0,D6:D7	/*	Do a long multiply with vd[i][0].	*/

		move.l	12(A3),D5	/*	Read item [1][j] into D5.			*/		
		muls.l	D1,D3:D5
		add.l	D5,D7		/*	Accumulate to D6:D7					*/
		
		move.l	28(A3),D5
		addx.l	D3,D6
		muls.l	D2,D3:D5
		add.l	D5,D7
		addx.l	D3,D6

		move.w	D6,D7
		swap	D7

		add.l	44(A3),D7
		move.l	D7,(A2)+
		
		dbra	D4,@onenumber
		
		addq.l	#4,A2		/*	Skip w								*/

		cmp.l	A1,A4
		bne.s	@onevector
@done:
		movem.l	(sp)+,D3-D7/A2-A4
@end:
	frfree
	rts
}

#define DOPROFILE_no
#ifdef DOPROFILE
static	long	a00 = 0;
static	long	a01 = 0;
static	long	a10 = 0;
static	long	a11 = 0;
#endif

asm
void	CombineTransforms68020(
	Matrix	*vs,
	Matrix	*vd,
	Matrix	*m)
{
	fralloc
	machine 68020
		movem.l	D3-D7/A2-A4,-(sp)
		
		move.l	m,A0
		moveq.l	#1,D0
		move.l	vs,A1
		move.l	vd,A2

#ifdef DOPROFILE
		cmp.w	(A1),D0
		bne.s	@failx1
		cmp.w	20(A1),D0
		bne.s	@failx1

		cmp.w	(A0),D0
		bne.s	@fail11
		cmp.w	20(A0),D0
		bne.s	@fail11

		addq.l	#1,a11
		bra.s	@endProf

@fail11:
		addq.l	#1,a01
		bra.s	@endProf
@failx1:
		cmp.w	(A0),D0
		bne.s	@fail10
		cmp.w	20(A0),D0
		bne.s	@fail10
		addq.l	#1,a10
		bra.s	@endProf
@fail10:
		addq.l	#1,a00
@endProf:
#endif
	/*	Check if second matrix (m) has an identity matrix as the 
	**	rotation matrix: (This catches about 22.5% of all cases)
	*/

		cmp.w	(A0),D0
		bne.s	@failFirstTest
		cmp.w	20(A0),D0
		bne.s	@failFirstTest

		movem.l	(A1)+,D0-D7
		movem.l	D0-D7,(A2)
		lea		48(A0),A0
		movem.l	(A1)+,D0-D7
		add.l	(A0)+,D4
		add.l	(A0)+,D5
		add.l	(A0)+,D6
		movem.l	D0-D7,32(A2)
		bra		@end

@failFirstTest:
	/*	Check if first matrix (vs) has an identity matrix as the 
	**	rotation matrix: (This catches about 37.4% of the rest of the cases)
	*/
		cmp.w	(A1),D0
		bne.s	@hardCase
		cmp.w	20(A1),D0
		bne.s	@hardCase

		movem.l	(A0),D0-D7
		movem.l	D0-D7,(A2)
		movem.l	32(A0),D0-D3
		lea		48(A1),A1
		movem.l	D0-D3,32(A2)
		lea		48(A2),A2
		
		bra.s	@doLastRow
@hardCase:
		lea		48(A1),A4
@firstThreeRows:
		movem.l	(A1)+,D0-D3

		moveq.l	#2,D4		/*	Loop counter for dbra 3 loops.		*/
		move.l	A0,A3		/*	Matrix into A3						*/
@onenumber:
		move.l	(A3)+,D7	/*	Read matrix item [0][j] into D7		*/
		muls.l	D0,D6:D7	/*	Do a long multiply with vd[i][0].	*/

		move.l	12(A3),D5	/*	Read item [1][j] into D5.			*/		
		muls.l	D1,D3:D5
		add.l	D5,D7
		
		move.l	28(A3),D5
		addx.l	D3,D6
		muls.l	D2,D3:D5
		add.l	D5,D7
		addx.l	D3,D6

		move.w	D6,D7
		swap	D7
		
		move.l	D7,(A2)+	/*	Store into vs[i][j]					*/
		
		dbra	D4,@onenumber
		
		clr.l	(A2)+		/*	Clear result[0..2][3]				*/

		cmp.l	A1,A4
		bne.s	@firstThreeRows
@doLastRow:
		movem.l	(A1)+,D0-D2

		moveq.l	#2,D4		/*	Loop counter for dbra 3 loops.		*/
@lastrownumbers:
		move.l	(A0)+,D7	/*	Read matrix item [0][j] into D7		*/
		muls.l	D0,D6:D7	/*	Do a long multiply with vd[i][0].	*/
		move.l	12(A0),D5	/*	Read item [1][j] into D5.			*/		
		muls.l	D1,D3:D5
		add.l	D5,D7		/*	Accumulate to D6:D7					*/
		
		move.l	28(A0),D5
		addx.l	D3,D6
		muls.l	D2,D3:D5
		add.l	D5,D7
		addx.l	D3,D6

		move.w	D6,D7
		swap	D7

		add.l	44(A0),D7
		
		move.l	D7,(A2)+	/*	Store into vs[i][j]					*/

		dbra	D4,@lastrownumbers

		move.l	(A1)+,(A2)+	/*	store result[3][3]					*/
@end:
		movem.l	(sp)+,D3-D7/A2-A4
	
	frfree
	rts
}