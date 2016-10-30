/*/
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CNetManager.c
    Created: Monday, May 15, 1995, 22:25
    Modified: Tuesday, September 17, 1996, 3:21
/*/

#include "CNetManager.h"
#include "CAvaraGame.h"
#include "CPlayerManager.h"
#include "CProtoControl.h"
#include "CommandList.h"

#include "CCommServer.h"
#include "CCommClient.h"
#include "CUDPComm.h"

#include "CEventHandler.h"
#include "CommDefs.h"
#include "CRosterWindow.h"
#include "CAvaraApp.h"
#include "CAbstractPlayer.h"
#include "CInfoPanel.h"
#include "InfoMessages.h"
#include "CScoreKeeper.h"
#include "CAsyncBeeper.h"
#include "Ambrosia_Reg.h"
#include "CTracker.h"

#define	DEBUG_FASTTRACK_no

#define	AUTOLATENCYPERIOD	64
#define	AUTOLATENCYDELAY	8
#define	LOWERLATENCYCOUNT	3
#define	HIGHERLATENCYCOUNT	8

#define	kAvaraNetVersion	6

#define	kMessageBufferMaxAge	90
#define	kMessageBufferMinAge	30
#define	kMessageWaitTime		12

void	CNetManager::INetManager(
	CAvaraGame	*theGame)
{
	short			i;

	itsGame = theGame;
	readyPlayers = 0;
	unavailablePlayers = 0;

	netStatus = kNullNet;
	itsCommManager = new CCommManager;
	itsCommManager->ICommManager(NULLNETPACKETS);
	
	itsProtoControl = new CProtoControl;
	itsProtoControl->IProtoControl(itsCommManager, itsGame);

	theRoster = ((CAvaraApp *)gApplication)->theRosterWind;

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	playerTable[i] = new CPlayerManager;
		playerTable[i]->IPlayerManager(itsGame, i, this, theRoster);
		slotToPosition[i] = i;
		positionToSlot[i] = i;
		teamColors[i] = (i/3) * 2;
	}

	totalDistribution = 0;
	playerCount = 0;
	isConnected = false;
	isPlaying = false;
	InitFastTrack();
	
	netOwner = NULL;
	loaderSlot = 0;
	
	serverOptions = gApplication->ReadShortPref(kServerOptionsTag, kDefaultServerOptions);
	
	lastMsgTick = TickCount();
	firstMsgTick = lastMsgTick;
	msgBufferLen = 0;
	
	lastLoginRefusal = 0;
}

void	CNetManager::LevelReset()
{
	playerCount = 0;
}
void	CNetManager::Dispose()
{
	short	i;

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	playerTable[i]->Dispose();
	}

	itsProtoControl->Dispose();
	itsCommManager->Dispose();
}

Boolean	CNetManager::ConfirmNetChange()
{
	CCommander	*theActive;
	short		dItem;
	
	theActive = gApplication->BeginDialog();
	dItem = CautionAlert(402, NULL);
	gApplication->EndDialog(theActive);
	
	return dItem == 1;
}

void	CNetManager::ChangeNet(
	short			netKind,
	CDirectObject	*owner)
{
	CCommander		*theActive;
	CCommManager	*newManager = NULL;
	Boolean			confirm	= true;
	CAvaraApp		*theApp = (CAvaraApp *)gApplication;

	if(owner != netOwner || netKind != netStatus)
	{	if(netKind != netStatus || !isConnected)
		{	if(netStatus != kNullNet || !isConnected)
			{	if(isConnected)
				{	confirm = ConfirmNetChange();
				}
			}
			
			if(confirm)
			{	switch(netKind)
				{	case kNullNet:
						newManager = new CCommManager;
						newManager->ICommManager(NULLNETPACKETS);
						break;
					case kServerNet:
						if(theApp->networkOptions == 3)
						{	CUDPComm	*theServer;
						
							theServer = new CUDPComm;
							theServer->IUDPComm(kMaxAvaraPlayers-1, TCPNETPACKETS, kAvaraNetVersion, itsGame->frameTime);
							theActive = gApplication->BeginDialog();
							theServer->StartServing();
							gApplication->EndDialog(theActive);
							newManager = theServer;
							confirm = theServer->isConnected;
						}
						else
						{	CCommServer	*theServer;

							SetCursor(&gWatchCursor);
							theServer = new CCommServer;
							theServer->ICommServer(kMaxAvaraPlayers-1, SERVERNETPACKETS);
							theServer->StartServing();
							confirm = theServer->isServing;
							newManager = theServer;
							SetCursor(&qd.arrow);
						}	
						break;
					case kClientNet:
						if(theApp->networkOptions == 3)
						{	CUDPComm	*theClient;
						
							theClient = new CUDPComm;
							theClient->IUDPComm(kMaxAvaraPlayers-1, TCPNETPACKETS, kAvaraNetVersion, itsGame->frameTime);
							theActive = gApplication->BeginDialog();
							theClient->Connect();
							gApplication->EndDialog(theActive);
							newManager = theClient;
							confirm = theClient->isConnected;
						}
						else
						{	CCommClient	*theClient;
						
							theClient = new CCommClient;
							theClient->ICommClient(CLIENTNETPACKETS);
							theActive = gApplication->BeginDialog();
							theClient->Connect();
							gApplication->EndDialog(theActive);
							newManager = theClient;
							confirm = theClient->isConnected;
						}
						break;
				}
			}

			if(confirm && newManager)
			{
				itsProtoControl->Detach();
				itsCommManager->Dispose();
				itsCommManager = newManager;
				itsProtoControl->Attach(itsCommManager);
				netOwner = owner;
				netStatus = netKind;
				isConnected = true;
				DisconnectSome(kdEveryone);

				totalDistribution = 0;
				itsCommManager->SendPacket(kdServerOnly, kpLogin, 0,0,0, 0L, NULL);
				if(itsGame->loadedTag)
				{	itsGame->LevelReset(true);
					theRoster->InvalidateArea(kBottomBox, 0);
				}
				gApplication->BroadcastCommand(kNetChangedCmd);
			}
			else
			{	if(newManager)	newManager->Dispose();
			}
		}
		else
		{	netOwner = owner;
			playerTable[itsCommManager->myId]->NetDisconnect();
			itsCommManager->SendPacket(kdServerOnly, kpLogin, 0,0,0, 0L, NULL);
			itsCommManager->SendPacket(kdEveryone, kpZapMugShot, 0,0,0, 0L, NULL);
			gApplication->BroadcastCommand(kNetChangedCmd);
		}
	}
}

