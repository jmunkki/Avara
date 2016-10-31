/*
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CBSPPart.h
    Created: Thursday, June 30, 1994, 10:24
    Modified: Tuesday, May 14, 1996, 18:30
*/

#pragma once
#include "CDirectObject.h"
#include "BSPResStructures.h"
#include "FastMat.h"

#define	MAXLIGHTS	4

//	hither/yon cliplist maximum size:
#define	MAXCROSSPOINTS	32
#define	INDEXSTACKDEPTH	2048	/*	For removal of recursion	*/

class	CBSPPart;
class	CViewParameters;

typedef	struct
{
	short	colorIndex;
	short	visibility;	//	frontVisible or backVisible
} FaceVisibility;

enum
{	noLineClip = 0,
	touchFrontClip = 1,
	touchBackClip = 2,
	lineVisibleClip = 4,
	edgeExitsFlag = 0x8000
};

typedef	union
{
	Fixed	f;
	short	i;
} FixedAndShort;

typedef struct
{
	FixedAndShort	ax;
	FixedAndShort	ay;
	FixedAndShort	bx;
	FixedAndShort	by;
} EdgeCacheRecord;

typedef	struct
{
	FixedAndShort	x;
	FixedAndShort	y;
}	Fixed2DPoint;

void	ClipListHandler(
	short			*countP,
	Fixed2DPoint	*pointList,
	Fixed			x,
	Fixed			y);

/*
**	Special macros for rotations and translations for speed (unfortunately):
*/
#define	TranslatePart(part, dx, dy, dz)	part->itsTransform[3][0] += dx; \
										part->itsTransform[3][1] += dy; \
										part->itsTransform[3][2] += dz

#define	TranslatePartX(part, delta)		part->itsTransform[3][0] += delta
#define	TranslatePartY(part, delta)		part->itsTransform[3][1] += delta
#define	TranslatePartZ(part, delta)		part->itsTransform[3][2] += delta

#define	InitialRotatePartZ(part, angle)	\
	{	Fixed	tempAngle = angle;	\
		part->itsTransform[0][0] = part->itsTransform[1][1] = FOneCos(tempAngle);	\
		part->itsTransform[1][0] = -(part->itsTransform[0][1] = FOneSin(tempAngle));	\
	}

#define	InitialRotatePartY(part, angle)	\
	{	Fixed	tempAngle = angle;	\
		part->itsTransform[0][0] = part->itsTransform[2][2] = FOneCos(tempAngle);	\
		part->itsTransform[0][2] = -(part->itsTransform[2][0] = FOneSin(tempAngle));	\
	}

#define	InitialRotatePartX(part, angle)	\
	{	Fixed	tempAngle = angle;	\
		part->itsTransform[1][1] = part->itsTransform[2][2] = FOneCos(tempAngle);	\
		part->itsTransform[2][1] = -(part->itsTransform[1][2] = FOneSin(tempAngle));	\
	}

#define	NegateTransformRow(part, row)	\
	part->itsTransform[row][0] = -part->itsTransform[row][0];	\
	part->itsTransform[row][1] = -part->itsTransform[row][1];	\
	part->itsTransform[row][2] = -part->itsTransform[row][2];

#define	PreFlipX(part)	NegateTransformRow(part, 1);	NegateTransformRow(part, 2);
#define	PreFlipY(part)	NegateTransformRow(part, 0);	NegateTransformRow(part, 2);
#define	PreFlipZ(part)	NegateTransformRow(part, 0);	NegateTransformRow(part, 1);

class	CBSPPart : public CDirectObject
{
public:	
	Handle				itsBSPResource;
	FaceVisibility		**faceTemp;
	BSPResourceHeader	header;

	CViewParameters		*currentView;

	Handle				colorReplacements;	//	Table of colors that replace defaults.

	Matrix				itsTransform;	//	Transforms to world coordinates.
	Matrix				invGlobTransform;
	Fixed				hither;
	Fixed				yon;

	Fixed				extraAmbient;
	Fixed				privateAmbient;

	//	Bounding volumes:
	Vector				sphereCenter;			//	In view coordinates.
	Vector				sphereGlobCenter;		//	In global coordinates.
	Vector				localViewOrigin;		//	View origin point in local coordinates

	Fixed				bigRadius;				//	Bounding sphere radius from origin of object coordinates
	Fixed				tolerance;

	Fixed				minZ;					//	In view coordinates
	Fixed				maxZ;					//	In view coordinates
	
	Fixed				minX;					//	In screen coordinates.
	Fixed				maxX;
	Fixed				minY;
	Fixed				maxY;
	
