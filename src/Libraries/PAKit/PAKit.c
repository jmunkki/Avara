/*/
    Copyright ©1992-1996, Juri Munkki
    All rights reserved.

    File: PAKit.c
    Created: Thursday, November 26, 1992, 13:52
    Modified: Saturday, February 10, 1996, 18:36
/*/

#define PAKITMAIN

#include "PAKit.h"
#include "PowerPlugs.h"
#include "FastMat.h"
#include <GestaltEqu.h>
#include <QDOffscreen.h>

#define	QD32COMPATIBLE		0x0101

		Boolean				hasColorQD;

static	long	mMaxEdges = MAXEDGES;
static	long	mDisplayRegionSize = DISPLAYREGIONSIZE;
static	long	mDeltaRegionSize = DELTAREGIONSIZE;
static	long	mMaxHeight = DEFAULTMAXHEIGHT;

static	Point	firstPt, lastPt;
PolyWorld		*thePolyWorld;

void	BuildGrayPatterns(
	PolyColorTable	*aColorTable,
	Handle			destTable)
{
	int		i,j;
	short	lightness;
	long	theColor;
	Pattern	pat;
	char	*src,*dest;
	
	SetHandleSize(destTable,aColorTable->polyColorCount * 32L);
	if(MemError() == noErr)
	{	HLock(destTable);
		dest = *destTable;
		
		for(i=0;i<aColorTable->polyColorCount;i++)
		{	theColor = (*aColorTable->colorEntryHandle)[i].color;
			lightness  = (theColor & 0xFF) * 11;		//	Blue (should be 11%)
			lightness += ((theColor>>8) & 0xFF) * 59;	//	Green (should be 59%)
			lightness += ((theColor>>16) & 0xFF) * 30;	//	Red (should be 30%)
			lightness /= 1423;							//	Scale to [0..17] range
			
			GetIndPattern(&pat, 128, 1+lightness);
			src = (char *)&pat;
			
			for(j=0;j<8;j++)
			{	*dest++ = *src;
				*dest++ = *src;
				*dest++ = *src;
				*dest++ = *src++;
			}
		}
		
		HUnlock(destTable);
	}
}

void	Build24BitColorMap(
	PolyColorTable	*aColorTable,
	Handle			destTable)
{
	int		i;
	short	lightness;
	long	theColor;
	long	*dest;
	
	SetHandleSize(destTable,aColorTable->polyColorCount * 4L);
	if(MemError() == noErr)
	{
		HLock(destTable);
		dest = *(long **)destTable;
		
		for(i=0;i<aColorTable->polyColorCount;i++)
		{	*dest++ = 0xffFFff & (*aColorTable->colorEntryHandle)[i].color;
		}
		
		HUnlock(destTable);
	}
}

void	Build16BitColorMap(
	PolyColorTable	*aColorTable,
	Handle			destTable)
{
	int		i;
	short	lightness;
	long	theColor;
	short	*dest;
	
	SetHandleSize(destTable,aColorTable->polyColorCount * 2 * sizeof(short));
	if(MemError() == noErr)
	{	HLock(destTable);
		dest = *(short **)destTable;
		
		for(i=0;i<aColorTable->polyColorCount;i++)
		{	long	c1;
			short	c2;
		
			c1 = (*aColorTable->colorEntryHandle)[i].color;
			c2 = (c1 & 0xF80000) >> (24-10-5);
			c2 += (c1 & 0x00F800) >> (16-5-5);
			c2 += (c1 & 0x0000F8) >> (8-5);
			*dest++ = c2;
			*dest++ = c2;
		}
		
		HUnlock(destTable);
	}
}

