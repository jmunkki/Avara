/*/
    Copyright ©1992-1996, Juri Munkki
    All rights reserved.

    File: PAKit.h
    Created: Sunday, November 22, 1992, 21:12
    Modified: Tuesday, January 2, 1996, 3:51
/*/

#pragma once
#include "PolyColor.h"

#define	MAXEDGES			16384L
#define	DISPLAYREGIONSIZE	(2*65536L)
#define	DELTAREGIONSIZE		(2*65536L)
#define	ACTIVELISTSIZE		2048
#define	DEFAULTMAXHEIGHT	2048
#define	PAKITPOWERPLUGID	128

typedef struct
{
	void				*nextOn;
	void				*nextOff;
	Fixed				dx;
	Fixed				x;
	short				color;
	short				polyID;
}	PolyEdge,*PolyEdgePtr;

struct PolyDevice
{
	Handle				colorMap;
	long				colorTableSeed;
	long				ctSeed;
	short				pixelDepth;
	GDHandle			itsDevice;
	char				*baseAddr;
	short				rowBytes;
	void				*pixelDriver;	//	Really a function pointer. See below for typedef.
	Boolean				needs32bit;

	struct PolyDevice	**nextPolyDevice;
};

typedef struct PolyDevice PolyDevice;

enum	{
	kShareNothing = 0,
	kShareEdgeBuffer = 1,
	kShareDeltaRegions = 2,
	kShareEverything = 1+2
		};

typedef struct
{
	short			backColor;
	short			currentColor;

	short			sharedDataFlags;
	short			polyCount;
	PolyEdge		*edgeBuffer;
	PolyEdge		*newEdge;
	PolyEdge		*endEdge;
	
	PolyEdgePtr		*onLists;
	PolyEdgePtr		*offLists;

	long			displayRegionSize;
	long			deltaRegionSize;
	short			shownRegion;
	short			*displayRegions[2];
	short			*deltaRegion;

	WindowPtr		parentWindow;
	Rect			localBounds;
	Rect			parentRect;
	Boolean			useWindowPortRect;
	
	Rect			bounds;		//	In global coordinates.
	short			xOffset;
	Boolean			shielding;
	short			height;
	short			maxHeight;

	PolyColorTable	*itsPolyColors;
	PolyDevice		**itsPolyDevices;
	
}	PolyWorld;

/*
**	Depending on the processor type used, different versions of
**	a function may be called. To make this easier, function types
**	are defined for these routines:
*/
typedef	void	AddPolySegmentFunctionType(Point p1, Point p2);
		void	AddPolySegment68000(Point p1, Point p2);
		void	AddPolySegment68020(Point p1, Point p2);

typedef	void	PixelDriverFunctionType(
								short			*diffp,
								void			*colorTable,
								short			rowBytes,
								void			*baseAddr,
								short			rowCount,
								short			rightEdge);

/*	Use: User
**	Create and initialize a PolyWorld:
**
**	¥	polys is a pointer to a PolyWorld structure that you have to provide and allocate.
**	Æ	parent is the parent window, if NULL, the polyworld is fixed to absolute coordinates.
**	Æ	bounds is the boundary rectangle within the parent window or in global coordinates.
**	×	aGDevice should be specified when you are NOT drawing to the screens. Otherwise NULL.
**	×	sharedFlags tells which structures (if any) will be shared with the baseWorld.
**	×	baseWorld is another polyworld that you can share data structures with.
**		(Parameters with × are optional you can just pass 0)
**		(You have to pass either one or both of Æ)
*/
void		InitPolyWorld(		PolyWorld		*polys,
								WindowPtr		parent,
								Rect			*bounds,
								GDHandle		aGDevice,
								short			sharedFlags,
								PolyWorld		*baseWorld);

/*	Use: Advanced user.
**	Change buffer sizes that are used when creating a PolyWorld with InitPolyWorld.
**
**	The following routines alters the parameters that are used to
**	initialize a PolyWorld. Don't call this routine if you aren't
**	absolutely sure of what you want to do. If you have plenty of
**	memory and your rendering requirements are about average, you
**	should never need to call it. If you want to save memory and
**	lower the buffer sizes (which can be extremely dangerous if
**	your renderings are complicated), you can call this routine
**	before you call InitPolyWorld. If it returns true, go ahead
**	and call InitPolyWorld. Otherwise you have a problem.
**
**	If your renderings are extremely complicated or you expect very
**	large display sizes (above the default MAXHEIGHT), you can call
**	this routine to increase the buffer sizes that are used for the
**	next InitPolyWorld.
**
**	Pass a negative value for parameters when you want to use the
**	default value.
*/
Boolean	TryPolyBufferSizes(		long	tryMaxEdges,
								long	tryDisplayRegionSize,
								long	tryDeltaRegionSize,
								long	tryMaxHeight);

