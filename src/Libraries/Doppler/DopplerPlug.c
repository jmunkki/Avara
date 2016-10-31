/*
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: DopplerPlug.c
    Created: Friday, May 5, 1995, 16:21
    Modified: Saturday, May 20, 1995, 14:00
*/

#define DOPPLERPLUG

#include "DopplerPlug.h"
#include "PowerPlugs.h"

void	ClearMemoryAreaC(
register	long	*p,
register	long	memCount)
{
	do
	{	*p++ = 0;			*p++ = 0;
		*p++ = 0;			*p++ = 0;
		*p++ = 0;			*p++ = 0;
		*p++ = 0;			*p++ = 0;
	} while(--memCount);
}

/*
	The RateMixer function is used by sound channels that have a playing
	rate other than 1.0. The samples are stored at source. The copying
	ends when the buffer is full (outCount samples have been written) or
	we reach endOffset. The record pointed to by 'current' is updated at
	the end and the number of samples not yet written is returned.
*/

#ifdef THINK_C
short	RateMixer68K(
	register	Sample			*source,
	register	WordSample		*dest,
	register	WordSample		*converter,
	register	short			outCount,
				long			endOffset,
				SampleIndex		*current,
				Fixed			theRate)
{
	register	long			offset;
	register	short			fracOffset;
	register	long			rate;
	register	short			fracRate;

asm	{
		move.w	outCount,D0			;	D0 stores original outCount until the end.
		beq.s	@doNothing

		move.l	current,A1			;	A1 points to "current" record
		move.l	(A1)+,offset
		move.w	(A1)+,fracOffset

		move.l	endOffset,D1
		add.l	D1,source			;	change source to point to end
		sub.l	D1,offset			;	subtract EndOffset from current
		bpl.s	@doNothing

		subq.w	#1,outCount

		move.l	theRate,rate		;	Convert a fixed point variable to SampleIndex
		move.w	rate,fracRate		;	fractional part (16 bits)
		clr.w	rate				;	integer part high bits
		swap.w	rate				;	swap integer part into position

		clr.w	D2					;	Needs to be cleared for loop
		
	@loop
		move.b	0(source,offset.l),D2	;	Read a sample
		move.w	0(converter,D2.w*2),D1	;	Amplify sample with converter
		add.w	D1,(dest)+				;	Write to output

		add.w	fracRate, fracOffset	;	Add fractional rate
		addx.l	rate, offset			;	Add integer rate and carry from fractional
		dbcs	outCount,@loop			;	loop back, if we are not done yet

		moveq.l	#-1,D1					;	Adjust outCount after loop ends.
		subx.w	D1,outCount				;	Devious stuff.
		
		sub.w	outCount,D0				;	Subtract outCount (hopefully 0) from original outCount
	
		move.w	fracOffset,-(A1)		;	Adjust current record (A1 was set at beginning).
		add.l	endOffset,offset
		move.l	offset,-(A1)
	@doNothing
	}
}
#else
short	RateMixerC(
	register	Sample			*source,
	register	WordSample		*dest,
	register	WordSample		*converter,
	register	short			outCount,
				long			endOffset,
				SampleIndex		*current,
				Fixed			theRate)
{
	register	long			offset;
	register	long			fracOffset;
	register	long			rate;
	register	long			fracRate;
				long			i;

	if(outCount > 0)
	{	i = outCount;
		offset = current->i - endOffset;
		fracOffset = current->f;
		
		source += endOffset;

		if(offset < 0)
		{	rate = theRate;
			fracRate = (unsigned short) rate;
			rate >>= 16;
			
			do
			{	*(dest++) += converter[source[offset]];
				fracOffset = fracRate + (unsigned short) fracOffset;
				offset += (fracOffset >> 16) + rate;
			} while(--i && offset < 0);
			
			current->i = offset + endOffset;
			current->f = fracOffset;
		}
		
		outCount -= i;
	}
	
	return outCount;
}
#endif

void	InterleaveStereoChannelsC(
			WordSample	*leftCh,
			WordSample	*rightCh,
			WordSample	*blendTo,
			short		bufferSize)
{
	short	i = bufferSize;

	do
	{	*blendTo++ = *leftCh++;
		*blendTo++ = *rightCh++;
	} while(--i);
}