/*
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.
*/

#include "CAvaraApp.h"
#include "OptionsUtil.h"
#include "CAvaraGame.h"
#include "CommandList.h"

#define	kSoundOptionsDialog	130

enum	{	ksoOK = 1,
			ksoCancel,
			ksoSampRate,
			ksoMaxChan,
			kso16bit,
			ksoInterpolate,
			ksoAmbient,
			ksoMusic,
			ksoMissileLoop,
			ksoFootsteps
		};


void	SetSoundOptionControls(
	short	soundSwitches,
	short	bit16)
{
	SetOptionValue(kso16bit, bit16);
	SetOptionValue(ksoInterpolate, soundSwitches & kInterpolateToggle);
	SetOptionValue(ksoAmbient, soundSwitches & kAmbientSoundToggle);

	SetOptionValue(ksoMusic, soundSwitches & kMusicToggle);
	SetOptionValue(ksoMissileLoop, soundSwitches & kMissileLoopToggle);
	SetOptionValue(ksoFootsteps, soundSwitches & kFootStepToggle);
}

void	SoundOptionsDialog()
{
	CAvaraApp		*theApp;
	CCommander		*saved;
	GrafPtr			savedPort;
	short			itemHit;
	ModalFilterUPP	myFilter;
	short			slot;
	short			sampRate, maxChan, soundSwitches, bit16;
	short			oSampRate, oMaxChan, oSoundSwitches, oBit16;
	ControlHandle	rateCtrl, chanCtrl;
	short			iType;
	Rect			iRect;

	defaultButton = optOK;
	
	theApp = (CAvaraApp *)gApplication;
	saved = theApp->BeginDialog();
	
	GetPort(&savedPort);

	myFilter = NewModalFilterProc(OptionsFilter);

	optsDialog = GetNewDialog(kSoundOptionsDialog, 0, (WindowPtr)-1);
	SetPort(optsDialog);
	
	oSoundSwitches = soundSwitches = gCurrentGame->soundSwitches;
	oBit16 = bit16 = gApplication->ReadShortPref(k16BitSoundOptionTag, true);
	oSampRate = sampRate = gApplication->ReadShortPref(kSoundRateOptionTag, theApp->fastMachine ? 4 : 7);
	oMaxChan = maxChan = gApplication->ReadShortPref(kSoundChannelsOptionTag, theApp->fastMachine ? 4 : 2);

	GetDItem(optsDialog, ksoSampRate, &iType, (Handle *)&rateCtrl, &iRect);
	SetCtlMin(rateCtrl, sampRate);

	GetDItem(optsDialog, ksoMaxChan, &iType, (Handle *)&chanCtrl, &iRect);
	SetCtlMin(chanCtrl, maxChan - 1);

	do
	{	SetSoundOptionControls(soundSwitches, bit16);

		ModalDialog(myFilter, &itemHit);

		switch(itemHit)
		{
			case kso16bit:			bit16 = !bit16;									break;
			case ksoInterpolate:	soundSwitches ^= kInterpolateToggle;			break;
			case ksoAmbient:		soundSwitches ^= kAmbientSoundToggle;			break;
			case ksoMusic:			soundSwitches ^= kMusicToggle;					break;
			case ksoMissileLoop:	soundSwitches ^= kMissileLoopToggle;			break;
			case ksoFootsteps:		soundSwitches ^= kFootStepToggle;				break;
#if 0
			ksoSampRate,
			ksoMaxChan			
#endif
		}
		
	} while(itemHit != ksoOK && itemHit != ksoCancel);

	if(itemHit == ksoOK)
	{
		if(soundSwitches != oSoundSwitches)
		{	gCurrentGame->soundSwitches = soundSwitches;
			gApplication->WriteShortPref(kSoundSwitchesTag, soundSwitches);
		}
		
		if(bit16 != oBit16)
		{	gCurrentGame->sound16BitStyle = bit16;
			gApplication->WriteShortPref(k16BitSoundOptionTag, bit16);
		}
		
		maxChan = (*chanCtrl)->contrlMin + 1;	//	GetCtrlMin(chanCtrl);
		if(maxChan != oMaxChan)
		{	gApplication->WriteShortPref(kSoundChannelsOptionTag, maxChan);
		}
		
		sampRate = (*rateCtrl)->contrlMin;	//	GetCtlMin(rateCtrl);
		if(sampRate != oSampRate)
		{	gApplication->WriteShortPref(kSoundRateOptionTag, sampRate);
		}
	}
	
	DisposeDialog(optsDialog);
	DisposeRoutineDescriptor(myFilter);

	SetPort(savedPort);

	theApp->EndDialog(saved);	
}