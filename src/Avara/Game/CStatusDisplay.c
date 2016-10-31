/*
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: CStatusDisplay.c
    Created: Monday, March 6, 1995, 4:11
    Modified: Tuesday, November 7, 1995, 19:25
*/

#include "CStatusDisplay.h"
#include "AvaraDefines.h"
#include "Palettes.h"
#include "CAvaraGame.h"
#include "InfoMessages.h"

void	CStatusDisplay::IStatusDisplay(
	WindowPtr	theWindow,
	CAvaraGame	*theGame)
{
	short		i;
	FontInfo	fontInfo;
	Str255		temp;
	
	itsWindow = theWindow;
	itsGame = theGame;
	
	currentScore = 0;
	currentTime = 0;
	currentShields = 80;
	currentEnergy = 40;
	
	GetIndString(shieldsLabelString, STATUSSTRINGSLISTID, 4);
	GetIndString(energyLabelString, STATUSSTRINGSLISTID, 3);
	GetIndString(timeLabelString, STATUSSTRINGSLISTID, 2);
	GetIndString(scoreLabelString, STATUSSTRINGSLISTID, 1);
	
	
	for(i=0;i<BUFFEREDLINES;i++)
	{	lineLens[i] = 0;
	}
	
	botLine = 0;
	SetPort(theWindow);
	TextFont(monaco);
	TextSize(9);
	TextFace(0);
	
	GetFontInfo(&fontInfo);
	mWidth = fontInfo.widMax;

	UpdateStatusRects();
	
	GetIndString(temp, STATUSSTRINGSLISTID, kmWelcome1);
	StringLine(temp);
	GetIndString(temp, STATUSSTRINGSLISTID, kmWelcome2);
	StringLine(temp);
}

void	CStatusDisplay::UpdateStatusRects()
{
	shieldsRect.right = itsWindow->portRect.right - 2;
	shieldsRect.left = shieldsRect.right - kIndicatorAreaWidth - 1;
	shieldsRect.top = kShieldsTop;
	shieldsRect.bottom = kShieldsTop + kIndicatorBarHeight;
	
	energyRect.right = itsWindow->portRect.right - 2;
	energyRect.left = energyRect.right - kIndicatorAreaWidth - 1;
	energyRect.top = kEnergyTop;
	energyRect.bottom = kEnergyTop + kIndicatorBarHeight;
		
	scoreRect.left = 2;
	scoreRect.right = 1 + scoreRect.left + kIndicatorAreaWidth;
	scoreRect.top = kScoreTop;
	scoreRect.bottom = kScoreTop+kIndicatorBarHeight;
	
	timeRect.left = 2;
	timeRect.right = 1 + scoreRect.left + kIndicatorAreaWidth;
	timeRect.top = kTimeTop;
	timeRect.bottom = kTimeTop+kIndicatorBarHeight;

	textRect.top = 2;
	textRect.bottom = kStatusAreaHeight - 3;
	textRect.left = 5+kIndicatorAreaWidth+kIndicatorLabelWidth;
	textRect.right = energyRect.left - kIndicatorLabelWidth - 2;
	
	charsShown = (textRect.right - textRect.left - kMessageAreaMargin*2) / mWidth;
}

void	CStatusDisplay::DrawScore()
{
	Str31	scoreText;

	NumToString(currentScore, scoreText);

	MoveTo(scoreRect.right - 2 - StringWidth(scoreText), kScoreLine);
	DrawString(scoreText);
	
}

void	CStatusDisplay::DrawTime()
{
	Str31	timeText;
	char	secs[2];
	short	timeTemp = currentTime;
	
	secs[0] = '0' + timeTemp % 10;
	timeTemp /= 10;
	secs[1] = '0' + timeTemp % 6;
	timeTemp /= 60;
	NumToString(timeTemp, timeText);
	timeText[++timeText[0]] = ':';
	timeText[++timeText[0]] = secs[1];
	timeText[++timeText[0]] = secs[0];

	MoveTo(timeRect.right - 2 - StringWidth(timeText), kTimeLine);
	DrawString(timeText);
}

void	CStatusDisplay::DrawStatusBar(
	Rect	*inRect,
	short	value)
{
	Rect	tempRect;

	tempRect = *inRect;
	tempRect.right = tempRect.left + value;
	PmForeColor(kStatusGreenColor);
	PaintRect(&tempRect);

	tempRect.left = tempRect.right;
	tempRect.right++;
	PmForeColor(kStatusTextAndBorderColor);
	PaintRect(&tempRect);	
	
	tempRect.left = tempRect.right;
	tempRect.right = inRect->right;
	PmForeColor(kStatusRedColor);
	PaintRect(&tempRect);	
}

void	CStatusDisplay::DrawMessageArea()
{
	short	i,y,theLine;
	
	y = kFirstStatusLine;
	TextMode(srcOr);
	
	for(i=0;i<SHOWNLINES;i++)
	{	short	theLen;
	
		theLine = botLine + i - SHOWNLINES + 1;
		if(theLine < 0) theLine += BUFFEREDLINES;
		
		MoveTo(textRect.left + kMessageAreaMargin, y);
		theLen = lineLens[theLine];
		if(theLen > charsShown) theLen = charsShown;
		
		DrawText(screenBuffer[theLine], 0, theLen);
		y += kStatusLineHeight;
	}
}

void	CStatusDisplay::SetTextSettings()
{
	PmBackColor(kStatusBackColor);
	PmForeColor(kStatusTextAndBorderColor);
	TextMode(srcCopy);
	TextFont(monaco);
	TextFace(0);
	TextSize(9);
}

