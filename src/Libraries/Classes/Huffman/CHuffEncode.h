/*
    Copyright ©1994, Juri Munkki
    All rights reserved.

    File: CHuffEncode.h
    Created: Wednesday, September 21, 1994, 0:02
    Modified: Monday, October 3, 1994, 20:36
*/

#pragma once
#include "CAbstractHuffPipe.h"

/*
**	This data encodes its input using a form of adaptive Huffman compression.
*/

//	Size of output buffer in long ints.
#define	ENCODEBUFFERSIZE	128

class	CHuffEncode : public CAbstractHuffPipe
{
public:
	long		codeStrings[NUMSYMBOLS];
	short		codeLengths[NUMSYMBOLS];

	long		*outputBuffer;
	long		bitsInBuffer;
	
	virtual	OSErr	Open();
	virtual	void	ClearBuffer();
	virtual	OSErr	PipeData(Ptr dataPtr, long len);
	virtual	void	CreateSymbolTables();
	virtual	OSErr	Close();
	virtual	void	Dispose();
};