void	CNetManager::ProcessQueue()
{
	long	curTicks;
	
	itsCommManager->ProcessQueue();
	
	if(msgBufferLen)
	{	curTicks = TickCount();
		
		if(		(curTicks - firstMsgTick > kMessageBufferMaxAge)
			||	((curTicks - firstMsgTick > kMessageBufferMinAge)
			&&	(curTicks - lastMsgTick > kMessageWaitTime)))
		{	FlushMessageBuffer();
		}
	}
	
	if(gAsyncBeeper->beepChannel)
	{	gAsyncBeeper->BeepDoneCheck();
	}
}

void	CNetManager::SendRealName(
	short		toSlot)
{
	Str255				realName;
	char				regStatus;
	short				distr;

	if(IsRegistered(510))
	{	LicenseHandle		license;

		license = (LicenseHandle)GetLicenseData(510);
		BlockMoveData((*license)->licensee, realName, sizeof(realName));
		DisposeHandle((Handle)license);
		theRoster->TruncatePlayerName(realName);
		regStatus = -1;
		if(realName[0] > 63)
			realName[0] = 63;
	}
	else
	{	unsigned long		longDate;

		GetDateTime(&longDate);

		realName[0] = 0;
		longDate = longDate - gApplication->ReadLongPref(kFirstPrefsCreate, longDate);
		regStatus = longDate > 2592000;
	}
	
	if(itsCommManager->myId == toSlot)
	{	distr = kdEveryone;
	}
	else
	{	distr = 1 << toSlot;
	}
	itsCommManager->SendPacket(distr,
								kpRealName,
								regStatus, 0, 0,
								realName[0]+1, (Ptr)realName);
}

void	CNetManager::RealNameReport(
	short		slotId,
	short		regStatus,
	StringPtr	realName)
{
	CPlayerManager	*thePlayer;
	
	thePlayer = playerTable[slotId];
	if(regStatus < 0)
	{	thePlayer->isRegistered = true;
		BlockMoveData(realName, thePlayer->playerRegName, realName[0] + 1);
	}
	else
	{	thePlayer->isRegistered = false;
		GetIndString(thePlayer->playerRegName, 133, 5 + regStatus);
	}

	theRoster->InvalidateArea(kRealNameBox, slotToPosition[slotId]);
}

void	CNetManager::NameChange(
	StringPtr	newName)
{
	short				theStatus;
	MachineLocation		myLocation;
	Point				loc;
	
	ReadLocation(&myLocation);
	loc.h = myLocation.longitude >> 16;
	loc.v = myLocation.latitude >> 16;
	
	theStatus = playerTable[itsCommManager->myId]->loadingStatus;
	itsCommManager->SendPacket(kdEveryone,
								kpNameChange,
								0, theStatus, *(long *)&loc,
								newName[0]+1, (void *)newName);
}

void	CNetManager::RecordNameAndLocation(
	short		theId,
	StringPtr	theName,
	short		status,
	Point		location)
{
	if(theId >= 0 && theId < kMaxAvaraPlayers)
	{	totalDistribution |= 1 << theId;
		if(status != 0)
			playerTable[theId]->SetPlayerStatus(status, -1);

		playerTable[theId]->ChangeNameAndLocation(theName, location);
	}
}

void	CNetManager::SwapPositions(
	short	ind1,
	short	ind2)
{
	char	p[kMaxAvaraPlayers];
	char	temp;
	short	i;
	
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	p[i] = positionToSlot[i];
	}
	
	temp = p[ind1];
	p[ind1] = p[ind2];
	p[ind2] = temp;
	itsCommManager->SendPacket(kdEveryone, kpOrderChange,
								0,0,0, kMaxAvaraPlayers, p);
}

void	CNetManager::PositionsChanged(
	char	*p)
{
	short	i;
	
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	if(p[i] != positionToSlot[i])
		{	positionToSlot[i] = p[i];
			playerTable[positionToSlot[i]]->SetPosition(i);
		}
	}

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	slotToPosition[positionToSlot[i]] = i;
	}
}

void	CNetManager::FlushMessageBuffer()
{
	if(msgBufferLen)
	{	itsCommManager->SendPacket(~(1 << itsCommManager->myId), kpRosterMessage, 0,0,0, msgBufferLen, msgBuffer);
		msgBufferLen = 0;
	}
}

void	CNetManager::BufferMessage(
	short		len,
	char		*c)
{
	if(len)
	{	lastMsgTick = TickCount();
		if(msgBufferLen == 0)
		{	firstMsgTick = lastMsgTick;
		}
		
		while(len--)
		{	msgBuffer[msgBufferLen++] = *c++;
		}
		
		if(msgBufferLen == kMaxChatMessageBufferLen)
		{	FlushMessageBuffer();
		}
	}
}

void	CNetManager::SendRosterMessage(
	short	len,
	char	*c)
{
	if(len > kMaxChatMessageBufferLen)
	{	FlushMessageBuffer();
		itsCommManager->SendPacket(kdEveryone, kpRosterMessage, 0,0,0, len, c);
	}
	else
	{
		itsCommManager->SendPacket(1 << itsCommManager->myId, kpRosterMessage, 0,0,0, len, c);
		
		if(len + msgBufferLen > kMaxChatMessageBufferLen)
		{	FlushMessageBuffer();
		}
		
		BufferMessage(len, c);
	}	
}

void	CNetManager::ReceiveRosterMessage(
	short	slotId,
	short	len,
	char	*c)
{
	CPlayerManager	*thePlayer;
	
	if(slotId >= 0 && slotId < kMaxAvaraPlayers)
	{	char	*cp;
		short	i;
	
		cp = c;
		i = len;
		thePlayer = playerTable[slotId];
		while(i--)
		{	thePlayer->RosterKeyPress(*cp++);
		}
		
		theRoster->InvalidateArea(kUserBoxBottomLine, slotToPosition[slotId]);
		
		thePlayer->RosterMessageText(len, c);
	}
}

void	CNetManager::SendColorChange()
{
	itsCommManager->SendPacket(kdEveryone, kpColorChange, 0,0,0, kMaxAvaraPlayers, teamColors);
}

void	CNetManager::ReceiveColorChange(
	char	*newColors)
{
	short	i;
	
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	if(newColors[i] != teamColors[i])
		{	teamColors[i] = newColors[i];
			theRoster->InvalidateColorBox(i);
		}	
	}
}

void	CNetManager::DisconnectSome(
	short	mask)
{
	short	i;

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	if((1L << i) & mask)
		{	playerTable[i]->NetDisconnect();
		}
	}
	
	totalDistribution &= ~mask;
}

void	CNetManager::HandleDisconnect(
	short	slotId,
	short	why)
{
	itsCommManager->DisconnectSlot(slotId);

	if(slotId == itsCommManager->myId)
	{	isConnected = false;
		netOwner = NULL;
		DisconnectSome(kdEveryone);
		itsCommManager->SendPacket(1<<slotId, kpKillNet, 0,0,0, 0,0);
	}
	else
	{	DisconnectSome(1L<<slotId);
	}
}

