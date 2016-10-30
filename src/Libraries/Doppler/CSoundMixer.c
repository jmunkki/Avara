/*/
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.

    File: CSoundMixer.c
    Created: Friday, December 23, 1994, 8:25
    Modified: Tuesday, June 25, 1996, 6:06
/*/

#include "CSoundMixer.h"
#include "CBasicSound.h"
#include "GestaltEqu.h"
#include "QuickTimeComponents.h"
#include "Movies.h"
#include "PowerPlugs.h"
#include "DopplerPlug.h"
#include "CSoundHub.h"
#include "SoundComponents.h"

#ifdef THINK_C
#if __option(a4_globals)
#define	A4World
#endif
#endif

#define	DONT_USE_MINIMUM_VOLUME

void	InitDopplerSoundSystem()
{
	ClearMemoryArea = GetPowerRoutine(ClearMemoryAreaC, DOPPLERPLUGSID, kPowerClearMemory);
	RateMixer = GetPowerRoutine(RateMixer68K, DOPPLERPLUGSID, kPowerRateMixer);
	InterleaveStereoChannels = GetPowerRoutine(NULL, DOPPLERPLUGSID, kPowerInterleave);
}

static
void	SoundMixerExit(
	long theData)
{
	gExitHandler->RemoveExit(&((CSoundMixer *) theData)->itsExit);

	((CSoundMixer *)theData)->Dispose();
}

pascal
void	SoundMixerDoubleBack(
	SndChannelPtr		theChan,
	SndDoubleBufferPtr	theBuffer)
{
	CSoundMixer	*theMaster;
	long			myGlob;

	myGlob = theBuffer->dbUserInfo[1];

asm	{	
	movem.l	D3-D7/A2-A5,-(sp)
#ifdef A4WORLD
	move.l	myGlob, A4
#else
	move.l	myGlob, A5
#endif
	}

	theMaster = (CSoundMixer *)theBuffer->dbUserInfo[0];
	theMaster->DoubleBack(theBuffer);

asm	{
	movem.l	(sp)+,D3-D7/A2-A5
	}

}

void	CSoundMixer::StartTiming()
{
	short				i;
	
	timerInstance =	OpenDefaultComponent(clockComponentType, systemMicrosecondClock);

	if(timerInstance)
	{	TimeRecord	myTime;

		ClockGetTime(timerInstance, &myTime);
		baseTime = *(Int64Bit *)&myTime.value;
	}
	else
	{	Microseconds((void *)&baseTime);
	}
}

long	CSoundMixer::ReadTime()
{
	TimeRecord	myTime;
	long		loBase, hiBase;

	if(timerInstance)
	{	ClockGetTime(timerInstance, &myTime);
	}
	else
	{	Microseconds((void *)&myTime.value);
	}

	loBase = baseTime.loLong;
	hiBase = baseTime.hiLong;

	asm	{
#if 1
			move.l	myTime.value.lo, D0
#else
			move.l	myTime.value.loLong, D0
#endif
			sub.l	loBase,D0
#if 1
			move.l	myTime.value.hi, D1
#else
			move.l	myTime.value.hiLong, D1
#endif
			move.l	hiBase,D2
			subx.l	D2,D1
			move.l	#1000,D2
			divu.l	D2,D1:D0
#if 1
			move.l	D0,myTime.value.lo
#else
			move.l	D0,myTime.value.loLong

#endif
		}
#if 1
	return myTime.value.lo;
#else
	return myTime.value.loLong;
#endif
}

void	CSoundMixer::SetSoundEnvironment(
	Fixed	speedOfSound,
	Fixed	distanceToOne,
	long	timeUnit)
{
	if(speedOfSound)
	{	soundSpeed = speedOfSound;
		distanceToSamples = (unsigned long)samplingRate / (unsigned long)soundSpeed;
	}
	
	if(timeUnit)
	{	timeConversion = 65536L / timeUnit;
	}
	else
	{	timeConversion = 65536;	//	Assume time unit is milliseconds.
	}
	
	if(distanceToOne)
	{	distanceToLevelOne = distanceToOne;
		distanceToOne >>= 4;
		distanceAdjust = FMul(distanceToOne, distanceToOne);
	}

	if(stereo)	maxAdjustedVolume = MAXADJUSTEDVOLUME;
	else		maxAdjustedVolume = MAXADJUSTEDVOLUME / 2;
}

