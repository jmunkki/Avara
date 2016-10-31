/*
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: PPCStuff.h
    Created: Thursday, February 23, 1995, 15:57
    Modified: Thursday, February 23, 1995, 15:58
*/

#pragma once
#include "PPCToolbox.h"

typedef struct	
{
	PPCInformPBRec	r;
	CDirectObject	*t;
} InformEnvelope;

typedef struct
{
	PPCReadPBRec	r;
	CDirectObject	*t;
} ReadEnvelope;

typedef struct
{
	PPCWritePBRec	r;
	CDirectObject	*t;
} WriteEnvelope;
