/*
    Copyright ©1995, Juri Munkki
    All rights reserved.

    File: Blank CommandList.h
    Created: Thursday, July 27, 1995, 23:35
    Modified: Wednesday, August 9, 1995, 21:54
*/

#pragma once

enum	{
			kNullCmd,
			kNewCmd,
			kOpenCmd,
			kCloseCmd,
			kSaveCmd,
			kSaveAsCmd,
			kQuitCmd,
			kOpenApplicationCmd,
			
			kAboutCmd = 10,
			
			kUndoCmd = 20,
			kCutCmd = 22,
			kCopyCmd,
			kPasteCmd,
			
			kYesCmd = 1000,
			kCancelCmd,
			kNoCmd,

			kInternalCommandNumbers = 64000,
			kSubTargetCmd,	//	Used internally for subpanes.

			kDummyLastCmd
		};