/*
**	Since what you hear depends very much on what kind of environment
**	is used to listen to the audio, two different stereo environments
**	are provided.
**
**	Strong stereo (strongStereo = true) is provided for users who have
**	speakers. Since both ears can hear the sound from speakers, the
**	stereo separation can be very clear as follows:
**
**	Strong stereo:	Sound position:	LEFT		MIDDLE		RIGHT
**	(Speakers)		Left channel	1.0			0.5			0.0
**					Right channel	0.0			0.5			1.0
**
**	Users with headphones can only hear the left channel with the
**	left ear and the right channel with the right ear. It seems the
**	human hearing system doesn't like hearing sounds from just one
**	ear (it gives me a headache), so even when the signal is coming
**	from the left side, the channel will still play it at one third
**	of the volume on the left and vice versa.
**
**	Weak stereo:	Sound position:	LEFT		MIDDLE		RIGHT
**	(Headphones)	Left channel	1.0			0.66		0.33
**					Right channel	0.33		0.66		1.0
*/

void	CSoundMixer::SetStereoSeparation(
	Boolean	strongFlag)
{
	strongStereo = strongFlag;
}

void	CSoundMixer::UpdateRightVector(
	Fixed	*right)
{
	//	Lock with metaphor
	newRightMeta = false;
	newRight[0] = right[0];	
	newRight[1] = right[1];	
	newRight[2] = right[2];	
	
	//	Announce that it is ok to access these.
	newRightMeta = true;
}

