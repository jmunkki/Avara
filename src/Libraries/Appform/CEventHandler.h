/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CEventHandler.h
    Created: Sunday, November 13, 1994, 21:59
    Modified: Monday, June 24, 1996, 3:00
/*/

#pragma once
#include "CCommander.h"

#define	MENUITEMTOLONG(menuid, itemNumber) ((((long)menuid)<<16)+itemNumber)

#define	kApplicationStringList	131
enum
{	kPreferencesFileNameString = 1,
	kFirstMemoryWarningString,
	kSecondMemoryWarningString,
	kLastEventHandlerString
};

class	CTagBase;
class	CCompactTagBase;
class	CWindowCommander;

class	CEventHandler : public CCommander
{
public:
			long				activeSleep;
			long				suspendSleep;
			
			long				commandParam[4];
			long				commandResult;

			EventRecord			theEvent;
			Boolean				quitFlag;
			Boolean				isSuspended;
			short				modelessLevel;

			FSSpec				appSpec;
			short				appResRef;

			CTagBase			*itsMenuToCmdBase;
			CTagBase			*itsCmdToMenuBase;
			long				lastMenuSelection;

			CWindowCommander	*itsWindowList;
			short				defaultSystemFont;
			short				defaultSystemFontSize;

			Ptr					firstReserve;
			Ptr					secondReserve;
			Boolean				warnUser;

			Boolean				needsNewDocument;

	//	Preferences:
			short				prefsFileRef;
			CCompactTagBase		*prefsBase;
			
			NMRec				myNotify;
			
			Str31				shortVersString;


	virtual	void		IEventHandler();
	virtual	void		AddOneMenu(MenuHandle theMenu, short beforeId);
	virtual	void		RegisterResMenu(short menuId, long itemCommand);
	virtual	void		AddMenus();
	virtual	void		InitApp();
	virtual	long		StringToCommand(StringPtr theString);
	
	virtual	void		EnableCommand(long theCommand);
	virtual	void		DisableCommand(long theCommand);
	virtual	void		CheckMarkCommand(long theCommand, Boolean flag);
	virtual	void		AdjustMenus(CEventHandler *master);
	virtual	void		PrepareMenus();
	virtual	void		AttemptQuit();
	virtual	void		DoCommand(long theCommand);
	virtual	void		BroadcastCommand(long theCommand);
	virtual	void		SetCommandParams(long param1, long param2, long param3, long param4);
	virtual	long		GetCommandResult();
	
	virtual	void		GetLastMenuString(StringPtr dest);
	virtual	void		DoMenuCommand(long cmdTag);
	virtual	void		DoMouseDown();
	
	virtual	void		HandleOneEvent(short eventMask);
	virtual	void		UnqueueMaskedEvents(short eventMask);
	virtual	void		EventLoop();
	virtual	void		Dispose();
	
	virtual	void		GotEvent();
	virtual	void		Idle();
	virtual	void		ExternalEvent(EventRecord *anEvent);
	virtual	void		DispatchEvent();

	virtual	OSErr		DoAppleEvent(AppleEvent *theEvent, AppleEvent *theReply);

	virtual	OSErr		OpenFSSpec(FSSpec *theFile, Boolean notify);
	virtual	Boolean		AvoidReopen(FSSpec *theFile, Boolean doSelect);
	
	virtual	CCommander *BeginDialog();
	virtual	void		EndDialog(CCommander *theOldActive);

	//	Preferences:
	virtual	void		LoadPreferences();
	virtual	void		SavePreferences();
	virtual	short		ReadShortPref(OSType tag, short defaultValue);
	virtual	void		WriteShortPref(OSType tag, short value);

	virtual	long		ReadLongPref(OSType tag, long defaultValue);
	virtual	void		WriteLongPref(OSType tag, long value);

	virtual	void		ReadStringPref(OSType tag, StringPtr dest);
	virtual	void		WriteStringPref(OSType tag, StringPtr value);

	//	Misc utility routines for whole application:
	virtual	void		Geneva9SystemFont();
	virtual	void		StandardSystemFont();
	
	virtual	void		NotifyUser();
	
	virtual	long		DoGrowZone(Size bytesNeeded);
	virtual	void		InstallGrowZone(long firstReserveSize, long secondReserveSize);
};

#ifdef EVENTHANDLERMAIN
		OSType			gApplicationSignature = '????';
		OSType			gApplicationPreferencesType = 'PREF';
		CEventHandler	*gApplication;
		Cursor			gIBeamCursor;
		Cursor			gWatchCursor;
#else
extern	OSType			gApplicationSignature;
extern	OSType			gApplicationPreferencesType;
extern	CEventHandler	*gApplication;
extern	Cursor			gIBeamCursor;
extern	Cursor			gWatchCursor;
#endif