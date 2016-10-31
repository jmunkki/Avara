/*
    Copyright ©1993-1995, Juri Munkki
    All rights reserved.

    File: CStringDictionary.h
    Created: Sunday, July 25, 1993, 18:48
    Modified: Thursday, June 1, 1995, 1:23
*/

/*
**	This class maintains a sort of dictionary of words. The idea
**	is to have a fast way to find a dictionary word. A hashed
**	table is used to speed up the searches while still keeping
**	this class fairly simple.
**
**	NOTE:
**		While data is being added to the object, the
**		object can not be locked.
*/
#pragma once
#include "CBaseObject.h"

#define	DICTIONARYCLUMPSIZE		(16*sizeof(DictEntry))
#define	WORDCLUMPSIZE			256
#define HASHTABLESIZE			128	/* Must be a power of 2	*/

/*
**	The amount of storage required for a single item is
**	determined by the token type. A tokentype of "short"
**	will allow 32767 dictionary entries, which should be
**	sufficient for the kinds of data that this class was
**	written for.
*/
typedef	short	tokentype;

/*
**	Dictionary entries are stored into two separate handles.
**	The other one is the list of words and the other one
**	a list of hash table links and offsets to the words.
*/
typedef struct
{
	long	nameOffset;
	short	hashLink;
} DictEntry;

class	CStringDictionary : public CBaseObject
{
public:
			/*	Variables:			*/
			short		dictCount;				//	Amount of dictionary entries.
			long		logicalDictSize;		//	Used memory of dictionary handle.
			long		realDictSize;			//	Actual size of dictionary handle.
			DictEntry	**dictionary;			//	Handle to dictionary entries.
			
			long		logicalWordListSize;	//	Used memory of word list handle.
			long		realWordListSize;		//	Actual size of word list handle.
			unsigned char **wordList;			//	Word list.
				
			tokentype	hashTable[HASHTABLESIZE];	//	A hash table is used to accelerate lookups.

			/*	Methods:			*/	
	virtual	void		IStringDictionary();
	virtual	tokentype	AddDictEntry(unsigned char *entry, short len);
	virtual	tokentype	FindEntry(unsigned char *entry, short len);
	virtual	tokentype	SearchForEntry(unsigned char *entry, short len);
	virtual	void		ReadFromStringList(short strListID);
	
	virtual	short		GetDictionarySize();
	virtual	void		GetIndEntry(short index, StringPtr theEntry);
	
	virtual	void		Dispose();
	virtual	void		Lock();
	virtual	void		Unlock();


	virtual	short		GetIndEntrySize(short index);
	virtual	Handle		WriteToHandle();
	virtual	void		ReadFromHandle(Handle source);
};