void	CStatusDisplay::DrawStatus()
{
	SetPort(itsWindow);
	DrawStatusBar(&shieldsRect, currentShields);
	DrawStatusBar(&energyRect, currentEnergy);
	
	SetTextSettings();
	DrawScore();
	DrawTime();
	DrawMessageArea();
}

void	CStatusDisplay::DrawGameWindowContents()
{
	Rect			statusRect;

	SetPort(itsWindow);
	UpdateStatusRects();
	
	itsGame->RefreshWindow();

	PmForeColor(kStatusBackColor);
	statusRect = itsWindow->portRect;
	statusRect.bottom = kStatusAreaHeight;
	PaintRect(&statusRect);
	
	PmForeColor(kStatusTextAndBorderColor);
	MoveTo(0, kStatusAreaHeight - 1);
	Line(statusRect.right, 0);

	statusRect.bottom -= 1;
	InsetRect(&statusRect, 1, 1);
	FrameRect(&statusRect);

	MoveTo(3+kIndicatorAreaWidth, 2);
	Line(0, kStatusAreaHeight - 5);

	MoveTo(3+kIndicatorAreaWidth+kIndicatorLabelWidth+1, 2);
	Line(0, kStatusAreaHeight - 5);
	
	MoveTo(statusRect.right - 3 - kIndicatorAreaWidth, 2);
	Line(0, kStatusAreaHeight - 5);
	Move(-kIndicatorLabelWidth-1,0);
	Line(0, -kStatusAreaHeight + 5);

	TextFont(monaco);
	TextSize(9);
	TextMode(srcOr);

	MoveTo(energyRect.left-kBarLabelOffset, kEnergyLine);
	DrawString(energyLabelString);

	MoveTo(shieldsRect.left-kBarLabelOffset, kShieldsLine);
	DrawString(shieldsLabelString);

	MoveTo(scoreRect.right + kIndicatorLabelWidth + 2 - kBarLabelOffset, kScoreLine);
	DrawString(scoreLabelString);

	MoveTo(timeRect.right + kIndicatorLabelWidth + 2 - kBarLabelOffset, kTimeLine);
	DrawString(timeLabelString);

	DrawStatus();

#ifdef FUNNYTEXT
	PmForeColor(kStatusTextAndBorderColor);

	TextFont(geneva);
	TextSize(9);
	TextMode(srcOr);
	MoveTo(2+kIndicatorAreaWidth+4+kIndicatorLabelWidth+1,kFirstStatusLine);
	DrawString("\PWelcome to Avara. Copyright ©1994, Juri Munkki");
	MoveTo(2+kIndicatorAreaWidth+4+kIndicatorLabelWidth+1,kSecondStatusLine);
	DrawString("\PAll Rights Reserved");
#endif
}

void	CStatusDisplay::TextLine(
	char	*theText,
	short	len)
{
	char	*dest;
	
	SetPort(itsWindow);
	if(len > MAXLINECHARS) len = MAXLINECHARS;
	botLine++;

	if(botLine >= BUFFEREDLINES) botLine = 0;
	lineLens[botLine] = len;
	
	dest = screenBuffer[botLine];

	while(len--)
	{	*dest++ = *theText++;
	}
	SetTextSettings();
	EraseRect(&textRect);
	DrawMessageArea();
}

void	CStatusDisplay::StringLine(
	StringPtr	theString)
{
	TextLine((char *)theString+1, theString[0]);
}

void	CStatusDisplay::MessageLine(
	short		index)
{
	Str255	temp;
	
	GetIndString(temp, STATUSSTRINGSLISTID, index);
	StringLine(temp);
}
void	CStatusDisplay::SetScore(
	long	newScore,
	Boolean	doDraw)
{
	if(doDraw && currentScore != newScore)
	{
		currentScore = newScore;

		SetTextSettings();
		DrawScore();
	}
	else
		currentScore = newScore;

}

void	CStatusDisplay::SetGameTime(
	long	newTime,
	Boolean	doDraw)
{
	currentTime = newTime;
	if(doDraw)
	{	SetTextSettings();
		DrawTime();
	}
}

short	CStatusDisplay::AdjustIndicator(
	Rect	*theRect,
	short	fromValue,
	short	toValue,
	Boolean	doDraw)
{
	Rect	temp;
	
	if(toValue > kIndicatorAreaWidth)	toValue = kIndicatorAreaWidth;
	else if(toValue < 0)				toValue = 0;

	if(doDraw && toValue != fromValue)
	{	SetPort(itsWindow);
		PmForeColor(kStatusTextAndBorderColor);
		
		temp = *theRect;
	
		temp.left += toValue;
		temp.right = temp.left + 1;
		PaintRect(&temp);
		
		if(toValue < fromValue)
		{	PmForeColor(kStatusRedColor);
			temp.left = temp.right;
			temp.right += fromValue - toValue;
		}
		else
		{	PmForeColor(kStatusGreenColor);
			temp.right = temp.left;
			temp.left += fromValue - toValue;
		}
		
		PaintRect(&temp);
	}
	
	return toValue;
}

void	CStatusDisplay::SetShields(
	short	newShields,
	Boolean	doDraw)
{
	currentShields = AdjustIndicator(&shieldsRect, currentShields, newShields, doDraw);
}
void	CStatusDisplay::SetEnergy(
	short	newEnergy,
	Boolean	doDraw)
{
	currentEnergy = AdjustIndicator(&energyRect, currentEnergy, newEnergy, doDraw);
}
