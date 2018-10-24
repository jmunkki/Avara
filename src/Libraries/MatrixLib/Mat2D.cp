/*/
    Copyright ©1992-1995, Juri Munkki
    All rights reserved.

    File: Mat2D.c
    Created: Thursday, December 3, 1992, 8:12
    Modified: Wednesday, May 3, 1995, 6:50
/*/

#include "FastMat.h"
#include "Mat2D.h"

/*
**	vd[i][j] = sum(k=0 to 2, vs[i][k] * m[k][j])
**	where	i=0 to n-1
**			j=0 to 3
**
**	This is effectively a vector to matrix multiplication
**	if n is 1 and a matrix with matrix multiplication when
**	n is 3.
**
**	m[0][2] = 0, m[1][2] = 0, m[2][2] = 1
*/
asm
void	Transform2D(
	long	n,
	V2D		*vs,
	V2D		*vd,
	M2D		*m)
{
	fralloc
	machine 68020
	
		movem.l	D3-D5/A2-A3,-(sp)
		subq.l	#1,n
		bmi		@end
		
		move.l	m,A0
		move.l	vs,A1
		move.l	vd,A2
		
@onevector:
		move.l	(A1)+,D0	/*	Load vector into registers D0-D1	*/
		move.l	(A1)+,D1
		
		moveq.l	#1,D2		/*	Loop counter for dbra 2 loops.		*/
		move.l	A0,A3		/*	Matrix into A3						*/
@onenumber:
		move.l	(A3)+,D3	/*	Read matrix item [0][j] into D3		*/
		muls.l	D0,D4:D3	/*	Do a long multiply with vd[i][0].	*/
		move.w	D4,D3		/*	Convert to							*/
		swap	D3			/*				fixed point.			*/
		
		move.l	4(A3),D4	/*	Read item [1][j] into D4.			*/
		muls.l	D1,D5:D4
		move.w	D5,D4
		swap	D4
		add.l	D4,D3		/*	Accumulate to D3					*/
		add.l	12(A3),D3	/*	Add item to [2][j] to total.		*/
		
		move.l	D3,(A2)+	/*	Store into vs[i][j]					*/
		
		dbra	D2,@onenumber
		
		subq.l	#1,n
		bpl		@onevector
		
@end:
		movem.l	(sp)+,D3-D5/A2-A3
	
	frfree
	rts
}

/*
**	Same as above, except operates on circles and does a fixed
**	point multiply to scale the radius.
*/
asm
void	TransformCircles(
	long		n,
	FixedCircle	*cs,
	FixedCircle	*cd,
	M2D			*m,
	Fixed		scaleFactor)
{
	fralloc
	machine 68020

		movem.l	D3-D6/A2-A3,-(sp)
		subq.l	#1,n
		bmi		@end
		
		move.l	m,A0
		move.l	cs,A1
		move.l	cd,A2
		move.l	scaleFactor,D6
		
@onevector:
		move.l	(A1)+,D0	/*	Load vector into registers D0-D1	*/
		move.l	(A1)+,D1
		
		moveq.l	#1,D2		/*	Loop counter for dbra 2 loops.		*/
		move.l	A0,A3		/*	Matrix into A3						*/
@onenumber:
		move.l	(A3)+,D3	/*	Read matrix item [0][j] into D3		*/
		muls.l	D0,D4:D3	/*	Do a long multiply with vd[i][0].	*/
		move.w	D4,D3		/*	Convert to							*/
		swap	D3			/*				fixed point.			*/
		
		move.l	4(A3),D4	/*	Read item [1][j] into D4.			*/
		muls.l	D1,D5:D4
		move.w	D5,D4
		swap	D4
		add.l	D4,D3		/*	Accumulate to D3					*/
		add.l	12(A3),D3	/*	Add item to [2][j] to total.		*/
		
		move.l	D3,(A2)+	/*	Store into vs[i][j]					*/

		dbra	D2,@onenumber
		
		move.l	(A1)+,D3	/*	Read radius							*/
		muls.l	D6,D4:D3	/*	Multiply by scaleFactor				*/
		move.w	D4,D3
		swap	D3			/*	Convert to fixed point from 64 bit.	*/
		move.l	D3,(A2)+	/*	Write radius						*/
		
		subq.l	#1,n
		bpl		@onevector
		
@end:
		movem.l	(sp)+,D3-D6/A2-A3

	frfree
	rts
}

