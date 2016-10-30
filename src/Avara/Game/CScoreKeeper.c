/*/
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: CScoreKeeper.c
    Created: Sunday, May 5, 1996, 13:11
    Modified: Saturday, August 17, 1996, 0:08
/*/

#include "CScoreKeeper.h"
#include "CAvaraGame.h"
#include "CAvaraApp.h"
#include "CAbstractPlayer.h"
#include "CPlayerManager.h"
#include "CEventHandler.h"
#include "CCompactTagBase.h"
#include "CRosterWindow.h"
#include "Aliases.h"
#include "CNetManager.h"
#include "CommDefs.h"
#include "JAMUtil.h"
#include "CInfoPanel.h"
#include "InfoMessages.h"
#include "Palettes.h"

#define	kScoringStringsList		135

enum
{		//	For personal pages:
		kResultsStr = 1,
		kPersonalScoresStr,
		kTeamScoresStr,
		kScoreRankStr,
		kExitRankStr,

		//	For summary page:
		kPlayerStr,
		kTeamStr,
		kPointsStr,
		kPRStr,
		kXRStr,
		kLivesStr,
		kKillsStr,

		kTeamNames
};

static	short	ExitPointsTable[kMaxAvaraPlayers] = {	10, 6, 3, 2, 1, 0	};

void	CScoreKeeper::IScoreKeeper(
	CAvaraGame	*theGame)
{
	AliasHandle			oldAlias;
	OSErr				result;

	itsGame = theGame;
	resRefNum = 0;
	appResRefNum = CurResFile();
	
	interface.command = 0;
	interface.result = 0;
	interface.capabilities = 0;
	interface.plugIn = NULL;
	interface.maxPlayers = kMaxAvaraPlayers;
	interface.maxTeams = kMaxTeamColors;
	interface.frameTime = itsGame->frameTime;
	interface.frameNumber = -1;

	interface.resultsHandle = GetResource('TEXT', 300);
	DetachResource(interface.resultsHandle);

	interface.resultsChanged = false;
	interface.resultsWindow = ((CAvaraApp *)gApplication)->theRosterWind->itsWindow;
	SetRect(&interface.resultsRect, 0,0, 0,0);
	interface.theEvent = NULL;
	interface.levelName = itsGame->loadedLevel;
	interface.levelName = itsGame->loadedDesigner;
	interface.levelName = itsGame->loadedInfo;
	interface.directoryTag = 0;
	interface.levelTag = 0;
	interface.playerID = 0;
	interface.playerTeam = 0;
	interface.playerLives = 0;
	interface.playerName = 0;
	interface.winFrame = -1;

	interface.scorePoints = 0;
	interface.scoreEnergy = 0;
	interface.scoreReason = 0;
	interface.scoreTeam = 0;
	interface.scoreID = 0;
	
	interface.consoleLine = NULL;
	interface.consoleJustify = centerAlign;
	
	entryPoint = NULL;

	oldAlias = (AliasHandle) gApplication->prefsBase->ReadHandle(kScoreInterfaceFileType);

	if(oldAlias)
	{	Boolean			wasChanged;
		FSSpec			theFile;

		result = ResolveAlias(&gApplication->appSpec, oldAlias, &theFile, &wasChanged);

		if(result == noErr)
		{	result = OpenPlugIn(&theFile);
		}

		DisposeHandle((Handle)oldAlias);
	}
	
	ZeroScores();
		
	netScores = localScores;
}

void	CScoreKeeper::Dispose()
{
	ClosePlugIn();
	
	DisposeHandle(interface.resultsHandle);
	interface.resultsHandle = NULL;

	inherited::Dispose();
}

