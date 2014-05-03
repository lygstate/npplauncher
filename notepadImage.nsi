;notepadImage Setup Script
;--------------------------------
; Installer for notepadImage.exe
; Creation : 2013-01-24
; Author: Mattes H. mattesh(at)gmx.net
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either
; version 2 of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;--------------------------------

!include versionInfo.inc

!ifndef NPPL_MAIN_VERSION
  !define NPPL_MAIN_VERSION '0.9.x'
!endif
!ifndef APPWEBSITE
  !define APPWEBSITE "http://sourceforge.net/projects/notepadImage/"
!endif
!ifndef APPNAME
  !define APPNAME "notepadImage"
!endif
;--------------------------------
;Configuration

!ifdef OUTFILE
  OutFile "${OUTFILE}"
!else
  OutFile .\notepadImage-Setup.exe
!endif

SetCompressor /SOLID lzma

InstallDir $PROGRAMFILES\notepadImage
InstallDirRegKey HKLM Software\notepadImage ""

RequestExecutionLevel admin

;--------------------------------
;Header Files

!include "MUI2.nsh"
!include "Sections.nsh"
!include "LogicLib.nsh"
!include "Memento.nsh"
!include "WordFunc.nsh"

;--------------------------------
;Definitions

!define SHCNE_ASSOCCHANGED 0x8000000
!define SHCNF_IDLIST 0

;--------------------------------
;Configuration

;Names
Name "notepadImage"
Caption "notepadImage ${NPPL_MAIN_VERSION} Setup"

Var PluginsFolder 
Var NotePadPath

;Memento Settings
!define MEMENTO_REGISTRY_ROOT HKLM
!define MEMENTO_REGISTRY_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\notepadImage"

;Interface Settings
!define MUI_ABORTWARNING

!define MUI_HEADERIMAGE
; !define MUI_WELCOMEFINISHPAGE_BITMAP ".\notepadImage.bmp"

!define MUI_COMPONENTSPAGE_SMALLDESC

;Pages
!define MUI_WELCOMEPAGE_TITLE "Welcome to the notepadImage ${NPPL_MAIN_VERSION} Setup Wizard"
!define ILINE1 "This wizard will install notepadImage ${NPPL_MAIN_VERSION}."
!define ILINE2 "This program replaces the notepad.exe by a different editor like Notepad++."
!define ILINE3 "It creates a blocking systemtray application and sends the open request to NotePad++."
!define ILINE4 "When the file is getting closed in NotePad++ it releases the blocking call."
!define MUI_WELCOMEPAGE_TEXT "${ILINE1}$\r$\n${ILINE2}$\r$\n${ILINE3}$\r$\n${ILINE4}$\r$\n$\r$\n$_CLICK"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE ".\license.txt"
; !ifdef VER_MAJOR & VER_MINOR & VER_REVISION & VER_BUILD
; Page custom PageReinstall PageLeaveReinstall
; !endif

!insertmacro MUI_PAGE_DIRECTORY

!define MUI_DIRECTORYPAGE_TEXT_TOP "Select folder for NotePad++/plugin"
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "Select Notepad++ plugins directory"
!define MUI_DIRECTORYPAGE_VARIABLE "$PluginsFolder"
!insertmacro MUI_PAGE_DIRECTORY 

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_LINK "Visit the notepadImage site for the latest news, FAQs and support"
!define MUI_FINISHPAGE_LINK_LOCATION ${APPWEBSITE}

!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

Function .onInit
StrCpy $PluginsFolder "$PROGRAMFILES\Notepad++\plugins\"
FunctionEnd
;--------------------------------
;Installer Sections
Section "notepadImage" SecCore

  SetDetailsPrint textonly
  DetailPrint "Installing notepadImage ..."
  SetDetailsPrint listonly

  SectionIn 1 RO
  SetOutPath $INSTDIR

  SetOverwrite on
  File .\Release\notepadImage.exe
  File .\license.txt
  File .\readme.txt

  SetOutPath $PluginsFolder
  File .\Release\notepadImagePlugin.dll
  
  IfFileExists  $PluginsFolder\..\NotePad++.exe 0 +3
    StrCpy $NotePadPath $PluginsFolder -8
    StrCpy $NotePadPath "$NotePadPath\NotePad++.exe"
    
  WriteRegStr HKLM "Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe" "Debugger" "$INSTDIR\notepadImage.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe" "readme" "call notepadImage.exe instead of original notepad.exe! To disable this option just remove notepad.exe entry"
  WriteRegStr HKLM "Software\notepadImage" "cmd" '"$NotePadPath"'
  WriteRegStr HKLM "Software\notepadImage" "WaitForClose" "yes"
  WriteRegStr HKLM "Software\notepadImage" "Debug" "no"

  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'

  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\notepadImage.exe" "" "$INSTDIR\notepadImage.exe"
SectionEnd

Section -post

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$INSTDIR\notepadImage.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${NPPL_MAIN_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLInfoAbout" "${APPWEBSITE}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "PluginsFolder" $PluginsFolder
  
  WriteUninstaller $INSTDIR\uninstall.exe

SectionEnd

;--------------------------------
;Descriptions


;--------------------------------
;Uninstaller Section

Section Uninstall

  SetDetailsPrint textonly
  DetailPrint "Uninstalling notepadImage..."
  SetDetailsPrint listonly

  IfFileExists $INSTDIR\notepadImage.exe notepadImage_installed
    MessageBox MB_YESNO "It does not appear that notepadImage is installed in the directory '$INSTDIR'.$\r$\nContinue anyway (not recommended)?" IDYES notepadImage_installed
    Abort "Uninstall aborted by user"
  notepadImage_installed:

  SetDetailsPrint textonly
  DetailPrint "Deleting Registry Keys..."
  SetDetailsPrint listonly

  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'

  ReadRegStr $PluginsFolder HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "PluginsFolder"
  IfFileExists $PluginsFolder\notepadImagePlugin.dll 0 +2
    Delete $PluginsFolder\notepadImagePlugin.dll

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\notepadImage"
  DeleteRegKey HKLM "Software\notepadImage"
  DeleteRegKey HKLM "Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe"

  SetDetailsPrint textonly
  DetailPrint "Deleting Files..."
  SetDetailsPrint listonly
 
  RMDir /r $INSTDIR
  SetDetailsPrint both

SectionEnd
