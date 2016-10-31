/*
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CLogicDistributor.c
    Created: Wednesday, November 22, 1995, 7:50
    Modified: Saturday, January 27, 1996, 12:03
*/

#include "CLogicDistributor.h"

CAbstractActor *
	CLogicDistributor::EndScript()
{
	short	i;

	if(inherited::EndScript())
	{	sendNext = 0;
		
		for(i=0;i<LOGIC_OUT;i++)
		{	if(out[i])	return this;
		}
	}
	
	Dispose();
	return NULL;
}

void	CLogicDistributor::FrameAction()
{
	short	i;
	short	theCount = 0;
	
	inherited::FrameAction();
	
	if(isActive & kHasMessage)
	{	isActive &= ~kHasMessage;

		for(i=0;i<LOGIC_IN;i++)
		{	if(in[i].messageId && enabled)
				theCount += in[i].triggerCount;
			in[i].triggerCount = 0;
		}
		
		while(theCount--)
		{	if(out[sendNext])
			{	itsGame->FlagMessage(out[sendNext]);
			}
			
			do
			{	sendNext++;
				if(sendNext == LOGIC_OUT)
				{	sendNext = 0;
					if(!restart)
					{	Dispose();
						return;
					}
				}
			} while(!out[sendNext]);
		}
	}
}