void	ResetDefaultPolyBufferSizes();

/*	Use: User
**	The PolyWorld equivalents of SetPort and GetPort.
*/
void		SetPolyWorld(			PolyWorld	*polys);
PolyWorld	*GetPolyWorld();

/*	Use: User
**	Call this routine immediately after your event handling
**	routines. If the polyworld is attached to a window and
**	that window has moved, polyworld structures will be updated
**	to reflect the new window position.
*/
void		TrackWindow();

/*	Use: User
**	Resizes the rectangle used for rendering.
*/
void		ResizeRenderingArea(Rect *theArea);

/*	Use: User
**	Sets the background color for the current polyworld.
*/
void		SetPolyWorldBackground(short colorInd);


/*	Use: Internal
**	Creates a run length encoded description of the rendered polygons.
*/
void		ScanConvertPolys68K(PolyWorld		*polys,
								short			*destBuffer,
								PolyEdgePtr		*thisOn,
								PolyEdgePtr		*thisOff,
								Rect			*theBounds,
								short			bgColor);
void		ScanConvertPolysC(	PolyWorld		*polys,
								short			*destBuffer,
								PolyEdgePtr		*thisOn,
								PolyEdgePtr		*thisOff,
								Rect			*theBounds,
								short			bgColor);
typedef
void		ScanConvertPolysFunc(	PolyWorld		*polys,
								short			*destBuffer,
								PolyEdgePtr		*thisOn,
								PolyEdgePtr		*thisOff,
								Rect			*theBounds,
								short			bgColor);
/*	Use: Internal
**	Compares two run length encoded descriptions and outputs
**	the differences between these two.
*/
void		DiffPolyRegions68K(	PolyWorld		*polys,
								short			*oldScreen,
								short			*newScreen,
								short			*diffScreen,
								short			rowCount);
void		DiffPolyRegionsC(	PolyWorld		*polys,
								short			*oldScreen,
								short			*newScreen,
								short			*diffScreen,
								short			rowCount);
typedef
void		DiffPolyRegionsFunc(	PolyWorld		*polys,
								short			*oldScreen,
								short			*newScreen,
								short			*diffScreen,
								short			rowCount);
/*	Use: Internal
**	Used to clip the run length encoded differences
**	to display (graphics device) boundaries.
*/
short	*ExtractDiff(	short	*source,
						short	*dest,
						short	skipLines,
						short	extractLines,
						short	left,
						short	right,
						short	oldRight);

/*	Use: User
**	Disposes of a PolyWorld. Frees memory that InitPolyWorld allocates.
*/
void		DisposePolyWorld(	PolyWorld		*polys);

/*	Use: Internal & User
**	Effectively clears the screen causing a full update on the next
**	call to RenderPolygons. Called by InitPolyWorld.
*/
void		ClearRegions();

/*	Use: User
**	Renders the polygons on screen drawing only differences from the previous
**	frame.
*/
void		RenderPolygons();

/*	Use: Advanced user
**	Renders the polygons into a an encoded record that is returned. You can
**	use any color numbers for these polygons. This allows you to examine
**	which polygons were actually drawn. Useful for hit testing and face visibility
**	analysis.
*/
short *	VirtualScanConvert(short backColor);

/*	Use: User
**	Instead of drawing the changes, nothing is drawn. This is useful when your
**	animation is lagging behind and you have to skip frames.
*/
void		SkipRendering();

/*	Use: Almost useless
**	Advancest the polygon ID counter and stores a color. Call StartPolyLine instead.
*/
void		NewSpritePolygon(	short			color);

/*	Use: User
**	To draw a polygon, start off with calling StartPolyLine. See PolyColor.h to
**	discover how to request colors. (Think of this as the equivalent of MoveTo)
*/
void		StartPolyLine(		short			h,
								short			v,
								short			color);

/*	Use: User
**	Draw a polygon segment. Can be called only after StartPolyLine!
**	(Think of this as the equivalent of LineTo)
*/
void		AddPolyPoint(		short			h,
								short			v);
/*	Use: User
**	End a polygon. This verifies that the polygon is connected and
**	adds an edge if it isn't.
*/
void		EndPolyLine();