void	CNetManager::SendLoadLevel(
	OSType	theLevelTag)
{
	CAvaraApp			*theApp;
	PacketInfo			*aPacket;
	
	ProcessQueue();
	
	aPacket = itsCommManager->GetPacket();

	theApp = (CAvaraApp *)gApplication;
	theApp->GetDirectoryLocator((DirectoryLocator *)aPacket->dataBuffer);

	*(Fixed *)(aPacket->dataBuffer+sizeof(DirectoryLocator)) = TickCount();
	aPacket->dataLen = sizeof(DirectoryLocator)+sizeof(Fixed);
	aPacket->command = kpLoadLevel;
	aPacket->p1 = 0;
	aPacket->p2 = 0;
	aPacket->p3 = theLevelTag;
	aPacket->distribution = kdEveryone;

	itsCommManager->WriteAndSignPacket(aPacket);
}

void	CNetManager::ReceiveLoadLevel(
	short		senderSlot,
	void		*theDir,
	OSType		theTag)
{
	CAvaraApp			*theApp;
	OSErr				iErr;
	short				crc;
	DirectoryLocator *	theDirectory = theDir;

	if(!isPlaying)
	{	loaderSlot = senderSlot;
		theApp = (CAvaraApp *)gApplication;
		itsCommManager->SendPacket(kdEveryone, kpLevelLoadErr, 0, 1, theTag, 0,0);
		FRandSeed = *(Fixed *)(theDirectory + 1);
		FRandSeedBeta = FRandSeed;
		
		iErr = theApp->FetchLevel(theDirectory, theTag, &crc);
	
		if(iErr) 
		{	itsCommManager->SendPacket(kdEveryone, kpLevelLoadErr, 0, iErr, theTag, 0,0);
		}
		else
		{	itsCommManager->SendPacket(kdEveryone, kpLevelLoaded, 0, crc, theTag, 0,0);
		}
	}
}

void	CNetManager::LevelLoadStatus(
	short		senderSlot,
	short		crc,
	OSErr		err,
	OSType		theTag)
{
	short	i;

	CPlayerManager	*thePlayer;
	
	thePlayer = playerTable[senderSlot];
	thePlayer->levelCRC = crc;
	thePlayer->levelTag = theTag;
	thePlayer->levelErr = err;

	if(senderSlot == loaderSlot)
	{	for(i=0;i<kMaxAvaraPlayers;i++)
		{	playerTable[i]->LoadStatusChange(crc, err, theTag);
		}
	}
	else
	{	thePlayer->LoadStatusChange(playerTable[loaderSlot]->levelCRC,
									playerTable[loaderSlot]->levelErr,
									playerTable[loaderSlot]->levelTag);
	}
}

Boolean	CNetManager::GatherPlayers(
	Boolean			isFreshMission)
{
	short			i;
	Boolean			goAhead;
	long			lastTime;

	totalDistribution = 0;
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	playerTable[i]->ResumeGame();
	}

	OpenFastTrack();
	itsCommManager->SendUrgentPacket(kdEveryone, kpFastTrack, 0, 0, fastTrack.addr.value, 0,0);

	itsCommManager->SendUrgentPacket(activePlayersDistribution, kpReadySynch, 0,0,0, 0,0);
	lastTime = TickCount();
	do
	{	ProcessQueue();
		goAhead = (TickCount() - lastTime < 1800);
		
		gApplication->BroadcastCommand(kBusyTimeCmd);
		if(gApplication->commandResult)
		{	goAhead = false;
		}
	} while(((activePlayersDistribution & (unavailablePlayers | readyPlayers)) ^ activePlayersDistribution)
				&& goAhead);


	if(unavailablePlayers)
	{	itsCommManager->SendPacket(unavailablePlayers, kpUnavailableZero, 0,0,0, 0,NULL);
		if(goAhead)
		{	goAhead = isFreshMission;

			if(goAhead)
			{	activePlayersDistribution &= ~unavailablePlayers;
				startPlayersDistribution = activePlayersDistribution;
			}
		}		
	}

	readyPlayers = 0;
	unavailablePlayers = 0;	

	return goAhead;
}

void	CNetManager::UngatherPlayers()
{
	isPlaying = false;
	CloseFastTrack();
}

void	CNetManager::ResumeGame()
{
	short			i;
	Point			tempPoint;
	CPlayerManager	*thePlayerManager;
	long			lastTime;
	Boolean			notReady;
	Boolean			allOk = false;

	config.numGrenades = 0;
	config.numMissiles = 0;
	config.numBoosters = 3;
	config.hullType = 1;
	config.latencyTolerance = 0;
	gApplication->BroadcastCommand(kConfigurePlayerCmd);

	fragmentDetected = false;

	maxRoundTripLatency = 0;
	addOneLatency = 0;
	localLatencyVote = 0;
	autoLatencyVote = 0;
	autoLatencyVoteCount = 0;

	thePlayerManager = playerTable[itsCommManager->myId];
	if(thePlayerManager->itsPlayer)
	{
		thePlayerManager->DoMouseControl(&tempPoint, true);
		if(itsGame->moJoOptions & kJoystickMode)
			thePlayerManager->DoMouseControl(&tempPoint, false);
		
		itsCommManager->SendUrgentPacket(kdEveryone, kpStartSynch, 0,kLActive, FRandSeed, sizeof(PlayerConfigRecord), (Ptr)&config);
	
		//	Synchronize players:
		lastTime = TickCount();
		notReady = true;
		do
		{	short	statusTest;
		
			statusTest = 0;
			for(i=0;i<kMaxAvaraPlayers;i++)
			{	if(playerTable[i]->loadingStatus == kLActive)
				{	statusTest |= 1 << i;
				}
			}
			
			if((statusTest & activePlayersDistribution) == activePlayersDistribution)
			{	notReady = false;
			}
			
			ProcessQueue();
			theRoster->DoUpdate();

			gApplication->BroadcastCommand(kBusyTimeCmd);
			if(gApplication->commandResult)
			{	break;
			}
	
		} while(TickCount()-lastTime < 1800 && notReady);
		
		for(i=0;i<kMaxAvaraPlayers;i++)
		{	if(activePlayersDistribution & (1<<i))
			{	DoConfig(i);	
			}
		}
	
		ProcessQueue();
		if(notReady)
		{	SysBeep(10);
			itsGame->statusRequest = kAbortStatus;
		}
	}
	else
	{	itsCommManager->SendUrgentPacket(kdEveryone, kpRemoveMeFromGame, 0,0,0, 0,0);
		itsGame->statusRequest = kNoVehicleStatus;
		SysBeep(10);
	}
}