void	CSoundMixer::ISoundMixer(
	Fixed	sampRate,
	short	maxChannelCount,
	short	maxMixCount,
	Boolean	stereoEnable,
	Boolean	sample16Enable,
	Boolean	interpolateEnable)
{
	OSErr	iErr;
	long	globTemp;
	short	i,j;
	Vector	rightVect = {FIX(1),0,0,0};
	long	soundCapabilities;

#ifdef A4WORLD
	asm	{	move.l	A4,globTemp	}
#else
	asm	{	move.l	A5,globTemp	}
#endif

	Gestalt(gestaltSoundAttr, &soundCapabilities);
	
	if(soundCapabilities & gestalt16BitAudioSupport)
		sample16flag = sample16Enable;
	else
		sample16flag = false;

	if(sampRate)
	{	samplingRate = sampRate;
	}
	else
	{	ComponentInstance	audioComp;
	
		audioComp = OpenDefaultComponent(kSoundOutputDeviceType, 0);
		if(audioComp)
		{	ComponentResult	theResult;
		
			samplingRate = rate22khz;

			theResult = SoundComponentGetInfo(audioComp, NULL, 'srat', (void *)&samplingRate);
			if(theResult != noErr)
			{	samplingRate = rate22khz;
			}
			CloseComponent(audioComp);
		}
	}
	
	if(samplingRate == rate22khz)
	{	standardRate = FIX(1);
	}
	else
	{	standardRate = FDivNZ(rate22050hz, samplingRate >> 1) >> 1;
	}

	soundBufferBits = BASESOUNDBUFFERBITS;

	if(samplingRate > rate22khz)
	{	soundBufferBits++;
	}

	soundBufferSize = 1<<soundBufferBits;
	
	mixBuffers[0] = (WordSample *)NewPtr(2 * soundBufferSize * sizeof(WordSample));
	mixBuffers[1] = mixBuffers[0] + soundBufferSize;
	
	frameTime = FMulDivNZ(soundBufferSize, FIX(500), samplingRate>>1);

	SetSoundEnvironment(FIX(343), FIX(1), 1);

	UpdateRightVector(rightVect);
	motion.loc[0] = motion.loc[1] = motion.loc[2] = 0;
	motion.speed[0] = motion.speed[1] = motion.speed[2] = 0;

	motionLink = NULL;
	altLink = NULL;
	useAltLink = false;
	
	globRegister = globTemp;
	maxChannels = maxChannelCount;
	maxMix = maxMixCount;

	if(maxMix > maxChannels) maxMix = maxChannels;

	stereo = stereoEnable;
	strongStereo = false;
	stopNow = false;
	hushFlag = false;

	if(maxChannels)
	{	if(stereo)
		{	SndCommand	stereoTest;
		
			stereoTest.cmd = availableCmd;
			stereoTest.param1 = 0;
			stereoTest.param2 = initStereo;
			
			iErr = SndControl(sampledSynth, &stereoTest);
			
			stereo = (iErr == 0) && (stereoTest.param1 != 0);
		}
	}

	infoTable = (MixerInfo *)NewPtr(sizeof(MixerInfo) * maxChannels);
	sortSpace[0] = (MixerInfo **)NewPtr(sizeof(MixerInfo *) * (maxChannels+1) * 2);
	sortSpace[1] = sortSpace[0] + maxChannels+1;

	for(i=0;i<maxChannels;i++)
	{	infoTable[i].intro = NULL;
		infoTable[i].introBackup = NULL;
		infoTable[i].active = NULL;
		infoTable[i].release = NULL;
		infoTable[i].volume = 0;
		infoTable[i].isFree = true;
		
		for(j=0;j<2;j++)
			sortSpace[j][i+1] = &infoTable[i];
	}

	if(sample16flag)
	{	
		scaleLookup = NULL;

#ifndef DONT_USE_MINIMUM_VOLUME
		if(soundCapabilities & gestalt16BitSoundIO)
				minimumVolume = 0;
		else	minimumVolume = MINIMUM8BITVOLUME;
#endif
	}
	else
	{	PrepareScaleLookup();
#ifndef DONT_USE_MINIMUM_VOLUME
		minimumVolume = MINIMUM8BITVOLUME;
#endif
	}

	volumeLookup = (SampleConvert *) NewPtr(sizeof(SampleConvert) * VOLUMERANGE);
	PrepareVolumeLookup();
	
	sortSpace[0][0] = &dummyChannel;
	sortSpace[1][0] = &dummyChannel;

	dummyChannel.volume = 32767;
	volumeMax = VOLUMERANGE;
	
	StartTiming();
	StartExitHandler();
	
	{	long	bufferRAM;
		
		bufferRAM = sizeof(SndDoubleBuffer) + sizeof(WordSample) * 2*soundBufferSize;
		doubleBuffers[0] = (SndDoubleBufferPtr) NewPtr(bufferRAM);
		doubleBuffers[1] = (SndDoubleBufferPtr) NewPtr(bufferRAM);
	}

	itsExit.theData = (long) this;
	itsExit.exitFunc = SoundMixerExit;
	StartExitHandler();
	gExitHandler->AddExit(&itsExit);

	SilenceBuffers();
	doubleHeader.dbhNumChannels = 1 + stereo;
	doubleHeader.dbhSampleSize = sample16flag ? 16 : 8;
	doubleHeader.dbhCompressionID = 0;
	doubleHeader.dbhPacketSize = 0;
	doubleHeader.dbhSampleRate = samplingRate;
	doubleHeader.dbhBufferPtr[0] = doubleBuffers[0];
	doubleHeader.dbhBufferPtr[1] = doubleBuffers[1];
	doubleHeader.dbhDoubleBack = SoundMixerDoubleBack;

	frameCounter = -2;
	itsChannel = NULL;
	
	if(maxChannels)
	{	iErr = SndNewChannel(&itsChannel,
								sampledSynth,
								(interpolateEnable ? (initNoInterp + initNoDrop) : 0)
								+ (stereo ? initStereo : initMono),
								NULL);
		iErr = SndPlayDoubleBuffer(itsChannel, &doubleHeader);
	}
}

void	CSoundMixer::PrepareScaleLookup()
{
	long		i;
	long		value;
	Sample		*output;
	long		scaleLookupSize;

	scaleLookupSize = sizeof(Sample) << (VOLUMEBITS+BITSPERSAMPLE-1);
	scaleLookup = NewPtr(scaleLookupSize);
	output = (Sample *)scaleLookup;
	scaleLookupSize >>= 1;
	scaleLookupZero = scaleLookup + scaleLookupSize;

	for(i= -scaleLookupSize; i < scaleLookupSize; i++)
	{	value = (i+(i>>1)) >> (BITSPERSAMPLE + VOLUMEBITS - 10);
		value = (value + 257) >> 1;
		if(value > 255)		value = 255;
		else if(value < 0)	value = 0;

		*output++ = value;
	}
}