OSErr	CScoreKeeper::OpenPlugIn(
	FSSpec	*theFile)
{
	short		savedResRef;
	OSErr		result;
	
	ClosePlugIn();

	savedResRef = CurResFile();
	UseResFile(appResRefNum);

	resRefNum = HOpenResFile(theFile->vRefNum, theFile->parID, theFile->name, fsCurPerm);
	result = ResError();
	if(resRefNum)
	{	Handle		appSpec;
		OSErr		iErr;
		AliasHandle	newAlias;
	
		iErr = NewAlias(&gApplication->appSpec, theFile, &newAlias);
		if(iErr == noErr)
		{	gApplication->prefsBase->WriteHandle(kScoreInterfaceFileType, (Handle)newAlias);
			DisposeHandle((Handle) newAlias);
		}
		
		interface.plugIn = GetResource(kScoreInterfaceCodeType, 128);
		result = ResError();
		HLock(interface.plugIn);
		entryPoint = (ScoreInterfaceCallType *)*interface.plugIn;
		
		interface.command = ksiInit;
		
		CallPlugIn();
	}
	
	UseResFile(savedResRef);
	
	return result;
}

void	CScoreKeeper::ClosePlugIn()
{
	interface.command = ksiClose;
	CallPlugIn();

	if(resRefNum)
	{	CloseResFile(resRefNum);
		resRefNum = 0;
	}

	interface.plugIn = NULL;
	entryPoint = NULL;
}

void	CScoreKeeper::CallPlugIn()
{
	short	savedRef;
	
	if(interface.plugIn && entryPoint)
	{
		interface.frameNumber = itsGame->frameNumber;

		savedRef = CurResFile();
		UseResFile(resRefNum);
		entryPoint(&interface);
		UseResFile(savedRef);
		
		if(interface.resultsChanged)
		{	((CAvaraApp *)gApplication)->theRosterWind->InvalidateArea(kCustomResultsBox, 0);
		}
	}
	
	if(interface.consoleLine && itsGame->infoPanel)
	{	itsGame->infoPanel->UnfilteredStringLine(interface.consoleLine, interface.consoleJustify);
		interface.consoleLine = NULL;
	}
}

void	CScoreKeeper::EndScript()
{
	interface.command = ksiLevelLoaded;
	interface.levelName = itsGame->loadedLevel;
	interface.directoryTag = itsGame->loadedTag;
	interface.levelTag = itsGame->loadedDirectory;
	
	CallPlugIn();
}

void	CScoreKeeper::StartResume(
	Boolean		didStart)
{
	if(didStart)
	{	interface.command = ksiLevelStarted;
		ZeroScores();
	}
	else
	{	interface.command = ksiLevelRestarted;
	}

	CallPlugIn();
}

void	CScoreKeeper::PlayerIntros()
{
	short			i;
	CNetManager		*theNet = itsGame->itsNet;
	CPlayerManager	*thePlayer;
		
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	thePlayer = theNet->playerTable[i];
		if(thePlayer->itsPlayer)
		{
			localScores.player[i].lives = thePlayer->itsPlayer->lives;
			localScores.player[i].team = thePlayer->itsPlayer->teamColor;

			interface.playerLives = thePlayer->itsPlayer->lives;
			interface.playerTeam = thePlayer->itsPlayer->teamColor;
			interface.playerID = i;
			interface.command = ksiPlayerIntro;
			interface.winFrame = thePlayer->itsPlayer->winFrame;
			interface.playerName = thePlayer->playerName;
			
			CallPlugIn();
		}
	}
}

void	CScoreKeeper::StopPause(
	Boolean		didPause)
{
	NetResultsUpdate();

	interface.command = didPause ? ksiLevelPaused : ksiLevelEnded;
	
	CallPlugIn();	
}

void	CScoreKeeper::NetResultsUpdate()
{
	CNetManager		*theNet = itsGame->itsNet;
	short			i;
	CPlayerManager	*thePlayer;

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	thePlayer = theNet->playerTable[i];
		if(thePlayer->itsPlayer)
		{	localScores.player[i].lives = thePlayer->itsPlayer->lives;
		}
	}

	theNet->itsCommManager->SendPacket(kdEveryone, kpResultsReport, 0,0,0,
										sizeof(AvaraScoreRecord), (Ptr)&localScores);
										
}

void	CScoreKeeper::Click(
	EventRecord	*theEvent,
	Rect		*theRect)
{
	interface.theEvent = theEvent;
	interface.resultsRect = *theRect;
	interface.command = ksiResultsClick;
	CallPlugIn();
}

