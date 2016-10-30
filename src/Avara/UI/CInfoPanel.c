/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.
/*/

#include "CInfoPanel.h"
#include "QDOffscreen.h"
#include "CAvaraApp.h"
#include "Palettes.h"
#include "CommandList.h"
#include "FastMat.h"
#include "AvaraDefines.h"
#include "CRosterWindow.h"
#include "CPlayerManager.h"
#include "CNetManager.h"
#include "InfoMessages.h"
#include "CAvaraGame.h"
#include "CScoreKeeper.h"

#define	FIRSTPANELPICT		210
#define	LASTPANELPICT		213
#define	DEFAULTPANELPICT	212

static	Rect		onePixelRect = {0,0,1,1};
static	CInfoPanel	*ip;

		Boolean		GetCurrentColor(RGBColor *theColor, GrafVerb grafVerb);

static	Boolean	GetCurrentColor(
	RGBColor	*theColor,
	GrafVerb	grafVerb)
{
	Boolean		gotColor = false;

	switch(grafVerb)
	{
		case frame:
		case invert:
		case fill:
		case paint:
			gotColor = true;
			GetForeColor(theColor);
			break;
		case erase:
			gotColor = true;
			GetBackColor(theColor);
			break;
	}

	return gotColor;
}

void	SmartUnionRect(
	Rect	*rectA,
	Rect	*rectB)
{
	if(rectB->top >= rectB->bottom ||
		rectB->left >= rectB->right)
	{	*rectB = *rectA;
	}
	else
	{	UnionRect(rectA, rectB, rectB);
	}
}

static	short	gotFlags;

static
pascal	void	SIPStdPoly(
	GrafVerb	grafVerb,
	PolyHandle	thePoly)
{
	RGBColor	currentColor;
	Rect		bounds;
	long		theLongColor;
	
	bounds = (*thePoly)->polyBBox;

	GetCurrentColor(&currentColor, grafVerb);
	theLongColor = RGBToLongColor(&currentColor);

	if(	theLongColor == 0x993300 && !(gotFlags & 1))
	{	//	Red polygon
		gotFlags += 1;
		SmartUnionRect(&bounds, &ip->grafRect);
	}
	else
	if(	theLongColor == 0x6600 && !(gotFlags & 2))
	{	//	Green polygon
		gotFlags += 2;
		SmartUnionRect(&bounds, &ip->grafRect);
	}
	else
	if(	theLongColor == 0x3399 && !(gotFlags & 4))
	{	//	Blue polygon
		gotFlags += 4;
		SmartUnionRect(&bounds, &ip->grafRect);
	}
	else
	{	short		i = -1;
	
		if(theLongColor == 0x660066)		i = 0;
		else if(theLongColor == 0x990066)	i = 1;
		else if(theLongColor == 0xCC0099)	i = 2;
		else if(theLongColor == 0xFF0099)	i = 3;
	
		if(i >= 0)
		{	short	j = ip->indicatorCounts[i];
		
			if(j < kNumIndicatorLights)
			{	ip->indicatorCounts[i]++;
				ip->indicatorPolys[i][j] = thePoly;
				HandToHand((Handle *)&ip->indicatorPolys[i][j]);
			}
		}
		else
		{	StdPoly(grafVerb, thePoly);
		}
	}
}

static	Handle	keyStringHandle;

static
pascal	void	SIPStdText(
	short		byteCount,
	StringPtr	theText,
	Point		numer,
	Point		denom)
{	
	StringPtr	string;
	StringPtr	charp;
	short		tokCount;
	short		ind;
	short		i;
	
	tokCount = *(short *)(*keyStringHandle);
	string = ((StringPtr)*keyStringHandle)+2;
	
	for(ind = 1;ind <= tokCount; ind++)
	{	if(string[0] == byteCount)
		{	charp = string+1;
		
			for(i=0;i<byteCount;i++)
			{	if(*charp++ != theText[i])
				{	break;
				}
			}
			
			if(i==byteCount)
			{	break;
			}
		}
		
		string += string[0] + 1;
	}
	

	if(--ind >= kipKeyCount)
	{	StdText(byteCount, theText, numer, denom);
	}
	else
	{	TextFieldRecord	*tf;
		FontInfo		info;
	
		tf = &ip->textFields[ind];
		GetPen(&tf->location);
		tf->location.h -= ip->contentRect.left;
		tf->location.v -= ip->contentRect.top;

		tf->width = TextWidth(theText, 0, byteCount);
		Move(tf->width, 0);

		tf->txFont = qd.thePort->txFont;
		tf->txFace = qd.thePort->txFace;
		tf->txSize = qd.thePort->txSize;
		
		GetFontInfo(&info);
		
		if(ind >= kipPlayerName1 && ind <= kipPlayerName6)
		{	tf->width += tf->width+info.ascent+info.descent;
		}

		tf->box.top = tf->location.v - info.ascent;
		tf->box.bottom = tf->location.v + info.descent;
		tf->box.left = tf->location.h;
		tf->box.right = tf->location.h + tf->width;
	}
}

static
pascal
void	SIPStdRect(
	GrafVerb verb,
	Rect *r)
{
	if(ip->isFirstRect && (verb == paint || verb == fill || verb == erase))
	{	ip->isFirstRect = false;
		ip->contentRect = *r;
		InsetRect(&ip->contentRect, 1, 1);
	}
	else if(!ip->isFirstRect)
	{	StdRect(verb, r);
	}
}

void	CInfoPanel::KillIndicators()
{
	short	i,j;
	
	for(i=0;i<kNumIndicators;i++)
	{	for(j=0;j<indicatorCounts[i];j++)
		{	KillPoly(indicatorPolys[i][j]);
		}

		indicatorCounts[i] = 0;
	}
}

void	CInfoPanel::IInfoPanel(
	CCommander			*theCommander,
	CWindowCommander	**theList,
	PolyWorld			*basePolyWorld)
{
	RGBColor		aColor;
	PaletteHandle	pal;
	short			i;
	Rect			gdr;

	keyStringHandle = GetResource('STR#', 132);

	inherited::IWindowCommander(theCommander, theList);
	wantsOnWindowsMenu = false;

	itsWindow = GetNewCWindow(131, 0, FrontWindow());

	pal = GetNewPalette(128);
	SetPalette(itsWindow, pal, true);

	AddToList();
	
	LoadMicroText();
	
	for(i=0;i<kBufferedLines;i++)
	{	lineLens[i] = 0;
	}
	topLine = 0;
	
	for(i=0;i<kNumIndicators;i++)
	{	indicatorCounts[i] = 0;
		indicatorValues[i] = kNumIndicatorLights;
	}

	TryPolyBufferSizes(64, 16384, -1, -1);
	InitPolyWorld(&itsPolyWorld, itsWindow,
					&itsWindow->portRect,
					NULL,
					kShareNothing, basePolyWorld);

	SetPolyWorld(&itsPolyWorld);

	GetEntryColor(pal, kAvaraBackColor, &aColor);
	SetPolyWorldBackground(FindPolyColor(RGBToLongColor(&aColor)));

	GetEntryColor(pal, kRedGrafColor, &aColor);
	redIndex = FindPolyColor(RGBToLongColor(&aColor));

	GetEntryColor(pal, kGreenGrafColor, &aColor);
	greenIndex = FindPolyColor(RGBToLongColor(&aColor));

	GetEntryColor(pal, kBlueGrafColor, &aColor);
	blueIndex = FindPolyColor(RGBToLongColor(&aColor));

	buttonsPict = GetPicture(200);
	buttonsHeldPict = GetPicture(201);
	buttonsMaskPict = GetPicture(202);

	templateId = 0;
	artPicture = NULL;
	UsePictTemplate(gApplication->ReadShortPref(kInfoPanelStyleTag, DEFAULTPANELPICT));

	MessageLine(kmWelcome1, centerAlign);
	MessageLine(kmWelcome2, centerAlign);
	MessageLine(kmWelcome3, centerAlign);
	MessageLine(kmWelcome4, centerAlign);

	gdr = (*GetMainDevice())->gdRect;
	MoveWindow(itsWindow,
				(gdr.right + gdr.left - itsWindow->portRect.right) >> 1,
				gdr.bottom - itsWindow->portRect.bottom - 4, false);

	RestoreFromPreferences(kInfoPanelRectTag, kInfoPanelVisTag, true, true);
	
	shields = FIX(1);
	energy = FIX(1);
	guns[0] = FIX(1);
	guns[1] = FIX(1);
	score = 0;
	time = 0;
	lives = 0;
	livesChanged = false;
	scoreChanged = false;
	timeChanged = false;
	
	brightShown = 0;
	LevelReset();
}

void	CInfoPanel::Dispose()
{
	if(artPicture)		KillPicture(artPicture);
	if(buttonsPict)		ReleaseResource((Handle)buttonsPict);
	if(buttonsHeldPict)	ReleaseResource((Handle)buttonsHeldPict);
	if(buttonsMaskPict)	ReleaseResource((Handle)buttonsMaskPict);

	DisposePolyWorld(&itsPolyWorld);

	KillIndicators();
	
	DisposePtr(microTextMap.baseAddr);

	inherited::Dispose();
}

void	CInfoPanel::LevelReset()
{
	short	i;
	
	for(i = 0; i < kNumBrightFrames; i++)
	{	brightColorFrames[i] = 0;
	}
}

void	CInfoPanel::LoadMicroText()
{
	PicHandle	microPic;
	GrafPort	anyPort;
	GrafPtr		savedPort;
	PicHandle	thePic;
	Rect		tempRect;
	long		RAMNeeded;
	
	GetPort(&savedPort);
	OpenPort(&anyPort);
	
	thePic= GetPicture(203);
	tempRect=(*thePic)->picFrame;

	OffsetRect(&tempRect,-tempRect.left,-tempRect.top);
	microTextMap.bounds = tempRect;

	microTextMap.rowBytes = ((tempRect.right + 15) >> 4) << 1;
	RAMNeeded = microTextMap.rowBytes * tempRect.bottom;
	microTextMap.baseAddr = NewPtr(RAMNeeded);
	
	SetPortBits(&microTextMap);
	anyPort.portRect = tempRect;
	RectRgn(anyPort.visRgn, &tempRect);
	RectRgn(anyPort.clipRgn, &tempRect);

	EraseRect(&tempRect);
	DrawPicture(thePic, &tempRect);

	ReleaseResource((Handle)thePic);
	ClosePort(&anyPort);
	SetPort(savedPort);
}

void	CInfoPanel::CopyMicroRect(
	short	offset,
	short	width,
	short	destX,
	short	destY,
	short	tMode)
{
	Rect	fromRect;
	Rect	destRect;
	
	fromRect.left = offset;
	fromRect.right = offset + width;
	fromRect.top = microTextMap.bounds.top;
	fromRect.bottom = microTextMap.bounds.bottom;
	
	destRect.left = destX;
	destRect.right = destX + width;
	destRect.top = destY + microTextMap.bounds.top - microTextMap.bounds.bottom;
	destRect.bottom = destY;
	
	CopyBits(&microTextMap, &qd.thePort->portBits, &fromRect, &destRect, tMode, NULL);
}

void	CInfoPanel::DrawMicroChar(
	short	theChar,
	short	destX,
	short	destY,
	short	tMode)
{
	CopyMicroRect((1+theChar) << 2, 4, destX, destY, tMode);
}


void	CInfoPanel::ReviveWindowRect(
	Rect	*theRect)
{
	theRect->right = theRect->left+itsWindow->portRect.right;
	theRect->bottom = theRect->top+itsWindow->portRect.bottom;
	
	inherited::ReviveWindowRect(theRect);
}

CloseRequestResult	CInfoPanel::CloseRequest(Boolean isQuitting)
{
	if(isQuitting)
	{	SaveIntoPreferences(kInfoPanelRectTag, kInfoPanelVisTag);
		if(templateId != gApplication->ReadShortPref(kInfoPanelStyleTag, DEFAULTPANELPICT))
			gApplication->WriteShortPref(kInfoPanelStyleTag, templateId);
	}

	HideWindow(itsWindow);
	
	return kDidHideDontDispose;
}

void	CInfoPanel::UsePictTemplate(
	short	resId)
{
	PicHandle	thePicture;
	short		i,j;
	
	if(resId != templateId)
	{	thePicture = GetPicture(resId);
		if(thePicture)
		{
			CQDProcs	myQDProcs;
			short		i;
	
			templateId = resId;
			ip = this;
			gotFlags = 0;
			SetRect(&contentRect, 0,0,0,0);
			grafRect = contentRect;
			isFirstRect = true;
			
			for(i=0;i<kipKeyCount;i++)
			{	textFields[i].txSize = 0;
				textFields[i].txFont = 0;
				textFields[i].txFace = 0;
				textFields[i].width = 0;
				SetRect(&textFields[i].box, 0,0,0,0);
				textFields[i].location.h = 0;
				textFields[i].location.v = 0;
				
			}

			KillIndicators();

			SetPort(itsWindow);
			SetStdCProcs(&myQDProcs);
			ClipRect(&((*thePicture)->picFrame));
		
			myQDProcs.polyProc = (void *)SIPStdPoly;
			myQDProcs.textProc = (void *)SIPStdText;
			myQDProcs.rectProc = (void *)SIPStdRect;
		//	myQDProcs.commentProc = (void *)SIPStdComment;
			itsWindow->grafProcs = (void *)&myQDProcs;
			
			if(artPicture)	KillPicture(artPicture);
			artPicture = OpenPicture(&((*thePicture)->picFrame));
			DrawPicture(thePicture,&((*thePicture)->picFrame));
			
			ClosePicture();
			itsWindow->grafProcs = 0;
			
			ReleaseResource((Handle)thePicture);
			
			PmBackColor(kAvaraBackColor);
			SizeWindow(itsWindow, contentRect.right - contentRect.left,
									contentRect.bottom - contentRect.top,
									false);
			InvalRect(&itsWindow->portRect);
			ClipRect(&itsWindow->portRect);
			
			OffsetRect(&grafRect, -contentRect.left, -contentRect.top);
			SetPolyWorld(&itsPolyWorld);
			ResizeRenderingArea(&grafRect);
			
			for(i=0;i<kNumIndicators;i++)
			{	for(j=0;j<indicatorCounts[i];j++)
				{	OffsetPoly(indicatorPolys[i][j], -contentRect.left, -contentRect.top);
				}
			}
		}
	}
}

short	CInfoPanel::HitTestControls(
	Point	thePoint)
{
	GrafPtr		saved;
	GrafPort	bitPort;
	long		bufferSpace = 0;
	
	thePoint.h -= 5;
	thePoint.v -= 5;

	GetPort(&saved);
	OpenPort(&bitPort);
	bitPort.portBits.bounds.left = 0;
	bitPort.portBits.bounds.right = 1;
	bitPort.portBits.bounds.top = 0;
	bitPort.portBits.bounds.bottom = 1;
	bitPort.portBits.rowBytes = 2;
	bitPort.portBits.baseAddr = (Ptr)&bufferSpace;
	bitPort.portRect = bitPort.portBits.bounds;
	SetPort(&bitPort);
	SetOrigin(thePoint.h, thePoint.v);
	
	DrawPicture(buttonsMaskPict, &(*buttonsMaskPict)->picFrame);
	
	ClosePort(&bitPort);
	SetPort(saved);
	
	if(bufferSpace)
	{	if(thePoint.h <= 15)	return 1;
		if(thePoint.h <= 32)	return 2;
		return 3;
	}
	else
	{	return 0;
	}
}

Boolean	CInfoPanel::PrepareTextField(
	short		i)
{
	if(textFields[i].txSize)
	{	MoveTo(textFields[i].location.h, textFields[i].location.v);
		TextFont(textFields[i].txFont);
		TextSize(textFields[i].txSize);
		TextFace(textFields[i].txFace);
		ClipRect(&textFields[i].box);
		
		return true;
	}
	else
		return false;
}

void	CInfoPanel::DrawTime(
	Boolean	doErase,
	Boolean	doValidate)
{
	Str31	timeText;
	char	mins[2];
	char	secs[2];
	short	timeTemp = time;
	
	timeChanged = false;
	if(PrepareTextField(kipTimer))
	{	Rect			*theRect;
	
		secs[0] = '0' + timeTemp % 10;
		timeTemp /= 10;
		secs[1] = '0' + timeTemp % 6;
		timeTemp /= 6;

		mins[0] = '0' + timeTemp % 10;
		timeTemp /= 10;
		mins[1] = '0' + timeTemp % 6;
		timeTemp /= 6;
		
		NumToString(timeTemp, timeText);
		timeText[++timeText[0]] = ':';
		timeText[++timeText[0]] = mins[1];
		timeText[++timeText[0]] = mins[0];
		timeText[++timeText[0]] = ':';
		timeText[++timeText[0]] = secs[1];
		timeText[++timeText[0]] = secs[0];
	
		if(doErase)
		{	PmBackColor(kAvaraBackColor);
			EraseRect(&textFields[kipTimer].box);
		}

		MoveTo(textFields[kipTimer].box.right-StringWidth(timeText),
				textFields[kipTimer].location.v);
		DrawString(timeText);
		
		
		if(doValidate)
		{	ValidRect(&textFields[kipTimer].box);
		}
	}
}

void	CInfoPanel::DrawScore(
	Boolean	doErase,
	Boolean	doValidate)
{
	Str31	scoreText;
	
	scoreChanged = false;
	if(PrepareTextField(kipScore))
	{	Rect			*theRect;
			
		NumToString(score, scoreText);
		
		if(doErase)
		{	PmBackColor(kAvaraBackColor);
			EraseRect(&textFields[kipScore].box);
		}

		MoveTo(textFields[kipScore].box.right-StringWidth(scoreText),
				textFields[kipScore].location.v);
		DrawString(scoreText);
		
		
		if(doValidate)
		{	ValidRect(&textFields[kipScore].box);
		}
	}
}

void	CInfoPanel::DrawLives(
	Boolean	doErase,
	Boolean	doValidate)
{
	Str31	livesText;
	
	livesChanged = false;
	if(PrepareTextField(kipLives))
	{	Rect			*theRect;
			
		NumToString(lives, livesText);
		
		if(doErase)
		{	PmBackColor(kAvaraBackColor);
			EraseRect(&textFields[kipLives].box);
		}

		MoveTo(textFields[kipLives].box.right-StringWidth(livesText),
				textFields[kipLives].location.v);
		DrawString(livesText);
		
		
		if(doValidate)
		{	ValidRect(&textFields[kipLives].box);
		}
	}
}


#define	boxOffsetRight	5

void	CInfoPanel::DrawUserInfoPart(
	short	i,
	short	partList)
{
	CNetManager		*theNet;
	Rect			tempRect;
	char			*colorInd;
	CPlayerManager	*thePlayer;
	short			slot;

	SetPort(itsWindow);
	PmBackColor(kAvaraBackColor);

	theNet = ((CAvaraApp *)gApplication)->gameNet;
	colorInd = theNet->teamColors;

	slot = theNet->positionToSlot[i];
	thePlayer = theNet->playerTable[slot];

	if(partList & (kipDrawName + kipDrawColorBox))
	{	if(thePlayer->playerName[0])
		{	if(PrepareTextField(i+kipPlayerName1))
			{
				Rect		tempRect;
				tempRect = textFields[i+kipPlayerName1].box;
				
				DrawMicroChar(thePlayer->GetStatusChar(),
								tempRect.left, tempRect.top + 6, srcCopy);
				DrawMicroChar(thePlayer->GetMessageIndicator(),
								tempRect.left, tempRect.bottom, srcCopy);
				
				tempRect.left += boxOffsetRight;
				tempRect.right = tempRect.left + tempRect.bottom - tempRect.top;
	
				if(partList & kipDrawColorBox)
				{	if(brightShown & (1<<i))
					{	Rect	inRect;
					
						inRect = tempRect;
						InsetRect(&inRect, 1, 1);
						PmForeColor(kFirstPlayerColor+colorInd[i]);
						FrameRect(&tempRect);
						PmForeColor(kFirstBrightColor + colorInd[i]);
						PaintOval(&inRect);
					}
					else
					{	PmForeColor(kFirstPlayerColor+colorInd[i]);

						PaintRect(&tempRect);
					}
					PmForeColor(kStatusTextAndBorderColor);
					if(kipDrawValidate & partList)	ValidRect(&tempRect);
				}
	
				tempRect.left = tempRect.right + 2;
				tempRect.right = textFields[i+kipPlayerName1].box.right;
					
				if(partList & (kipDrawName + kipDrawErase))
				{	EraseRect(&tempRect);
					if(kipDrawValidate & partList)	ValidRect(&tempRect);
				}
	
				if(partList & kipDrawName)
				{	MoveTo(tempRect.left, textFields[i+kipPlayerName1].location.v);
					//	if(slot == 0)	TextFace(qd.thePort->txFace | underline);
					DrawString(thePlayer->playerName);
				}
			}
		}
		else
		{	tempRect = textFields[i+kipPlayerName1].box;
			if(partList & kipDrawErase)
			{	ClipRect(&tempRect);
				EraseRect(&tempRect);
				if(partList & kipDrawValidate)
				{	ValidRect(&tempRect);
				}
			}
		}
	}

	if((partList & kipDrawMessage) && PrepareTextField(i+kipMessage1))
	{	if(partList & kipDrawErase)
		{	EraseRect(&textFields[i+kipMessage1].box);
			if(partList & kipDrawValidate)
			{	ValidRect(&textFields[i+kipMessage1].box);
			}
		}
	
		if(thePlayer->message[0] <= kMaxInfoMessageLen)
		{	short	width = StringWidth(thePlayer->message);
		
			Move(textFields[i+kipMessage1].width - width,0);
			DrawString(thePlayer->message);
		}
		else
		{	DrawText(thePlayer->message, thePlayer->message[0]+1-kMaxInfoMessageLen, kMaxInfoMessageLen);
		}
	}
}

void	CInfoPanel::DrawTextFields()
{
	short	i;
	
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	DrawUserInfoPart(i, kipDrawName+kipDrawColorBox+kipDrawMessage);
	}
}

void	CInfoPanel::DrawIndicatorPanel(
	short	i)
{
	short	cur;
	short	max;
	
	cur = indicatorValues[i];

	max = indicatorCounts[i];
	if(cur > max)
		cur = max;

	if(cur > 0)
	{	
		ClipRect(&itsWindow->portRect);
		do
		{	cur--;
		
			PmForeColor(kLastIndicatorColor-cur);
			PaintPoly(indicatorPolys[i][cur]);
		} while(cur);
	
		PmForeColor(kStatusTextAndBorderColor);
	}
}

void	CInfoPanel::DrawMessageLine(
	short	i,
	Boolean	doErase,
	Boolean	doValidate)
{
	Rect	*theRect;
	
	if(PrepareTextField(i+kipConsole1))
	{	theRect = &textFields[i+kipConsole1].box;
	
		if(doErase)
		{	PmBackColor(kAvaraBackColor);
			EraseRect(theRect);
		}
	
		if(doValidate)
			ValidRect(theRect);
	
		i = (i+topLine) & (kBufferedLines - 1);
		DrawText(screenBuffer[i], 0, lineLens[i]);
	}
}

void	CInfoPanel::DrawMessageArea()
{
	short	i;
	
	for(i=0;i<kBufferedLines;i++)
	{	DrawMessageLine(i, false, false);
	}
}

extern	Boolean	newWayFlag;

void	CInfoPanel::DrawGrafPanel()
{
	Point	panelSize;
	short	indic;

	panelSize.h = grafRect.right - grafRect.left;
	panelSize.v = grafRect.bottom - grafRect.top;

	SetPort(itsWindow);
	SetPolyWorld(&itsPolyWorld);
	TrackWindow();
	
	indic = FMul(FSqrt(energy), panelSize.v);

	StartPolyLine(0,0, greenIndex);
	AddPolyPoint(0, indic);
	AddPolyPoint(indic, indic);
	EndPolyLine();
	
	indic = FMul(FSqrt(shields), panelSize.v);

	StartPolyLine(panelSize.h, panelSize.v, blueIndex);
	AddPolyPoint(panelSize.h, panelSize.v - indic);
	AddPolyPoint(panelSize.h - indic, panelSize.v - indic);
	EndPolyLine();

	indic = FMul(32768 - (guns[0]>>1), panelSize.v);
	StartPolyLine(indic, indic, redIndex);
	AddPolyPoint(panelSize.h - panelSize.v + indic, indic);

	indic = FMul(32768 + (guns[1]>>1), panelSize.v);
	AddPolyPoint(panelSize.h - panelSize.v + indic, indic);
	AddPolyPoint(indic, indic);
	EndPolyLine();

	if(newWayFlag)
	{	StartPolyLine(0,0, FindPolyColor(0xFFFF00));
		AddPolyPoint(0,8);
		AddPolyPoint(8,8);
		AddPolyPoint(8,0);
		EndPolyLine();
	}
	
	if(gCurrentGame->longWait)
	{	StartPolyLine(4,4, FindPolyColor(0xFFC000));
		AddPolyPoint(4,12);
		AddPolyPoint(12,12);
		AddPolyPoint(12,4);
		EndPolyLine();
	}

	PolygonizeVisRegion(itsWindow->visRgn);
	RenderPolygons();
}
void	CInfoPanel::DrawContents()
{
	RGBColor		blackRGB = {0,0,0};
	RgnHandle		tempRgn;

	ClipRect(&itsWindow->portRect);
	tempRgn = NewRgn();
	OpenRgn();
	FrameRect(&itsWindow->portRect);
	FrameRect(&grafRect);
	CloseRgn(tempRgn);

	PmBackColor(kAvaraBackColor);
	PmForeColor(kStatusTextAndBorderColor);
	EraseRgn(tempRgn);
	DisposeRgn(tempRgn);
	
	if(artPicture)
	{	Rect	drawInto;
		short	i;

		drawInto = (*artPicture)->picFrame;
		OffsetRect(&drawInto, -contentRect.left, -contentRect.top);
		
		DrawPicture(artPicture, &drawInto);
		
		drawInto = (*buttonsPict)->picFrame;
		OffsetRect(&drawInto, 5,5);
		DrawPicture(buttonsPict, &drawInto);
		
		SetPolyWorld(&itsPolyWorld);
		ClearRegions();
		DrawGrafPanel();
		DrawTextFields();
		DrawMessageArea();
		DrawTime(false, false);
		DrawScore(false, false);
		DrawLives(false, false);
		for(i=0;i<kNumIndicators;i++)
		{	DrawIndicatorPanel(i);
		}
	}
}

void	CInfoPanel::CornerClick(
	EventRecord	*theEvent,
	short		what)
{
	Rect	zapRect;
	Rect	buttonsRect;
	short	newWhat;
	Boolean	shownStatus = false;
	Boolean	newStatus;
	Point	where;
	
	buttonsRect = (*buttonsPict)->picFrame;
	OffsetRect(&buttonsRect, 5,5);

	zapRect.top = 5;
	zapRect.left = 5;
	zapRect.right = zapRect.left + 14;
	zapRect.bottom = zapRect.top + 14;
	OffsetRect(&zapRect, ((what > 1) ? 6 : 0) + (what-1) * 12, 0);
	PmBackColor(kAvaraBackColor);
	PmForeColor(kStatusTextAndBorderColor);
	ClipRect(&zapRect);

	do
	{	GetNextEvent(mUpMask+mDownMask, theEvent);
		where = theEvent->where;
		GlobalToLocal(&where);
		
		newWhat = HitTestControls(where);
		newStatus = newWhat == what;
		
		if(newStatus != shownStatus)
		{	
			shownStatus = newStatus;
			PmBackColor(kAvaraBackColor);
			EraseRect(&zapRect);
			DrawPicture(newStatus ? buttonsHeldPict : buttonsPict,
						&buttonsRect);
		}
	} while(theEvent->what != mouseUp);
	
	if(shownStatus)
	{	short	newResId = templateId;
	
		PmBackColor(kAvaraBackColor);
		EraseRect(&zapRect);
		DrawPicture(buttonsPict, &buttonsRect);

		switch(what)
		{	case 1:
				DoCommand(kCloseCmd);
				break;
			case 2:
				if(theEvent->modifiers & (optionKey | cmdKey))
				{	newResId = FIRSTPANELPICT;
				}
				else
				{	newResId--;
					if(newResId < FIRSTPANELPICT)
						newResId = FIRSTPANELPICT;
				}
				UsePictTemplate(newResId);
				break;
			case 3:
				if(theEvent->modifiers & (optionKey | cmdKey))
				{	newResId = LASTPANELPICT;
				}
				else
				{	newResId++;
					if(newResId > LASTPANELPICT)
						newResId = LASTPANELPICT;
				}
				UsePictTemplate(newResId);
				break;
		}
	}

	ClipRect(&itsWindow->portRect);
}

void	CInfoPanel::RawClick(
	WindowPtr	theWind,
	short		partCode,
	EventRecord	*theEvent)
{
	if(theWind == itsWindow)
	{	Point	where;
		short	what;
				
		SetPort(itsWindow);
		where = theEvent->where;
		GlobalToLocal(&where);
		
		what = HitTestControls(where);
		if(what == 0)
		{
			Rect	dragRect;
			GrafPtr	wMgrPort;

			GetWMgrPort(&wMgrPort);
			dragRect = wMgrPort->portRect;
			InsetRect(&dragRect, 4, 4);
			DragWindow(theWind,theEvent->where,&dragRect);
		}
		else
		{	CornerClick(theEvent, what);
		}
	}
	else
	if(nextWindow)
	{	nextWindow->RawClick(theWind, partCode, theEvent);
	}
}

void	CInfoPanel::InvalidateRosterData(
	short	index,
	short	areaCode)
{
	SetPort(itsWindow);

	switch(areaCode)
	{	case kRosterRowBox:
		case kUserBox:
			DrawUserInfoPart(index, kipDrawValidate+kipDrawErase+kipDrawName+kipDrawColorBox+kipDrawMessage);
			break;

		case kNumberBox:
			DrawUserInfoPart(index, kipDrawColorBox);
			break;

		case kUserBoxTopLine:
			DrawUserInfoPart(index, kipDrawValidate+kipDrawErase+kipDrawName+kipDrawColorBox);
			break;
		case kUserBoxBottomLine:
			DrawUserInfoPart(index, kipDrawValidate+kipDrawErase+kipDrawMessage);
			break;
	}
}

void	CInfoPanel::SetTime(
	long	newTime)
{
	if(newTime != time)
	{	time = newTime;
		timeChanged = true;
	}
}


void	CInfoPanel::SetScore(
	long	newScore)
{
	if(newScore != score)
	{	score = newScore;
		scoreChanged = true;
	}
}

void	CInfoPanel::SetIndicatorDisplay(
	short	i,
	short	v)
{
	short	cur;
	short	max;
	GrafPtr	saved;
	
	cur = indicatorValues[i];
	max = indicatorCounts[i];
	indicatorValues[i] = v;

	if(cur < 0) 		cur = 0;
	else if(cur > max)	cur = max;
	
	if(v < 0)			v = 0;
	else if(v > max)	v = max;

	if(v != cur)
	{	GetPort(&saved);
		SetPort(itsWindow);
		ClipRect(&itsWindow->portRect);
		
		if(cur < v)
		{	do
			{	PmForeColor(kLastIndicatorColor-cur);
				PaintPoly(indicatorPolys[i][cur++]);
			} while(cur < v);
			PmForeColor(kStatusTextAndBorderColor);
		}
		else
		{	PmBackColor(kAvaraBackColor);
		
			do
			{	ErasePoly(indicatorPolys[i][--cur]);
			} while(cur > v);
		}
		
		SetPort(saved);
	}
}

void	CInfoPanel::SetLives(
	short	newLives)
{
	if(newLives != lives)
	{	lives = newLives;
		livesChanged = true;
		SetIndicatorDisplay(kliLives, newLives);
	}
}

void	CInfoPanel::UpdateChanged()
{
	short	i;
	short	brightSet;

	if(scoreChanged)
	{	DrawScore(true, false);
	}
	if(livesChanged)
	{	DrawLives(true, false);
	}
	if(timeChanged)
	{	DrawTime(true, false);
	}
	
	brightSet = 0;
	for(i=0; i<kNumBrightFrames; i++)
	{	brightSet |= brightColorFrames[i];
	}
	
	if(brightSet != brightShown)
	{	short	oldBright = brightShown;

		brightShown = brightSet;
		brightSet ^= oldBright;

		for(i=0;i<kMaxAvaraPlayers;i++)
		{	if((1<<i) & brightSet)
				DrawUserInfoPart(i, kipDrawColorBox);
		}
	}
}
void	CInfoPanel::SingleTextLine(
	char	*theText,
	short	len,
	short	align)
{
	short	destLine;
	short	i;
	char	*dest;
	
	SetPort(itsWindow);
	if(len > kMaxLineChars) len = kMaxLineChars;
	topLine = (topLine + 1) & (kBufferedLines - 1);
	destLine = (topLine + kBufferedLines - 1) & (kBufferedLines - 1);
	lineLens[destLine] = len;
	
	dest = screenBuffer[destLine];

	if(len)
	{	if(align)
		{	i = kMaxLineChars - len;
			if(align > 0)
				i >>= 1;

			lineLens[destLine] += i;

			while(i--)
				*dest++ = ' ';
		}

		while(len--)
		{	*dest++ = *theText++;
		}
	}

	for(i=0;i<kBufferedLines;i++)
	{	DrawMessageLine(i, true, false);
	}
}

void	CInfoPanel::TextLine(
	char	*theText,
	short	len,
	short	align)
{
	char	*lastWordBreak;
	char	*lineStart;
	short	lineLen;
	
	lineStart = theText;
	lastWordBreak = lineStart;
	lineLen = 0;
	
	while(len--)
	{	unsigned char	theChar;
	
		theChar = *theText++;
		if(theChar == 13)
		{	SingleTextLine(lineStart, lineLen, align);
			lineLen = 0;
			lineStart = theText;
			lastWordBreak = theText;
		}
		else
		{	lineLen++;

			if(theChar <= 32 || theChar == 'Ê')	//	option space
			{	lastWordBreak = theText;
			}
			
			if(lineLen > kMaxLineChars)
			{	if(lastWordBreak != lineStart)
				{	SingleTextLine(lineStart, lastWordBreak - lineStart - 1, align);
					lineLen -= lastWordBreak - lineStart;
					lineStart = lastWordBreak;
				}
				else
				{	SingleTextLine(lineStart, kMaxLineChars, align);
					lineLen -= kMaxLineChars;
					lineStart += kMaxLineChars;
					lastWordBreak = lineStart;
				}
			}
		}
	}
	
	if(lastWordBreak == theText)
	{	lineLen--;
	}
	
	if(lineLen > 0)
	{	SingleTextLine(lineStart, lineLen, align);
	}
}


void	CInfoPanel::UnfilteredStringLine(
	StringPtr	theString,
	short		align)
{
	TextLine((char *)theString+1, theString[0], align);
}

void	CInfoPanel::StringLine(
	StringPtr	theString,
	short		align)
{
	if(gCurrentGame && gCurrentGame->scoreKeeper)
	{	gCurrentGame->scoreKeeper->FilterConsoleLine(theString, align);
	}
	else
	{	UnfilteredStringLine(theString, align);
	}
}

void	CInfoPanel::MessageLine(
	short		index,
	short		align)
{
	Str255	temp;
	
	GetIndString(temp, STATUSSTRINGSLISTID, index);
	StringLine(temp, align);
}

void	CInfoPanel::NumberLine(
	long		theNum,
	short		align)
{
	Str63	temp;
	
	NumToString(theNum, temp);
	StringLine(temp, align);
}

void	CInfoPanel::ComposeParamLine(
	StringPtr	destStr,
	short		index,
	StringPtr	param1,
	StringPtr	param2)
{
	Str255			sourceStr;
	short			sourceLen, destLen;
	unsigned char	*sourceP;
	unsigned char	*destP;
	
	GetIndString(sourceStr, STATUSSTRINGSLISTID, index);
	
	sourceP = sourceStr;
	sourceLen = *sourceP++;

	destP = destStr + 1;
	destLen = 0;
	
	while(sourceLen-- > 0 && destLen < 256)
	{	unsigned char	theChar;
	
		theChar = *sourceP++;
		if(theChar == (unsigned char)'¸' && (*sourceP == '1' || *sourceP == '2'))
		{	unsigned char	*repStr;
			short			count;
			
			if(*sourceP == '1')
			{	repStr = param1;
			}
			else
			{	repStr = param2;
			}
			
			sourceP++;
			sourceLen--;
			if(repStr)
			{	count = *repStr++;
				while(count-- && destLen < 255)
				{	*destP++ = *repStr++;
					destLen++;
				}
			}
		}
		else
		{	*destP++ = theChar;
			destLen++;
		}
	}
	
	destStr[0] = destLen;
}
	

void	CInfoPanel::ParamLine(
	short		index,
	short		align,
	StringPtr	param1,
	StringPtr	param2)
{
	Str255			destStr;

	ComposeParamLine(destStr, index, param1, param2);
	StringLine(destStr, align);
}

void	CInfoPanel::BrightBox(
	long	frameNum,
	short	position)
{
	if(position >= 0)
	{	brightColorFrames[frameNum & (kNumBrightFrames - 1)] |= 1 << position;
	}
}

void	CInfoPanel::StartFrame(
	long	frameNum)
{
	brightColorFrames[frameNum & (kNumBrightFrames - 1)] = 0;
}