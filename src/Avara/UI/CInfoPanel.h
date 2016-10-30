/*
    Copyright ©1994-1996, Juri Munkki
    All rights reserved.
*/

#include "CWindowCommander.h"
#include "PAKit.h"

#define	kMaxLineChars		40
#define	kBufferedLines		4
#define	kMaxInfoMessageLen	24
#define	kNumIndicators		4
#define	kNumIndicatorLights	4

enum	{	kliMissiles, kliBoosters, kliGrenades, kliLives	};
enum	{	rightAlign = -1, leftAlign, centerAlign	};

#define	kNumBrightFrames	16

typedef struct
{
	Point	location;
	Rect	box;
	short	width;
	short	txFont;
	short	txFace;
	short	txSize;
	
} TextFieldRecord;

enum
{
	kipScore,
	kipOtherScore1,	kipOtherScore2,	kipOtherScore3,
	kipOtherScore4,	kipOtherScore5,	kipOtherScore6,
	kipLives,
	kipTimer,
	kipConsole1,	kipConsole2,	kipConsole3,	kipConsole4,
	kipMessage1,	kipMessage2,	kipMessage3,
	kipMessage4,	kipMessage5,	kipMessage6,
	kipPlayerName1,	kipPlayerName2,	kipPlayerName3,
	kipPlayerName4,	kipPlayerName5,	kipPlayerName6,
	
	kipKeyCount
};

enum
{	kipDrawMessage = 1,
	kipDrawName = 2,
	kipDrawColorBox = 4,
	kipDrawErase = 8,
	kipDrawValidate = 16
};

class	CInfoPanel : public CWindowCommander
{
public:

	TextFieldRecord	textFields[kipKeyCount];
	Rect			contentRect;
	Rect			grafRect;
	short			templateId;
	PicHandle		artPicture;

	PicHandle		buttonsPict;
	PicHandle		buttonsHeldPict;
	PicHandle		buttonsMaskPict;
	
	short			indicatorValues[kNumIndicators];
	short			indicatorCounts[kNumIndicators];
	PolyHandle		indicatorPolys[kNumIndicators][kNumIndicatorLights];
	
	char			screenBuffer[kBufferedLines][kMaxLineChars];
	short			lineLens[kBufferedLines];
	short			topLine;

	short			redIndex;
	short			greenIndex;
	short			blueIndex;
	
	Boolean			isFirstRect;
	
	PolyWorld		itsPolyWorld;
	
	Fixed			shields;
	Fixed			energy;
	Fixed			guns[2];

	long			score;
	short			lives;
	long			time;
	
	short			brightColorFrames[kNumBrightFrames];
	short			brightShown;
	
	BitMap			microTextMap;

	Boolean			grafChanged;
	Boolean			scoreChanged;
	Boolean			livesChanged;
	Boolean			timeChanged;
	
	virtual	void				IInfoPanel(CCommander *theCommander,
												CWindowCommander **theList, PolyWorld *basePolyWorld);
	virtual	void				Dispose();
	virtual	void				KillIndicators();
	
	virtual	void				LevelReset();

	virtual	void				LoadMicroText();
	virtual	void				CopyMicroRect(short offset, short width, short destX, short destY, short tMode); 
	virtual	void				DrawMicroChar(short theChar, short destX, short destY, short tMode);

	virtual	CloseRequestResult	CloseRequest(Boolean isQuitting);
	virtual	void				ReviveWindowRect(Rect *theRect);

	virtual	void				UsePictTemplate(short resId);
	virtual	Boolean				PrepareTextField(short i);

	virtual	void				DrawIndicatorPanel(short i);
	virtual	void				DrawMessageLine(short i, Boolean doErase, Boolean doValidate);
	virtual	void				DrawTime(Boolean doErase, Boolean doValidate);
	virtual	void				DrawScore(Boolean doErase, Boolean doValidate);
	virtual	void				DrawLives(Boolean doErase, Boolean doValidate);
	virtual	void				DrawMessageArea();
	virtual	void				DrawUserInfoPart(short i, short partList);
	virtual	void				DrawTextFields();
	virtual	void				DrawGrafPanel();
	virtual	void				DrawContents();
	virtual	void				InvalidateRosterData(short index, short areaCode);

	virtual	void				SetIndicatorDisplay(short i, short v);
	virtual	void				SetTime(long newTime);
	virtual	void				SetScore(long newScore);
	virtual	void				SetLives(short newLives);
	virtual	void				UpdateChanged();

	virtual	void				SingleTextLine(char *theText, short len, short align);
	virtual	void				TextLine(char *theText, short len, short align);

	virtual	void				UnfilteredStringLine(StringPtr theString, short align);
	virtual	void				StringLine(StringPtr theString, short align);
	virtual	void				MessageLine(short index, short align);
	virtual	void				NumberLine(long theNum, short align);
	virtual	void				ComposeParamLine(StringPtr destStr, short index, StringPtr param1, StringPtr param2);
	virtual	void				ParamLine(short index, short align, StringPtr param1, StringPtr param2);

	virtual	short				HitTestControls(Point thePoint);
	virtual	void				CornerClick(EventRecord *theEvent, short what);
	virtual	void				RawClick(WindowPtr theWind, short partCode, EventRecord *theEvent);

	virtual	void				BrightBox(long frameNum, short position);
	virtual	void				StartFrame(long frameNum);
};