asm
void	MatrixProduct2D(
	M2D		*vs,
	M2D		*vd,
	M2D		*m)
{
	fralloc
	machine 68020

		movem.l	D3-D6/A2-A3,-(sp)
		
		moveq.l	#1,D6		/*	Loop 2 times with DBRA.				*/
		move.l	m,A0
		move.l	vs,A1
		move.l	vd,A2
		
@onevector:
		move.l	(A1)+,D0	/*	Load matrix row into registers D0-D3	*/
		move.l	(A1)+,D1
		
		moveq.l	#1,D2		/*	Loop counter for dbra 2 loops.		*/
		move.l	A0,A3		/*	Matrix into A3						*/
@onenumber:
		move.l	(A3)+,D3	/*	Read matrix item [0][j] into D3		*/
		muls.l	D0,D4:D3	/*	Do a long multiply with vd[i][0].	*/
		move.w	D4,D3		/*	Convert to							*/
		swap	D3			/*				fixed point.			*/
		
		move.l	4(A3),D4	/*	Read item [1][j] into D4.			*/
		muls.l	D1,D5:D4
		move.w	D5,D4
		swap	D4
		add.l	D4,D3		/*	Accumulate to D3					*/		
		move.l	D3,(A2)+	/*	Store into vs[i][j]					*/

		dbra	D2,@onenumber
		
		dbra	D6,@onevector
		
		move.l	(A1)+,D0	/*	Load  last matrix row into registers D0-D3	*/
		move.l	(A1)+,D1
		
		moveq.l	#1,D2		/*	Loop counter for dbra 2 loops.		*/
		move.l	A0,A3		/*	Matrix into A3						*/
@onenumber_last_row:
		move.l	(A3)+,D3	/*	Read matrix item [0][j] into D3		*/
		muls.l	D0,D4:D3	/*	Do a long multiply with vd[i][0].	*/
		move.w	D4,D3		/*	Convert to							*/
		swap	D3			/*				fixed point.			*/
		
		move.l	4(A3),D4	/*	Read item [1][j] into D4.			*/
		muls.l	D1,D5:D4
		move.w	D5,D4
		swap	D4
		add.l	D4,D3		/*	Accumulate to D3					*/
		
		add.l	12(A3),D3
		move.l	D3,(A2)+	/*	Store into vs[i][j]					*/

		dbra	D2,@onenumber_last_row

		movem.l	(sp)+,D3-D6/A2-A3
	
	frfree
	rts
}

asm
Fixed	DistanceEstimate(
			Fixed	x1,
register	Fixed	y1,
			Fixed	x2,
			Fixed	y2)
{
	fralloc +

		move.l	x1,D0
		sub.l	x2,D0
		bpl.s	@posx
		neg.l	D0
@posx:
		sub.l	y2,y1
		bpl.s	@posy
		neg.l	y1
@posy:
		cmp.l	D0,y1
		bls.s	@noswap
		exg		D0,y1
@noswap:
		add.l	y1,D0
		lsr.l	#1,y1
		subx.l	y1,D0
		lsr.l	#2,y1
		sub.l	y1,D0
	
	frfree
	rts
}

#ifdef DEBUGMAT2D
#include "PAKit.h"
extern 	Boolean			hasColorQD;
extern	WindowPtr		myWindow;
extern	PolyWorld		myPolyWorld;

#define	NUMPTS	5
#define WINDOWRESID 129

V2D		shape[]	=	{	{  0, -50},
						{100, -50},
						{ 25,  25},
						{ 50,  50},
						{-50,  50}};

V2D		dest[NUMPTS];

void	ShowShape(
	V2D		*theShape,
	short	n,
	short	color)
{
	StartPolyLine(theShape[0][0] + 100, theShape[0][1] + 100, color);
	while(--n>0)
	{	AddPolyPoint(theShape[n][0] + 100, theShape[n][1] + 100);
	}
	EndPolyLine();
}

void	mainMat2D()
{
	M2D		trans,trans2,trans3;
	long	j;
	Fixed	myCos, mySin;
	long	timer;
	short	quitFlag = 0;
	GrafPtr	curPort;
	
	DoInits();
	ColorSetup();
	InitPolyGraphics();
	
	if(hasColorQD)		myWindow = GetNewCWindow(WINDOWRESID,0,(WindowPtr)-1);
		else			myWindow = GetNewWindow(WINDOWRESID,0,(WindowPtr)-1);

	SetPort(myWindow);
	InitPolyWorld(&myPolyWorld,myWindow, NULL,NULL, kShareNothing, NULL);
	SetPolyWorld(&myPolyWorld);

	GetPort(&curPort);

	timer = TickCount();
	do
	{
		for(j=0;j<600;j++)
		{	long	viewScale;
			long	i;
					
	#ifdef LARGEPOLY
			StartPolyLine(0,0,1);
			AddPolyPoint(160,50);
			AddPolyPoint(320,0);
			AddPolyPoint(270,100);
			AddPolyPoint(320,200);
			AddPolyPoint(160,150);
			AddPolyPoint(0,200);
			AddPolyPoint(50,100);
			EndPolyLine();
	#endif	
			viewScale = FIX3(2000) - (j<<9);
			trans[0][0] = viewScale;	trans[0][1] = 0;
			trans[1][0] = 0;			trans[1][1] = viewScale;
			trans[2][0] = 200;			trans[2][1] = 20;
	
			Transform2D(NUMPTS, shape, dest, &trans);
			ShowShape(dest, NUMPTS, 4);
	
			for(i=0;i<4;i++)
			{
				myCos = FDegCos((i*20+j*7)<<16);
				mySin = FDegSin((i*20+j*7)<<16);
		
				trans2[0][0] = myCos;		trans2[0][1] = mySin;
				trans2[1][0] = -mySin;		trans2[1][1] = myCos;
				trans2[2][0] = j+i*10+50;
		
				trans2[2][1] = FMul(FRadSin((i*3+j)<<13),80);
		
				MatrixProduct2D(&trans2,&trans3,&trans);
				Transform2D(NUMPTS, shape, dest, &trans3);
				ShowShape(dest, NUMPTS, 1 + (i & 3));
			}
			{	EventRecord	theEvent;
			
				PolygonizeVisRegion(curPort->visRgn);
				RenderPolygons();
	
				if(GetNextEvent(everyEvent, &theEvent))
				{	quitFlag = theEvent.what == keyDown;
				}
			}
	
			if(quitFlag) break;
		}
	}
	while(!quitFlag);
	
	timer = timer-TickCount();
	FlushEvents(everyEvent,0);	
}
#endif