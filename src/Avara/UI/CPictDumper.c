/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.
/*/

#include "CPictDumper.h"

void	CPictDumper::IPictDumper(
	StringPtr	fileNameStart)
{
	numDumps = 0;
	BlockMoveData(fileNameStart, nameHeader, fileNameStart[0] + 1);
}

void	CPictDumper::TakeShot(
	WindowPtr	theWindow)
{
	if(numDumps < kMaxDumps)
	{	Rect	picRec;
		GrafPtr	saved;
		
		GetPort(&saved);
		SetPort(theWindow);
	
		picRec = theWindow->portRect;
		ClipRect(&picRec),
		allDumps[numDumps++] = OpenPicture(&picRec);
		CopyBits(&thePort->portBits, &thePort->portBits,
					&picRec, &picRec, srcCopy, NULL);
		ClosePicture();
		
		SetPort(saved);
	}
}

void	CPictDumper::CloseDumper(
	Boolean	doSave)
{
	if(doSave)
	{	short	i,j;
		Str255	fullName;
		short	baseLen;
		long	header[128];
		
		for(i=0;i<128;i++)
		{	header[i] = 0;
		}
		
		baseLen = nameHeader[0];
		BlockMoveData(nameHeader+1, fullName+1, baseLen);
		
		for(i=0;i < numDumps; i++)
		{	short	fRefNum;
			long	inOutCount;
			OSErr	iErr;
		
			NumToString(i+100, fullName + baseLen + 1);
			fullName[0] = baseLen + 1 + fullName[baseLen+1];
			fullName[baseLen+1] = ' ';
			iErr = Create(fullName, 0, '????', 'PICT');
			iErr = FSOpen(fullName, 0, &fRefNum);
			inOutCount = 512;
			iErr = FSWrite(fRefNum, &inOutCount, (Ptr)header);

			inOutCount = GetHandleSize((Handle)allDumps[i]);
			HLock((Handle)allDumps[i]);
			iErr = FSWrite(fRefNum, &inOutCount, (Ptr)*allDumps[i]);
			KillPicture(allDumps[i]);
			iErr = FSClose(fRefNum);
		}
	}
	else
	{	short	i;

		for(i=0;i < numDumps; i++)
			KillPicture(allDumps[i]);
	}
	
	Dispose();	
}