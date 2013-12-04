/* -------------------------------------
This file is part of AnalysePlugin for NotePad++ 
Copyright (C)2013 Matthias H. mattesh(at)gmx.net

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
------------------------------------- */
#ifndef NPPL_DEFINES_H
#define NPPL_DEFINES_H

#include "versioninfo.inc"
#define NPPL_VER_NAME TEXT("Version ")
#define NPPL_APP_TITLE TEXT("NppLauncher")
#define NPPL_SPACE TEXT(" ")
#define NPPL_ID_FILE_CLOSED 103401
#define NPPL_ID_PROG_CLOSED 103402
#define NPPL_HEADLINE NPPL_APP_TITLE NPPL_SPACE NPPL_VERSION
#define NPPL_VERSION_INFO NPPL_VER_NAME NPPL_VERSION
#define NPPL_SPC TEXT(" ")

#define NPPL_PLGN_TITLE TEXT("NppLauncher Plugin ")
#define NPPL_PLGN_HELP TEXT("\n\
Copyright: Yonggang Luo, Mattes H.\n\
This plugin is the hook for NppLauncher observing if an edit window\n\
becomes closed. If you close a window in NoptePad++ this plugin\n\
will send a release message to NppLauncher systemtray App to release \n\
the blocked call.\n\
For further information see http://sourceforge.net/projects/npplauncher/")

#define NPPL_PLGN_HELP_TEXT NPPL_PLGN_TITLE NPPL_VERSION_INFO NPPL_PLGN_HELP

#define NPPL_PRG_HELP TEXT("\n\
Copyright: Mattes H.\n\
This program replaces the notepad.exe by a different editor like Notepad++\n\
It creates a blocking application in the systemtray and sends a request\n\
to open the text file in NotePad++.\n\
When the file is getting closed in NotePad++ it receives a close message\n\
and releases the blocking call.\n\
For further information see http://sourceforge.net/projects/npplauncher/")

#define NPPL_PRG_HELP_TEXT NPPL_APP_TITLE NPPL_SPC NPPL_VERSION_INFO NPPL_PRG_HELP

#endif