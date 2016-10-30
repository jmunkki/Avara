/*
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CAvaraApp.c
    Created: Wednesday, November 16, 1994, 1:26
    Modified: Monday, August 21, 1995, 4:26
*/

#define	MAINAVARAAPP
#include "CAvaraApp.h"
#include "CAvaraGame.h"
#include "CGameWind.h"
#include "CRosterWindow.h"
#include "CInfoPanel.h"
#include "CCapWind.h"
#include "CommandList.h"
#include "LevelLoader.h"
#include "PAKit.h"
#include "CLevelDescriptor.h"
#include "CLevelListWind.h"
#include "CSoundMixer.h"
#include "CRC.h"
#include "CCompactTagBase.h"
#include "Aliases.h"
#include "KeyFuncs.h"
#include "CNetManager.h"
#include "Ambrosia_Reg.h"
#include "AvaraScoreInterface.h"
#include "CAsyncBeeper.h"
#include "Editions.h"
#include "CBusyWindow.h"

#define	LEVELDIRECTORYFILETYPE	'AVAL'

#define	LEVELLOADSTRINGS		136
enum	{	kDefaultLevelDirectory = 1,
			kBusyCatSearching,
			kBusyLoadingLevel
		};


#define	NETHIERMENU				139
#define	RETRANSMITHIERMENU		140
#define	TCPNETTYPEHIERMENU		141
#define	BEEPARRIVALMENU			142
#define	BELLMENU				143

void	CAvaraApp::InitApp()
{
	CCapWind			*cap;
	DialogPtr			splashDialog;
	Boolean				doSplash;
	long				gestaltReply;
	MenuHandle			beepMenu;
	unsigned long		longDate;
	
	gApplicationSignature = 'AVAR';
	gApplicationPreferencesType = 'AVAP';

	inherited::InitApp();
	
	GetDateTime(&longDate);
	WriteLongPref(kFirstPrefsCreate, ReadLongPref(kFirstPrefsCreate, longDate));
	
	Gestalt(gestaltSysArchitecture, &gestaltReply);
	if(gestaltPowerPC & gestaltReply)
	{	fastMachine = true;
	}
	else
	{	Gestalt(gestaltProcessorType, &gestaltReply);
		fastMachine = gestaltReply > gestalt68030;
	}
	
	doSplash = (NULL != Get1Resource('CODE', 0));

	if(doSplash)
	{	
		RegistrationCheck(510);
		splashDialog = (DialogPtr)GetNewDialog(128, 0, (WindowPtr)-1);
		DrawDialog(splashDialog);
	}

	InstallGrowZone(	128 * 1024L,	256 * 1024L);
	
	if(noErr == Gestalt(gestaltEditionMgrAttr, &gestaltReply))
	{	InitEditionPack();
	}

	gAsyncBeeper = new CAsyncBeeper;
	gAsyncBeeper->IAsyncBeeper();

	AddOneMenu(GetMenu(NETHIERMENU), hierMenu);
	AddOneMenu(GetMenu(RETRANSMITHIERMENU), hierMenu);
	AddOneMenu(GetMenu(TCPNETTYPEHIERMENU), hierMenu);

	beepMenu = GetMenu(BEEPARRIVALMENU);
	AddResMenu(beepMenu, 'snd ');
	AddOneMenu(beepMenu, hierMenu);
	RegisterResMenu(BEEPARRIVALMENU, kBellArrivalsCmd);

	beepMenu = GetMenu(BELLMENU);
	AddResMenu(beepMenu, 'snd ');
	AddOneMenu(beepMenu, hierMenu);
	RegisterResMenu(BELLMENU, kBellSelectCmd);

	InitDopplerSoundSystem();

	levelList = LoadLevelListFromResource(&currentLevelDirectory);
	directoryRef = appResRef;
	loadedDirectoryRef = appResRef;

	{	Str63		levelsDirName;
		DirInfo		info;
		OSErr		err;
	
		GetIndString(levelsDirName, LEVELLOADSTRINGS, kDefaultLevelDirectory);
		
		info.ioCompletion = 0;
		info.ioNamePtr = levelsDirName;
		info.ioVRefNum = 0;
		info.ioFDirIndex = 0;
		info.ioDrDirID = 0;
		err = PBGetCatInfo((CInfoPBPtr) &info, false);
		if((err == noErr) && (info.ioFlAttrib & 16))
		{	directorySpec.vRefNum = info.ioVRefNum;
			defaultLevelDirectoryDirID = info.ioDrDirID;
			directorySpec.parID = defaultLevelDirectoryDirID;
		}
		else
		{	directorySpec.vRefNum = 0;
			defaultLevelDirectoryDirID = 0;
			directorySpec.parID = 0;
		}
	}
	InitPolyColorTable(&myPolyColorTable);
	SetPolyColorTable(&myPolyColorTable);
	FindPolyColor(0xFFFFFF);
	FindPolyColor(0x000000);
	InitPolyGraphics();
	
	busyWindow = new CBusyWindow;
	busyWindow->IWindowCommander((CCommander *)this, &itsWindowList);
	
	theInfoPanel = NULL;

	theGameWind = new CGameWind;
	theGameWind->IWindowCommander((CCommander *)this, &itsWindowList);
	
	theRosterWind = new CRosterWindow;
	theRosterWind->IWindowCommander((CCommander *)this, &itsWindowList);

	itsGame = new CAvaraGame;
	gCurrentGame = itsGame;
	itsGame->IAvaraGame(theGameWind->itsWindow, this);	

	theInfoPanel = new CInfoPanel;
	itsGame->infoPanel = theInfoPanel;	
	theInfoPanel->IInfoPanel((CCommander *)this, &itsWindowList, &itsGame->itsPolyWorld);

	theGameWind->PositionWindow();

	capEditor = NULL;
	if(ReadShortPref(kKeyWindowVisTag, false))
	{	capEditor = new CCapWind;
		capEditor->IWindowCommander((CCommander *)this, &itsWindowList);
	}
	else
	{	prefsBase->ReadOldHandle(kFunctionMapTag, GetResource(FUNMAPTYPE,FUNMAPID));
	}
	
	networkOptions = ReadShortPref(kNetworkOptionsTag, 2);
	
	if(doSplash)
		DisposeDialog(splashDialog);
	
	if(noErr == Gestalt(gestaltMacTCP, &gestaltReply))
	{	macTCPAvailable = true;
	}
	else
	{	macTCPAvailable = false;
		if(networkOptions == 3)
		{	networkOptions = 2;
			WriteShortPref(kNetworkOptionsTag, 2);
		}
	}
}

