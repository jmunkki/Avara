/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CLevelDescriptor.h
    Created: Wednesday, November 30, 1994, 8:10
    Modified: Tuesday, June 6, 1995, 9:13
/*/

#pragma once
#include "CDirectObject.h"
#include "LevelScoreRecord.h"

#define	LEVELLISTRESTYPE		'LEDI'
#define	LEVELLISTRESID			128

class	CTagBase;

class	CLevelDescriptor : public CDirectObject
{
public:
			short				orderNumber;
			OSType				tag;
			Str63				name;
			Str63				access;
			Str255				intro;
			Boolean				fromFile;

			short				enablesNeeded;
			short				enablesReceived;

			short				countEnables;
			OSType				winEnables[MAXENABLES];

	class	CLevelDescriptor	*nextLevel;
		
	virtual	void				ILevelDescriptor(Ptr levelInfo, short levelsLeft);
	virtual	void				UpdateLevelInfoTags(CTagBase *theTags, CLevelDescriptor *levelList);
	virtual	void				UpdateEnableCounts(CTagBase *theTags, CLevelDescriptor *levelList);
	virtual	void				IncreaseEnableCounts(short count, OSType *enableList);
	virtual	void				PrepareForLevelTagUpdate();

	virtual	void				FindDescriptor(OSType theTag, StringPtr description);
	virtual	void				FindLevelInfo(OSType theTag, LevelScoreRecord *newRecord);
	
	virtual	Handle				GetLevelData(OSType theTag, StringPtr levelName);

	virtual	void				Dispose();
};

CLevelDescriptor *LoadLevelListFromResource(OSType *currentDir);