void	Build8BitColorMap(
	PolyDevice		**aPolyDevice,
	PolyColorTable	*aColorTable)
{
	GDHandle		savedGD;
	GrafPtr			saved;
	RGBColor		tempColor;
	long			colorA;
	short			i;
	char			*dest;
	Handle			destTable;

	GetPort(&saved);
	savedGD = GetGDevice();

	SetGDevice((*aPolyDevice)->itsDevice);

	destTable = (*aPolyDevice)->colorMap;
	SetHandleSize(destTable,aColorTable->polyColorCount * 8L);
	if(MemError() == noErr)
	{	HLock(destTable);
	
		dest = *destTable;
	
		for(i=0;i<aColorTable->polyColorCount;i++)
		{	RGBColor	friendColor;
			long		colorB;
			long		redDelta, greenDelta, blueDelta;
			long		oneErr, twoErr;
		
			colorA = (*aColorTable->colorEntryHandle)[i].color;
	
			tempColor.red = ((unsigned char *)&colorA)[1];
			tempColor.green = ((unsigned char *)&colorA)[2];
			tempColor.blue = ((unsigned char *)&colorA)[3];
			
			tempColor.red += tempColor.red << 8;
			tempColor.green += tempColor.green << 8;
			tempColor.blue += tempColor.blue << 8;
	
			colorA = Color2Index(&tempColor);
			Index2Color(colorA, &friendColor);
			
			redDelta = friendColor.red - (long)tempColor.red;
			greenDelta = friendColor.green - (long)tempColor.green;
			blueDelta = friendColor.blue - (long)tempColor.blue;
	
			oneErr = FDistanceOverEstimate(redDelta, greenDelta, blueDelta);
			
			if(oneErr)
			{	redDelta = tempColor.red - redDelta;
				greenDelta = tempColor.green - greenDelta;
				blueDelta = tempColor.blue - blueDelta;
		
	#define	BOUNDVALUE(a)	((a < 0) ? 0 : ((a > 65536) ? 65535 : a))
		
				friendColor.red = BOUNDVALUE(redDelta);
				friendColor.green = BOUNDVALUE(greenDelta);
				friendColor.blue = BOUNDVALUE(blueDelta);
		
				colorB = Color2Index(&friendColor);
				Index2Color(colorB, &friendColor);
				
				friendColor.red = (friendColor.red + (long)tempColor.red) >> 1;
				friendColor.green = (friendColor.green + (long)tempColor.green) >> 1;
				friendColor.blue = (friendColor.blue + (long)tempColor.blue) >> 1;
		
				redDelta = tempColor.red - (long)friendColor.red;
				greenDelta = tempColor.green - (long)friendColor.green;
				blueDelta = tempColor.blue - (long)friendColor.blue;
		
				twoErr = FDistanceOverEstimate(redDelta, greenDelta, blueDelta);
				
				if(twoErr > oneErr)
				{	colorB = colorA;
				}
			}
			else
			{	colorB = colorA;
			}
	
			//	Form a pattern using the two pixels:
			*dest++ = colorA;	*dest++ = colorB;	*dest++ = colorA;	*dest++ = colorB;
			*dest++ = colorB;	*dest++ = colorA;	*dest++ = colorB;	*dest++ = colorA;
	
		}
	
		HUnlock(destTable);
	}
	SetGDevice(savedGD);
	SetPort(saved);
}

void	BuildPixelPatternMap(
	PolyDevice		**aPolyDevice,
	PolyColorTable	*aColorTable)
{
	GDHandle		savedGD;
	GrafPtr			saved;
	RGBColor		tempColor;
	long			theColor;
	int				i;
	char			*dest;
	CGrafPort		tempPort;
	PixMapPtr		pixels;
	short			width;
	PixPatHandle	colorPat;
	Handle			destTable;

	GetPort(&saved);
	savedGD = GetGDevice();

	SetGDevice((*aPolyDevice)->itsDevice);
	OpenCPort(&tempPort);

	destTable = (*aPolyDevice)->colorMap;
	SetHandleSize(destTable,aColorTable->polyColorCount * 32L);
	if(MemError() == noErr)
	{
		HLock(destTable);
	
		pixels = *(tempPort.portPixMap);
		pixels->baseAddr = StripAddress(*destTable);
		pixels->rowBytes = 4;
		width = 32 / pixels->pixelSize;
		SetRect(&pixels->bounds,0,0,width,8);
		tempPort.portRect = pixels->bounds;
		RectRgn(tempPort.visRgn, &tempPort.portRect);
	
		SetPort((GrafPtr)&tempPort);
		
		colorPat = NewPixPat();
		
		for(i=0;i<aColorTable->polyColorCount;i++)
		{	theColor = (*aColorTable->colorEntryHandle)[i].color;
	
			tempColor.red = ((unsigned char *)&theColor)[1];
			tempColor.red += tempColor.red << 8;
	
			tempColor.green = ((unsigned char *)&theColor)[2];
			tempColor.green += tempColor.green << 8;
	
			tempColor.blue = ((unsigned char *)&theColor)[3];
			tempColor.blue += tempColor.blue << 8;
			
			MakeRGBPat(colorPat, &tempColor);
			FillCRect(&tempPort.portRect, colorPat);
	
			(*(tempPort.portPixMap))->baseAddr += 32;
		}
	
		HUnlock(destTable);
	}

	CloseCPort(&tempPort);
	DisposPixPat(colorPat);
	SetGDevice(savedGD);
	SetPort(saved);
}