#define	SECONDSPLASH	true

void	CAvaraApp::DoAbout()
{
	CCommander			*savedCommander;
	EventRecord		anEvent;
	DialogPtr			splashDialog;
#if SECONDSPLASH
	DialogPtr			splashDialog2;
#endif

	savedCommander = BeginDialog();

	splashDialog = GetNewDialog(128, 0, (WindowPtr)-1);
#if SECONDSPLASH
	splashDialog2 = GetNewDialog(131, 0, splashDialog);
#endif
	DrawDialog(splashDialog);
	
	do
	{	GetOSEvent(keyDownMask+mDownMask, &anEvent);
	} while(!anEvent.what);
	
	DisposeDialog(splashDialog);

#if SECONDSPLASH
	DrawDialog(splashDialog2);
	FlushEvents(everyEvent, 0);
	
	do
	{	GetOSEvent(keyDownMask+mDownMask, &anEvent);
	} while(!anEvent.what);
	
	DisposeDialog(splashDialog2);
#endif
	FlushEvents(everyEvent, 0);

	EndDialog(savedCommander);
}

void	CAvaraApp::UpdateLevelInfoTags(
	CTagBase *theTags)
{
	levelList->PrepareForLevelTagUpdate();
	levelList->UpdateEnableCounts(theTags, levelList);
	levelList->UpdateLevelInfoTags(theTags, levelList);
}

void	CAvaraApp::Dispose()
{
	itsGame->Dispose();
	theInfoPanel->Dispose();
	theGameWind->Dispose();
	theRosterWind->Dispose();
	levelList->Dispose();
	DeallocParser();
	
	if(gAsyncBeeper)
		gAsyncBeeper->Dispose();
	
	inherited::Dispose();
}

OSErr	CAvaraApp::OpenDirectoryFile(
	FSSpec	*theFile)
{
	short				newDirectoryRef;
	CLevelDescriptor	*newLevels;
	OSType				newLevelsTag;
	OSErr				theErr;

	UseResFile(appResRef);

	newDirectoryRef = FSpOpenResFile(theFile, fsRdPerm);
	theErr = ResError();
	if(theErr == noErr)
	{	newLevels = LoadLevelListFromResource(&newLevelsTag);
		if(newLevels)
		{	levelList->Dispose();
			levelList = newLevels;
			currentLevelDirectory = newLevelsTag;
			
			if(loadedDirectoryRef != directoryRef &&
				directoryRef != appResRef)
			{	CloseResFile(directoryRef);
			}

			directorySpec = *theFile;
			directoryRef = newDirectoryRef;
			BroadcastCommand(kLevelDirectoryChangedCmd);
		}
		else
		{	CloseResFile(newDirectoryRef);
		}
	}

	UseResFile(appResRef);

	return theErr;
}

