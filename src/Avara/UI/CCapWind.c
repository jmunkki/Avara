/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CCapWind.c
    Created: Wednesday, November 16, 1994, 2:23
    Modified: Monday, June 12, 1995, 6:44
/*/

#include "CCapWind.h"
#include "CCapMaster.h"
#include "CommandList.h"
#include "CEventHandler.h"
#include "KeyFuncs.h"
#include "CCompactTagBase.h"
#include "CAvaraApp.h"
#include "Palettes.h"
#include "AvaraDefines.h"

void	CCapWind::IWindowCommander(
	CCommander			*theCommander,
	CWindowCommander	**theList)
{
	Rect			oldPlace;
	Boolean			doShow;
	PaletteHandle	pal;

	inherited::IWindowCommander(theCommander, theList);
	
	mapRes = GetResource(FUNMAPTYPE,FUNMAPID);
	theMaster = new CCapMaster;
	theMaster->ICapMaster(NULL, 128);

	gApplication->prefsBase->ReadOldHandle(kFunctionMapTag, mapRes);
	theMaster->SetCapMap(mapRes);
	theMaster->CreateWindow(FrontWindow(), false);

	itsWindow = theMaster->itsWindow;

	pal = GetNewPalette(128);
	SetPalette(itsWindow, pal, true);
	SetPort(itsWindow);
	PmBackColor(kAvaraBackColor);

	GetGlobalWindowRect(&oldPlace);
	gApplication->prefsBase->ReadRect(kKeyWindowRectTag, &oldPlace);

	//	Size can't be restored.
	oldPlace.right = oldPlace.left + itsWindow->portRect.right - itsWindow->portRect.left;
	oldPlace.bottom = oldPlace.top + itsWindow->portRect.bottom - itsWindow->portRect.top;
	
	doShow = gApplication->prefsBase->ReadShort(kKeyWindowVisTag, false);
	ReviveWindowRect(&oldPlace);
	if(doShow)	ShowWindow(itsWindow);

	AddToList();
}

void	CCapWind::Dispose()
{
	itsCommander->DoCommand(kKeyEditorClosed);

	Close();

	theMaster->itsWindow = NULL;
	UpdateKeyResource();
	theMaster->Dispose();
	
	inherited::Dispose();
}

void	CCapWind::Close()
{
	if(itsWindow)
	{	inherited::Close();
		theMaster->itsWindow = NULL;
	}
}

CloseRequestResult	CCapWind::CloseRequest(
	Boolean isQuitting)
{
	if(isQuitting)
	{	SaveIntoPreferences(kKeyWindowRectTag, kKeyWindowVisTag);
	}

	HideWindow(itsWindow);
	
	return kDidHide;
}

Boolean	CCapWind::DoEvent(
	CEventHandler	*master,
	EventRecord		*theEvent)
{
	if(theEvent->what == updateEvt && theEvent->message == (long) itsWindow)
	{	SetPort(itsWindow);
		PmBackColor(kAvaraBackColor);
	}

	if(!theMaster->DoEvent(theEvent))
	{	inherited::DoEvent(master, theEvent);
	}
}

void	CCapWind::ContentClick(
	EventRecord	*theEvent,
	Point		where)
{
	theMaster->DoEvent(theEvent);
}

void	CCapWind::UpdateKeyResource()
{
	theMaster->GetCapMap(mapRes);
	gApplication->prefsBase->WriteHandle(kFunctionMapTag, mapRes);
#if 0
	ChangedResource(mapRes);
	WriteResource(mapRes);
#endif
}

OSErr	CCapWind::OpenFile(
	FSSpec		*theFile)
{
	OSErr	theErr;
	short	refNum;
	long	fileLen;
	Handle	theMap;

	theErr = FSpOpenDF(theFile, fsRdPerm, &refNum);
	if(theErr == noErr)
	{	theErr = GetEOF(refNum, &fileLen);

		if(theErr == noErr)
		{	theMap = NewHandle(fileLen);
			if(theMap)
			{	HLock(theMap);
				theErr = FSRead(refNum, &fileLen, *theMap);
				
				if(theErr == noErr)
				{	theMaster->SetCapMap(theMap);
					UpdateKeyResource();
				}
				
				DisposeHandle(theMap);
			}
			else
			{	theErr = MemError();
			}
		}
		
		FSClose(refNum);
	}
	
	return theErr;
}


void	CCapWind::AdjustMenus(
	CEventHandler	*master)
{
	master->EnableCommand(kSaveAsCmd);
	master->EnableCommand(kSaveCmd);

	master->EnableCommand(kCopyCmd);
	master->EnableCommand(kPasteCmd);

	inherited::AdjustMenus(master);
}

OSErr	CCapWind::Save()
{
	StandardFileReply	theReply;
	Str63				thePrompt;
	Str63				theName;
	CCommander			*theActive;
	OSErr				iErr = noErr;
	Handle				theMap;
	FSSpec				itsFile;
	short				refNum;
	

	GetIndString(theName, KEYSTRINGLIST, 42);
	GetIndString(thePrompt, KEYSTRINGLIST, 43);

	theActive = gApplication->BeginDialog();
	StandardPutFile(thePrompt, theName, &theReply);

	if(theReply.sfGood)
	{	itsFile = theReply.sfFile;

		if(theReply.sfReplacing)
		{	iErr = FSpDelete(&itsFile);
		}
		
		if(iErr == noErr)
		{	iErr = FSpCreate(&itsFile, gApplicationSignature, KEYBOARDSETTINGSFILETYPE, theReply.sfScript);		
			
			if(iErr == noErr);
			{	iErr = FSpOpenDF(&itsFile, fsWrPerm, &refNum);
				if(iErr == noErr)
				{	long	len;
				
					theMap = NewHandle(0);
					theMaster->GetCapMap(theMap);
				
					len = GetHandleSize(theMap);
					HLock(theMap);
					iErr = FSWrite(refNum, &len, *theMap);
		
					DisposeHandle(theMap);
					FSClose(refNum);
					if(iErr)
						FSpDelete(&itsFile);

					FlushVol(NULL, itsFile.vRefNum);
					
				}
			}
		}
	}


	if(iErr)
		OSErrorNotify(iErr);

	gApplication->EndDialog(theActive);
	
	return iErr;
}

void	CCapWind::DoCommand(
	long			theCommand)
{
	CAvaraApp	*theApp;
	
	theApp = (CAvaraApp *)gApplication;
	
	switch(theCommand)
	{	case kSaveCmd:
		case kSaveAsCmd:
				Save();
				break;
		case kCopyCmd:
			{	Handle	theMap;
				Str255	theText;
			
				ZeroScrap();
				theMap = NewHandle(0);
				theMaster->GetCapMap(theMap);
				HLock(theMap);
				PutScrap(GetHandleSize(theMap), KEYBOARDSETTINGSFILETYPE, *theMap);
				GetIndString(theText, KEYSTRINGLIST, 42);
				PutScrap(theText[0], 'TEXT', (Ptr)theText + 1);
				DisposeHandle(theMap);
			}
			break;
		case kPasteCmd:
			{	Handle	theMap;
				long	dataCount;
				long	scrapOffset;
			
				theMap = NewHandle(0);
				dataCount = GetScrap(theMap, KEYBOARDSETTINGSFILETYPE, &scrapOffset);
				if(dataCount > 0)
				{	theMaster->SetCapMap(theMap);
					UpdateKeyResource();
				}
				DisposeHandle(theMap);
			}
			break;
		default:
			inherited::DoCommand(theCommand);
	}
}

void	CCapWind::DrawContents()
{
	inherited::DrawContents();
	
	theMaster->DrawAll();
}
