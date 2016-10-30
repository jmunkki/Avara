/*
    Copyright ©1992-1994, Juri Munkki
    All rights reserved.

    File: PAMain.c
    Created: Sunday, November 22, 1992, 20:25
    Modified: Wednesday, November 9, 1994, 20:30
*/

#include "PAKit.h"

#define				WINDOWRESID		129

extern Boolean				hasColorQD;
WindowPtr					myWindow;
PolyWorld					myPolyWorld;
PolyColorTable				myPolyColorTable;

void	ColorSetup()
{
	int		i;
	
	InitPolyColorTable(&myPolyColorTable);
	SetPolyColorTable(&myPolyColorTable);

	FindPolyColor(0xFFFFFF);
	FindPolyColor(0xC0C0C0);
	FindPolyColor(0x808080);
	FindPolyColor(0x404040);
	FindPolyColor(0x000000);
	
	for(i=0;i<16;i++)
	{	FindPolyColor(
			((i*17L)<<16)+
			255-(i*17L));
	}
}

void	DoFlipTriangles(
	int		i)
{
	int		j,x,y;
	
	for(j=10;j<150;j+=10)
	{	x = j * 3;
		y = i;
		StartPolyLine(x,y,4+(j/10));
		x = i+j * 2;
		y = 50+i;
		AddPolyPoint(x,y);
		x = j * 4;
		y = 100+i;
		AddPolyPoint(x,y);
		EndPolyLine();
	}

}

void	DoDiamond(
	int		i)
{
	StartPolyLine(50+i,50,3);
	AddPolyPoint(75+i,75);
	AddPolyPoint(50+i,100);
	AddPolyPoint(25+i,75);
	EndPolyLine();

}

void	DoBigTriangle(
	int		i)
{
	StartPolyLine(140+i,i+10,4);
	AddPolyPoint(50+i,100+i/3);
	AddPolyPoint(120,100);
	EndPolyLine();
}

void	DoFades(
	int		i)
{
	Rect	foobie;
	GrafPtr	curPort;
	
	GetPort(&curPort);

	foobie = curPort->portRect;
	
	StartPolyLine(foobie.left+i,foobie.top+i,1);//+(i & 3));
	AddPolyPoint(foobie.right,foobie.top);
	AddPolyPoint(foobie.left,foobie.bottom-i);
	EndPolyLine();
	
	StartPolyLine(foobie.right,foobie.top+i,2);//+(i & 3));
	AddPolyPoint(foobie.left,foobie.bottom);
	AddPolyPoint(foobie.right-i,foobie.bottom-i);
	EndPolyLine();
	
	StartPolyLine(0,i,4);
	AddPolyPoint(0,i+1);
	AddPolyPoint(foobie.right,foobie.bottom-i);
	AddPolyPoint(foobie.right,foobie.bottom-i-1);
	EndPolyLine();
}
void	DoBatman(
	int		i)
{
	StartPolyLine(i,i,4);
	AddPolyPoint(i+i/2+10,i+10);
	AddPolyPoint(i+i+20,i);
	AddPolyPoint(i+i+20,10+i+i/2);
	AddPolyPoint(i,10+i+i/2);
	EndPolyLine();
}
void	DoSmallTriangles(
	int		i)
{
	int		j,k;
	int		x,y;
	
	for(j=0;j<10;j++)
	{	for(k=0;k<5;k++)
		{	x = j * 40+5 + i;
			y = k * 40+5;
			
			StartPolyLine(x,y,((j+k) & 3)+1);
			AddPolyPoint(x,y+35);
			AddPolyPoint(x+35,y+30);
			EndPolyLine();
		}
	}

	StartPolyLine(100,150,-1);
	AddPolyPoint(100,100);
	AddPolyPoint(150,100);
	AddPolyPoint(150,150);
	EndPolyLine();
}

void	mainPAM()
{
	int		i;
	int		j,x,y;
	long	timer;
	Rect	foobie;

	DoInits();
	InitPolyGraphics();
	ColorSetup();

	myWindow = GetNewWindow(WINDOWRESID,0,(WindowPtr)-1);

	SetPort(myWindow);
	PaintRect(&myWindow->portRect);

	foobie = myWindow->portRect;
	//SetRect(&foobie,0,0,320,200);
	LocalToGlobalRect(&foobie);

	InitPolyWorld(&myPolyWorld,myWindow, NULL,NULL, kShareNothing, NULL);
	SetPolyWorld(&myPolyWorld);
	
	timer = TickCount();
	
	for(i=0;i<220;i+=3)
	{
		DoBatman(i);
		DoDiamond(i);
		DoBigTriangle(i);
		DoFlipTriangles(i);
#ifdef MORETESTS
		DoFades(i);
		DoSmallTriangles(i);
		
		StartPolyLine(20,i,4);
		AddPolyPoint(0,50);
		AddPolyPoint(i,50);
		EndPolyLine();
#endif
	
		RenderPolygons();

		if(Button()) break;
	}
	
	timer = TickCount()-timer;
	while(!Button());

	FlushEvents(everyEvent,0);	
}