void	UpdatePolyDevice(
	PolyDevice		**aPolyDevice,
	PolyColorTable	*aColorTable)
{
	PixMapHandle	thesePix;
	PolyDevice		*pod;
	
	pod = *aPolyDevice;

	thesePix = (*(pod->itsDevice))->gdPMap;
//	pod->rowBytes = 0x3fff & (*thesePix)->rowBytes;
	pod->pixelDepth = (*thesePix)->pixelSize;
	pod->ctSeed = (*((*thesePix)->pmTable))->ctSeed;
	pod->colorTableSeed = thePolyWorld->itsPolyColors->seed;

	switch(pod->pixelDepth)
	{	case 1:
			pod->pixelDriver = BitmapPixelDriver020;
			BuildGrayPatterns(aColorTable, (*aPolyDevice)->colorMap);
			break;
		case 2:	
			pod->pixelDriver = TwoBitPixelDriver;
			BuildPixelPatternMap(aPolyDevice, aColorTable);
			break;
		case 4:	
			pod->pixelDriver = FourBitPixelDriver;
			BuildPixelPatternMap(aPolyDevice, aColorTable);
			break;
		case 8:
			pod->pixelDriver = GetPowerRoutine(BytePixelDriver, PAKITPOWERPLUGID, kPower8BitDriver);
			Build8BitColorMap(aPolyDevice, aColorTable);
			break;
		case 16:
			pod->pixelDriver = GetPowerRoutine(True16PixelDriver, PAKITPOWERPLUGID, kPower16BitDriver);
			Build16BitColorMap(aColorTable, (*aPolyDevice)->colorMap);
			break;
		case 32:
			pod->pixelDriver = GetPowerRoutine(True24PixelDriver, PAKITPOWERPLUGID, kPower24BitDriver);
			Build24BitColorMap(aColorTable, (*aPolyDevice)->colorMap);
			break;
		default:
			pod->pixelDriver = BitmapPixelDriver020;
			break;
	}

	pod = *aPolyDevice;
	pod->pixelDriver = StripAddress(pod->pixelDriver);

}

