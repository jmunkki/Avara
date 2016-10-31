/*
*/

#pragma once
#include <CWindowCommander.h>

#define	kBusyResourceId		6000
#define	kBusyStackDepth		16

typedef struct
{
	short		stringList;
	short		stringIndex;
	Boolean		canStop;
} BusyMessage;

class	CBusyWindow : public CWindowCommander
{
public:
			short			busyLevel;
			BusyMessage		busyStack[kBusyStackDepth];
			ControlHandle	abortButton;
			Rect			textRect;
			long			firstTime;
			Boolean			isVisible;
			Boolean			abortVisible;
			WindowPtr		needsHilite;
		
	virtual	void			IWindowCommander(CCommander *theCommander,
											CWindowCommander **theList);
	virtual	void			DoCommand(long theCommand);

	virtual	void			ButtonVisibilityControl();

	virtual	void			DoBusyHide();
	virtual	void			PushBusyWindow(short listId, short index, Boolean canAbort);
	virtual	Boolean			IdleBusyWindow();
	virtual	void			PopBusyWindow();
	
	virtual	void			DrawContents();
};