OSErr	CAvaraApp::OpenFSSpec(
	FSSpec	*theFile,
	Boolean	notify)
{
	CLevelListWind	*newWind;
	FInfo			theInfo;
	OSErr			theErr = noErr;

	if(!AvoidReopen(theFile, true))
	{	theErr = FSpGetFInfo(theFile, &theInfo);

		if(theErr == noErr)
		switch(theInfo.fdType)
		{	case LEVELLISTFILETYPE:
				newWind = new CLevelListWind;
				newWind->IWindowCommander((CCommander *)this, &itsWindowList);
			
				theErr = newWind->OpenFile(theFile);
				if(theErr)
				{	newWind->Dispose();
					if(notify) OSErrorNotify(theErr);
				}
				else
				{	newWind->Select();
					needsNewDocument = false;
				}
				break;
			case LEVELDIRECTORYFILETYPE:
				theErr = OpenDirectoryFile(theFile);
				if(notify && theErr)
				{	OSErrorNotify(theErr);
				}
				break;
			case KEYBOARDSETTINGSFILETYPE:
				if(!capEditor)
				{	capEditor = new CCapWind;
					capEditor->IWindowCommander((CCommander *)this, &itsWindowList);
				}
				theErr = capEditor->OpenFile(theFile);
				if(notify && theErr)
				{	OSErrorNotify(theErr);
				}
				break;
		}
	}

	return theErr;
}
void	CAvaraApp::OpenStandardFile()
{
	StandardFileReply	theReply;
	OSType				typeList[] = { LEVELLISTFILETYPE, LEVELDIRECTORYFILETYPE, KEYBOARDSETTINGSFILETYPE, kScoreInterfaceFileType };
	CCommander			*savedCommander;

	savedCommander = BeginDialog();
	StandardGetFile(NULL, 4, typeList, &theReply);
	EndDialog(savedCommander);
	
	if(theReply.sfGood)
	{	OpenFSSpec(&theReply.sfFile, true);
	}
}