void	CNetManager::FrameAction()
{
	playerTable[itsCommManager->myId]->EventHandler();
	itsCommManager->ProcessQueue();

	if(!isConnected)
	{	itsGame->statusRequest = kAbortStatus;
	}
}

void	CNetManager::AutoLatencyControl(
	long		frameNumber,
	Boolean		didWait)
{
	if(didWait)
	{	localLatencyVote++;
	}
	
	if(frameNumber >= AUTOLATENCYPERIOD)
	{	if((frameNumber & (AUTOLATENCYPERIOD-1)) == 0)
		{	long	maxRoundLatency;
		
			maxRoundLatency = itsCommManager->GetMaxRoundTrip(activePlayersDistribution);
			itsCommManager->SendUrgentPacket(activePlayersDistribution, kpLatencyVote,
												localLatencyVote, maxRoundLatency, FRandSeed, 0, NULL);
			localLatencyVote = 0;
		}
		else
		if(((frameNumber + AUTOLATENCYDELAY) & (AUTOLATENCYPERIOD-1)) == 0)
		{	
			if(fragmentDetected)
			{	itsGame->infoPanel->MessageLine(kmFragmentAlert, centerAlign);
				fragmentDetected = false;
			}

			if((serverOptions & (1<<kUseAutoLatencyBit)) && autoLatencyVoteCount)
			{	Boolean		didChange = false;
				long		curLatency = itsGame->latencyTolerance;
				long		maxFrameLatency;

				autoLatencyVote /= autoLatencyVoteCount;
				
				if(autoLatencyVote > HIGHERLATENCYCOUNT)
				{	addOneLatency = 1;
				}

				maxFrameLatency = addOneLatency +
								(maxRoundTripLatency + itsGame->frameTime) / (itsGame->frameTime+itsGame->frameTime);

				if(maxFrameLatency > 8)
					maxFrameLatency = 8;

				if(maxFrameLatency < curLatency)
				{	addOneLatency = 0;
					itsGame->latencyTolerance--;
					gApplication->WriteShortPref(kLatencyToleranceTag, itsGame->latencyTolerance);
					didChange = true;
				}
				else
				if(maxFrameLatency > curLatency && autoLatencyVote > LOWERLATENCYCOUNT)
				{	itsGame->latencyTolerance++;
					gApplication->WriteShortPref(kLatencyToleranceTag, itsGame->latencyTolerance);
					didChange = true;
				}
				
				if(didChange)
				{	Str255		tempStr;
					short		len1,len2;
				
					GetIndString(tempStr, STATUSSTRINGSLISTID, kmNewLatency1);
					len1 = tempStr[0] + 1;
					GetIndString(tempStr+len1, STATUSSTRINGSLISTID, kmNewLatency2);
					tempStr[0] += tempStr[len1] + 1;
					tempStr[len1] = '0' + itsGame->latencyTolerance;
					itsGame->infoPanel->StringLine(tempStr, centerAlign);
				}					
			}

			autoLatencyVote = 0;
			autoLatencyVoteCount = 0;
			maxRoundTripLatency = 0;
		}
	}
}

void	CNetManager::ViewControl()
{
	playerTable[itsCommManager->myId]->ViewControl();
}

void	CNetManager::SendStartCommand()
{
	short		i;
	
	activePlayersDistribution = 0;
	startPlayersDistribution = 0;
	readyPlayers = 0;
	unavailablePlayers = 0;

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	if(	playerTable[i]->loadingStatus == kLLoaded)
		{	activePlayersDistribution |= 1 << i;
		}
	}
	
	itsCommManager->SendPacket(activePlayersDistribution,
								kpStartLevel, 0,activePlayersDistribution, 0, 0,0);
}

void	CNetManager::ReceiveStartCommand(
	short	activeDistribution,
	short	fromSlot)
{
	if(gApplication->modelessLevel == 0 && !isPlaying)
	{	deadOrDonePlayers = 0;
		activePlayersDistribution = activeDistribution;
		startPlayersDistribution = activeDistribution;
		gApplication->DoCommand(kGetReadyToStartCmd);
		isPlaying = true;
		itsGame->ResumeGame();
	}
	else
	{	itsCommManager->SendPacket(activeDistribution, kpUnavailableSynch, fromSlot,0,0, 0, NULL);
	}
}

void	CNetManager::ReceiveResumeCommand(
	short	activeDistribution,
	short	fromSlot,
	Fixed	randomKey)
{
	short	i;
	activePlayersDistribution = activeDistribution;

	if(gApplication->modelessLevel == 0 &&
		!isPlaying && randomKey == FRandSeed)
	{	theRoster->DoUpdate();
	
		gApplication->DoCommand(kGetReadyToStartCmd);
	
		isPlaying = true;
		itsGame->ResumeGame();	
	}
	else
	{	itsCommManager->SendPacket(activeDistribution, kpUnavailableSynch, fromSlot,0,0, 0, NULL);
	}
}

void	CNetManager::ReceivedUnavailable(
	short	slot,
	short	fromSlot)
{
	unavailablePlayers |= 1 << slot;

	if(slot == itsCommManager->myId)
	{	itsGame->infoPanel->ParamLine(kmStartFailure, centerAlign, playerTable[fromSlot]->playerName, NULL);
	}
	else
	{	itsGame->infoPanel->ParamLine(kmUnavailableNote, centerAlign, playerTable[slot]->playerName, NULL);
	}
}

void	CNetManager::SendResumeCommand()
{
	short		i;
	Fixed		myKey;
	
	activePlayersDistribution = 0;
	
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	if(playerTable[i]->itsPlayer && !playerTable[i]->itsPlayer->isOut)
		{	activePlayersDistribution |= 1 << i;
		}
	}
	
	itsCommManager->SendPacket(activePlayersDistribution,
								kpResumeLevel, 0,activePlayersDistribution, FRandSeed, 0,0);
}

Boolean	CNetManager::ResumeEnabled()
{
	//	Check to see if all the players in 'my' game
	//	have the correct randomKey and are indeed in paused
	//	state. If not, tell the caller that resume is not available.

	short		i;
	
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	if(		playerTable[i]->itsPlayer
			&&	!playerTable[i]->itsPlayer->isOut
			&&
			!(		playerTable[i]->randomKey == FRandSeed
				&&	playerTable[i]->loadingStatus == kLPaused))
		{	return false;
		}
	}
	
	return true;
}