Handle	CScoreKeeper::GetCustomText()
{
	interface.command = ksiResultsUpdate;
	CallPlugIn();

	if(interface.resultsChanged)
	{	interface.resultsChanged = false;
		return NULL;
	}
	else
	{	return interface.resultsHandle;
	}
}

void	CScoreKeeper::DrawCustomWindow(
	Rect	*theRect)
{
	interface.resultsRect = *theRect;
	interface.command = ksiResultsDraw;
	CallPlugIn();
}

void	CScoreKeeper::HideShow(
	Boolean	doHide)
{
	interface.command = doHide ? ksiResultsHide : ksiResultsShow;
	CallPlugIn();
}

void	CScoreKeeper::Score(
	ScoreInterfaceReasons	reason,
	short					team,
	short					player,
	long					points,
	Fixed					energy,
	short					hitTeam,
	short					hitPlayer)
{
	interface.command = ksiScore;
	interface.scoreReason = reason;
	interface.playerID = player;
	interface.playerTeam = team;
	if(player >= 0 && player <= kMaxAvaraPlayers)
	{	interface.playerName = itsGame->itsNet->playerTable[player]->playerName;
		if(reason == ksiKillBonus && hitPlayer >= 0 && hitPlayer <= kMaxAvaraPlayers)
		{	Str255		destStr;

			if(hitTeam != team)
			{	localScores.player[player].kills++;
			}
			itsGame->infoPanel->ComposeParamLine(destStr, kmAKilledBPlayer,
											interface.playerName,
											itsGame->itsNet->playerTable[hitPlayer]->playerName);

			interface.consoleLine = destStr;
			interface.consoleJustify = centerAlign;
		}
	}
	else
	{	interface.playerName = NULL;
	}
	
	if(reason == ksiExitBonus)
	{	interface.winFrame = itsGame->frameNumber;
	}
	
	interface.scorePoints = points;
	interface.scoreEnergy = energy;
	interface.scoreTeam = hitTeam;
	interface.scoreID = hitPlayer;
	
	CallPlugIn();
	
	if(player >= 0 && player < kMaxAvaraPlayers)
	{	localScores.player[player].points += points;
		if(reason == ksiExitBonus)
		{	localScores.player[player].exitRank = ExitPointsTable[exitCount];
			exitCount++;
		}
	}
	
	if(team >= 0 && team <= kMaxTeamColors)
	{	localScores.teamPoints[team] += points;
	}
}

void	CScoreKeeper::ZeroScores()
{
	short		i;
	
	exitCount = 0;
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	localScores.player[i].points = 0;
		localScores.player[i].team = 0;
		localScores.player[i].lives = -1;
		localScores.player[i].exitRank = 0;
		localScores.player[i].kills = 0;
	}
	
	for(i=0;i<=kMaxTeamColors;i++)
	{	localScores.teamPoints[i] = 0;
	}
}

void	CScoreKeeper::ReceiveResults(
	AvaraScoreRecord	*newResults)
{
	short		*a, *b;
	long		len;
	
	len = sizeof(AvaraScoreRecord) >> 1;
	a = (short *)newResults;
	b = (short *)&netScores;
	
	while(len--)
	{	if(*a++ != *b++)
		{	break;
		}
	}
	
	if(len >= 0)
	{	CRosterWindow	*theRoster;
	
		netScores = *newResults;
		theRoster = ((CAvaraApp *)gApplication)->theRosterWind;
		
		theRoster->InvalidateArea(kScoreInformationBox, 0);
	}
}

#define	kOnePTabFromRight	64