void	CAvaraApp::DoCommand(
	long	theCommand)
{
	switch(theCommand)
	{	case kAboutCmd:
			DoAbout();
			break;
		case kShowKeyEditorCmd:
			if(capEditor == NULL)
			{	capEditor = new CCapWind;
				capEditor->IWindowCommander((CCommander *)this, &itsWindowList);
			}
			
			capEditor->Select();
			break;
		case kChatAndPlayCmd:
			WriteShortPref(kMouseJoystickTag,
				ReadShortPref(kMouseJoystickTag, 0) ^ kSimulChatMode);
			break;
		case kShowGameWind:
			theGameWind->Select();
			break;
		case kShowRosterWind:
			theRosterWind->Select();
			break;
		case kShowInfoPanel:
			theInfoPanel->Select();
			break;
		
		case kOpenApplicationCmd:
			DoOpenApplication();
			break;
		case kNewCmd:
			{	CLevelListWind	*newDoc;
			
				newDoc = new CLevelListWind;
				newDoc->IWindowCommander((CCommander *)this, &itsWindowList);
				newDoc->Select();
			}
			break;
		case kOpenCmd:
			OpenStandardFile();
			break;

		case kKeyEditorClosed:
			capEditor = NULL;
			break;

		case kStartGame:
		case kResumeGame:
			SavePreferences();
			itsGame->SendStartCommand();
			break;
		
		case kGetReadyToStartCmd:
			if(capEditor)
				capEditor->UpdateKeyResource();

			theInfoPanel->Select();
			SendBehind(theGameWind->itsWindow, theInfoPanel->itsWindow);
			ShowWindow(theGameWind->itsWindow);
			UnqueueMaskedEvents(activMask+updateMask);
			theInfoPanel->DoUpdate();
			theGameWind->DoUpdate();
			break;
			
		case kUseDefaultDirectory:
			if(directoryRef != appResRef &&
				directoryRef != loadedDirectoryRef)
			{	CloseResFile(directoryRef);
			}
			
			UseResFile(appResRef);
			directoryRef = appResRef;
			directorySpec.vRefNum = 0;
			directorySpec.name[0] = 0;
			directorySpec.parID = defaultLevelDirectoryDirID;
			levelList = LoadLevelListFromResource(&currentLevelDirectory);
			BroadcastCommand(kLevelDirectoryChangedCmd);
			break;
		case kAutoSaveScores:
			WriteShortPref(kAutoSaveScoresTag,
				!ReadShortPref(kAutoSaveScoresTag, true));
			break;
		case kAllowBackgroundProcessCmd:
			WriteShortPref(kBackgroundProcessTag,
				!ReadShortPref(kBackgroundProcessTag, false));
			break;

		case kMuteSoundCmd:
		case kMonoSoundCmd:
		case kStereoHeadphonesCmd:
		case kStereoSpeakersCmd:		
		case k16BitMixCmd:
		case kAmbientToggleCmd:
		case kTuijaToggleCmd:
		case kSoundInterpolateCmd:
		case kMissileSoundLoopCmd:
		case kFootStepSoundCmd:
			itsGame->GameOptionCommand(theCommand);
			break;
		case kJoystickModeCmd:
			WriteShortPref(kMouseJoystickTag,
				ReadShortPref(kMouseJoystickTag, 0) ^ kJoystickMode);
			break;

		case kFlipAxisCmd:
			WriteShortPref(kMouseJoystickTag,
				ReadShortPref(kMouseJoystickTag, 0) ^ kFlipAxis);
			break;

		case kLowSensitivityCmd:
		case kNormalSensitivityCmd:
		case kHighSensitivityCmd:
			WriteShortPref(
				kMouseSensitivityTag,
				theCommand - kNormalSensitivityCmd);
			break;
			
		case kPPCToolboxNetCmd:
		case kDDPNetCmd:
		case kDDPBroadcastNetCmd:
		case kMacTCPNetCmd:
			networkOptions = theCommand - kPPCToolboxNetCmd;
			WriteShortPref(kNetworkOptionsTag, networkOptions);
			break;
		case kShowLevelListWind:
			if(bottomLevelListWindow)
			{	bottomLevelListWindow->Select();
			}
			break;
		case kSimpleHorizonCmd:
		case kLowDetailHorizonCmd:
		case kHighDetailHorizonCmd:
			WriteShortPref(kHorizonDetailTag, theCommand - kSimpleHorizonCmd);
			itsGame->GameOptionCommand(theCommand);
			break;
		case kAmbientHologramsCmd:
			WriteShortPref(kAmbientHologramsTag,
						!ReadShortPref(kAmbientHologramsTag, true));

			itsGame->AmbientHologramControl();
			SetPort(theGameWind->itsWindow);
			InvalRect(&theGameWind->itsWindow->portRect);
			break;
		case kServerOptionsDialogCmd:
			ServerOptionsDialog();
			break;
		case kBellArrivalsCmd:
			{	Str255		lastMenuString;
			
				GetLastMenuString(lastMenuString);
				WriteStringPref(kBellNewArrivalsTag, lastMenuString);
				gAsyncBeeper->PlayNamedBeep(lastMenuString);
			}
			break;

		case kBellSelectCmd:
			{	Str255		lastMenuString;
			
				GetLastMenuString(lastMenuString);
				WriteStringPref(kBellSoundTag, lastMenuString);
				gAsyncBeeper->PlayNamedBeep(lastMenuString);
			}
			break;

		case kAutoLatencyCmd:
			{	short	curOptions;
				
				curOptions = ReadShortPref(kServerOptionsTag, kDefaultServerOptions);
				curOptions ^= 1 << kUseAutoLatencyBit;
				WriteShortPref(kServerOptionsTag, curOptions);
				gameNet->ChangedServerOptions(curOptions);
			}
			break;
	
		case kReconfigureNetCmd:
			gameNet->itsCommManager->Reconfigure();
			break;
		case kGameOptionsDialogCmd:
			GameOptionsDialog();
			break;
		case kSoundOptionsDialogCmd:
			SoundOptionsDialog();
			break;

		default:
			if(	theCommand >= kLatencyToleranceZero &&
				theCommand <= kLatencyToleranceMax)
			{	WriteShortPref(kLatencyToleranceTag, theCommand - kLatencyToleranceZero);
			}
			else
			if(	theCommand >= kRetransmitMin &&
				theCommand <= kRetransmitMax)
			{	WriteShortPref(kUDPResendPrefTag, theCommand - kRetransmitMin);

				if(gameNet->itsCommManager)
				{	gameNet->itsCommManager->OptionCommand(theCommand);
				}
			}
			if(	theCommand >= kSlowestConnectionCmd && theCommand <= kFastestConnectionCmd)
			{	WriteShortPref(kUDPConnectionSpeedTag, theCommand - kSlowestConnectionCmd);
				
				if(gameNet->itsCommManager)
				{	gameNet->itsCommManager->OptionCommand(theCommand);
				}				
			}
			else
				inherited::DoCommand(theCommand);
			break;
	}
}