void	CNetManager::StopGame(
	short	newStatus)
{
	short				playerStatus;
	short				slot = itsCommManager->myId;
	CPlayerManager		*thePlayerManager;
	CAbstractPlayer		*thePlayer;
	long				winFrame = 0;
	
	isPlaying = false;
	if(newStatus == kPauseStatus)
	{	playerStatus = kLPaused;
	}
	else
	{	if(newStatus == kNoVehicleStatus)
			playerStatus = kLNoVehicle;
		else
			playerStatus = isConnected ? kLConnected : kLNotConnected;
	}

	thePlayerManager = playerTable[slot];
	thePlayer = thePlayerManager->itsPlayer;
	
	if(thePlayer)
	{	winFrame = thePlayer->winFrame;

		GetDateTime(&gameResult.r.when);
		gameResult.levelTag = itsGame->loadedTag;
		gameResult.directoryTag = itsGame->loadedDirectory;
		thePlayer->FillGameResultRecord(&gameResult);
		
		if(gameResult.r.time >= 0)
		{	itsGame->statusRequest = kWinStatus;
		}
		else
		{	itsGame->statusRequest = kLoseStatus;
		}
	}
	else
	{	//	Something is seriously wrong here.
		itsGame->statusRequest = kAbortStatus;
	}

	itsCommManager->SendPacket(kdEveryone, kpPlayerStatusChange, 0, playerStatus, FRandSeed, sizeof(long), (Ptr)&winFrame);
	
	gApplication->BroadcastCommand(kGameResultAvailableCmd);
}

void	CNetManager::ReceivePlayerStatus(
	short	slotId,
	short	newStatus,
	Fixed	randomKey,
	long	winFrame)
{
	if(slotId >= 0 && slotId < kMaxAvaraPlayers)
	{	playerTable[slotId]->randomKey = randomKey;
		playerTable[slotId]->SetPlayerStatus(newStatus, winFrame);
	}
}

void	CNetManager::AttachPlayers(
	CAbstractPlayer	*playerActorList)
{
	short			i;
	CAbstractPlayer	*nextPlayer;
	Boolean			changedColors = false;
	char			newColors[kMaxAvaraPlayers];
	
	//	Let active player managers choose actors for themselves.
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	short	slot = positionToSlot[i];

		if((1 << slot) & startPlayersDistribution)
		{	playerActorList =
				playerTable[slot]->ChooseActor(playerActorList, kGreenTeam+teamColors[i]);

#ifdef DEBUG_MESSAGE
			if(playerTable[slot]->itsPlayer)
			{	DebugMessage("\PGot Actor");
			}
#endif
		}
	}
	
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	short			slot = positionToSlot[i];
		CPlayerManager	*thePlayerMan;
	
		newColors[slot] = teamColors[i];
		thePlayerMan = playerTable[slot];
		if(((1 << slot) & startPlayersDistribution) && thePlayerMan->itsPlayer == NULL)
		{	if(	playerActorList)						//	Any actors left?
			{	playerActorList = thePlayerMan->TakeAnyActor(playerActorList);
				if(itsCommManager->myId == 0 &&
					thePlayerMan->itsPlayer &&
					thePlayerMan->playerColor != teamColors[slot])
				{	changedColors = true;
					newColors[slot] = thePlayerMan->playerColor-kGreenTeam;
				}
			}
			else
			{	if(thePlayerMan->IncarnateInAnyColor())
				{	changedColors = true;
					newColors[slot] = thePlayerMan->playerColor-kGreenTeam;
				}
					
#ifdef DEBUG_MESSAGE
				DebugMessage("\PAny color");
#endif
				if(thePlayerMan->itsPlayer == NULL && slot == itsCommManager->myId)
				{	long	noWin = -1;
				
					itsCommManager->SendPacket(kdEveryone, kpPlayerStatusChange,
								0,kLNoVehicle,0 , sizeof(long), (Ptr)&noWin);
				}
			}
		}
	}

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	if(playerTable[i]->itsPlayer)
		{	playerTable[i]->SpecialColorControl();
		}
	}

	if(changedColors)
	{	itsCommManager->SendPacket(kdEveryone, kpColorChange, 0,0,0, kMaxAvaraPlayers, newColors);
	}
	
	//	Throw away the rest.
	while(playerActorList)
	{	nextPlayer = (CAbstractPlayer *)playerActorList->nextActor;
		playerActorList->Dispose();
		playerActorList = nextPlayer;
	}
}

