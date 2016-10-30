/*/
    Copyright ©1994, Juri Munkki
    All rights reserved.

    File: CAbstractHuffPipe.h
    Created: Tuesday, September 20, 1994, 18:05
    Modified: Monday, October 3, 1994, 20:35
/*/

#pragma once
#include "CAbstractPipe.h"

//	Number of symbols in the compressed data. Don't change this unless you know what you are doing.
#define	NUMSYMBOLS			256

//	Defines a condition for when the tree should change.
#define	CHANGEOFPATTERNS	(dataCount == 256 || (dataCount & 2047) == 0)

typedef struct
{
	long	count;
	short	left;	//	If negative, this is a leaf node and right is the value.
	short	right;
} HuffTreeNode;

class	CAbstractHuffPipe : public CAbstractPipe
{
public:
			long			dataCount;
			long			symbCounters[NUMSYMBOLS];
			HuffTreeNode	nodes[NUMSYMBOLS*2-1];
	
	virtual	OSErr			Open();
	virtual	void			SortNodes(HuffTreeNode **table);
	virtual	HuffTreeNode *	BuildTree();
	virtual	OSErr			PipeData(Ptr dataPtr, long len);
};