void	CAvaraApp::AdjustBellMenu()
{
	MenuHandle	bellMenu;
	short		i, j, iCount;
	Str255		selectedSound;
	Str255		itemString;
	
	ReadStringPref(kBellNewArrivalsTag, selectedSound);

	bellMenu = GetMHandle(BEEPARRIVALMENU);
	iCount = CountMItems(bellMenu);
	EnableCommand(kBellArrivalsCmd);

	for(i = iCount; i > 1; i--)
	{	GetItem(bellMenu, i, itemString);
		if(itemString[0] == selectedSound[0])
		{	for(j = selectedSound[0]; j; j--)
			{	if(itemString[j] != selectedSound[j])
				{	break;
				}
			}
			
			if(j == 0)
			{	break;
			}
		}
	}

	CheckItem(bellMenu, i, true);

	ReadStringPref(kBellSoundTag, selectedSound);

	bellMenu = GetMHandle(BELLMENU);
	iCount = CountMItems(bellMenu);
	EnableCommand(kBellSelectCmd);

	for(i = iCount; i > 1; i--)
	{	GetItem(bellMenu, i, itemString);
		if(itemString[0] == selectedSound[0])
		{	for(j = selectedSound[0]; j; j--)
			{	if(itemString[j] != selectedSound[j])
				{	break;
				}
			}
			
			if(j == 0)
			{	break;
			}
		}
	}

	CheckItem(bellMenu, i, true);
}