void	CSoundMixer::PrepareVolumeLookup()
{
	WordSample	*dest;
	short		vol, samp;

	dest = &volumeLookup[0][0];

	if(sample16flag)
	{	for(vol = 1; vol <= VOLUMERANGE; vol++)
		{	for(samp = -SAMPLERANGE/2; samp < SAMPLERANGE/2; samp++)
			{	*dest++ = (samp * vol)<<(16-BITSPERSAMPLE-VOLUMEBITS);
			}
		}
	}
	else
	{	for(vol = 1; vol <= VOLUMERANGE; vol++)
		{	for(samp = -SAMPLERANGE/2; samp < SAMPLERANGE/2; samp++)
			{	*dest++ = samp * vol;
			}
		}
	}
}
void	CSoundMixer::SilenceBuffers()
{
	short		i,j;
	Sample		*p;
	WordSample	*q;
	
	for(j=0;j<2;j++)
	{	p = (Sample *) doubleBuffers[j]->dbSoundData;
		q = (WordSample *) p;
		doubleBuffers[j]->dbFlags = dbBufferReady;
		doubleBuffers[j]->dbNumFrames = soundBufferSize;
		doubleBuffers[j]->dbUserInfo[0] = (long) this;
		doubleBuffers[j]->dbUserInfo[1] = globRegister;

		if(sample16flag)
		{	for(i=0;i<soundBufferSize;i++)
			{	*q++ = WORDSILENCE;	//	left
				*q++ = WORDSILENCE;	//	right
			}
		}
		else
		{	for(i=0;i<soundBufferSize;i++)
			{	*p++ = SILENCE;	//	left
				*p++ = SILENCE;	//	right
			}
		}
	}
}

void	CSoundMixer::Dispose()
{
	SCStatus	status;
	OSErr		iErr;
	short		i;
	MixerInfo	*mix;
	
	gExitHandler->RemoveExit(&itsExit);
	
	stopNow = true;
	
	if(itsChannel)
	{	do
		{	SndChannelStatus(itsChannel, sizeof(SCStatus), &status);
		} while(status.scChannelBusy);
	
		iErr = SndDisposeChannel(itsChannel, true);
	}

	if(motionLink)	
	{	altLink = NULL;
		useAltLink = true;
		motionHub->ReleaseLink(motionLink);
		motionLink = NULL;
	}

	mix = infoTable;
	for(i=0;i<maxChannels;i++)
	{	if(mix->active)
			mix->active->Release();
		else if(mix->release)
			mix->release->Release();
		else if(mix->intro)
			mix->intro->Release();

		mix++;
	}
	
	if(mixBuffers[0])
	{	DisposePtr((Ptr) mixBuffers[0]);
		mixBuffers[0] = NULL;
	}

	if(timerInstance)
	{	CloseComponent(timerInstance);
	}

#define	OBLITERATE(pointer)	if(pointer)	{	DisposePtr((Ptr)pointer); pointer = NULL; }

	OBLITERATE(doubleBuffers[0])
	OBLITERATE(doubleBuffers[1])
	OBLITERATE(infoTable)
	OBLITERATE(volumeLookup)
	OBLITERATE(scaleLookup)
	OBLITERATE(sortSpace[0])

	inherited::Dispose();
}

void	CSoundMixer::HouseKeep()
{
	short		i;
	MixerInfo	*mix;
	
	mix = infoTable;
	for(i=0;i<maxChannels;i++)
	{	if(mix->release)
		{	mix->release->Release();
			mix->release = NULL;
			mix->isFree = true;
		}
		mix++;
	}
}

void	CSoundMixer::AddSound(
	CBasicSound	*theSound)
{
	short		i;
	MixerInfo	*mix;
	
	mix = infoTable;
	for(i=0;i<maxChannels;i++)
	{	if(mix->isFree)
		{	theSound->itsMixer = this;
			mix->isFree = false;
			mix->intro = theSound;
			mix->introBackup = theSound;
			return;
		}
		mix++;
	}

	//	Couldn't find a free slot, so release the sound immediately.
	theSound->Release();
}

