/*
    Copyright �1994-1995, Juri Munkki
    All rights reserved.

    File: LevelScoreRecord.h
    Created: Wednesday, November 30, 1994, 10:48
    Modified: Thursday, June 1, 1995, 2:14
*/

#pragma once

#define MAXENABLES	64

typedef	struct
{	long			score;
	long			time;
	unsigned long	when;
	short			by;
} GameResult;

typedef struct
{
	OSType		levelTag;
	OSType		directoryTag;
	GameResult	r;
} TaggedGameResult;

typedef struct
{

	Str63			name;	//	Level name.
	OSType			tag;	//	Level tag.
	short			orderNumber;
	Boolean			hasWon;	//	True, if level has been completed.
	Boolean			showResults;
	Boolean			isEnabled;

	GameResult	first;
	GameResult	fast;
	GameResult	high;	
} LevelScoreRecord;