/*	Use: User
**	Adds a polygon that contains one single pixel (h, v).
*/
void		PixelPoly(			short			h,
								short			v,
								short			color);

/*	Use: Internal
**	Occasionally the color environment of a graphics device changes
**	either due to a color table change or change in display depth
**	(obviously the depth change also changes the color table), so
**	the color environment has to be regenerated. This call updates
**	the color information. The routine is automatically called by
**	RenderPolygons.
*/
void		UpdatePolyDevice(	PolyDevice		**aPolyDevice,
								PolyColorTable	*aColorTable);

/*	Use: User
**	Even though PolyWorlds are attached to a fixed point in the
**	global QD space, you usually want to give the impression that
**	they are contained in a window. Call this routine with the
**	thePort set to that window and pass the visRgn as theRegion
**	to add a polygonal clipping path that will be equivalent to
**	the visible region of the window.
*/
void	PolygonizeVisRegion(	RgnHandle		theRegion);

/*	Use: Internal
**	Conversion of non-rectangular regions to polygons.
*/
void		PunchRegionHole(	RgnHandle		theRegion,
								Rect			*theArea,
								short			inversionFlag);

/*	Use: Internal
**	Called to add vertical line segments. Uses its own coordinate
**	system, so is not useful to the user. This coordinate system is
**	set by PolygonizeVisRegion and can not be changed by the user.
*/
void	AddVerticalPolySegment(	short			x,
								short			y1,
								short			y2);

/*	Use: User. 68020 ONLY
**
**	Adds a polygon segment where the coordinates are defined in
**	Fixed point. Used mainly by the 3D libraries. For most 2D work,
**	integer coordinates are sufficient and work (slightly) faster.
*/
PolyEdgePtr	AddFPolySegment(Fixed ax, Fixed ay, Fixed bx, Fixed by);
PolyEdgePtr	AddFPolySegmentC(Fixed ax, Fixed ay, Fixed bx, Fixed by);

/*	Use: User. With AddFPolySegment only.
**
**	AddFPolySegment return a pointer to a polysegment or NULL, if
**	no segment was produced. This routine duplicates the edge
**	parameters into a new record, but uses the current color and
**	polyId. This is extremely convenient when two polygons share
**	an edge.
*/
void		ReplicatePolySegment(PolyEdgePtr theEdge);
void		ReplicatePolySegmentC(PolyEdgePtr theEdge);

/*	Use: Internal
**	Pixel drivers for different screen depths and processor
**	types.
*/
PixelDriverFunctionType		BitmapPixelDriver68K;
PixelDriverFunctionType		BitmapPixelDriver020;
PixelDriverFunctionType		TwoBitPixelDriver;
PixelDriverFunctionType		FourBitPixelDriver;
PixelDriverFunctionType		BytePixelDriver;
PixelDriverFunctionType		BytePixelDriverC;
PixelDriverFunctionType		True24PixelDriver;
PixelDriverFunctionType		True24PixelDriverC;
PixelDriverFunctionType		True16PixelDriver;
PixelDriverFunctionType		True16PixelDriverC;

/*	Use: Internal, user
**	Non-toolbox version of SectRect
*/
short	QSectRect(Rect *srca, Rect *srcb, Rect *dest);

/*	Use: Internal, user
**	Converts a rectangle from local to global coordinates.
*/
void	LocalToGlobalRect(Rect *r);

#ifdef PAKITMAIN
#ifdef THINK_C
	#if __option(a4_globals)
		#define	_INITVALUE_(a)
	#else
		#define	_INITVALUE_(a) = a
	#endif
#else
	#define	_INITVALUE_(a) = a
#endif

AddPolySegmentFunctionType	*AddPolySegment		_INITVALUE_(AddPolySegment68000);
ScanConvertPolysFunc		*ScanConvertPolys	_INITVALUE_(ScanConvertPolys68K);
DiffPolyRegionsFunc			*DiffPolyRegions	_INITVALUE_(DiffPolyRegions68K);
#else

extern	AddPolySegmentFunctionType	*AddPolySegment;
extern	ScanConvertPolysFunc		*ScanConvertPolys;
extern	DiffPolyRegionsFunc			*DiffPolyRegions;

#endif

enum	{	kPowerScanConvert,
			kPowerPADiff,
			kPower8BitDriver,
			kPower16BitDriver,
			kPower24BitDriver,
			kBSPLighting,
			kBSPClipping,
			kBSPDrawPolygons,
			kBSPInitPlug,
			kBSPPreProcess,
			kBSPPreProcessList,
			kBSPDrawPolygonsList
		};
