/*
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.
*/

#include "CTabCards.h"
#include "JAMUtil.h"
#include "Palettes.h"

void	CTabCards::ITabCards(
	WindowPtr	aWindow,
	short		resId,
	Rect		*tabRect, 
	short		tFont,
	short		tSize,
	short		foreIndex,
	short		backIndex)
{
	short			totalWidth;
	short			adjust;
	short			i;
	TextSettings	sText;
	GrafPtr			savedPort;
	FontInfo		fInfo;

	GetPort(&savedPort);
	itsWindow = aWindow;
	SetPort(itsWindow);

	listNumber = resId;
	listSize = **(short **)GetResource('STR#', resId);
	if(listSize > kMaxTabCardCount)
	{	listSize = kMaxTabCardCount;
	}
	
	foreInd = foreIndex;
	backInd = backIndex;
	fontNumber = tFont;
	fontSize = tSize;
	
	area = *tabRect;
	
	slant.v = area.bottom - area.top;
	slant.h = slant.v >> 1;
	
	GetSetTextSettings(&sText, fontNumber, 0, fontSize, srcCopy);
	GetFontInfo(&fInfo);
	baseLine = fInfo.ascent + fInfo.descent + fInfo.leading;
	if(baseLine)
	{	baseLine = (fInfo.ascent + (fInfo.leading >> 1)) * slant.v / baseLine;
	}
	else
	{	baseLine = slant.v - 4;
	}

	extraWidth = CharWidth(32)*3/2;

	totalWidth = 0;
	
	listTabs[0] = 0;
	for(i=1;i<=listSize;i++)
	{	short		width;
		Str255		tempLabel;

		GetIndString(tempLabel, listNumber, i);
		listTabs[i] = StringWidth(tempLabel) + extraWidth * 2;
		listTabs[i] += listTabs[i-1] + slant.h;
	}
	
	listTabs[i] = listTabs[i-1];
	totalWidth = listTabs[i] + slant.h;
	adjust = area.right - totalWidth - 1;

	for(i=0;i<=listSize;i++)
	{	listTabs[i] += adjust;
	}

	RestoreTextSettings(&sText);
	SetPort(savedPort);
	
	activeTab = 1;
}

void	CTabCards::Draw()
{
	GrafPtr			savedPort;
	TextSettings	sText;
	short			i;
	
	GetPort(&savedPort);
	SetPort(itsWindow);
	GetSetTextSettings(&sText, fontNumber, 0, fontSize, srcCopy);
	
	PmForeColor(foreInd);
	MoveTo(area.left, area.top-1);
	LineTo(listTabs[activeTab-1], area.top-1);
	PmForeColor(backInd);
	MoveTo(listTabs[activeTab-1]+1, area.top-1);
	LineTo(listTabs[activeTab]+slant.h, area.top-1);
	PmForeColor(foreInd);
	MoveTo(listTabs[activeTab]+slant.h, area.top-1);
	LineTo(area.right-1, area.top-1);
		
	for(i=1;i<=listSize;i++)
	{	Str255	tempLabel;
	
		MoveTo(listTabs[i-1] + slant.h + extraWidth,
				area.top + baseLine);
		GetIndString(tempLabel, listNumber, i);
		
		if(i == activeTab)
		{	TextFace(bold+condense);
			DrawString(tempLabel);
			TextFace(0);	
		}
		else
			DrawString(tempLabel);
		
		if(i <= activeTab)
		{	MoveTo(listTabs[i-1], area.top);
		}
		else
		{	MoveTo(listTabs[i-1] + (slant.h >> 1),
					area.top + (slant.v >> 1));
		}

		LineTo(listTabs[i-1] + slant.h, area.bottom);
		LineTo(listTabs[i], area.bottom);

		if(i >= activeTab)
		{	LineTo(listTabs[i] + slant.h, area.top);
		}
		else
		{	LineTo(listTabs[i] + (slant.h >> 1),
					area.top + (slant.v >> 1));
		}
	}
	RestoreTextSettings(&sText);
	SetPort(savedPort);
}

void	CTabCards::SelectTab(
	short		index)
{
	SetPort(itsWindow);

	if(index > 0 && index <= listSize)
	{	Rect	temp;
		short	i;
		short	a,b;
	
		temp.top = area.top;
		temp.bottom = area.bottom;
		
		if(activeTab < index)
		{	a = activeTab;
			b = index;
		}
		else
		{	a = index;
			b = activeTab;
		}
		
		for(i = a; i < b; i++)
		{	temp.left = listTabs[i];
			temp.right = temp.left + slant.h + 1;
			InvalRect(&temp);
		}
		
		temp.left = listTabs[index-1] + slant.h;
		temp.right= listTabs[index];
		InvalRect(&temp);

		temp.left = listTabs[activeTab-1] + slant.h;
		temp.right= listTabs[activeTab];
		InvalRect(&temp);

		temp.bottom = temp.top;
		temp.top--;
		temp.left = listTabs[index-1];
		temp.right= listTabs[index] + slant.h;
		InvalRect(&temp);

		temp.left = listTabs[activeTab-1];
		temp.right= listTabs[activeTab] + slant.h;
		InvalRect(&temp);

		activeTab = index;
	}
}

Boolean	CTabCards::CheckClick(
	Point		where,
	short		index)
{
	if(where.v >= area.top && where.v <= area.bottom)
	{	where.v = slant.v - (where.v - area.top);
		where.v >>= 1;
		
		return (where.h >= listTabs[index - 1] - where.v + slant.h) &&
			   (where.h <= listTabs[index] + where.v);
	}
	else
	{	return false;
	}
}

short	CTabCards::Click(
	EventRecord	*theEvent,
	Point		where)
{
	short	i;

	if(CheckClick(where, activeTab))
	{	return 0;	//	no change
	}
	else
	{	for(i = activeTab - 1; i > 0; i--)
		{	if(CheckClick(where, i))
			{	SelectTab(i);
				
				return i;
			}
		}
		
		for(i = activeTab + 1; i <= listSize; i++)
		{	if(CheckClick(where, i))
			{	SelectTab(i);
				
				return i;
			}
		}
	}
}