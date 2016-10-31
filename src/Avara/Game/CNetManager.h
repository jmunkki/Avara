/*
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CNetManager.h
    Created: Monday, May 15, 1995, 22:14
    Modified: Sunday, September 1, 1996, 19:31
*/

#pragma once
#include "CDirectObject.h"
#include "AvaraDefines.h"
#include "KeyFuncs.h"
#include "CCommManager.h"
#include "LevelScoreRecord.h"
#include "PlayerConfig.h"

#define	NULLNETPACKETS		(32+MINIMUMBUFFERRESERVE)
#define	SERVERNETPACKETS	(16*2*kMaxAvaraPlayers)
#define	CLIENTNETPACKETS	(16*kMaxAvaraPlayers)
#define	TCPNETPACKETS		(32*kMaxAvaraPlayers)

#define	kMaxChatMessageBufferLen	12

#define	kMugShotWindowSize		8

#define	kMaxLatencyTolerance	8

enum	{	kNullNet, kServerNet, kClientNet	};

//	Server option bits
enum	{	kAllowLoadBit, kAllowResumeBit, kAllowLatencyBit,
			kAllowPositionBit, kAllowOwnColorBit, kFreeColorBit,
			kAllowKickBit,
			kUseAutoLatencyBit
		};
void	ServerOptionsDialog();

#define	kAllowTeamControlBitMask	((1<<kFreeColorBit)+(1<<kAllowOwnColorBit))
class	CAvaraGame;
class	CCommManager;
class	CPlayerManager;
class	CProtoControl;
class	CRosterWindow;
class	CAbstractPlayer;
class	CTracker;

typedef union
{
	AddrBlock	block;
	long		value;
} BlockAndValue;

//	FastTrack networking (DDP based protocol that supplements the PPCToolbox classes)
typedef struct
{	
	BlockAndValue	addresses[kMaxAvaraPlayers];
	FrameFunction	*frameFuncs[kMaxAvaraPlayers];
	BlockAndValue	addr;

	//	Access to CAvaraGame field:
	long			*topReceivedFramePointer;

	short			calcSum;
	short			receivedSum;
	short			senderId;

	char			bufSpace[586];

#ifdef DEBUG_FASTTRACK
	long			headersCopy[5];
	BlockAndValue	fromage;
	char			debugFlag;
#endif

	Boolean			longFrameFlag;
} FTrackVars;

class	CNetManager : public CDirectObject
{
public:
			CAvaraGame		*itsGame;
			CCommManager	*itsCommManager;
			CProtoControl	*itsProtoControl;
			CRosterWindow	*theRoster;

			short			playerCount;
			CPlayerManager	*playerTable[kMaxAvaraPlayers];

			char			teamColors[kMaxAvaraPlayers];
			char			slotToPosition[kMaxAvaraPlayers];
			char			positionToSlot[kMaxAvaraPlayers];

			short			activePlayersDistribution;
			short			readyPlayers;
			short			unavailablePlayers;
			short			startPlayersDistribution;
			short			totalDistribution;
			short			netStatus;
			CDirectObject	*netOwner;
			short			deadOrDonePlayers;
			Boolean			isConnected;
			Boolean			isPlaying;
			
			short			serverOptions;
			short			loaderSlot;
			
		PlayerConfigRecord	config;
			
			FTrackVars		fastTrack;
		TaggedGameResult	gameResult;
		
			long			fragmentCheck;
			Boolean			fragmentDetected;

			Boolean			autoLatencyEnabled;
			long			localLatencyVote;
			long			autoLatencyVote;
			long			autoLatencyVoteCount;
			short			maxRoundTripLatency;
			short			addOneLatency;
			
			long			lastLoginRefusal;
			
			long			lastMsgTick;
			long			firstMsgTick;
			short			msgBufferLen;
			char			msgBuffer[kMaxChatMessageBufferLen];
			
	virtual	void			INetManager(CAvaraGame *theGame);
	virtual	void			LevelReset();
	virtual	void			Dispose();
	virtual	Boolean			ConfirmNetChange();
	virtual	void			ChangeNet(short netKind, CDirectObject *owner);
	
	virtual	void			ProcessQueue();
	virtual	void			SendRealName(short toSlot);
	virtual	void			RealNameReport(short slot, short regStatus, StringPtr realName);
	virtual	void			NameChange(StringPtr newName);
	virtual	void			RecordNameAndLocation(short slotId, StringPtr theName, short status, Point location);
	
	virtual	void			SwapPositions(short ind1, short ind2);
	virtual	void			PositionsChanged(char *p);

	virtual	void			FlushMessageBuffer();
	virtual	void			BufferMessage(short len, char *c);	
	virtual	void			SendRosterMessage(short len, char *c);
	virtual	void			ReceiveRosterMessage(short slotId, short len, char *c);
	virtual	void			SendColorChange();
	virtual	void			ReceiveColorChange(char *newColors);
	
	virtual	void			DisconnectSome(short mask);
	virtual	void			HandleDisconnect(short slotId, short why);
	
	virtual	void			SendLoadLevel(OSType theTag);
	virtual	void			ReceiveLoadLevel(short senderSlot, void *theDir, OSType theTag);
	virtual	void			LevelLoadStatus(short senderSlot, short crc, OSErr err, OSType theTag);
	
	virtual	void			SendStartCommand();
	virtual	void			SendResumeCommand();
	virtual	Boolean			ResumeEnabled();
	virtual	void			ReceiveStartCommand(short activeDistribution, short fromSlot);
	virtual	void			ReceiveResumeCommand(short activeDistribution, short fromSlot, Fixed randomKey);
	virtual	void			ReceivedUnavailable(short slot, short fromSlot);

	virtual	void			ReceivePlayerStatus(short slotId, short newStatus, Fixed randomKey, long winFrame);

	//	Game loop methods:
	
	virtual	Boolean			GatherPlayers(Boolean isFreshMission);
	virtual	void			UngatherPlayers();
	
	virtual	void			ResumeGame();
	virtual	void			FrameAction();
	virtual	void			AutoLatencyControl(long frameNumber, Boolean didWait);
	virtual	void			ViewControl();
	virtual	void			AttachPlayers(CAbstractPlayer *thePlayer);

	virtual	void			StopGame(short newStatus);
	//	FastTrack networking (DDP based protocol that supplements the PPCToolbox classes)
	
	virtual	void			InitFastTrack();
	virtual	void			OpenFastTrack();
	virtual	void			CloseFastTrack();
	virtual	void *			OpenFastTrackListener();
	virtual	void			FastTrackDeliver(PacketInfo *outPacket);
	virtual	void			TestFastTrack();
	
	virtual	void			ConfigPlayer(short senderSlot, Ptr configData);
	virtual	void			DoConfig(short senderSlot);
	
	virtual	void			StoreMugShot(Handle mugPict);
	virtual	void			MugShotRequest(short sendTo, long sendFrom);
	virtual	void			ReceiveMugShot(short fromPlayer, short seqNumber,
											long totalLength,
											short dataLen,
											Ptr dataBuffer);
	virtual	void			ZapMugShot(short slot);
	
	virtual	Boolean			PermissionQuery(short reason, short index);
	virtual	void			ChangedServerOptions(short curOptions);
	virtual	void			NewArrival(short slot);
	
	virtual	void			ResultsReport(Ptr results);
	
	virtual	void			Beep();
	
	virtual	void			BuildTrackerTags(CTracker *tracker);
	virtual	void			LoginRefused();
};