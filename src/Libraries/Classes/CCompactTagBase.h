/*/
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: CCompactTagBase.h
    Created: Thursday, June 1, 1995, 1:26
    Modified: Thursday, June 1, 1995, 1:26
/*/

#pragma once
#include "CTagBase.h"

class	CCompactTagBase : public CTagBase
{
public:
	virtual	Handle		ConvertToHandle();		//	Convert to something that can be stored as a resource.
	virtual	void		ConvertFromHandle(Handle theHandle);	//	Does the opposite of ConvertToHandle.
};