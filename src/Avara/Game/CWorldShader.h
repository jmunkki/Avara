/*/
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: CWorldShader.h
    Created: Saturday, December 30, 1995, 8:12
    Modified: Saturday, December 30, 1995, 8:38
/*/

#pragma once
#include "CDirectObject.h"

#define	MAXTOTALSHADES	64

class	CAvaraGame;
class	CViewParameters;

class	CWorldShader : public CDirectObject
{
public:
			CAvaraGame	*itsGame;
			
			long		lowSkyColor;
			long		highSkyColor;
			long		groundColor;
			short		skyShadeCount;
			Fixed		lowSkyAltitude;
			Fixed		highSkyAltitude;
			
			short		numShades;
			short		shadeColors[MAXTOTALSHADES];
			Fixed		altitudes[MAXTOTALSHADES];
	
	virtual	void		IWorldShader(CAvaraGame *theGame);
	virtual	void		Reset();
	virtual	void		Apply();
	virtual	void		ShadeWorld(CViewParameters *theView);
};