void	RenderPolygons()
{
	PolyWorld	*polys;
	Point		nilPt = {0,0};
	char		mode = QD32COMPATIBLE;
	Boolean		rendered = FALSE;

	polys = thePolyWorld;

	if(polys->height > 0)
	{	PolyDevice	**aPolyDevice;
		PolyDevice	*pdp;
	
		aPolyDevice = polys->itsPolyDevices;
		
		if(hasColorQD)
		{
			while(aPolyDevice != NULL)
			{	GDHandle		itsDevice;
				Rect			destRect;
				short			*differences;
				PixMapHandle	gdMap;
				CTabHandle		gdColors;
				char			*baseAddr;
				Rect			gdRect;
			
				itsDevice = (*aPolyDevice)->itsDevice;
				gdRect = (*itsDevice)->gdRect;
				
				if(QSectRect(&gdRect,&polys->bounds,&destRect))
				{
					//	Do rendering when the first intersecting display is found.
					if(rendered == FALSE)
					{	ScanConvertPolys(polys,
								polys->displayRegions[polys->shownRegion],
								polys->onLists,
								polys->offLists,
								&polys->bounds,
								polys->backColor);

						DiffPolyRegions(polys,
										polys->displayRegions[!polys->shownRegion],
										polys->displayRegions[polys->shownRegion],
										polys->deltaRegion,
										polys->height);
	
						rendered = TRUE;
					}
				
					//	Rectangles intersect, but is view totally on this device?
					if(	polys->bounds.left != destRect.left ||
						polys->bounds.right != destRect.right ||
						polys->bounds.top != destRect.top ||
						polys->bounds.bottom != destRect.bottom)
					{	//	View is only partially on this device, have to clip further.
					
						differences = polys->displayRegions[!polys->shownRegion];
			
						ExtractDiff(	polys->deltaRegion,
										differences,
										destRect.top-polys->bounds.top,
										destRect.bottom - destRect.top,
										destRect.left,
										destRect.right,
										polys->bounds.right);
					}
					else
					{	differences = polys->deltaRegion;
					}
					
					//	Find out if colors or screen depth have changed.
					
					gdMap = (*itsDevice)->gdPMap;
					gdColors = (*gdMap)->pmTable;
					
					if((*gdColors)->ctSeed != (*aPolyDevice)->ctSeed
						|| polys->itsPolyColors->seed != (*aPolyDevice)->colorTableSeed)
					{	UpdatePolyDevice(aPolyDevice,polys->itsPolyColors);
					}
					
					pdp = (void *)StripAddress(*aPolyDevice);
					pdp->rowBytes = (*gdMap)->rowBytes & 0x3FFF;
					pdp->baseAddr = (*gdMap)->baseAddr +
									(destRect.top-gdRect.top) * (long)pdp->rowBytes -
									((gdRect.left * (*gdMap)->pixelSize) >> 3);
	 
					if(polys->shielding)	ShieldCursor(&destRect,nilPt);
					if(pdp->needs32bit)
						SwapMMUMode((SInt8 *)&mode);
			
					((PixelDriverFunctionType *)(*aPolyDevice)->pixelDriver)
							(	differences,
								pdp->colorMap,
								pdp->rowBytes,
								pdp->baseAddr,
								destRect.bottom-destRect.top,
								destRect.right);
			
					if(pdp->needs32bit)
						SwapMMUMode((SInt8 *)&mode);
					if(polys->shielding)	ShowCursor();
				}
				
				aPolyDevice = (void *)(*aPolyDevice)->nextPolyDevice;
			}
		}
		else	/*	Classic Quickdraw code follows:	*/
		{
			Ptr		realBaseAddress;

			
			pdp = *aPolyDevice;

			if(polys->itsPolyColors->seed != pdp->colorTableSeed)
			{	pdp->colorTableSeed = polys->itsPolyColors->seed;
			
				BuildGrayPatterns(polys->itsPolyColors, (*aPolyDevice)->colorMap);
			}

			realBaseAddress = pdp->baseAddr + polys->bounds.top * (long)pdp->rowBytes;
			ScanConvertPolys(	polys,
								polys->displayRegions[polys->shownRegion],
								polys->onLists,
								polys->offLists,
								&polys->bounds,
								polys->backColor);
	
			DiffPolyRegions(	polys,
								polys->displayRegions[!polys->shownRegion],
								polys->displayRegions[polys->shownRegion],
								polys->deltaRegion,
								polys->height);

			if(polys->shielding)	ShieldCursor(&polys->bounds,nilPt);
			((PixelDriverFunctionType *)(*aPolyDevice)->pixelDriver)
							(	polys->deltaRegion,
								pdp->colorMap,
								pdp->rowBytes,
								realBaseAddress,
								polys->bounds.bottom-polys->bounds.top,
								polys->bounds.right);

			if(polys->shielding)	ShowCursor();
		}

		polys->shownRegion = !polys->shownRegion;
		polys->newEdge = polys->edgeBuffer;
		polys->polyCount = 0;
	}
}

