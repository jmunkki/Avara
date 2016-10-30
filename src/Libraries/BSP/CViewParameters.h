/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CViewParameters.h
    Created: Thursday, June 30, 1994, 10:45
    Modified: Tuesday, February 20, 1996, 7:19
/*/

#pragma once

#include "CDirectObject.h"
#include "CBSPPart.h"


enum	{	kLightOff, kLightViewCoordinates, kLightGlobalCoordinates };

class	CBSPPart;

class	CViewParameters : public CDirectObject
{
public:
	Vector	fromPoint;
	Vector	atPoint;

	Matrix	viewMatrix;
	Matrix	inverseViewMatrix;

	Point	viewPixelDimensions;
	Point	viewPixelCenter;
	
	Fixed	viewWidth;
	Fixed	viewDistance;
	
	Fixed	xOffset;
	Fixed	yOffset;
	Fixed	screenScale;
	Fixed	fWidth;
	Fixed	fHeight;
	
	Vector	normal[4];	//	Normals for viewing pyramid bounds.
	
	Vector	corners[8];	//	Corners of truncated viewing pyramid.
	
	//	For light sources, only white light sources are currently supported.

	Vector	lightSources[MAXLIGHTS];

	short	lightMode[MAXLIGHTS];
	Vector	internalLights[MAXLIGHTS];			//	Vector length == light source power.

	Fixed	ambientLight;						//	Intensity of ambient (nondirectional) light
	
	Fixed	yonBound;
	Fixed	hitherBound;
	Fixed	horizonBound;

	long	lightSeed;
	short	lightSourceCount;	
	Boolean	dirtyLook;
	Boolean	inverseDone;

	virtual	void	IViewParameters();
	virtual	void	CalculateViewPyramidCorners();
	virtual	void	CalculateViewPyramidNormals();
	virtual	void	RenderPlanes(short numPlanes, short *colorList, Fixed *altitudeList);
	virtual	void	Dispose();
	
	virtual	void	SetLightValues(short n, Fixed dx, Fixed dy, Fixed dz, short mode);
	virtual	void	SetLight(short n, Fixed angle1, Fixed angle2, Fixed intensity, short mode);
	virtual	void	DoLighting();
	
	virtual	void	SetViewRect(short width, short height, short centerX, short centerY);
	virtual	void	Recalculate();
	
	virtual	void	LookFrom(Fixed x, Fixed y, Fixed z);
	virtual	void	LookAt(Fixed x, Fixed y, Fixed z);
	virtual	void	LookFromPart(CBSPPart *thePart);
	virtual	void	LookAtPart(CBSPPart *thePart);
	virtual	void	PointCamera();
	
	virtual	void		SetMatrix(Matrix *aViewMatrix);
	virtual	Matrix *	GetInverseMatrix();
};