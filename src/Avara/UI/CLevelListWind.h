/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CLevelListWind.h
    Created: Wednesday, November 30, 1994, 8:50
    Modified: Monday, August 21, 1995, 4:25
/*/

#include "CWindowCommander.h"
#include "LevelScoreRecord.h"

#define	LEVELLISTFILETYPE	'AVAG'

enum	{	kGrenadesPop,
			kMissilesPop,
			kBoostersPop,
			kHullTypePop,
			kNumWeaponPopups
		};

class	CTagBase;
class	CCompactTagBase;
class	CStringDictionary;

class	CLevelListWind : public CWindowCommander
{
public:
			Boolean				isDirty;
			Boolean				hasFile;
			FSSpec				itsFile;
			
			OSType				currentDirectory;
			CTagBase			*levels;
			CCompactTagBase		*directory;
			CCompactTagBase		*misc;
			CStringDictionary	*winnerNames;
			ControlHandle		vScroll;
			ControlHandle		hScroll;

			ControlHandle		netButtons[4];
			Boolean				shownButtons[4];
			
			long				tagCount;
			Handle				sortedTags;

			Rect				levelsRect;
			Str63				infoString[3];
			short				infoWidth[3];
			short				maxInfoWidth;
			Str31				popTitles[kNumWeaponPopups];
			short				popTitleWidths[kNumWeaponPopups];
			short				popSelection[kNumWeaponPopups];
			Handle				popProcHandle;

			short				maxWindowWidth;
			
			Str32				userName;
			Rect				loginBox;
			Rect				loginFrame;
			TEHandle			loginText;
			Boolean				loginEnabled;

	virtual	void				IWindowCommander(CCommander *theCommander,
												CWindowCommander **theList);
	virtual	CloseRequestResult	CloseRequest(Boolean isQuitting);
	virtual	void				Dispose();

	virtual	void				GetRect(Rect *theRect, short partCode, short index);
	virtual	void				InvalLevelByTag(OSType tag);
	virtual	void				DrawDividingLines();
	virtual	void				DrawLoginArea();
	virtual	void				DrawHeaderStrings();
	virtual	void				DrawPops();
	virtual	void				DrawHeaders();
	virtual	void				DrawContents();
	
	virtual	void				ValidateBars();
	virtual	void				ScrollList(Boolean adjustBars, short h, short v);
	virtual	void				AdjustScrollbars();
	virtual	void				AdjustScrollRanges();
	virtual	Boolean				DoGrow(EventRecord *theEvent);
	virtual	Boolean				DoZoom(short partCode, EventRecord *theEvent);
	virtual	void				LevelBoxClick(EventRecord *theEvent, Point where);
	virtual	void				PopBox(short i, EventRecord *theEvent);
	virtual	void				ContentClick(EventRecord *theEvent, Point where);

	virtual	void				PrepareForShow();

	//	CCommander methods:
	virtual	void				AdjustMenus(CEventHandler *master);
	virtual	void				DoCommand(long theCommand);
	virtual	void				DoActivateEvent();
	
	virtual	void				PlayerPaste();
	
	//	Level related stuff:
	virtual	void				CurrentLevelsToDirectory();
	virtual	void				UpdateTagBase(Boolean freshFile);
	virtual	Boolean				CompareTags(long tag1, long tag2);
	virtual	void				SortTags();

	virtual	void				DrawListItem(short i);
	virtual	void				DrawRecordLine(short leftEdge, short baseLine,
												GameResult *theResult, short lineIndex);

	virtual	void				MiscDataToTags();
	virtual	void				TagsToMiscData();
	virtual	Boolean				Save();
	virtual	Boolean				SaveAs();
	virtual	OSErr				OpenFile(FSSpec *theFile);
	virtual	Boolean				AvoidReopen(FSSpec *theFile, Boolean doSelect);
	virtual	Boolean				DoEvent(CEventHandler *master, EventRecord *theEvent);

	virtual	void				UserNameChanged();
	virtual	void				SetButtonVisibility(short ind, Boolean visible);
	virtual	void				UpdateNetButtons();
	
	virtual	void				StoreGameResult();
	virtual	void				GetPlayerConfiguration();
};
