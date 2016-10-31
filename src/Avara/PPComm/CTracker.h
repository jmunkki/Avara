/*
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: CTracker.h
    Created: Thursday, April 18, 1996, 18:49
    Modified: Sunday, September 1, 1996, 9:43
*/

#pragma once

#include <CDirectObject.h>
#include "MacTCPLib.h"
#include "TrackerTags.h"

#define	kTrackerPortNumber				21541
#define	TRACKERSTREAMBUFFERSIZE			8192

typedef	char			TrackTag;

class	CTracker : public CDirectObject
{
public:
			long		logicalSize;
			long		realSize;
			Handle		trackerDataHandle;
			
			Handle		infoLeftHandle;
			Handle		infoRightHandle;
			Rect		infoRect;

	class	CTagBase	*prefs;
	class	CUDPComm	*owner;

			StreamPtr	stream;

			UDPiopb		receivePB;
			UDPiopb		sendPB;
			wdsEntry	writeDataTable[2];
			short		localPort;
			char		streamBuffer[TRACKERSTREAMBUFFERSIZE];
			long		wakeUp;
			long		sleepPeriod;
			
			DialogPtr	trackerDialog;
			ListHandle	gameList;
			Rect		gameRect;
			long		queryId;
			Point		globalLocation;
			
			Boolean		isTracking;
			Boolean		enabledState;
			Boolean		hasSpecialMessage;
			Boolean		isPrepared;
			short		curMessage;
			short		shownSelected;

	virtual	void		ITracker(CUDPComm *theComm, CTagBase *thePrefs);
	virtual	void		Dispose();
	
	virtual	void		SendTrackerInfo(Boolean sayGoodBye);
	virtual	void		WakeUp();
	virtual	void		OpenStream();
	virtual	void		CloseStream();
	virtual	OSErr		PrepareToSend();

	virtual	void		ReceivedNewGameRecord(Ptr theData, long len);
	virtual	void		SetStatusDisplay(short stat, StringPtr theStr);
	virtual	void		ListClicked();	
	virtual	void		DrawInfoBox(DialogPtr itsWindow, Rect *theRect);
	virtual	Boolean		QueryTracker(StringPtr hostName, short *hostPort);
	virtual	void		PrepareGameList();

	virtual	Ptr			WriteBuffer(TrackTag tag, short len);
	virtual	void		WriteNullTag(TrackTag tag);
	virtual	void		WriteCharTag(TrackTag tag, char value);
	virtual	void		WriteShortTag(TrackTag tag, short value);
	virtual	void		WriteLongTag(TrackTag tag, long value);
	virtual	void		WriteStringTag(TrackTag tag, StringPtr str);

	virtual	Ptr			WriteBufferIndexed(TrackTag tag, char index, short len);	
	virtual	void		WriteNullTagIndexed(TrackTag tag, char index);
	virtual	void		WriteCharTagIndexed(TrackTag tag, char index, char value);
	virtual	void		WriteShortTagIndexed(TrackTag tag, char index, short value);
	virtual	void		WriteLongTagIndexed(TrackTag tag, char index, long value);
	virtual	void		WriteStringTagIndexed(TrackTag tag, char index, StringPtr str);
	
	virtual	void		ResetTagBuffer();
	virtual	void		BuildServerTags();
	
	virtual	void		StartTracking();
	virtual	void		StopTracking();
};