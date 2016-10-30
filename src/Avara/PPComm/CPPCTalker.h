/*
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CPPCTalker.h
    Created: Monday, December 19, 1994, 0:19
    Modified: Tuesday, February 21, 1995, 21:10
*/

#pragma once
#include "CDirectObject.h"
#include "PPCToolbox.h"

class	CPPCTalker;

typedef struct	
{
	PPCInformPBRec	r;
	CPPCTalker		*t;
} InformEnvelope;

typedef struct
{
	PPCReadPBRec	r;
	CPPCTalker		*t;
} ReadEnvelope;

typedef struct
{
	PPCWritePBRec	r;
	CPPCTalker		*t;
} WriteEnvelope;

#define	TALKERSTRINGS		1000
#define	PPCTALKERBUFFERSIZE	512
class	CPPCTalker : public CDirectObject
{
public:
			char			dataBuffer[PPCTALKERBUFFERSIZE];
			Boolean			dataArrived;

			short			connectCount;
			short			theRefNum;
			Boolean			nbpRegistered;
			Boolean			isServer;
			Str32			serverKind;
			InformEnvelope	informE;
			ReadEnvelope	readE;
			WriteEnvelope	writeE;

	virtual	OSErr	IPPCTalker();
	virtual	OSErr	Open();

	virtual	OSErr	Inform();
	virtual	void	InformComplete();

	virtual	OSErr	Browse();
	
	virtual	void	AsyncRead();
	virtual	OSErr	Write(long len, Ptr theData);

	virtual	OSErr	End();
	virtual	OSErr	Close();

};