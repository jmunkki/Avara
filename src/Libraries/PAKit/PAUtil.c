/*/
    Copyright ©1992, Juri Munkki
    All rights reserved.

    File: PAUtil.c
    Created: Sunday, December 6, 1992, 14:14
    Modified: Sunday, December 6, 1992, 15:37
/*/

#include "PAKit.h"

/*
>>	Fast version of SectRect.
>>	Look for use & help in IM-I (QuickDraw)
*/
short	QSectRect(
	Rect	*srca,
	Rect	*srcb,
	Rect	*dest)
{
asm	{	move.l	srca,A0		/*	Source rect address a in A0 */
		move.l	srcb,A1		/*	Source rect address b in A1	*/

		move.l	(A0)+,D0	/*	Load topleft from src a		*/
		move.l	(A1)+,D1	/*	Load topleft from src b		*/
		cmp.w	D1,D0		/*	Compare left coordinates	*/
		bge.s	@leftcomp
		move.w	D1,D0		/*	Choose higher value			*/
@leftcomp
		swap.w	D0
		swap.w	D1
		cmp.w	D1,D0		/*	Compare top coordinates		*/
		bge.s	@topcomp
		move.w	D1,D0		/*	Choose lower value			*/
@topcomp
		move.l	(A0)+,D1	/*	Load botright from src a	*/
		move.l	(A1)+,D2	/*	Load botright from src b	*/
		
		cmp.w	D1,D2		/*	Compare right coordinates	*/
		bge.s	@rightcomp
		move.w	D2,D1		/*	Choose lower value			*/
@rightcomp
		swap.w	D1
		swap.w	D2
		cmp.w	D1,D2		/*	Compare bott. coordinates	*/
		bge.s	@botcomp
		
		move.w	D2,D1
@botcomp
		move.l	dest,A0		/*	Destination rect in A0		*/
		cmp.w	D1,D0		/*	Does rect contain pixels?	*/
		bge.s	@badcase	/*	If not, make it {0,0,0,0}	*/
		swap.w	D0
		swap.w	D1
		cmp.w	D1,D0		/*	Does rect contain pixels?	*/
		bge.s	@badcase	/*	If not, make it {0,0,0,0}	*/
		move.l	D0,(A0)+
		move.l	D1,(A0)
		moveq.l	#-1,D0		/*	Yes, rectangles intersect	*/
		return
@badcase
		clr.l	(A0)+		/*	Empty rectangle				*/
		clr.l	(A0)
		moveq.l	#0,D0		/*	No intersection				*/
	}
}

void	LocalToGlobalRect(
	Rect	*r)
{
	Point	*p;
	
	p = (Point *)r;
	LocalToGlobal(p++);
	LocalToGlobal(p);
}