void *	CNetManager::OpenFastTrackListener()
{
	FTrackVars				**varSpot;
	void *					theListener;

	asm
	{
		lea		@asmVars, A0
		move.l	A0, varSpot
		lea		@asmListener, A0
		move.l	A0, theListener;
	}

	*varSpot = &fastTrack;
	return (*varSpot == NULL) ? NULL : theListener;

//	DDP Listener code follows. Due to the braindead way that
//	DDP listeners are called, a pointer to variables is stored
//	within the code @asmVars, so this listener can only be
//	instantiated once and the memory pointing to it should not
//	be protected.

/*	Inside Macintosh states:

		Registers on call to DDP socket listener		

		A0	Reserved for internal use by the .MPP driver.
			You must preserve this register until after the ReadRest routine
			has completed execution.	

		A1	Reserved for internal use by the .MPP driver.
			You must preserve this register until after the ReadRest routine
			has completed execution.	

		A2	Pointer to the .MPP driver’s local variables.
			The value at the offset toRHA from the value in the A2 register
			points to the start of the RHA.	

		A3	Pointer to the first byte in the RHA past the DDP header bytes
			(the first byte after the DDP protocol type field).	

		A4	Pointer to the ReadPacket routine. The ReadRest routine starts
			2 bytes after the start of the ReadPacket routine.	

		A5	Free for your use before and until your socket listener calls 
			the ReadRest routine. 	

		D0	Lower byte is the destination socket number of the packet. 	

		D1	Word indicating the number of bytes in the DDP packet left to be
			read (that is, the number of bytes following the DDP header).	

		D2	Free for your use.	

		D3	Free for your use.
		
	AppleTalk.h		SysEqu.h
*/

#define	toRHA	1
	asm
	{
@asmVars		dc.l	0

@asmListener
		move.l	@asmVars,A5			//	A5 points to a record of variables


		move.b	3(A2),D2			//	D2 is the frame type (1 = short, 2 = long)
		subq.b	#1,D2				//	D2 is now true, if the frame is long
									//	Store frame length flag

		move.b	D2,FTrackVars.longFrameFlag(A5)
		beq.s	@shortHeader

@longHeader							//	Long headers may have checksums
		clr.w	D3
		move.b	16(A2),D3			//	Protocol type contains sender id.
		sub.b	#kAvaraDDPProtocolOffset,D3
		move.w	D3,FTrackVars.senderId(A5)

#ifdef DEBUG_FASTTRACK
		lea		FTrackVars.headersCopy(A5),A3
		move.l	(A2),D2
		move.l	D2,(A3)

		move.l	4(A2),D2
		move.l	D2,4(A3)

		move.l	8(A2),D2
		move.l	D2,8(A3)

		move.l	12(A2),D2
		move.l	D2,12(A3)

		move.l	16(A2),D2
		move.l	D2,16(A3)
#endif
		move.w	6(A2),D2			//	Read the checksum that was sent to us
									//	Store it for later use
		move.w	D2,FTrackVars.receivedSum(A5)
		beq.s	@noHeaderChecksum	//	If the checksum is zero, then it should be ignored

		lea		8(A2),A3			//	We have to checksum some of the header data
		clr.l	D2					//	D2 is used to accumulate our checksum

		moveq.l	#8,D3				//	Nine header bytes need to be checksummed (adjust for DBRA)
		move.w	D1,-(sp)
		clr.w	D1					//	Use D0 as a temp variable

@checkheaderloop
		move.b	(A3)+,D1			//	Read a byte
		add.w	D1,D2				//	Add it to checksum
		rol.w	#1,D2				//	Rotate checksum
		dbra	D3,@checkheaderloop	//	Loop until header has been checksummed
									//	Store the header checksum for later use
		move.w	D2,FTrackVars.calcSum(A5)		
		move.w	(sp)+,D1

@noHeaderChecksum
		move.l	10(A2),D2			//	Get sender zone number into D2.high
		lsl.w	#8,D2				//	Shift sender node number into D2.word.high
		move.b	15(A2),D2			//	Place sender socket number into D2.word.low

		bra.s	@headerDone			//	Long header was handled

@shortHeader
		clr.w	D3
		move.b	8(A2),D3			//	Protocol type contains sender id.
		sub.b	#kAvaraDDPProtocolOffset,D3
		move.w	D3,FTrackVars.senderId(A5)

		clr.w	FTrackVars.receivedSum(A5)	//	Short header doesn't have a checksum
		move.l	FTrackVars.addr(A5),D2	//	Short header sender zone number is same as ours
		move.w	2(A2),D2				//	Read sender node number
		move.b	-2(A3),D2				//	Read sender socket number to complete AddrBlock
@headerDone

#ifdef DEBUG_FASTTRACK
		move.l	D2,FTrackVars.fromage(A5)	//	Store it just out of curiosity
#endif
		move.w	FTrackVars.senderId(A5),D3
		cmp.l	FTrackVars.addresses(A5,D3.w*4),D2	//	Compare sender address with our address book.
		bne		@badPacket			//	If they do not match, this packet is not for us.

		lea		FTrackVars.bufSpace(A5),A3	//	Read the rest of the packet into buffer space
		move.w	D1,D3
		move.w	D3,D2			//	Store the packet length for later use.
		jsr		2(A4)			//	Call ReadRest.
		bne		@badData		//	ReadRest reports a problem in the data!
	
		move.l	@asmVars,A0
		tst.w	FTrackVars.receivedSum(A0)	//	Is there a checksum?
		beq		@noDataCheck	//	If not, skip calculation

		move.w	D2,D3			//	Move length to D3
								//	What is the checksum so far?
		move.w	FTrackVars.calcSum(A0),D1
		sub.w	D2,A3			//	Back up to start of buffer

		subq.w	#1,D3			//	Adjust count for DBRA
		bmi.s	@nilRange		//	If count was zero, skip calculation

		clr.w	D0				//	Used as a temporary variable

@checkloop
		move.b	(A3)+,D0		//	Read a byte
		add.w	D0,D1			//	Add to checksum
		rol.w	#1,D1			//	Rotate checksum
		dbra	D3,@checkloop	//	Next byte
		
@nilRange
		tst.w	D1				//	A zero result is a special case
		bne.s	@noCheckAdjust	//	Nonzero checksums are used
		subq.w	#1,D1			//	A zero checksum is replaced with -1
@noCheckAdjust					//	Compare calculated checksum with received
		move.w	D1,FTrackVars.calcSum(A0)
		cmp.w	FTrackVars.receivedSum(A0),D1
		bne.s	@badData		//	Mismatch is bad news, so ignore packet

@noDataCheck					//	The data is assumed to be ok.
		sub.w	D2,A3			//	Back up to start of data buffer.
		move.w	FTrackVars.senderId(A0),D1
		move.l	FTrackVars.frameFuncs(A0,D1.w*4),A1
		move.l	(A3)+,D1
		moveq.l	#FUNCTIONBUFFERS-1,D0
		and.w	D1,D0
		mulu.w	#sizeof(FrameFunction),D0

		add.l	D0,A1
		move.l	D1,FrameFunction.validFrame(A1)

#if 0
		move.l	FTrackVars.topReceivedFramePointer(A0),A2
		cmp.l	(A2),D1
		ble.s	@notHigher
		move.l	D1,(A2)
@notHigher
#endif
		move.b	(A3),D3
		move.w	(A3)+,D0

		clr.l	D1
		add.b	D3,D3
		bcs.s	@noDownData
		move.l	(A3)+,D1		//	ft.down
@noDownData
		move.l	D1,(A1)+
		clr.l	(A1)+

		clr.l	D1
		add.b	D3,D3
		bcs.s	@noUpData
		move.l	(A3)+,D1		//	ft.up
@noUpData
		move.l	D1,(A1)+
		clr.l	(A1)+

		clr.l	D1
		add.b	D3,D3
		bcs.s	@noHeldData
		move.l	(A3)+,D1		//	ft.held
@noHeldData
		move.l	D1,(A1)+
		clr.l	(A1)+

		clr.w	D1
		add.b	D3,D3
		bcs.s	@noMouseV
		move.w	(A3)+,D1
@noMouseV
		move.w	D1,(A1)+

		clr.w	D1
		add.b	D3,D3
		bcs.s	@noMouseH
		move.w	(A3)+,D1
@noMouseH
		move.w	D1,(A1)+
		move.w	D0,(A1)+
		rts

@badPacket
		clr.w	D3
		jsr		2(A4)
@badData
		rts
	}
}

void	CNetManager::InitFastTrack()
{
	short			i;

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	fastTrack.addresses[i].value = 0;
	}
}
void	CNetManager::OpenFastTrack()
{
	OSErr			iErr;
	MPPParamBlock	thePB;
	short			myNode, myNet;
	short			i;
	CAvaraApp		*theApp;
	
	theApp = (CAvaraApp *)gApplication;
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	fastTrack.frameFuncs[i] = playerTable[i]->frameFuncs;
	}

	GetNodeAddress(&myNode, &myNet);
	fastTrack.addr.value = 0;
	
	if(theApp->networkOptions == 1 || theApp->networkOptions == 2)
	{
		MPPOpen();	//	AppleTalk.h
		thePB.DDPlistener = OpenFastTrackListener();
		
		if(theApp->networkOptions == 2)
			thePB.DDPsocket = kAvaraBroadcastDDPSocket;
		else
			thePB.DDPsocket = 0;
	
		if(thePB.DDPlistener)
		{	
			iErr = POpenSkt(&thePB, false);

			if(iErr && theApp->networkOptions == 2)
			{	thePB.DDPsocket = 0;
				iErr = POpenSkt(&thePB, false);
			}

			if(iErr == noErr)
			{	fastTrack.addr.block.aNet = myNet;
				fastTrack.addr.block.aNode = myNode;
				fastTrack.addr.block.aSocket = thePB.DDPsocket;
			}
		}
	}
}