void	CAvaraApp::AdjustMenus(
	CEventHandler	*master)
{
	WindowPtr	front = FrontWindow();
	short		mouseOptions;
	short		horizonDetail;
	short		i, j, toler;
	short		connType;
	short		slot;
	Boolean		permission;
	
	slot = gameNet->itsCommManager->myId;
	if(slot == 0)
	{	EnableCommand(kAutoLatencyCmd);
		CheckMarkCommand(kAutoLatencyCmd, (1<<kUseAutoLatencyBit) & ReadShortPref(kServerOptionsTag, kDefaultServerOptions));
		permission = true;
	}
	else
	{	permission = gameNet->PermissionQuery(kAllowLatencyBit, 0);
		if(gameNet->PermissionQuery(kUseAutoLatencyBit, 0))
			CheckMarkCommand(kAutoLatencyCmd, true);
	}

	toler = ReadShortPref(kLatencyToleranceTag, kDefaultLatencyTolerance);

	for(i=0;i <= kLatencyToleranceMax - kLatencyToleranceZero;i++)
	{	if(permission)
			EnableCommand(i+kLatencyToleranceZero);
		if(i==toler)
			CheckMarkCommand(i+kLatencyToleranceZero, true);
	}

	EnableCommand(kServerOptionsDialogCmd);

	if(gameNet->itsCommManager->ReconfigureAvailable())
	{	EnableCommand(kReconfigureNetCmd);
	}

	connType = ReadShortPref(kUDPConnectionSpeedTag, kDefaultUDPSpeed);
	for(i=0;i <= kFastestConnectionCmd - kSlowestConnectionCmd;i++)
	{	EnableCommand(i+kSlowestConnectionCmd);
		if(i==connType)
			CheckMarkCommand(i+kSlowestConnectionCmd, true);
	}

	horizonDetail = ReadShortPref(kHorizonDetailTag, fastMachine ? 2 : 0);
	EnableCommand(kSimpleHorizonCmd);
	EnableCommand(kLowDetailHorizonCmd);
	EnableCommand(kHighDetailHorizonCmd);
	CheckMarkCommand(kSimpleHorizonCmd + horizonDetail, true);

	EnableCommand(kAmbientHologramsCmd);
	if(ReadShortPref(kAmbientHologramsTag, true))
		CheckMarkCommand(kAmbientHologramsCmd, true);


	EnableCommand(kNewCmd);
	EnableCommand(kShowKeyEditorCmd);
	CheckMarkCommand(kShowKeyEditorCmd, capEditor->itsWindow == front);

	EnableCommand(kShowGameWind);
	CheckMarkCommand(kShowGameWind, theGameWind->itsWindow == front);
	
	EnableCommand(kShowRosterWind);
	CheckMarkCommand(kShowRosterWind, theRosterWind->itsWindow == front);

	EnableCommand(kShowInfoPanel);
	CheckMarkCommand(kShowInfoPanel, theInfoPanel->itsWindow == front);

	EnableCommand(kOpenCmd);
	EnableCommand(kAboutCmd);

	EnableCommand(1000);

	EnableCommand(1014);

	EnableCommand(kMuteSoundCmd);
	EnableCommand(kMonoSoundCmd);
	EnableCommand(kStereoHeadphonesCmd);
	EnableCommand(kStereoSpeakersCmd);
	CheckMarkCommand(kMonoSoundCmd + itsGame->soundOutputStyle, true);
	
	EnableCommand(k16BitMixCmd);
	CheckMarkCommand(k16BitMixCmd, itsGame->sound16BitStyle);
	
	EnableCommand(kAmbientToggleCmd);
	EnableCommand(kTuijaToggleCmd);

	j = 1;
	for(i=kAmbientToggleCmd;i<=kFootStepSoundCmd;i++)
	{	if(j & itsGame->soundSwitches)
		{	CheckMarkCommand(i, true);
		}
		EnableCommand(i);
		j += j;
	}

	EnableCommand(kPPCToolboxNetCmd);
	EnableCommand(kDDPNetCmd);
	EnableCommand(kDDPBroadcastNetCmd);

	if(macTCPAvailable)
		EnableCommand(kMacTCPNetCmd);
	CheckMarkCommand(networkOptions + kPPCToolboxNetCmd, true);

	if(directoryRef != appResRef)
	{	EnableCommand(kUseDefaultDirectory);
	}
	CheckMarkCommand(kUseDefaultDirectory, directoryRef == appResRef);

	EnableCommand(kAutoSaveScores);
	CheckMarkCommand(kAutoSaveScores, ReadShortPref(kAutoSaveScoresTag, true));
	EnableCommand(kAllowBackgroundProcessCmd);
	CheckMarkCommand(kAllowBackgroundProcessCmd, 
						ReadShortPref(kBackgroundProcessTag, false));

	if(gameNet->PermissionQuery(kAllowResumeBit, 0))
	{	if(itsGame->gameStatus == kPauseStatus)
		{	if(gameNet->ResumeEnabled())
			{	EnableCommand(kResumeGame);
			}
		}
		else
		if(itsGame->gameStatus == kReadyStatus)
		{	EnableCommand(kStartGame);
		}
	}

	bottomLevelListWindow = NULL;
	BroadcastCommand(kPrepareShowLevelListWind);
	if(bottomLevelListWindow)
	{	EnableCommand(kShowLevelListWind);
		CheckMarkCommand(kShowLevelListWind,
						bottomLevelListWindow->itsWindow == front);
	}
	else
	{	CheckMarkCommand(kShowLevelListWind, false);
	}

	mouseOptions = ReadShortPref(kMouseJoystickTag, 0);

	EnableCommand(kJoystickModeCmd);
	CheckMarkCommand(kJoystickModeCmd, mouseOptions & kJoystickMode);
	EnableCommand(kFlipAxisCmd);
	CheckMarkCommand(kFlipAxisCmd, mouseOptions & kFlipAxis);

	EnableCommand(kGameOptionsDialogCmd);
	EnableCommand(kSoundOptionsDialogCmd);

	EnableCommand(kLowSensitivityCmd);
	EnableCommand(kNormalSensitivityCmd);
	EnableCommand(kHighSensitivityCmd);
	CheckMarkCommand(kNormalSensitivityCmd +
		ReadShortPref(kMouseSensitivityTag, 0), true);

	CheckMarkCommand(ReadShortPref(kUDPResendPrefTag, 2) + kRetransmitMin, true);
	for(i = kRetransmitMin; i <= kRetransmitMax; i++)
	{	EnableCommand(i);
	}

	AdjustBellMenu();

	inherited::AdjustMenus(master);
}

void	CAvaraApp::GotEvent()
{
	inherited::GotEvent();
	
	itsGame->GotEvent();
	theRosterWind->GotEvent();
}

void	CAvaraApp::GetDirectoryLocator(
	DirectoryLocator	*theDir)
{
	HFileParam	theInfo;
	OSErr		iErr;

	theDir->dirTag = currentLevelDirectory;

	if(directoryRef == appResRef)
	{	theDir->fileName[0] = 0;
	}
	else
	{	BlockMoveData(directorySpec.name, theDir->fileName, directorySpec.name[0]+1);

		theInfo.ioCompletion = NULL;
		theInfo.ioNamePtr = directorySpec.name;
		theInfo.ioVRefNum = directorySpec.vRefNum;
		theInfo.ioDirID = directorySpec.parID;
		theInfo.ioFVersNum = 0;
		theInfo.ioFDirIndex = 0;
		iErr = PBHGetFInfo((HParmBlkPtr)&theInfo, false);
		
		theDir->changeDate = theInfo.ioFlMdDat;
		theDir->createDate = theInfo.ioFlCrDat;
		theDir->forkSize = theInfo.ioFlRLgLen;
		theDir->creator = theInfo.ioFlFndrInfo.fdCreator;
	}
}