short *	VirtualScanConvert(
	short	backColor)
{
	PolyWorld	*polys;

	polys = thePolyWorld;
	ScanConvertPolys(	polys,
						polys->displayRegions[polys->shownRegion],
						polys->onLists,
						polys->offLists,
						&polys->bounds,
						backColor);
	
	polys->newEdge = polys->edgeBuffer;
	polys->polyCount = 0;

	return polys->displayRegions[polys->shownRegion];
}
void	SkipRendering()
{
	PolyWorld		*polys = thePolyWorld;
	int				i;
	PolyEdgePtr		*ons,*offs;
	short			*clearing;
	
	polys->polyCount = 0;
	polys->newEdge = polys->edgeBuffer;

	ons = polys->onLists;
	offs = polys->offLists;

	for(i=0;i<polys->height;i++)
	{	*ons++ = *offs++ = 0;
	}
}

void	AddPolySegment68000(
	Point	p1,
	Point	p2)
#include "AddPolySegment.body"

#define mot68020
void	AddPolySegment68020(
	Point	p1,
	Point	p2)
#include "AddPolySegment.body"
#undef mot68020

void	NewSpritePolygon(
	short	color)
{
	thePolyWorld->currentColor = color;
	thePolyWorld->polyCount++;
}

void	PixelPoly(
	short	h,
	short	v,
	short	color)
{
	short			thisPoly;
	PolyEdgePtr	aEdge;
	Rect			*theBounds;
	PolyWorld		*thePolys;
	
	thePolys = thePolyWorld;

	if(h >= thePolys->bounds.left && h < thePolys->bounds.right &&
	   v >= 0 && v < thePolys->height)
	{
		h += thePolys->xOffset;
		thisPoly = ++thePolys->polyCount;
		aEdge = thePolys->newEdge++;

		aEdge->nextOn = thePolys->onLists[v];
		thePolys->onLists[v] = aEdge;
		aEdge->nextOff = thePolys->offLists[v];
		thePolys->offLists[v] = aEdge;

		aEdge->dx = 0;
		aEdge->x = ((long)h)<<16;
		aEdge->color = color;
		aEdge->polyID = thisPoly;
		
		if((++h)-thePolys->xOffset < thePolys->bounds.right)
		{	aEdge = thePolys->newEdge++;
	
			aEdge->nextOn = thePolys->onLists[v];
			thePolys->onLists[v] = aEdge;
			aEdge->nextOff = thePolys->offLists[v];
			thePolys->offLists[v] = aEdge;
	
			aEdge->dx = 0;
			aEdge->x = ((long)h)<<16;
			aEdge->color = color;
			aEdge->polyID = thisPoly;
		}
	}
}
void	StartPolyLine(
	short	h,
	short	v,
	short	color)
{
asm	{
	move.l	thePolyWorld,A0
	addq.w	#1,PolyWorld.polyCount(A0)

	move.w	color,PolyWorld.currentColor(A0)
	move.l	h,D0
	swap	D0
	move.l	D0,firstPt
	move.l	D0,lastPt
	}
}

void	AddPolyPoint(
	short	h,
	short	v)
{
asm	{
	lea		lastPt,A0
	move.l	(A0),-(sp)
	move.w	v,(A0)+
	move.w	h,(A0)+
	move.l	-(A0),-(sp)
	move.l	AddPolySegment,A0
	jsr		(A0)				; Note that the UNLK that follows will pop the parameters.
	}	//	Compiler always inserts an UNLK here, so no stack adjust is necessary!
}

void	EndPolyLine()
{
asm	{
		move.l	lastPt,D0
		move.l	firstPt,D1
		cmp.l	D0,D1
		beq.s	@done
		move.l	D0,-(sp)
		move.l	D1,-(sp)
		move.l	AddPolySegment,A0
		jsr		(A0)
		addq.l	#8,sp
@done
	}
}

void	ClearRegions()
{
	PolyWorld		*polys = thePolyWorld;
	int				i;
	PolyEdgePtr	*ons,*offs;
	short			*clearing;
	
	polys->shownRegion = 0;

	polys->newEdge = polys->edgeBuffer;
	polys->polyCount = 0;

	ons = polys->onLists;
	offs = polys->offLists;
	clearing = polys->displayRegions[1];

	for(i=0;i<polys->height;i++)
	{	*ons++ = *offs++ = 0;
		*clearing++ = -2;	//	No color
		*clearing++ = polys->bounds.right;
	}
	*clearing = -1;	//	Mark end of buffer.
}

