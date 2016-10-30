/*/
/*/

#pragma once

#define	optOK		1
#define	optCancel	2

#ifdef	OPTIONSUTIL
#define	OPTIONEXTERN
#else
#define	OPTIONEXTERN	extern
#endif

OPTIONEXTERN	DialogPtr	optsDialog;
OPTIONEXTERN	short		defaultButton;

		void	DimOptionControl(short ind);
		void	SetOptionValue(short ind, short value);
pascal	Boolean	OptionsFilter(DialogPtr theDialog, EventRecord *theEvent, short *itemHit);