Boolean	SupportsCatSearch(short theVRef);

static	Boolean	SupportsCatSearch(
	short	theVRef)
{
	HParamBlockRec			hp;
	GetVolParmsInfoBuffer	vp;
	OSErr					myErr;
	
	hp.ioParam.ioCompletion = NULL;
	hp.ioParam.ioNamePtr = NULL;
	hp.ioParam.ioVRefNum = theVRef;
	hp.ioParam.ioBuffer = (void *)&vp;
	hp.ioParam.ioReqCount = sizeof(GetVolParmsInfoBuffer);
	
	myErr = PBHGetVolParms(&hp, false);

	return (myErr == noErr) && (vp.vMAttrib & (1<<bHasCatSearch));
}

Boolean	CAvaraApp::LookForDirectory(
	DirectoryLocator	*theDir)
{
	OSErr		theErr = noErr;
	short		i;
	
	for(i=1;theErr == noErr; i++)
	{	VolumeParam	theVolume;
		Str32		theName;
	
		theVolume.ioCompletion = NULL;
		theVolume.ioVolIndex = i;
		theVolume.ioNamePtr = theName;
		theVolume.ioVRefNum = 0;
		
		theErr = PBGetVInfo((ParmBlkPtr)&theVolume, false);
		if(theErr == noErr)
		{	if(SupportsCatSearch(theVolume.ioVRefNum))
			{	CSParam		sp;
				OSErr		iErr;
				FSSpec		foundSpec;
				HFileInfo	s1,s2;
				#define		OPTBUFFERSIZE	8192
				char		optBuffer[OPTBUFFERSIZE];
				
				s1.ioNamePtr = theDir->fileName;	s2.ioNamePtr = NULL;
				s1.ioFlRLgLen = theDir->forkSize;	s2.ioFlRLgLen = theDir->forkSize;
				s1.ioFlRPyLen = 0;					s2.ioFlRPyLen = 0;

				s1.ioFlLgLen = 0;					s2.ioFlLgLen = 0;
				s1.ioFlPyLen = 0;					s2.ioFlPyLen = 0;
				
				s1.ioFlCrDat = theDir->createDate;	s2.ioFlCrDat = theDir->createDate;
				s1.ioFlMdDat = theDir->changeDate;	s2.ioFlMdDat = theDir->changeDate;
				s1.ioFlBkDat = 0;					s2.ioFlBkDat = 0;

				s1.ioFlFndrInfo.fdType = LEVELDIRECTORYFILETYPE;
				s2.ioFlFndrInfo.fdType = -1;

				s1.ioFlFndrInfo.fdCreator = theDir->creator;
				s2.ioFlFndrInfo.fdCreator = -1;
				
				s1.ioFlFndrInfo.fdFlags = 0;	//	ignore these
				s1.ioFlFndrInfo.fdLocation.h = 0;
				s1.ioFlFndrInfo.fdLocation.v = 0;
				s1.ioFlFndrInfo.fdFldr = 0;

				s2.ioFlFndrInfo.fdFlags = 0x10;	//	ignore these
				s2.ioFlFndrInfo.fdLocation.h = 0;
				s2.ioFlFndrInfo.fdLocation.v = 0;
				s2.ioFlFndrInfo.fdFldr = 0;

				sp.ioCompletion = NULL;
				sp.ioNamePtr = NULL;
				sp.ioVRefNum = theVolume.ioVRefNum;
				sp.ioMatchPtr = &foundSpec;
				sp.ioReqMatchCount = 1;
				sp.ioSearchBits = 	fsSBFullName + fsSBFlFndrInfo;// +// fsSBFlRLgLen + 
									fsSBFlCrDat + fsSBFlMdDat;
				sp.ioSearchInfo1 = (void *)&s1;
				sp.ioSearchInfo2 = (void *)&s2;
				sp.ioSearchTime = 100000L;
				sp.ioCatPosition.initialize = 0;
				sp.ioOptBuffer = optBuffer;
				sp.ioOptBufSize = OPTBUFFERSIZE;
				
				do
				{	do
					{	BroadcastCommand(kBusyTimeCmd);
						if(commandResult)
							return false;

						iErr = PBCatSearch((CSParamPtr)&sp, false);
					} while(iErr == noErr && sp.ioActMatchCount == 0);

					if(!iErr)
					{	OpenFSSpec(&foundSpec, true);
						if(theDir->dirTag == currentLevelDirectory)
						{	return true;
						}
					}
				} while(iErr == noErr);
			}
		}
	}
	
	//	Watch out, there's are two "returns" hidden in the loop above!
	return false;
}