void	CNetManager::CloseFastTrack()
{
	OSErr			iErr;
	MPPParamBlock	thePB;
	short			i;

	if(fastTrack.addr.value)
	{	//	thePB.DDP.csCode = closeSkt;
		thePB.DDPsocket = fastTrack.addr.block.aSocket;
		iErr = PCloseSkt(&thePB, false);
	}

	fastTrack.addr.value = 0;

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	fastTrack.addresses[i].value = 0;
	}
}

void	CNetManager::TestFastTrack()
{
	MPPParamBlock	thePB;
	char			wdsPtr[16];
	char			headerPtr[18];
	StringPtr		dataPtr;
	short			i;
	OSErr			iErr;
	
	dataPtr = playerTable[itsCommManager->myId]->playerName;

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	if(fastTrack.addresses[i].value &&
			fastTrack.addresses[i].value != fastTrack.addr.value)
		{	BuildDDPwds(wdsPtr, headerPtr,
								(Ptr)dataPtr, fastTrack.addresses[i].block,
								itsCommManager->myId, dataPtr[0]+1);

			thePB.DDPwdsPointer = wdsPtr;
			thePB.DDPchecksumFlag = true;
			thePB.DDPsocket = fastTrack.addr.block.aSocket;
			iErr = PWriteDDP(&thePB, false);
		}
	}
}

void	CNetManager::FastTrackDeliver(
	PacketInfo	*outPacket)
{
	MPPParamBlock	thePB;
	WDSElement		wdsTable[6];
	char			headerPtr[18];
	short			i,j;
	short			myId;
	OSErr			iErr;
	AddrBlock		destAddr;
	short			distribution;
	
	if(fastTrack.addr.value)
	{
		distribution = outPacket->distribution;
	
		myId = itsCommManager->myId;

		fastTrack.addresses[myId].value = 0;
		
		thePB.DDPwdsPointer = (Ptr)wdsTable;
		thePB.DDPchecksumFlag = true;
		thePB.DDPsocket = fastTrack.addr.block.aSocket;

		//	First, deliver to all within broadcast range:
		for(i=0;i<kMaxAvaraPlayers;i++)
		{	if(	(distribution & (1<<i))
				&&	fastTrack.addresses[i].block.aSocket == kAvaraBroadcastDDPSocket)
			{	short	j;
			
				destAddr = fastTrack.addresses[i].block;
				destAddr.aNode = 255;	//	Broadcast node address.
				distribution ^= 1<<i;	//	Remove this node from distribution bitmap.
				
				//	Remove others within broadcast range from distribution bitmap
				for(j=i+1;j<kMaxAvaraPlayers;j++)
				{	if(	(distribution & (1<<i))
						&&	fastTrack.addresses[j].block.aSocket == kAvaraBroadcastDDPSocket
						&&	fastTrack.addresses[j].block.aNet == destAddr.aNet)
					{	distribution ^= 1<<j;
					}
				}
				
				BuildDDPwds((Ptr)wdsTable, headerPtr,
							(Ptr)&outPacket->p3, destAddr,
							myId+kAvaraDDPProtocolOffset, sizeof(long));
							
				wdsTable[2].entryLength = 1;
				wdsTable[2].entryPtr = (Ptr)&outPacket->p1;
				wdsTable[3].entryLength = 1;
				wdsTable[3].entryPtr = 1+(Ptr)&outPacket->p2;
				wdsTable[4].entryLength = outPacket->dataLen;
				wdsTable[4].entryPtr = (Ptr)outPacket->dataBuffer;
				wdsTable[5].entryLength = 0;

				iErr = PWriteDDP(&thePB, false);
			}
		}

		//	Then deliver individually:
		for(i=0;i<kMaxAvaraPlayers;i++)
		{	if(	(distribution & (1<<i))
				&&	fastTrack.addresses[i].value)
			{	
				distribution ^= 1<<i;
				BuildDDPwds((Ptr)wdsTable, headerPtr,
							(Ptr)&outPacket->p3, fastTrack.addresses[i].block,
							myId+kAvaraDDPProtocolOffset, sizeof(long));

				wdsTable[2].entryLength = 1;
				wdsTable[2].entryPtr = (Ptr)&outPacket->p1;
				wdsTable[3].entryLength = 1;
				wdsTable[3].entryPtr = 1+(Ptr)&outPacket->p2;
				wdsTable[4].entryLength = outPacket->dataLen;
				wdsTable[4].entryPtr = (Ptr)outPacket->dataBuffer;
				wdsTable[5].entryLength = 0;

				iErr = PWriteDDP(&thePB, false);
			}
		}

		outPacket->distribution = distribution;
	}
}

void	CNetManager::ConfigPlayer(
	short	senderSlot,
	Ptr		configData)
{
	playerTable[senderSlot]->theConfiguration = *(PlayerConfigRecord *)configData;
}

void	CNetManager::DoConfig(
	short	senderSlot)
{
	PlayerConfigRecord *theConfig = &playerTable[senderSlot]->theConfiguration;

	if(playerTable[senderSlot]->itsPlayer)
	{	playerTable[senderSlot]->itsPlayer->ReceiveConfig(theConfig);
	}
	
	if(PermissionQuery(kAllowLatencyBit, 0) || !(activePlayersDistribution & kdServerOnly))
	{	if(itsGame->latencyTolerance < theConfig->latencyTolerance)
			itsGame->latencyTolerance = theConfig->latencyTolerance;
	}
	else
	{	if(senderSlot == 0)
		{	itsGame->latencyTolerance = theConfig->latencyTolerance;
		}
	}
}

void	CNetManager::MugShotRequest(
	short		sendTo,
	long		sendFrom)
{
	CPlayerManager	*myPlayer;
	long			mugSize;
	
	myPlayer = playerTable[itsCommManager->myId];
	
	if(myPlayer->mugPict == NULL)
	{	gApplication->BroadcastCommand(kGiveMugShotCmd);
	}

	mugSize = myPlayer->mugSize;
	if(myPlayer->mugPict && mugSize > sendFrom)
	{	short		i;
		long		sendPoint;
		long		sendLen;

		HLock(myPlayer->mugPict);

		sendPoint = sendFrom;

		for(i = 0; i < kMugShotWindowSize; i++)
		{	sendLen = mugSize - sendPoint;
			if(sendLen > PACKETDATABUFFERSIZE)
				sendLen = PACKETDATABUFFERSIZE;

			if(sendLen > 0)
			{	itsCommManager->SendPacket(1L<<sendTo, kpMugShot, 0,
											sendPoint/PACKETDATABUFFERSIZE, 
											mugSize,
											sendLen,
											(*myPlayer->mugPict)+sendPoint);

				sendPoint += sendLen;
			}
			else break;
		}
		
		HUnlock(myPlayer->mugPict);
	}
}

