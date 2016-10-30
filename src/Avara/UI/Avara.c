/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: Avara.c
    Created: Sunday, November 13, 1994, 21:18
    Modified: Tuesday, May 30, 1995, 3:29
/*/

#include "CAvaraApp.h"

#if __option(profile)
#include "profile.h"
#else
int	errno;
#endif

void	main()
{
	CAvaraApp		*theEventHandler;
	short			i;
	long			theTime[2];

#if __option(profile)
	InitProfile(600, 9);
#endif
	DoInits();
	for(i=0;i<100;i++)
	{	MoreMasters();
	}

	AllocateQuickDirectStorage(320*1024L);
	StartExitHandler();

	if(!GetResource('AVAR',0))
	{	OpenResFile("\pAvara ¸.rsrc2");
//		StripPictComments(999);
//		ExitToShell();
	}

	if(!GetResource('AVAR',0))
	{	SysBeep(10);
	}
	else
	{
		InitMatrix();
		
		theEventHandler = new CAvaraApp;
		theEventHandler->IEventHandler();
				
		theEventHandler->EventLoop();
		theEventHandler->Dispose();
		
		FlushEvents(everyEvent, 0);
	}

	MemoryStatistics();
	TCPExitFunc(0);
}