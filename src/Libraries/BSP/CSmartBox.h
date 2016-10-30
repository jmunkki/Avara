/*/
    Copyright ©1995-1996, Juri Munkki
    All rights reserved.

    File: CSmartBox.h
    Created: Thursday, November 9, 1995, 12:18
    Modified: Wednesday, January 24, 1996, 5:22
/*/

#pragma once
#include "CSmartPart.h"

#define	BOXTEMPLATERESOURCE		400
#define	PLATETEMPLATERESOURCE	401
#define	BSPTEMPLATETYPE			'BSPT'
#define	BSPSCALETYPE			'BSPS'

class	CSmartBox : public CSmartPart
{
public:
	virtual	void		ISmartBox(	short			resId,
									Fixed			*dimensions,
									long			color,
									long			altColor,
									CAbstractActor	*anActor,
									short			aPartCode);
	virtual	void		Dispose();
	
	virtual	void		ScaleTemplate(Fixed *dimensions, Fixed baseSize);
	virtual	void		StretchTemplate(Fixed *dimensions, Fixed baseSize);

	virtual	void		FindEnclosure();
};