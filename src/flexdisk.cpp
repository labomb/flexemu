/*
    flexdisk.cpp


    FLEXplorer, An explorer for any FLEX file or disk container
    Copyright (C) 1998-2004  W. Schwotzer

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif
#include <wx/clipbrd.h>

#include <misc1.h>

#include "flexdisk.h"
#include "fdpframe.h"
#include "fdcframe.h"
#include "ffilecnt.h"
#include "fdirent.h"
#include "fclipbrd.h"
#include "fcopyman.h"
#include "fdlist.h"
#include "bregistr.h"
#include "brcfile.h"
#include "benv.h"


IMPLEMENT_APP(FLEXplorer)

/*------------------------------------------------------
 FLEXplorer implementation (The Application class)
--------------------------------------------------------*/
bool FLEXplorer::OnInit(void)
{
	FlexParentFrame *frame;
	
	ReadDefaultOptions();
	SetAppName("FLEXplorer");
#ifdef wxUSE_DRAG_AND_DROP
	wxTheClipboard->UsePrimarySelection();
#endif
	
	int width = 820;

	// Create the main frame window
	frame = new FlexParentFrame((wxFrame *) NULL, -1,
		(const char *)GetAppName(), wxPoint(-1, -1), wxSize(width, 700),
			wxDEFAULT_FRAME_STYLE | wxHSCROLL | wxVSCROLL);
	frame->Show(TRUE);
	SetTopWindow(frame);
	return TRUE;
}

int FLEXplorer::OnExit(void)
{
	WriteDefaultOptions();
	return 0;
}

void FLEXplorer::WriteDefaultOptions(void)
{
#ifdef WIN32
	BRegistry *reg;

	reg = new BRegistry(BRegistry::localMachine, FLEXPLOREREG);
	reg->SetValue(FLEXPLORERFILEVIEWER, FlexDiskListCtrl::fileViewer);
	reg->SetValue(FLEXPLORERBOOTSECTORFILE, FlexFileContainer::bootSectorFile);
	reg->SetValue(FLEXPLORERTEXTFLAG, FlexCopyManager::autoTextConversion ? 1 : 0);
	delete reg;
#endif
#ifdef UNIX
        BRcFile *rcFile;
        BString rcFileName;
        BEnvironment env;

        if (!env.GetValue("HOME", rcFileName))
                rcFileName = ".";
        rcFileName += PATHSEPARATORSTRING FLEXPLORERRC;
	rcFile = new BRcFile(rcFileName);
	rcFile->Initialize(); // truncate file
	rcFile->SetValue(FLEXPLORERFILEVIEWER, FlexDiskListCtrl::fileViewer);
	rcFile->SetValue(FLEXPLORERBOOTSECTORFILE, FlexFileContainer::bootSectorFile);
	rcFile->SetValue(FLEXPLORERTEXTFLAG,
		FlexCopyManager::autoTextConversion ? 1 : 0);
	delete rcFile;
#endif
}

void FLEXplorer::ReadDefaultOptions(void)
{
	int	autoTextFlag;
	BString str;
#ifdef WIN32
	BRegistry *reg;

	reg = new BRegistry(BRegistry::localMachine, FLEXPLOREREG);
	reg->GetValue(FLEXPLORERFILEVIEWER, str);
	reg->GetValue(FLEXPLORERBOOTSECTORFILE, FlexFileContainer::bootSectorFile);
	reg->GetValue(FLEXPLORERTEXTFLAG, &autoTextFlag);
	if (str.length() == 0)
		str = "Notepad.exe";
	FlexDiskListCtrl::fileViewer = (const char *)str;
	FlexCopyManager::autoTextConversion = (autoTextFlag != 0);
	delete reg;
#endif
#ifdef UNIX
        BRcFile *rcFile;
        BString rcFileName;
        BEnvironment env;

        if (!env.GetValue("HOME", rcFileName))
                rcFileName = ".";
        rcFileName += PATHSEPARATORSTRING FLEXPLORERRC;
        rcFile = new BRcFile(rcFileName);
        if (!rcFile->GetValue(FLEXPLORERFILEVIEWER, str) && str.length() > 0)
		FlexDiskListCtrl::fileViewer = (const char *)str;
        if (!rcFile->GetValue(FLEXPLORERBOOTSECTORFILE, str) && str.length() > 0)
		FlexFileContainer::bootSectorFile = (const char *)str;
        if (!rcFile->GetValue(FLEXPLORERTEXTFLAG, &autoTextFlag))
		FlexCopyManager::autoTextConversion = (autoTextFlag != 0);
	delete rcFile;					 
#endif
}