void	CSoundMixer::UpdateMotion()
{
	SoundLink	*aLink;

	frameStartTime = ReadTime();
	frameEndTime = frameStartTime + frameTime;

	if(newRightMeta)
	{	rightVector[0] = newRight[0];
		rightVector[1] = newRight[1];
		rightVector[2] = newRight[2];
		newRightMeta = false;
	}

	if(useAltLink)
		aLink = altLink;
	else
		aLink = motionLink;

	if(aLink && (aLink->meta == metaNewData))
	{	Fixed		*s,*d;
		Fixed		t;
		
		s = aLink->nSpeed;
		d = aLink->speed;
		*d++ = FMul(*s++, timeConversion);
		*d++ = FMul(*s++, timeConversion);
		*d++ = FMul(*s++, timeConversion);
		
		s = aLink->nLoc;
		d = aLink->loc;
		*d++ = *s++;	*d++ = *s++;	*d++ = *s++;
	
		s = aLink->speed;
		d = aLink->loc;
		t = aLink->t;
		
		*d++ -= *s++ * t;
		*d++ -= *s++ * t;
		*d++ -= *s++ * t;
	
		aLink->meta = metaNoData;
		
		s = aLink->loc;
		d = motion.loc;
		*d++ = *s++;	*d++ = *s++;	*d++ = *s++;

		s = aLink->speed;
		d = motion.speed;
		*d++ = *s++;	*d++ = *s++;	*d++ = *s++;
	}
}

#define	DEBUGSOUNDno
#ifdef DEBUGSOUND
short	gDebugMixCount;
#endif

void	CSoundMixer::DoubleBack(
	SndDoubleBufferPtr theBuffer)
{
	short			i;
	MixerInfo		*mix;
	MixerInfo		**sort, **inSort;
	long			volumeTotal;
	short			whichStereo;

	if(stopNow)
	{	theBuffer->dbFlags |= dbLastBuffer;
	}

	UpdateMotion();
	frameCounter++;

	if(stereo || !sample16flag)
	{	mixTo[0] = mixBuffers[0];
	}
	else
	{	mixTo[0] = (WordSample *)theBuffer->dbSoundData;
	}

	mixTo[1] = mixBuffers[1];	//	Will never actually be touched by monophonic sounds.

	for(whichStereo = 0; whichStereo <= stereo; whichStereo++)
	{	unsigned long	*p;
		
		ClearMemoryArea((long *)mixTo[whichStereo], soundBufferSize>>4);
		
	}

	/*
	**	Activate new channels.
	*/
	mix = infoTable;
	i = maxChannels;
	do
	{	if(mix->intro && mix->intro == mix->introBackup)
		{	mix->active = mix->intro;
			mix->intro = NULL;
			mix->introBackup = NULL;
			mix->active->FirstFrame();
		}

		mix++;
	} while(--i);

#ifdef DEBUGSOUND
gDebugMixCount = 0;
#endif

	if(!stopNow)
	for(whichStereo = 0; whichStereo <= stereo; whichStereo++)
	{	short	volumeLimit = 0;
		/*
		**	Go through channels, asking the volume for this side.
		*/
		mix = infoTable;
		i = maxChannels;
		do
		{	if(mix->active)
			{	volumeLimit += (mix->volume = mix->active->CalcVolume(whichStereo));
			}
			
			mix++;
		} while(--i);

		volumeLimit >>= 5;
		if(volumeLimit)
		{	mix = infoTable;
			i = maxChannels;
			do
			{	if(mix->active && volumeLimit > mix->volume)
				{	mix->volume = 0;
				}
				
				mix++;
			} while(--i);
		}

		/*
		**	Insertion sort according to volume.
		*/
		sort = sortSpace[whichStereo]+2;
		i = maxChannels;
		while(--i)
		{	inSort = sort++;

			while(inSort[-1]->volume < inSort[0]->volume)
			{	mix = inSort[0];
				inSort[0] = inSort[-1];
				inSort[-1] = mix;
				inSort--;
			}
		}
		
		/*
		**	Select as many of the loudest sounds that fit.
		*/
		volumeTotal = 0;
		sort = sortSpace[whichStereo]+1;

		i = maxMix;
		do
		{	mix = *sort;
		
			if(mix->volume)
			{	volumeTotal += mix->volume;

				if(volumeTotal >= volumeMax)
				{	mix->volume += volumeMax-volumeTotal;
					if(mix->volume) sort++;

					volumeTotal = volumeMax;
					break;
				}
			}
			else
				break;
			
			sort++;
		} while(--i);
		
		sort--;
		while(volumeTotal>0)
		{	mix = *sort--;
			
			volumeTotal -= mix->volume;
			mix->active->WriteFrame(whichStereo, mix->volume);
#ifdef DEBUGSOUND
			gDebugMixCount++;
#endif
		}
	}

	mix = infoTable;
	i=maxChannels;

	if(hushFlag) 
	{	do
		{	if(mix->active)
			{	mix->release = mix->active;
				mix->active = NULL;
				mix->volume = 0;
			}
			mix++;
		} while(--i);
		
		hushFlag = false;
	}
	else
	{	do
		{	if(mix->active && mix->active->EndFrame())
			{	mix->release = mix->active;
				mix->active = NULL;
				mix->volume = 0;
			}
			mix++;
		} while(--i);
	}

	PostMix(theBuffer);

	theBuffer->dbFlags |= dbBufferReady;
	theBuffer->dbNumFrames = soundBufferSize;
}

