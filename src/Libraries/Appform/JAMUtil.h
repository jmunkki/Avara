/*
    Copyright ©1996, Juri Munkki
    All rights reserved.

    File: JAMUtil.h
    Created: Wednesday, April 24, 1996, 1:43
    Modified: Thursday, July 25, 1996, 2:59
*/

typedef struct
{
	short		savedFont;
	short		savedFace;
	short		savedSize;
	short		savedMode;
	FontInfo	newInfo;
	short		nextLine;
} TextSettings;

short	GetSetTextSettings(TextSettings *saveTo, short newFont, short newFace, short newSize, short newMode);
void	RestoreTextSettings(TextSettings *savedSettings);
short	GameTimeToString(long theTime, StringPtr theString);