OSErr	CAvaraApp::FetchLevel(
	DirectoryLocator	*theDir,
	OSType				theLevel,
	short				*crc)
{
	Boolean				foundMatch = true;
	OSErr				result = noErr;

	SetCursor(&gWatchCursor);
	SetCommandParams(LEVELLOADSTRINGS, kBusyLoadingLevel, false, 0);
	BroadcastCommand(kBusyStartCmd);

	if(theDir->fileName[0] == 0)
	{	if(directoryRef != appResRef)
		{	DoCommand(kUseDefaultDirectory);
		}
	}
	else
	{	DirectoryLocator	curDir;
		short				i;
	
		GetDirectoryLocator(&curDir);
		for(i=0;i<=curDir.fileName[0];i++)
		{	if(curDir.fileName[i] != theDir->fileName[i])
			{	foundMatch = false;
				break;
			}
		}
		
		if(	//	curDir.changeDate != theDir->changeDate ||
			//	curDir.createDate != theDir->createDate ||
			//	curDir.forkSize != theDir->forkSize ||
			curDir.creator != theDir->creator)
		{	foundMatch = false;
		}
		
		if(!foundMatch)
		{	SetCommandParams(LEVELLOADSTRINGS, kBusyCatSearching, true, 0);
			BroadcastCommand(kBusyStartCmd);
			foundMatch = LookForDirectory(theDir);
			BroadcastCommand(kBusyEndCmd);
		}
	}
	
	if(foundMatch)
	{	Handle	levelData;
		Str255	levelNameTemp;
	
		UseResFile(directoryRef);
		levelData = levelList->GetLevelData(theLevel, levelNameTemp);
		*crc = CRCHandle(levelData);
		
		if(levelData)
		{
			UseResFile(loadedDirectoryRef);
			itsGame->LevelReset(false);
			
			if(	loadedDirectoryRef != appResRef &&
				loadedDirectoryRef != directoryRef)
			{	itsGame->ChangeDirectoryFile();
				CloseResFile(loadedDirectoryRef);
			}

			loadedDirectoryRef = directoryRef;
			UseResFile(directoryRef);

			itsGame->loadedTag = theLevel;
			itsGame->loadedDirectory = currentLevelDirectory;
			BlockMoveData(levelNameTemp, itsGame->loadedLevel, levelNameTemp[0]+1);
			theRosterWind->InvalidateArea(kBottomBox, 0);

			gCurrentGame = itsGame;

			ConvertToLevelMap((PicHandle)levelData);
			DisposeHandle(levelData);

			theInfoPanel->SetScore(FreeMem());
			SetPort(theInfoPanel->itsWindow);
			theInfoPanel->DrawScore(true, true);
			
		}
		else
		{	result = fnfErr;
		}

		UseResFile(appResRef);
	}
	else
	{	result = fnfErr;
	}

	SetCursor(&qd.arrow);
	BroadcastCommand(kBusyEndCmd);

	return result;
}

void	CAvaraApp::RememberLastSavedFile(
	FSSpec	*theFile)
{
	AliasHandle	newAlias;
	OSErr		iErr;
	
	iErr = NewAlias(&appSpec, theFile, &newAlias);

	if(iErr == noErr)
	{	prefsBase->WriteHandle(kLastSavedFileTag, (Handle)newAlias);
		DisposeHandle((Handle)newAlias);
	}
}

void	CAvaraApp::DoOpenApplication()
{
	AliasHandle		oldAlias;
	Boolean			wasChanged;
	FSSpec			theFile;
	OSErr			result;
	
	oldAlias = (AliasHandle) prefsBase->ReadHandle(kLastSavedFileTag);
	if(oldAlias)
	{	result = ResolveAlias(&appSpec, oldAlias, &theFile, &wasChanged);

		if(result == noErr)
		{	if(wasChanged)
			{	prefsBase->WriteHandle(kLastSavedFileTag, (Handle)oldAlias);
			}
			
			result = OpenFSSpec(&theFile, false);
		}

		DisposeHandle((Handle)oldAlias);
	}
	else
	{	result = -1;
	}
	
	if(result)	DoCommand(kNewCmd);
	needsNewDocument = false;
}

Boolean	CAvaraApp::DoEvent(
	CEventHandler	*master,
	EventRecord		*theEvent)
{
	Boolean	result = true;

	switch(theEvent->what)
	{	case keyDown:
		case autoKey:
			{	char			theChar = theEvent->message;
				
				theRosterWind->KeyEvent(theEvent);
			//	used to be: gameNet->SendRosterMessage(1, &theChar);
			}
			break;
			
		default:
			result = inherited::DoEvent(master, theEvent);
			break;
	}
	
	return result;
}