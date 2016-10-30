/*/
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CommDefs.h
    Created: Thursday, March 2, 1995, 22:27
    Modified: Sunday, September 1, 1996, 10:03
/*/

#pragma once

/*
**	Communications packet commands:
*/
enum	{	kpPacketProtocolRefusal = -9,
			kpPacketProtocolLogin,
			kpPacketProtocolLogout,
			kpPacketProtocolReject,
			kpPacketProtocolTOC,
			kpPacketProtocolControl,	//	Used for misc control information.

			kpKillConnection,
			kpDisconnect,
			kpError,

			kpLogin = 0,
			kpLoginAck,

			kpNameQuery,
			kpNameChange,
			kpColorChange,
			kpOrderChange,
			kpRosterMessage,
			kpPlayerStatusChange,
			
			kpLoadLevel,
			kpLevelLoaded,
			kpLevelLoadErr,
			kpStartLevel,
			kpResumeLevel,
			
			kpReadySynch,
			kpUnavailableSynch,
			kpUnavailableZero,
			kpStartSynch,
			
			kpKeyAndMouse,
			kpKeyAndMouseRequest,
			kpAskLater,
			
			kpKillNet,
			
			kpFastTrack,
			kpRemoveMeFromGame,
			
			kpGetMugShot,
			kpMugShot,
			kpZapMugShot,
			
			kpServerOptions,
			kpNewArrival,
			kpKickClient,

			kpLatencyVote,
			
			kpResultsReport,
			
			kpPing,
			kpRealName,
			
			kpLastCommandNumber
		};

/*
**	Distributions:
*/

#define	kdServerOnly	1
#define	kdEveryone		-1