void	CScoreKeeper::DrawOnePlayerResults(
	short		slot,
	Rect		*toRect)
{
	TextSettings	sText;
	short			y;
	Str255			tempStr;
	short			theTeam = netScores.player[slot].team;
	short			i,j;
	short			exitTotal;
	short			lifeTotal;
	short			scoreRank;
	short			totalScoreRank;
	short			myPoints;
	short			otherPlayers;
	
	if(theTeam)
	{	y = toRect->top + GetSetTextSettings(&sText, geneva, 0, 12, srcOr);
		
		myPoints = netScores.player[slot].points;
		scoreRank = -1;
		otherPlayers = -1;
		for(i=0;i<kMaxAvaraPlayers;i++)
		{	if(netScores.player[i].points >= myPoints)
			{	scoreRank++;
			}
			
			if(netScores.player[i].team != 0)
			{	otherPlayers++;
			}
		}
		scoreRank = ExitPointsTable[scoreRank];

		GetIndString(tempStr, kScoringStringsList, otherPlayers ? kPersonalScoresStr : kResultsStr);
		MoveTo(toRect->left, y);
		DrawString(tempStr);
		
		NumToString(myPoints, tempStr);
		MoveTo(toRect->right - kOnePTabFromRight * 2 - StringWidth(tempStr), y);
		DrawString(tempStr);
	
		if(otherPlayers)
		{	if(netScores.player[slot].exitRank)
			{	NumToString(netScores.player[slot].exitRank, 1+tempStr);
				tempStr[0] = tempStr[1] + 1;
				tempStr[1] = 'x';
				MoveTo(toRect->right - kOnePTabFromRight - 24 - StringWidth(tempStr), y);
				DrawString(tempStr);
			}
			
			MoveTo(toRect->right - kOnePTabFromRight - 20, y);
			NumToString(scoreRank, 1+tempStr);
			tempStr[0] = tempStr[1] + 1;
			tempStr[1] = 's';
			DrawString(tempStr);
		}

		if(netScores.player[slot].lives)
		{	i = netScores.player[slot].lives;
			
			if(i >= 7)
			{	NumToString(i, tempStr);
			}
			else
			{	tempStr[0] = i;
				do
				{	tempStr[i] = '¥';
				} while(--i);
			}
			
			MoveTo(toRect->right - StringWidth(tempStr), y);
			DrawString(tempStr);
		}
#if 0	
		if(otherPlayers)
		{	y += sText.nextLine;
			GetIndString(tempStr, kScoringStringsList, kTeamScoresStr);
			MoveTo(toRect->left, y);
			DrawString(tempStr);
	
			NumToString(netScores.teamPoints[theTeam], tempStr);
			MoveTo(toRect->right - kOnePTabFromRight * 2 - StringWidth(tempStr), y);
			DrawString(tempStr);
	
			exitTotal = 0;
			lifeTotal = 0;
			totalScoreRank = 0;
			for(i=0;i<kMaxAvaraPlayers;i++)
			{	if(theTeam == netScores.player[i].team)
				{	exitTotal += netScores.player[i].exitRank;
					lifeTotal += netScores.player[i].lives;
					
					scoreRank = -1;
					myPoints = netScores.player[i].points;
					for(j = 0;j<kMaxAvaraPlayers;j++)
					{	if(netScores.player[j].team && netScores.player[j].points >= myPoints)
						{	scoreRank++;
						}
					}
					totalScoreRank += ExitPointsTable[scoreRank];
				}
			}
			
			if(exitTotal)
			{	NumToString(exitTotal, 1+tempStr);
				tempStr[0] = tempStr[1] + 1;
				tempStr[1] = 'x';
				MoveTo(toRect->right - kOnePTabFromRight - 24 - StringWidth(tempStr), y);
				DrawString(tempStr);
			}
	
			MoveTo(toRect->right - kOnePTabFromRight - 20, y);
			NumToString(totalScoreRank, 1+tempStr);
			tempStr[0] = tempStr[1] + 1;
			tempStr[1] = 's';
			DrawString(tempStr);
	
			if(lifeTotal)
			{	i = lifeTotal;
	
				if(i >= 7)
				{	NumToString(i, tempStr);
				}
				else
				{	tempStr[0] = i;
					do
					{	tempStr[i] = '¥';
					} while(--i);
				}
				
				MoveTo(toRect->right - StringWidth(tempStr), y);
				DrawString(tempStr);
			}
		}
#endif
	}
}

void	CScoreKeeper::FilterConsoleLine(
	StringPtr	theString,
	short		align)
{
	interface.command = ksiConsoleText;
	interface.consoleLine = theString;
	interface.consoleJustify = align;

	CallPlugIn();
}

