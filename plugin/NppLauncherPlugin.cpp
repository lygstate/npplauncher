/* -------------------------------------
This file is part of NppLauncherPlugin for NotePad++ 
Copyright (C)2013 Matthias H. mattesh(at)gmx.net
partly copied from the NotePad++ project from 
Don HO donho(at)altern.org 

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

#include "PluginInterface.h"
#include "myDebug.h"
#include <stdio.h>
#include <string>
#include <vector>
#include "nppl_defines.h"
#define MNU_ABOUT 0

const int nbFunc = 1;
FuncItem _funcItem[nbFunc];
NppData _nppData;
HINSTANCE _hModule;

const TCHAR NPP_PLUGIN_NAME[] = TEXT("NppLauncherPlugin");
enum teNppWindows {
   scnMainHandle,
   scnSecondHandle,
   nppHandle,
   scnActiveHandle // used to take the one being active regardless if main or second
};

HWND getCurrentHScintilla(teNppWindows which) 
{  
   if(_nppData._nppHandle==0) {
      return 0;	
   }
   int activView=0;
   switch(which) 
   {
   case scnMainHandle:
      return _nppData._scintillaMainHandle;
   case scnSecondHandle:
      return _nppData._scintillaSecondHandle;
   case nppHandle:
      return _nppData._nppHandle;
   case scnActiveHandle:
      ::SendMessage(_nppData._nppHandle, 
         NPPM_GETCURRENTSCINTILLA,
         (WPARAM)0,
         (LPARAM)&activView);
      return (activView==0)?_nppData._scintillaMainHandle:_nppData._scintillaSecondHandle;
   default :
      return 0;
   } // switch
}

LRESULT execute(teNppWindows window, UINT Msg, WPARAM wParam=0, LPARAM lParam=0) 
{
   HWND hCurrentEditView = getCurrentHScintilla(window); // it's 0 for first = find text		
   return ::SendMessage(hCurrentEditView, Msg, wParam, lParam);
}

std::string wstring2string(const std::wstring & rwString, UINT codepage)
{
   int len = WideCharToMultiByte(codepage, 0, rwString.c_str(), -1, NULL, 0, NULL, NULL);
   if(len > 0) {		
      std::vector<char> vw(len);
      WideCharToMultiByte(codepage, 0, rwString.c_str(), -1, &vw[0], len, NULL, NULL);
      return &vw[0];
   } else {
      return "";
   }
}

extern "C" __declspec(dllexport) void doAboutDialog() 
{
   ::MessageBox(NULL, NPPL_PLGN_HELP_TEXT, NPPL_HEADLINE, MB_OK);
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                      DWORD  reasonForCall, 
                      LPVOID lpReserved )
{
   switch (reasonForCall) {
      case DLL_PROCESS_ATTACH:
         _hModule = (HINSTANCE)hModule;
         memset(_funcItem, 0, sizeof(_funcItem));
         _funcItem[MNU_ABOUT]._pFunc = doAboutDialog;
         lstrcpy(_funcItem[MNU_ABOUT]._itemName, TEXT("About..."));
         break;
      case DLL_PROCESS_DETACH:break;
      case DLL_THREAD_ATTACH:break;
      case DLL_THREAD_DETACH:break;
      default:break;
   }
   return TRUE;
}


extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
   _nppData = notpadPlusData;
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
   return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
   *nbF = nbFunc;
   return _funcItem;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification *notification)
{
   static TCHAR* path=0;
   static uptr_t BufferId=0;

   switch (notification->nmhdr.code) 
   {
   case NPPN_FILEBEFORECLOSE:
      {
         BufferId = notification->nmhdr.idFrom;
         DBG2("beNotified() NPPN_FILEBEFORECLOSE hwndNpp = %d BufferID = %d",
            notification->nmhdr.hwndFrom, BufferId);
         int len = (int)execute(nppHandle, NPPM_GETFULLPATHFROMBUFFERID, notification->nmhdr.idFrom);
         if (len <= 0) {
            break;
         }
         path = new TCHAR[len+1];
         len = (int)execute(nppHandle, NPPM_GETFULLPATHFROMBUFFERID, BufferId, (LPARAM)path);
         std::string s = wstring2string(path, CP_ACP);
         DBG2("beNotified() NPPN_FILEBEFORECLOSE BufferID = %d Path '%s' ",
            notification->nmhdr.idFrom, s.c_str());
         break;
      }
   case NPPN_FILECLOSED:
      {
         if(BufferId == notification->nmhdr.idFrom){
            int pos = (int)execute(nppHandle, NPPM_GETPOSFROMBUFFERID, BufferId);
            DBG2("beNotified() NPPN_FILECLOSED BufferID = %d pos %d ", 
               BufferId, pos);
            if(pos == -1) {
               // this was the last window we shall close the Launcher
               HWND hWnd = ::FindWindow(NPPL_APP_TITLE, NULL);
               if (hWnd != NULL) {
                  //std::string s = wstring2string(path, CP_ACP);
                  COPYDATASTRUCT cpst;                  
                  cpst.dwData = NPPL_ID_FILE_CLOSED;
                  cpst.cbData = ((DWORD)lstrlen(path)+1)*sizeof(TCHAR);
                  cpst.lpData = path;
                  DBGW1("beNotified() sending closed message for %s", path);
                  //DBG1("beNotified() sending closed message for %s", s.c_str());
                  ::SendMessage(hWnd, WM_COPYDATA, (WPARAM)_nppData._nppHandle, (LPARAM)&cpst);
               } else {
                  DBG0("beNotified() Didn't find a sutible window for close message.");
               }
            } else {
               DBG0("beNotified() this was not the last window of the file.");
            }

            if(path) {
               delete[] path;
               path=0;
            }
         }
         break;
      }
   case NPPN_SHUTDOWN:
      {
         HWND hWnd = ::FindWindow(NPPL_APP_TITLE, NULL);
         if (hWnd != NULL) {
            DBG0("beNotified() sending general close message... ");
            ::SendMessage(hWnd, NPPL_ID_PROG_CLOSED, 0, (LPARAM)0);
         } else {
            DBG0("beNotified() Didn't find a sutible window for general close message.");
         }
         break;
      }

   default:
      return;
   }
}

// http://sourceforge.net/forum/forum.php?forum_id=482781
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
   return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
   return TRUE;
}
#endif //UNICODE