void	InitPolyGraphics()
{
	long	theProcessor;
	long	theQuickDraw;
	
	Gestalt(gestaltQuickdrawVersion,&theQuickDraw);
	
	if(		theQuickDraw >= gestalt8BitQD)
	{	hasColorQD = true;
	}
	else
	{	hasColorQD = false;
	}
	Gestalt(gestaltProcessorType,&theProcessor);

	if(		theProcessor != gestalt68000
		&&	theProcessor != gestalt68010)
	{	
		AddPolySegment = AddPolySegment68020;
	}
	else
	{	
		AddPolySegment = AddPolySegment68000;
	}

	ScanConvertPolys = GetPowerRoutine(ScanConvertPolys68K, PAKITPOWERPLUGID, kPowerScanConvert);
	DiffPolyRegions = GetPowerRoutine(DiffPolyRegions68K, PAKITPOWERPLUGID, kPowerPADiff);

	SetMasks();
}

void	TrackWindow()
{
	PolyWorld	*polys = thePolyWorld;
	WindowPtr	parent = polys->parentWindow;
	
	if(parent)
	{	Rect	newBounds;
		Rect	*globalBounds;
	
		if(polys->useWindowPortRect)
		{	newBounds = polys->parentWindow->portRect;
		}
		else
		{	newBounds = polys->localBounds;
		}
		
		if(((CGrafPtr)parent)->portVersion < 0)
		{	globalBounds = &(*((CGrafPtr)parent)->portPixMap)->bounds;
		}
		else
		{	globalBounds = &parent->portBits.bounds;
		}

		newBounds.left -= globalBounds->left;
		newBounds.right -= globalBounds->left;
		newBounds.top -= globalBounds->top;
		newBounds.bottom -= globalBounds->top;
		
		if(newBounds.left != polys->bounds.left ||
			newBounds.right != polys->bounds.right ||
			newBounds.top != polys->bounds.top ||
			newBounds.bottom != polys->bounds.bottom)
		{	polys->bounds = newBounds;
			polys->height = newBounds.bottom - newBounds.top;
			polys->xOffset = newBounds.left;
			
			if(polys->height > polys->maxHeight)
			{	polys->height = polys->maxHeight;
				polys->bounds.bottom = polys->bounds.top + polys->height;
			}
			
			ClearRegions();
		}
	}
}

/*	Use: User
**	Resizes the rectangle used for rendering.
*/
void		ResizeRenderingArea(
	Rect	*theArea)
{
	PolyWorld	*polys = thePolyWorld;

	polys->localBounds = *theArea;

	if(!polys->parentWindow)
	{	polys->bounds = *theArea;
		polys->height = polys->bounds.bottom - polys->bounds.top;
		polys->xOffset = polys->bounds.left;

		if(polys->height > polys->maxHeight)
		{	polys->height = polys->maxHeight;
			polys->bounds.bottom = polys->bounds.top + polys->height;
		}
		
		ClearRegions();
	}
	else
	{
		polys->useWindowPortRect = !theArea;
		TrackWindow();
	}
}

