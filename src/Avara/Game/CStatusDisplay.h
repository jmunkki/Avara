/*/
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: CStatusDisplay.h
    Created: Monday, March 6, 1995, 4:07
    Modified: Tuesday, November 7, 1995, 19:12
/*/

#pragma once

#include "CDirectObject.h"

#define	MAXLINECHARS	80
#define	BUFFEREDLINES	4
#define	SHOWNLINES		2

class	CStatusDisplay : public CDirectObject
{
public:
			char			screenBuffer[BUFFEREDLINES][MAXLINECHARS];
			short			lineLens[BUFFEREDLINES];
			short			botLine;
			short			mWidth;
			short			charsShown;
			
			short			currentShields;
			short			currentEnergy;
			long			currentTime;
			long			currentScore;
			
			Str15			shieldsLabelString;
			Str15			energyLabelString;
			Str15			timeLabelString;
			Str15			scoreLabelString;
			
			Rect			textRect;

			Rect			shieldsRect;
			Rect			energyRect;
			
			Rect			scoreRect;
			Rect			timeRect;
			
			WindowPtr		itsWindow;
	class	CAvaraGame		*itsGame;
	
	virtual	void			IStatusDisplay(WindowPtr theWindow, CAvaraGame *theGame);
	virtual	void			UpdateStatusRects();

	virtual	void			SetTextSettings();
	virtual	void			DrawTime();
	virtual	void			DrawScore();
	virtual	void			DrawMessageArea();
	virtual	void			DrawStatusBar(Rect *inRect, short value);
	virtual	void			DrawStatus();
	virtual	void			DrawGameWindowContents();

	virtual	void			StringLine(StringPtr theString);
	virtual	void			TextLine(char *theText, short len);
	virtual	void			MessageLine(short index);

	virtual	void			SetScore(long newScore, Boolean doDraw);
	virtual	void			SetGameTime(long newTime, Boolean doDraw);
	virtual	void			SetShields(short newShields, Boolean doDraw);
	virtual	void			SetEnergy(short newEnergy, Boolean doDraw);
	
	virtual	short			AdjustIndicator(Rect *theRect, short fromValue, short toValue,
											Boolean doDraw);
};