	EdgeCacheRecord		**edgeCacheHandle;		//	Also used temporarily for fullTransform and invFullTransform
	EdgeCacheRecord		*edgeCacheTable;
	short				**clippingCacheHandle;
	short				*clippingCacheTable;

	//	members used during rendering:
	FaceVisibility		*faceTable;
	Vector				*pointTable;
	EdgeIndex			*edgeTable;		//	Indices to the following table.
	EdgeRecord			*uniqueEdgeTable;
	PolyRecord			*polyTable;
	
	//	Lighting vectors in object space
	long				lightSeed;				//	Seed value to detect lights change
	Vector				objLights[MAXLIGHTS];	//	Object space lights.

	
	//	Used by a CBSPWorld to score and sort objects:
	short				worldIndex;
	Boolean				worldFlag;
	CBSPPart			*nextTemp;		//	Used temporarily for linked lists.

	Boolean				invGlobDone;
	Boolean				usesPrivateHither;
	Boolean				usesPrivateYon;
	Boolean				isTransparent;
	Boolean				ignoreDirectionalLights;
	short				userFlags;		//	Can be used for various flags by user.

			void		GenerateColorLookupTable();
			void		PostRender();

#ifndef POWERPLUG_CLASS
	virtual	void		IBSPPart(short aResId);
	virtual	void		Initialize();
	virtual	void		BuildBoundingVolumes();
	virtual	void		Dispose();

	virtual	void		ReplaceColor(long origColor, long newColor);
	virtual	void		RepairColorTableSize();
	virtual	void		CacheOneColor(ColorRecord *aColor);
	virtual	void		CacheColors();
	
	virtual	Boolean		PrepareForRenderOne(CViewParameters *vp);
	virtual	Boolean		PrepareForRenderTwo();
	virtual	void		DrawPolygons();
	virtual	void		DrawNumberedPolygons();	//	Internal use only.

	virtual	void		PreRender();

	virtual	void		Render(CViewParameters *vp);
	virtual	Boolean		InViewPyramid();
	
	virtual	void		TransformLights();
	virtual	void		DoLighting();
	virtual	void		ClipEdges();
	virtual	void		MarkNode();
	virtual	void		MarkPoints();

	//	Matrix operations on this object:
	virtual	void		Reset();
	virtual	void		MoveDone();
//	virtual	void		Translate(Fixed xt, Fixed yt, Fixed zt);
	virtual	void		RotateX(Fixed angle);	//	Rotate around X axis by angle degrees.
	virtual	void		RotateY(Fixed angle);
	virtual	void		RotateZ(Fixed angle);
	virtual	void		RotateRadX(Fixed angle);	//	Rotate around X axis by angle (radians).
	virtual	void		RotateRadY(Fixed angle);
	virtual	void		RotateRadZ(Fixed angle);
	virtual	void		RotateOneX(Fixed angle);	//	Rotate around X axis by angle (one-based).
	virtual	void		RotateOneY(Fixed angle);
	virtual	void		RotateOneZ(Fixed angle);

	virtual	void		CopyTransform(Matrix *m);		//	itsTransform = m
	virtual	void		ApplyMatrix(Matrix *m);		//	itsTransform = itsTransform * m
	virtual	void		PrependMatrix(Matrix *m);	//	itsTransform = m * itsTransform
	virtual	Matrix	*	GetInverseTransform();

	//	Compare with another part to see which one is in front:
	virtual	Boolean		Obscures(CBSPPart *other);
	virtual	Boolean		HopeNotObscure(CBSPPart *other, Vector *otherCorners);
	virtual	Boolean		HopeDoesObscure(CBSPPart *other, Vector *otherCorners);
	
	//	Changes the resource!
	virtual	void		FlipNormals(void);
	virtual	void		WriteToDisk(void);
	
	virtual	void		UpdateNormalRecords();
	virtual	void		SetFaceVisibility(short visibilityFlags);

	virtual	void		TempDrawAllFaces(void);
	virtual	void		RestoreFaceDrawing(void);
	virtual	void		AdjustVisibilityFlags(CViewParameters *vp);
	virtual	void		ToggleFace(CViewParameters *vp, Point where);
	
	virtual	void		GetStatistics(long *stats);
	virtual	void		DoStatistics();
#else
			void		DoLightingF();
			void		ClipEdgesF();
#ifdef powerc
			void		MarkNode();
			void		MarkPoints();
			Boolean		PreProcessPartTwoPlug();
#endif
#endif
};