void	InitPolyWorld(
	PolyWorld	*polys,
	WindowPtr	parent,
	Rect		*bounds,
	GDHandle	aGDevice,
	short		sharedFlags,
	PolyWorld	*baseWorld)
{
	long		activationListSize;
	int			i;
	PolyDevice	**pdHandle;
	PolyDevice	*pdp;
	Handle		colorHandle;
	GrafPtr		curPort;
	
	GetPort(&curPort);
	
	if(!hasColorQD)
	{	if(parent)
		{	QSectRect(bounds, &parent->portBits.bounds, bounds);
		}
		else
		{	QSectRect(bounds, &curPort->portBits.bounds, bounds);
		}
	}

	polys->sharedDataFlags = sharedFlags;
	polys->parentWindow = parent;
	polys->useWindowPortRect = !bounds;

	if(polys->useWindowPortRect)
	{	bounds = &parent->portRect;
	}

	polys->localBounds = *bounds;
	polys->bounds = *bounds;
	polys->height = bounds->bottom - bounds->top;
	polys->maxHeight = polys->height;

	if(polys->maxHeight < mMaxHeight)
		polys->maxHeight = mMaxHeight;

	polys->xOffset = bounds->left;
	polys->backColor = 0;

	/*	Allocate storage:	*/
	activationListSize = sizeof(PolyEdgePtr) * (long)(polys->maxHeight);

	if(sharedFlags & kShareEdgeBuffer)
	{	polys->edgeBuffer = baseWorld->edgeBuffer;
		polys->endEdge = baseWorld->endEdge;
	}
	else
	{	polys->edgeBuffer = (PolyEdge *)NewPtr(sizeof(PolyEdge) * mMaxEdges);
		polys->endEdge = polys->edgeBuffer+mMaxEdges;
	}
	
	polys->displayRegionSize = mDisplayRegionSize;
	polys->displayRegions[0] = (short *)NewPtr(sizeof(short) * mDisplayRegionSize);
	polys->displayRegions[1] = (short *)NewPtr(sizeof(short) * mDisplayRegionSize);

	if(sharedFlags & kShareDeltaRegions)
	{	polys->deltaRegionSize = baseWorld->deltaRegionSize;
		polys->deltaRegion = baseWorld->deltaRegion;
	}
	else
	{	polys->deltaRegionSize = mDeltaRegionSize;
		polys->deltaRegion = (short *)NewPtr(sizeof(short) * mDeltaRegionSize);
	}

	polys->onLists = (PolyEdgePtr *)NewPtr(activationListSize);
	polys->offLists = (PolyEdgePtr *)NewPtr(activationListSize);	
	polys->itsPolyColors = GetPolyColorTable();
	
	thePolyWorld = polys;

	TrackWindow();
	ClearRegions();
	
	if(hasColorQD)
	{	PolyDevice	**lastDevice;
		long		someSeed;

		polys->deltaRegion = (void *)StripAddress(polys->deltaRegion);

		someSeed = GetCTSeed();	/*	We get a new seed to invalidate PolyDevice fields.	*/

		if(aGDevice == 0)
		{	
			aGDevice = GetDeviceList();
			lastDevice = 0;
			
			while(aGDevice)
			{
				pdHandle = (PolyDevice **)StripAddress(NewHandle(sizeof(PolyDevice)));
				colorHandle = (void *)StripAddress(NewHandle(0));
				(*pdHandle)->colorMap  = colorHandle;
				pdp = *pdHandle;
				pdp->ctSeed = someSeed;				//	Unknown color map.
				pdp->pixelDepth = -1;				//	Unknown bitdept for now.
				pdp->itsDevice = aGDevice;			//	This is the device we draw into.
				pdp->needs32bit = PixMap32Bit((*aGDevice)->gdPMap) & (false32b == GetMMUMode());
				pdp->nextPolyDevice = (void *)lastDevice;
				pdp->colorTableSeed = 0;
				
				lastDevice = pdHandle;
				
				aGDevice = GetNextDevice(aGDevice);
			}
			polys->itsPolyDevices = lastDevice;
			polys->shielding = TRUE;	//	Display is on screen, have to shield cursor
		}
		else
		{	//	Single device only. Probably offscreen, so no shield rectangle.
		
			pdHandle = (PolyDevice **)StripAddress(NewHandle(sizeof(PolyDevice)));
			colorHandle = (void *)StripAddress(NewHandle(0));
			(*pdHandle)->colorMap  = colorHandle;
			pdp = *pdHandle;
			pdp->ctSeed = someSeed;				//	Unknown color map.
			pdp->pixelDepth = -1;				//	Unknown bitdept for now.
			pdp->itsDevice = aGDevice;			//	This is the device we draw into.
			pdp->needs32bit = PixMap32Bit((*aGDevice)->gdPMap) & (false32b == GetMMUMode());
			pdp->nextPolyDevice = 0;
			pdp->colorTableSeed = 0;

			polys->itsPolyDevices = pdHandle;
			polys->shielding = FALSE;

		}
	}
	else	//	Classic quickdraw code follows:
	{
			pdHandle = (PolyDevice **)NewHandle(sizeof(PolyDevice));
			colorHandle = (void *)StripAddress(NewHandle(0));
			(*pdHandle)->colorMap  = colorHandle;
			pdp = *pdHandle;
			pdp->ctSeed = 0;					//	No color map for B&W devices.
			pdp->pixelDepth = 1;				//	Ignored anyway
			pdp->itsDevice = NULL;				//	This is the device we draw into.
			pdp->needs32bit = false;
			pdp->pixelDriver = BitmapPixelDriver68K;
			pdp->baseAddr = curPort->portBits.baseAddr;
			pdp->rowBytes = curPort->portBits.rowBytes;
			pdp->nextPolyDevice = 0;
			pdp->colorTableSeed = 0;


			polys->itsPolyDevices = pdHandle;
			polys->shielding = (pdp->baseAddr == screenBits.baseAddr);
			BuildGrayPatterns(polys->itsPolyColors,colorHandle);
	}
	
	ResetDefaultPolyBufferSizes();
}

