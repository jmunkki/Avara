/*
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: SoundSystemDefines.h
    Created: Friday, December 23, 1994, 10:58
    Modified: Saturday, December 2, 1995, 1:37
*/

#pragma once
#define	HSOUNDRESTYPE		'HSND'
#define	BITSPERSAMPLE		7
#define	SAMPLERANGE			(1<<BITSPERSAMPLE)
#define	VOLUMEBITS			7
#define	VOLUMERANGE			(1<<VOLUMEBITS)

/*	When using 8 bit sound output hardware, very low volumes tend
**	to sound very bad, since the amount of bits that are played
**	is reduced so much. The following constant is the cut off volume
**	below which volumes are set to 0.
*/
#define	MINIMUM8BITVOLUME	2

/*
**	The following constant is the maximum allowed volume before stereo
**	calculations are made. This is to ensure that loud sounds playing
**	close to the observer do not max out on both channels.
*/
#define MAXADJUSTEDVOLUME	FIX3(1000)

#define BASESOUNDBUFFERBITS		10
#define	BASESOUNDBUFFERSIZE		(1<<BASESOUNDBUFFERBITS)
#define	SILENCE			128
#define	WORDSILENCE		0

typedef unsigned char	Sample;
typedef short			WordSample;

enum	{	metaNoData, metaNewData, metaKillNow, metaSuspend, metaFade	};

typedef struct
{
	long			i;
	unsigned short	f;
} SampleIndex;

#define	FIXSAMPLEINDEX(ind)	*(long *)(1+(short *)&ind)

typedef struct
{
	short	refCount;
	short	meta;

	Fixed	loc[3];
	Fixed	speed[3];

	Fixed	nLoc[3];
	Fixed	nSpeed[3];
	Fixed	t;

} SoundLink;

struct SampleHeader
{
	short				resId;
	short				refCount;
	long				len;
	long				loopStart;
	long				loopEnd;
	long				loopCount;
	Fixed				baseRate;
    struct SampleHeader **nextSample;
    short				flags;
};

enum	{	kOldSampleFlag = 1	};

typedef struct SampleHeader SampleHeader;
typedef SampleHeader *SampleHeaderPtr;
typedef SampleHeaderPtr *SampleHeaderHandle;

typedef WordSample SampleConvert[SAMPLERANGE];

typedef struct
{
	long	versNum;
	long	loopStart;
	long	loopEnd;
	long	loopCount;
	long	dataOffset;
	Fixed	baseRate;
}	HSNDRecord;