void	CNetManager::ReceiveMugShot(
	short	fromPlayer,
	short	seqNumber,
	long	totalLength,
	short	dataLen,
	Ptr		dataBuffer)
{
	CPlayerManager	*thePlayer;
	
	thePlayer = playerTable[fromPlayer];
	
	if(seqNumber == 0)
	{	if(thePlayer->mugPict)
		{	SetHandleSize(thePlayer->mugPict, totalLength);
		}
		else
		{	thePlayer->mugPict = NewHandle(totalLength);
		}
		
		thePlayer->mugState = 0;
		thePlayer->mugSize = GetHandleSize(thePlayer->mugPict);
	}
	
	if(totalLength == thePlayer->mugSize)
	{	thePlayer->mugState = seqNumber * PACKETDATABUFFERSIZE;
	
		BlockMoveData(dataBuffer, (*thePlayer->mugPict) + thePlayer->mugState,
									dataLen);
		
		thePlayer->mugState += dataLen;

		if((seqNumber & (kMugShotWindowSize - 1)) == 0)
		{	long	nextRequest;
		
			nextRequest = thePlayer->mugState + (kMugShotWindowSize - 1) * PACKETDATABUFFERSIZE;
			if(nextRequest < totalLength)
			{	itsCommManager->SendPacket(1L<<fromPlayer, kpGetMugShot,
											0, 0, nextRequest, 0, NULL);
			}
		}
		
		if(thePlayer->mugState == totalLength && theRoster)
		{	theRoster->InvalidateArea(kFullMapBox, slotToPosition[fromPlayer]);
		}
	}
}

void	CNetManager::StoreMugShot(
	Handle		mugPict)
{
	short	slot = itsCommManager->myId;
	
	playerTable[slot]->StoreMugShot(mugPict);
}

void	CNetManager::ZapMugShot(
	short	slot)
{
	CPlayerManager	*thePlayer;
	
	if(slot == -1)
	{	slot = itsCommManager->myId;
		itsCommManager->SendPacket(kdEveryone & ~(1 << slot), kpZapMugShot, 0,0,0, 0,0);
	}

	thePlayer = playerTable[slot];
	DisposeHandle(thePlayer->mugPict);
	
	thePlayer->mugPict = NULL;
	thePlayer->mugSize = -1;
	thePlayer->mugState = 0;

	theRoster->InvalidateArea(kFullMapBox, slotToPosition[slot]);
}

Boolean	CNetManager::PermissionQuery(
	short	reason,
	short	index)
{
	short	slot = itsCommManager->myId;
	
	if(slot == 0 && reason != kAllowLatencyBit)
	{	return reason != kAllowKickBit || positionToSlot[index];
	}
	else
	{	if(reason == kFreeColorBit || reason == kAllowOwnColorBit)
		{	return		(serverOptions & (1<<kFreeColorBit))
					||	((serverOptions & (1<<kAllowOwnColorBit))
						&&	positionToSlot[index] == slot);
		}
		else
		{	return (serverOptions & (1<<reason)) != 0;
		}
	}
}

void	CNetManager::ChangedServerOptions(
	short	curOptions)
{
	if(itsCommManager->myId == 0)
	{	itsCommManager->SendPacket(kdEveryone, kpServerOptions, 0, 
									curOptions, 0, 0, NULL);
	}
}

void	CNetManager::NewArrival(
	short	slot)
{
	Str255		bellSoundName;
	Handle		theSound;
	
	gApplication->ReadStringPref(kBellNewArrivalsTag, bellSoundName);
	theSound = GetNamedResource('snd ', bellSoundName);
	
	if(theSound)
	{	gApplication->NotifyUser(theSound);
	}
}

void	CNetManager::ResultsReport(
	Ptr		results)
{
	itsGame->scoreKeeper->ReceiveResults((AvaraScoreRecord *)results);
}

void	CNetManager::Beep()
{
	if(gAsyncBeeper)
	{	Str255		bellSoundName;
		Handle		theSound;
		
		gApplication->ReadStringPref(kBellSoundTag, bellSoundName);
		gAsyncBeeper->PlayNamedBeep(bellSoundName);
	}
}

void	CNetManager::BuildTrackerTags(
	CTracker *tracker)
{
	CAvaraApp		*theApp = (CAvaraApp *)gApplication;
	short			i;
	CPlayerManager	*thePlayer;
	CScoreKeeper	*keeper;
	char			gameStat = ktgsNotLoaded;

	if(itsGame->loadedTag)
	{	if(theApp->directorySpec.name[0])
				tracker->WriteStringTag(ktsLevelDirectory, theApp->directorySpec.name);
		else	tracker->WriteStringTag(ktsLevelDirectory, theApp->appSpec.name);
		
		tracker->WriteStringTag(ktsLevelName, itsGame->loadedLevel);
		
		switch(playerTable[0]->loadingStatus)
		{	case kLActive:
				gameStat = ktgsActive;
				break;
			case kLPaused:
				gameStat = ktgsPaused;
				break;
			default:
				gameStat = ktgsLoaded;
				break;
		}
	}
	
	tracker->WriteCharTag(ktsGameStatus, gameStat);

	keeper = itsGame->scoreKeeper;
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	thePlayer = playerTable[i];
		
		if(thePlayer->loadingStatus != kLNotConnected)
		{	short	lives;

			tracker->WriteStringTagIndexed(kisPlayerNick, i, thePlayer->playerName);
			tracker->WriteLongTagIndexed(kisPlayerLocation, i, *(long *)&thePlayer->globalLocation);
			
			if(thePlayer->itsPlayer)
				lives = thePlayer->itsPlayer->lives;
			else
				lives = keeper->netScores.player[i].lives;

			if(lives >= 0)
			{	if(lives > 99)
					lives = 99;
				tracker->WriteShortTagIndexed(kisPlayerLives, i, lives);
			}
		}
	}
}

void	CNetManager::LoginRefused()
{
	long	thisTime;
	
	thisTime = TickCount();
	if(((unsigned long)thisTime - lastLoginRefusal) > 60*60*4)
	{	lastLoginRefusal = thisTime;
		
		itsGame->infoPanel->MessageLine(kmRefusedLogin, centerAlign);
	}
}