/*
    Copyright �1994-1996, Juri Munkki
    All rights reserved.
*/

#include "CAvaraApp.h"
#include "CNetManager.h"
#include "CCommManager.h"
#include "CommDefs.h"
#include "OptionsUtil.h"

#define	kServerOptionsDialog	420

enum	{	ksoOK = 1,
			ksoCancel,
			ksoLoad,
			ksoResume,
			ksoLatency,
			ksoPosition,
			ksoServerColor,
			ksoEachColor,
			ksoFreeColor
		};

void	SetServerOptionControls(
	short	curOptions)
{
	SetOptionValue(ksoLoad, curOptions & (1<<kAllowLoadBit));
	SetOptionValue(ksoResume, curOptions & (1<<kAllowResumeBit));
	SetOptionValue(ksoLatency, curOptions & (1<<kAllowLatencyBit));
	SetOptionValue(ksoPosition, curOptions & (1<<kAllowPositionBit));
	SetOptionValue(ksoServerColor, !(curOptions & kAllowTeamControlBitMask));
	SetOptionValue(ksoEachColor, curOptions & (1<<kAllowOwnColorBit));
	SetOptionValue(ksoFreeColor, curOptions & (1<<kFreeColorBit));
}

void	ServerOptionsDialog()
{
	CAvaraApp		*theApp;
	short			oldOptions;
	short			curOptions;
	CCommander		*saved;
	GrafPtr			savedPort;
	short			itemHit;
	ModalFilterUPP	myFilter;
	short			slot;

	theApp = (CAvaraApp *)gApplication;
	saved = theApp->BeginDialog();
	
	GetPort(&savedPort);

	myFilter = NewModalFilterProc(OptionsFilter);

	optsDialog = GetNewDialog(kServerOptionsDialog, 0, (WindowPtr)-1);
	SetPort(optsDialog);
	
	slot = theApp->gameNet->itsCommManager->myId;
	if(slot == 0)
	{	oldOptions = theApp->ReadShortPref(kServerOptionsTag, kDefaultServerOptions);
		defaultButton = ksoOK;
	}
	else
	{	oldOptions = theApp->gameNet->serverOptions;
		DimOptionControl(ksoLoad);
		DimOptionControl(ksoResume);
		DimOptionControl(ksoLatency);
		DimOptionControl(ksoPosition);
		DimOptionControl(ksoServerColor);
		DimOptionControl(ksoEachColor);
		DimOptionControl(ksoFreeColor);
		DimOptionControl(ksoOK);
		defaultButton = ksoCancel;
	}
	curOptions = oldOptions;

	do
	{	SetServerOptionControls(curOptions);

		ModalDialog(myFilter, &itemHit);

		switch(itemHit)
		{
			case ksoLoad:		curOptions ^= 1 << kAllowLoadBit;		break;
			case ksoResume:		curOptions ^= 1 << kAllowResumeBit;		break;
			case ksoLatency:		curOptions ^= 1 << kAllowLatencyBit;	break;
			case ksoPosition:	curOptions ^= 1 << kAllowPositionBit;	break;
			case ksoServerColor:
				curOptions &= ~kAllowTeamControlBitMask;
				break;
			case ksoEachColor:
				curOptions &= ~kAllowTeamControlBitMask;
				curOptions |= 1 << kAllowOwnColorBit;
				break;
			case ksoFreeColor:
				curOptions &= ~kAllowTeamControlBitMask;
				curOptions |= 1 << kFreeColorBit;
				break;
		}
		
	} while(itemHit != ksoOK && itemHit != ksoCancel);

	DisposeDialog(optsDialog);
	DisposeRoutineDescriptor(myFilter);

	SetPort(savedPort);

	if(itemHit == ksoOK && curOptions != oldOptions)
	{	theApp->WriteShortPref(kServerOptionsTag, curOptions);
		theApp->gameNet->ChangedServerOptions(curOptions);
	}
	
	theApp->EndDialog(saved);	
}