enum
{
	kNameTeamSort,
	kPointsSort,
	kPointsRankSort,
	kExitRankSort,
	kLivesSort,
	kKillsSort,
	kNumSortables
};

static
void	SortWithKey(
	short	items,
	short	key,
	long	**sortables)
{
	short	i,j;
	long	value;
	long	**curLine;
	
	for(i=1;i<items;i++)
	{
		curLine = sortables + i;
		value = (*curLine)[key];
		
		for(j = i - 1; j >= 0; j--)
		{	if(sortables[j][key] < value
				|| (	sortables[j][key] == value
					&& 	sortables[j][kPointsSort] < (*curLine)[kPointsSort]))
			{	long	*temp;
			
				temp = *curLine;
				*curLine = sortables[j];
				sortables[j] = temp;
				curLine = sortables + j;
			}
		}
	}
}

#define	kScoreTabSpace	24

static	short	tabsFromRight[] =
	{
		kScoreTabSpace * 6,	//	points
		kScoreTabSpace * 5,	//	score rank
		kScoreTabSpace * 4,	//	exit rank
		kScoreTabSpace * 2,	//	lives
		0					//	kills
	};

void	CScoreKeeper::DrawResultsSummary(
	Rect		*toRect)
{
	short			i,j,pointsRank;
	Rect			divisor;
	long			*sortedRows[kMaxAvaraPlayers];
	long			playerSortables[kMaxAvaraPlayers][kNumSortables];

	long			*sortedTeams[kMaxTeamColors];
	long			teamSortables[kMaxTeamColors][kNumSortables];
	short			y;
	TextSettings	sText;
	CNetManager		*theNet = itsGame->itsNet;
	Str255			tempStr;
	short			sortBy = gApplication->ReadShortPref(kResultsSortOrderTag, kPointsSort);;

	divisor = *toRect;
	divisor.top = (divisor.top + divisor.bottom) >> 1;
	divisor.bottom = divisor.top + 1;

	InsetRect(toRect, 4,1);

	y = toRect->top + GetSetTextSettings(&sText, geneva, 0, 9, srcOr);

	for(i=0;i<kMaxTeamColors;i++)
	{	sortedTeams[i] = teamSortables[i];
		teamSortables[i][kNameTeamSort] = 0;	//	no one here.
		teamSortables[i][kPointsSort] = netScores.teamPoints[i+1];
		teamSortables[i][kPointsRankSort] = 0;
		teamSortables[i][kExitRankSort] = 0;
		teamSortables[i][kLivesSort] = 0;
		teamSortables[i][kKillsSort] = 0;
	}

	for(i=0;i<kMaxAvaraPlayers;i++)
	{	long	myPoints = netScores.player[i].points;
	
		pointsRank = -1;
		for(j=0;j<kMaxAvaraPlayers;j++)
		{	if(netScores.player[j].team && myPoints <= netScores.player[j].points)
			{	pointsRank++;
			}
		}
		
		sortedRows[i] = playerSortables[i];
		playerSortables[i][kNameTeamSort] = i;
		playerSortables[i][kPointsSort] = myPoints;
		playerSortables[i][kPointsRankSort] = ExitPointsTable[pointsRank];
		playerSortables[i][kExitRankSort] = netScores.player[i].exitRank;
		playerSortables[i][kLivesSort] = netScores.player[i].lives;
		playerSortables[i][kKillsSort] = netScores.player[i].kills;
	}
	
	SortWithKey(kMaxAvaraPlayers, sortBy, sortedRows);

	
	GetIndString(tempStr, kScoringStringsList, kPlayerStr);
	MoveTo(toRect->left, y);
	DrawString(tempStr);
	
	for(i=kPointsStr;i<=kKillsStr;i++)
	{	short	width;
		
		GetIndString(tempStr, kScoringStringsList, i);
		width = StringWidth(tempStr);
		MoveTo(toRect->right - tabsFromRight[i-kPointsStr] - width, y);
		if(sortBy == i - kPointsStr + 1)
		{	TextFace(underline);
		}
		
		DrawString(tempStr);
		MoveTo(toRect->right - tabsFromRight[i-kPointsStr] - width, y + divisor.top - toRect->top + 1);
		DrawString(tempStr);
		
		TextFace(0);
	}
	
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	short	ind;
		short	team;
	
		ind = sortedRows[i][kNameTeamSort];
		team = netScores.player[ind].team;
		if(team)
		{	long	*teamInfo = teamSortables[team-1];
			
			teamInfo[kNameTeamSort] = team;
			teamInfo[kPointsRankSort] += sortedRows[i][kPointsRankSort];
			teamInfo[kExitRankSort] += sortedRows[i][kExitRankSort];
			teamInfo[kLivesSort] += sortedRows[i][kLivesSort];
			teamInfo[kKillsSort] += sortedRows[i][kKillsSort];
			
			y += sText.nextLine;
			MoveTo(toRect->left, y);

			MoveTo(toRect->left+1, y+1);
			TextSize(12);
			PmForeColor(kFirstPlayerColor+team-1);
			DrawChar('¥');

			TextSize(9);
			PmForeColor(kStatusTextAndBorderColor);
			DrawChar(' ');
			DrawString(theNet->playerTable[ind]->playerName);
		
			for(j=kPointsSort;j<=kKillsSort;j++)
			{	long	val;
			
				val = sortedRows[i][j];
				
				if(val || j == kPointsSort)
				{	if(j >= kLivesSort && val < 7)
					{	if(val > 0)
						{	tempStr[0] = val;
							while(val)
							{	tempStr[val--] = '¥';
							}
						}
						else
							tempStr[0] = 0;
					}
					else
					{	NumToString(val, tempStr);
					}
					MoveTo(toRect->right - tabsFromRight[j-kPointsSort] - StringWidth(tempStr), y);
					DrawString(tempStr);
				}
			}
		}
	}
	
	y = divisor.top + sText.newInfo.ascent + 1;
	GetIndString(tempStr, kScoringStringsList, kTeamStr);
	MoveTo(toRect->left, y);
	DrawString(tempStr);

	SortWithKey(kMaxTeamColors, sortBy, sortedTeams);

	for(i=0;i<kMaxTeamColors;i++)
	{	short	team;
	
		team = sortedTeams[i][kNameTeamSort];
		if(team)
		{	y += sText.nextLine;

			GetIndString(tempStr, kScoringStringsList, kTeamNames-1 + team);

			MoveTo(toRect->left+1, y+1);
			TextSize(12);
			PmForeColor(kFirstPlayerColor+team-1);
			DrawChar('¥');

			TextSize(9);
			PmForeColor(kStatusTextAndBorderColor);
			DrawString(tempStr);

			for(j=kPointsSort;j<=kKillsSort;j++)
			{	long	val;
			
				val = sortedTeams[i][j];
				
				if(val || j == kPointsSort)
				{	if(j >= kLivesSort && val < 7)
					{	if(val > 0)
						{	tempStr[0] = val;
							while(val)
							{	tempStr[val--] = '¥';
							}
						}
						else
							tempStr[0] = 0;
					}
					else
					{	NumToString(val, tempStr);
					}
					MoveTo(toRect->right - tabsFromRight[j-kPointsSort] - StringWidth(tempStr), y);
					DrawString(tempStr);
				}
			}
		}
	}

	PmForeColor(kShadowGrayColor);
	PaintRect(&divisor);
	PmForeColor(0);
	divisor.top++;
	divisor.bottom++;
	PaintRect(&divisor);
	PmForeColor(kStatusTextAndBorderColor);

}

void	CScoreKeeper::RegularClick(
	EventRecord	*theEvent,
	Rect		*theRect)
{
	short			i;
	Point			where;
	short			sortBy = gApplication->ReadShortPref(kResultsSortOrderTag, kPointsSort);;

	where = theEvent->where;
	GlobalToLocal(&where);

	InsetRect(theRect, 4,1);
	
	for(i = kKillsSort - kPointsSort - 1; i >= 0; i--)
	{	if(where.h > theRect->right - tabsFromRight[i])
		{	break;
		}
	}
	
	i += 2;
	
	if(sortBy != i)
	{	gApplication->WriteShortPref(kResultsSortOrderTag, i);
		InvalRect(theRect);
	}
}
