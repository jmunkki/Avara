/*
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.
*/

#include "CAvaraApp.h"
#include "OptionsUtil.h"
#include "CAvaraGame.h"
#include "CommandList.h"

#define	kGameOptionsDialog	129

enum	{	kgoOK = 1,
			kgoCancel,
			kgoJoystickMode,
			kgoFlipMode,

			kgoLowSens,
			kgoMediumSens,
			kgoHighSens,

			kgoBGTasks,
			kgoRosterUpdate,
			kgoAmbientHolograms,
			kgoFullExplosions,

			kgoSimpleHorizon,
			kgoLowHorizon,
			kgoHighHorizon
		};


void	SetGameOptionControls(
	short		mouseOptions,
	short		sensitivity,
	short		bgTasks,
	short		rosterUpdates,
	short		ambientHolos,
	short		horizon,
	short		simpleExplosions)
{

	SetOptionValue(kgoLowSens, sensitivity == -1);
	SetOptionValue(kgoMediumSens, sensitivity == 0);
	SetOptionValue(kgoHighSens, sensitivity == 1);

	SetOptionValue(kgoJoystickMode, mouseOptions & kJoystickMode);
	SetOptionValue(kgoFlipMode, mouseOptions & kFlipAxis);

	SetOptionValue(kgoSimpleHorizon, horizon == 0);
	SetOptionValue(kgoLowHorizon, horizon == 1);
	SetOptionValue(kgoHighHorizon, horizon == 2);
	
	SetOptionValue(kgoBGTasks, bgTasks);
	SetOptionValue(kgoRosterUpdate, rosterUpdates);
	SetOptionValue(kgoAmbientHolograms, ambientHolos);
	SetOptionValue(kgoFullExplosions, !simpleExplosions);
}

void	GameOptionsDialog()
{
	CAvaraApp		*theApp;
	CCommander		*saved;
	GrafPtr			savedPort;
	short			itemHit;
	ModalFilterUPP	myFilter;
	short			slot;
	short			mouseOptions, sensitivity, bgTasks, rosterUpdates, ambientHolos, horizon, simpleExplosions;
	short			oMouseOptions, oSensitivity, oBgTasks, oRosterUpdates, oAmbientHolos, oHorizon, oSimpleExplosions;

	defaultButton = optOK;
	
	theApp = (CAvaraApp *)gApplication;
	saved = theApp->BeginDialog();
	
	GetPort(&savedPort);

	myFilter = NewModalFilterProc(OptionsFilter);

	optsDialog = GetNewDialog(kGameOptionsDialog, 0, (WindowPtr)-1);
	SetPort(optsDialog);
	
	oMouseOptions = mouseOptions = gApplication->ReadShortPref(kMouseJoystickTag, 0);
	oSensitivity = sensitivity = gApplication->ReadShortPref(kMouseSensitivityTag, 0);

	oBgTasks = bgTasks = gApplication->ReadShortPref(kBackgroundProcessTag, false);
	oRosterUpdates = rosterUpdates = gApplication->ReadShortPref(kUpdateRosterTag, true);
	oAmbientHolos = ambientHolos = gApplication->ReadShortPref(kAmbientHologramsTag, true);
	oHorizon = horizon = gApplication->ReadShortPref(kHorizonDetailTag, theApp->fastMachine ? 2 : 0);
	oSimpleExplosions = simpleExplosions = gApplication->ReadShortPref(kSimpleExplosionsTag, !theApp->fastMachine);
	
	do
	{	SetGameOptionControls(mouseOptions, sensitivity, bgTasks, rosterUpdates, ambientHolos, horizon, simpleExplosions);

		ModalDialog(myFilter, &itemHit);

		switch(itemHit)
		{
			case kgoJoystickMode:	mouseOptions ^= kJoystickMode;				break;
			case kgoFlipMode:		mouseOptions ^= kFlipAxis;					break;
			case kgoLowSens:		sensitivity = -1;							break;
			case kgoMediumSens:		sensitivity = 0;							break;
			case kgoHighSens:		sensitivity = 1;							break;
			case kgoBGTasks:		bgTasks = !bgTasks;							break;
			case kgoRosterUpdate:	rosterUpdates = !rosterUpdates;				break;
			case kgoAmbientHolograms:	ambientHolos = !ambientHolos;			break;
			case kgoFullExplosions:	simpleExplosions = !simpleExplosions;		break;
			case kgoSimpleHorizon:	horizon = 0;								break;
			case kgoLowHorizon:		horizon = 1;								break;
			case kgoHighHorizon:	horizon = 2;								break;
		}
		
	} while(itemHit != kgoOK && itemHit != kgoCancel);

	DisposeDialog(optsDialog);
	DisposeRoutineDescriptor(myFilter);

	SetPort(savedPort);

	if(itemHit == kgoOK)
	{
		if(mouseOptions != oMouseOptions)	gApplication->WriteShortPref(kMouseJoystickTag, mouseOptions);
		if(sensitivity != oSensitivity)		gApplication->WriteShortPref(kMouseSensitivityTag, sensitivity);
		if(bgTasks != oBgTasks)				gApplication->WriteShortPref(kBackgroundProcessTag, bgTasks);
		if(rosterUpdates != oRosterUpdates)	gApplication->WriteShortPref(kUpdateRosterTag, rosterUpdates);
		if(simpleExplosions != oSimpleExplosions)
											gApplication->WriteShortPref(kSimpleExplosionsTag, simpleExplosions);
		if(ambientHolos != oAmbientHolos)	gApplication->DoCommand(kAmbientHologramsCmd);
		if(horizon != oHorizon)				gApplication->DoCommand(kSimpleHorizonCmd + horizon);
	}
	
	theApp->EndDialog(saved);	
}