/*
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
Boolean	TryPolyBufferSizes(
	long	tryMaxEdges,
	long	tryDisplayRegionSize,
	long	tryDeltaRegionSize,
	long	tryMaxHeight)
{
	long	memRequired;
	long	memAvailable;

	mMaxEdges = (tryMaxEdges >= 0) ? tryMaxEdges : MAXEDGES;
	mDisplayRegionSize = (tryDisplayRegionSize >= 0) ? tryDisplayRegionSize : DISPLAYREGIONSIZE;
	mDeltaRegionSize = (tryDeltaRegionSize >= 0) ? tryDeltaRegionSize : DELTAREGIONSIZE;
	mMaxHeight = (tryMaxHeight >= 0) ? tryMaxHeight : DEFAULTMAXHEIGHT;

	memRequired =		sizeof(PolyEdge) * mMaxEdges
					+	2 * mDisplayRegionSize * sizeof(short)
					+	mDeltaRegionSize * sizeof(short)
					+	2 * sizeof(PolyEdgePtr) * mMaxHeight;
					
	memAvailable = MaxBlock();
	
	return memAvailable >= memRequired;
}

void	ResetDefaultPolyBufferSizes()
{
	//	Reset to defaults:
	mMaxEdges = MAXEDGES;
	mDisplayRegionSize = DISPLAYREGIONSIZE;
	mDeltaRegionSize = DELTAREGIONSIZE;
	mMaxHeight = DEFAULTMAXHEIGHT;
}

void	DisposePolyWorld(
	PolyWorld	*polys)
{
	PolyDevice	**aPolyDevice;
	PolyDevice	**pdp;
	
	aPolyDevice = polys->itsPolyDevices;
	
	while(aPolyDevice)
	{	DisposeHandle((Handle)(*aPolyDevice)->colorMap);
		pdp = (PolyDevice **) (*aPolyDevice)->nextPolyDevice;
		DisposeHandle((Handle)aPolyDevice);
		
		aPolyDevice = pdp;
	}

	if(!(polys->sharedDataFlags & kShareEdgeBuffer))
		DisposPtr((Ptr)polys->edgeBuffer);

	DisposPtr((Ptr)polys->onLists);
	DisposPtr((Ptr)polys->offLists);
	DisposPtr((Ptr)polys->displayRegions[0]);
	DisposPtr((Ptr)polys->displayRegions[1]);
	
	if(!(polys->sharedDataFlags & kShareDeltaRegions))
		DisposPtr((Ptr)polys->deltaRegion);
}

void	SetPolyWorld(
	PolyWorld	*polys)
{
	thePolyWorld = polys;
}

PolyWorld	*GetPolyWorld()
{
	return thePolyWorld;
}

void	SetPolyWorldBackground(
	short	colorInd)
{
	thePolyWorld->backColor = colorInd;
}