void	CSoundMixer::PostMix(
	SndDoubleBufferPtr theBuffer)
{
	register	WordSample	*leftCh;
	register	WordSample	*rightCh;
				Ptr			lookupZero = scaleLookupZero;
				char		*blendTo = theBuffer->dbSoundData;
				short		bufferSize = (soundBufferSize >> 1) - 1;
	
	leftCh = mixTo[0];
	rightCh = mixTo[1];
		
	if(sample16flag)
	{	if(stereo)
		{
			if(InterleaveStereoChannels)
				InterleaveStereoChannels(leftCh, rightCh, (WordSample *)blendTo, soundBufferSize);
			else
			asm
			{	move.l	blendTo, A0
				move.w	bufferSize, D0
			@stereo16Loop
				move.l	(leftCh)+, D1
				move.w	D1,D2
				move.w	(rightCh)+, D1
				swap.w	D2
				move.w	(rightCh)+, D2
	
				move.l	D1,(A0)+
				move.l	D2,(A0)+
	
				dbra	D0,@stereo16Loop
			}
		}
	}
	else
	{	if(stereo)
		{	asm
			{	move.l	lookupZero, A1
				move.l	blendTo, A0
				move.w	bufferSize, D0
		@stereo8Loop
				move.l	(leftCh)+,D1		;	left
				move.b	0(A1,D1.w), D1		;	left

				move.l	(rightCh)+,D2		;			right
				move.b	D1,(A0)+			;	left
				move.b	0(A1,D2.w), D2		;			right
				swap.w	D1					;	left
				move.b	D2,(A0)+			;			right
				
				move.b	0(A1,D1.w), D1		;	left
				swap.w	D2					;			right
				move.b	D1,(A0)+			;	left
				move.b	0(A1,D2.w), D2		;			right
				move.b	D2,(A0)+			;			right
				
				dbra	D0,@stereo8Loop
			}
		}
		else
		{
#if 0
			static	short	minValue = 0;
			static	short	maxValue = 0;
			
			bufferSize++;
			while(bufferSize--)
			{	short	value;
				
				value = *leftCh++;
				if(value < minValue)		minValue = value;
				else if(value > maxValue)	maxValue = value;
				*blendTo++ = lookupZero[value];

				value = *leftCh++;
				if(value < minValue)		minValue = value;
				else if(value > maxValue)	maxValue = value;
				*blendTo++ = lookupZero[value];
			}
#else
			asm
			{	move.l	lookupZero, A1
				move.l	blendTo, A0
				move.w	bufferSize, D0
		@mono8Loop
				move.w	(leftCh)+,D1
				move.b	0(A1,D1.w), D2
				move.b	D2,(A0)+
				
				move.w	(leftCh)+,D1
				move.b	0(A1,D1.w), D2
				move.b	D2,(A0)+
	
				dbra	D0,@mono8Loop
			}
#endif
		}
	}
}