/*/
    Copyright ©1994-1995, Juri Munkki
    All rights reserved.

    File: CSoundHub.h
    Created: Sunday, December 25, 1994, 10:22
    Modified: Saturday, December 2, 1995, 1:48
/*/

#pragma once
#include "CDirectObject.h"
#include "SoundSystemDefines.h"
#include "CBasicSound.h"

#define	EXTRASOUNDLINKCOUNT		32

enum
{	hubBasic,
	hubRate,
	hubSoundKinds
};

class	CSoundMixer;
class	CHuffProcessor;

class	CSoundHub : public CDirectObject
{
public:
	CHuffProcessor	*	itsCompressor;
	CSoundMixer *		itsMixer;
	CBasicSound *		soundList[hubSoundKinds];
	SampleHeaderHandle	sampleList;
	
	Ptr					soundLinkStorage;
	SoundLink			*firstFreeLink;
	Boolean				muteFlag;
	
	virtual	void				ISoundHub(short numOfEachKind, short initialLinks);
	virtual	void				AttachMixer(CSoundMixer *aMixer);
	virtual	void				CreateSound(short kind);

	virtual	SampleHeaderHandle	LoadSample(short resId);
	virtual	SampleHeaderHandle	PreLoadSample(short resId);
	virtual	SampleHeaderHandle	RequestSample(short resId);
	virtual	void				FreeUnusedSamples();

	virtual	void				FreeOldSamples();
	virtual	void				FlagOldSamples();

	virtual	void				Restock(CBasicSound *aSound);
	virtual	CBasicSound *		Aquire(short kind);
	virtual	CBasicSound *		GetSoundSampler(short kind, short resId);	

	virtual	void				CreateSoundLinks(short n);
	virtual	void				DisposeSoundLinks();
	virtual	SoundLink *			GetSoundLink();
	virtual	void				ReleaseLink(SoundLink *linkPtr);
	virtual	void				ReleaseLinkAndKillSounds(SoundLink *linkPtr);
	
	virtual	void				SetMixerLink(SoundLink *newLink);
	virtual	SoundLink *			UpdateRightVector(Fixed	*right);
	virtual	long				ReadTime();
	virtual	void				HouseKeep();
	
	virtual	void				Dispose();
};

void	UpdateSoundLink(SoundLink *theLink, Fixed *s, Fixed *v, unsigned long t);
void	ZeroSoundLink(SoundLink *theLink);
void	PlaceSoundLink(SoundLink *theLink, Fixed *s);
