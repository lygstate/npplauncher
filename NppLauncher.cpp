//----------------------------------------------------------------------------
// Description: NppLauncher replaces the original wondows notepad 
// additional effort was done to suspend the calling task as long as the notepad++ 
// window with the edited text is open.
// Author  : Mattes H. mattesh(at)gmx.net
// Creation : 2013-01-13
// The source is based on an idea from Stepho (superstepho.free.fr)
// Création : 2005-08-22
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//----------------------------------------------------------------------------

#include <windows.h>
#include <winreg.h>
#include <stdio.h>
#include <string.h>
#include "versioninfo.inc"

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

#include <shellapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ERRNO.H>
#include "SystemTraySDK.h"
#include "resource.h"
#include <assert.h>
#include "nppl_defines.h"
#define MDBG_COMP "Nppl:"
#include "mydebug.h"

#define CAPTION_POST TEXT(" ")
#define CAPTION_PRE TEXT("NL ")
#define CAPTION CAPTION_PRE NPPL_VERSION CAPTION_POST // must be 9 chars long to fit to current copyName()
#define WM_ICON_NOTIFY WM_APP+10
#define MAX_TOOLTIP_LEN 64
#define MAX_TITLE_LEN 100
#define MAX_CMD_LEN 1024

#define MSG_TEXT_PRC_ERR TEXT("CreateProcess() failed with error %d: %s \n\
Please consider checking the registry at HKLM/Software/NppLauncher/.")

CSystemTray _TrayIcon;
HINSTANCE _hInst;
DWORD bDebug=0;
TCHAR arg[MAX_CMD_LEN] = TEXT("");

// this function copies in string into out string by reducing eventually the length to 
// an acceptable short. reducing the text in the middle.
void shortenText (TCHAR* out, size_t lenOut, const TCHAR* in) 
{
#define CPYN_NCHAR_FROM_BEGIN 8
#define CPYN_SEPERATOR_TEXT TEXT("...")

   size_t sepLen = lstrlen(CPYN_SEPERATOR_TEXT);
   assert(lenOut > CPYN_NCHAR_FROM_BEGIN + sepLen);// this is minimum len of result
   size_t len = lstrlen(in);
   if (lenOut > len) {
      // fine. string fits
      lstrcpy(out, in); 
   } else {
      // reduce length
      // take 8 from begin and remaining from back
      lstrncpy(out, in, CPYN_NCHAR_FROM_BEGIN);
      out[CPYN_NCHAR_FROM_BEGIN] = 0;
      lstrcat(out, CPYN_SEPERATOR_TEXT);
      size_t newPos = (len-(lenOut-CPYN_NCHAR_FROM_BEGIN-sepLen-1));// 1 for 0-char
      lstrcat(out, in + newPos); 
   }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int wmId, wmEvent;
   
   switch (message) 
   {
   case WM_COPYDATA:
      if(lParam){
         COPYDATASTRUCT * pcds = (COPYDATASTRUCT *)lParam;
         if (pcds->dwData == NPPL_ID_FILE_CLOSED) {
            LPCWSTR lpszString = (LPCWSTR)(pcds->lpData);
            DBGW1("WndProc: got message NPPL_ID_FILE_CLOSED: %s", lpszString);  
            if(0 == lstrcmp(arg, lpszString)) {
               PostQuitMessage(0);
            } else {
               if(bDebug) { 
                  MessageBox(NULL, lpszString, NPPL_APP_TITLE L" Not mine...", MB_OK);
               }
            }
         }
      } else  {
         DBG1("WndProc: got message WM_COPYDATA 0x%x", lParam);  
      }
      break;
   case WM_ICON_NOTIFY:
      return _TrayIcon.OnTrayNotification(wParam, lParam);

   case WM_COMMAND:
      wmId    = LOWORD(wParam); 
      wmEvent = HIWORD(wParam); 
      // Parse the menu selections:
      switch (wmId)
      {
      case IDM_ABOUT:
         ::MessageBox(hWnd, NPPL_PRG_HELP_TEXT, NPPL_HEADLINE, MB_OK);
         break;
      case IDM_EXIT:
         // cause call the close the program
         DestroyWindow(hWnd);
         break;

      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;

   case NPPL_ID_PROG_CLOSED:
      DBG1("WndProc: got message NPPL_ID_FILE_CLOSED %x", lParam);  
      // close my self and inform the others
      PostQuitMessage(0);
      return DefWindowProc(hWnd, message, wParam, lParam);

   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// register my application window class
ATOM MyRegisterClass(HINSTANCE hInstance)
{
   WNDCLASSEX wcex;
   TCHAR szTitle[MAX_TITLE_LEN]=NPPL_APP_TITLE;

   // Initialize global strings
   wcex.cbSize = sizeof(WNDCLASSEX); 
   wcex.style			= CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc	= (WNDPROC)WndProc;
   wcex.cbClsExtra		= 0;
   wcex.cbWndExtra		= 0;
   wcex.hInstance		= hInstance;
   wcex.hIcon			= ::LoadIcon(hInstance, (LPCTSTR)IDI_NPP_0);
   wcex.hCursor		= ::LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
   wcex.lpszMenuName	= MAKEINTRESOURCE(IDR_CNTXT_MENU);  // todo check if this is correct
   wcex.lpszClassName	= szTitle;
   wcex.hIconSm		= ::LoadIcon(wcex.hInstance, (LPCTSTR)IDI_NPP_0);

   return ::RegisterClassEx(&wcex);
}

// initialize my instance and show the tray icon
BOOL InitInstance(HINSTANCE hInstance, LPTSTR arg/*, int nCmdShow*/)
{
   HWND hWnd;
   TCHAR szTitle[MAX_TITLE_LEN];
   TCHAR toolTip[MAX_TOOLTIP_LEN];

   ::LoadString(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));

   hWnd = CreateWindow(szTitle, szTitle, WS_SYSMENU ,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, _hInst, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   // create tool tip string with correct length
   shortenText(toolTip, _countof(toolTip), arg);

   if (!_TrayIcon.Create( hInstance,
      hWnd,                            // Parent window
      WM_ICON_NOTIFY,                  // Icon notify message to use
      toolTip,                         // tooltip
      ::LoadIcon(hInstance, (LPCTSTR)IDI_NPP_0),
      IDR_CNTXT_MENU)) 
   {
      // problem, was'nt able to create icon
      return FALSE; 
   }

   _TrayIcon.SetMenuDefaultItem(IDM_EXIT);

   return TRUE;
}

// read the parameters from registry how to behave
int ReadRegistry(TCHAR cmd[], DWORD& bWaitForNotepadClose, /*DWORD& uWaitTime,*/ DWORD& bDebug) 
{
   HKEY hKey;
   DWORD i;  
   if( (i = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\NppLauncher"), 0, KEY_READ , &hKey )) == ERROR_SUCCESS )
   {
      DWORD   iType;
      TCHAR sData[MAX_CMD_LEN];
      DWORD   iDataSize = MAX_CMD_LEN;

      if( ::RegQueryValueEx( hKey, TEXT("cmd"), NULL, &iType, (LPBYTE)sData, &iDataSize )
         == ERROR_SUCCESS && iType == REG_SZ)
      {
         // we let also the name of the program so that user can have different versions
         // of the notepad
         lstrcpyn(cmd, sData, MAX_CMD_LEN);
         cmd[MAX_CMD_LEN-1]=0;
      } 

      // Flag for waiting or not...
      if( RegQueryValueEx( hKey, TEXT("WaitForClose"), NULL, &iType, (LPBYTE)sData, &iDataSize )
         == ERROR_SUCCESS && iType == REG_SZ)
      {
         bWaitForNotepadClose = (0==lstrcmp(sData, TEXT("yes")));
      }

      // Debugging or not...
      if( RegQueryValueEx( hKey, TEXT("Debug"), NULL, &iType, (LPBYTE)sData, &iDataSize )
         == ERROR_SUCCESS && iType == REG_SZ)
      {
         bDebug = (0==lstrcmp(sData, TEXT("yes")));
      }

      RegCloseKey(hKey);
   } else { 
      lsprintf(cmd, TEXT("open registry caused error %d"), i);
      MessageBox(NULL, cmd, TEXT("Error opening registry"), MB_OK);
      return 0;
   }
   return 1;
}

// create from lpszArgument a valid argumetn list for program to be called 
// this is especially to remove the notepad.exe name from the line
int CreateCommandLine(TCHAR cmd[], TCHAR arg[], DWORD bDebug, LPTSTR lpszArgument)
{
   // different calling conventions
   LPTSTR notepad1 = TEXT("NOTEPAD.EXE "); 
   LPTSTR notepad2 = TEXT("NOTEPAD.EXE\" "); 
   LPTSTR notepad3 = TEXT("NOTEPAD.EXE\""); 
   LPTSTR notepad4 = TEXT("NOTEPAD.EXE"); 
   LPTSTR notepad5 = TEXT("NOTEPAD "); 
   LPTSTR notepad6 = TEXT("NOTEPAD\" "); 
   LPTSTR notepad7 = TEXT("NOTEPAD\""); 
   LPTSTR notepad8 = TEXT("NOTEPAD"); 
   LPTSTR notepad;

   LPTSTR lpszMyArgument;
   TCHAR upperApp[MAX_CMD_LEN];
   if(bDebug) { 
      LPTSTR cp = (*lpszArgument)?lpszArgument:TEXT("\"\"");
      if(IDCANCEL == MessageBox(NULL, cp, TEXT("NppLauncher initial command line"), MB_OKCANCEL))
         return 0; 
   }

   lstrcpyn(upperApp, lpszArgument, MAX_CMD_LEN);
   upperApp[MAX_CMD_LEN-1]=0;

   lstrupr(upperApp); // win is agnostic to the case
   notepad = notepad1;
   LPTSTR ch = lstrstr(upperApp, notepad);
   if(ch == NULL) { notepad = notepad2; ch = lstrstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad3; ch = lstrstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad4; ch = lstrstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad5; ch = lstrstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad6; ch = lstrstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad7; ch = lstrstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad8; ch = lstrstr(upperApp, notepad); }

   size_t arglen = lstrlen(lpszArgument);
   if(ch != NULL) {
      // the first parameter is the notepad.exe we will remove this if some remainder is behind
      ch += lstrlen(notepad);
      if (arglen >= (size_t)(ch - upperApp)) {
         lpszMyArgument = lpszArgument + (ch - upperApp);
      } else {
         lpszMyArgument = lpszArgument;
      }
   } else {
      lpszMyArgument = lpszArgument;
   }
   // remove preceding spaces
   while((*lpszMyArgument) &&  // not EOS
      (lpszMyArgument < (lpszMyArgument + arglen)) && // not past max len
      (*lpszMyArgument==TEXT(' '))) { // still space
         lpszMyArgument++;
   }
   arg[0] =0;
   // construction de la ligne de commande
   if( lstrlen(lpszMyArgument) )
   {
      // remove quotes to call _fullpath()
      if( lpszMyArgument[0] == TEXT('"') )
      {
         ++lpszMyArgument;
         int len = lstrlen(lpszMyArgument);
         if(lpszMyArgument[len-1] == TEXT('"')) {
            lpszMyArgument[len-1] = 0;
         }
      }
      lfullpath( arg, lpszMyArgument, MAX_CMD_LEN);
      struct _stat st;
      memset(&st, 0, sizeof(st));
      if(arg[lstrlen(arg)-1] == '\\') {
         arg[lstrlen(arg)-1] = 0;
      }
      int ret = lstat(arg,&st);
      const TCHAR* szerr = 0;

      if(!ret && (st.st_mode &_S_IFDIR )!=0){
         size_t len = lstrlen(arg)+1;
         lstrncat(arg, TEXT("\\.txt"), MAX_CMD_LEN-len);
         arg[MAX_CMD_LEN-1] = 0;
      } else {
         switch (errno) {
            case ENOENT: szerr = TEXT("ENOENT"); break;
            case EINVAL: szerr = TEXT("EINVAL"); break;
            default: break;
         }; // switch
      }
      if(bDebug) { 
         if(IDCANCEL == MessageBox(NULL, arg, TEXT("NppLauncher expanded parameters"), MB_OKCANCEL)) 
            return 0; 
      }
      if(lstrlen(arg)){
         lstrcat(cmd, TEXT(" \"")); // separate from call and add quotes 
         size_t len = MAX_CMD_LEN-lstrlen(cmd);
         lstrncat(cmd, arg, len);
         cmd[MAX_CMD_LEN-1] = 0;
         lstrcat(cmd, TEXT("\"")); // closing quotes
      }
   }
   return 1;
}

// main 
int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArg,
                    int nFunsterStil)
{     
   TCHAR lpszArgument[MAX_CMD_LEN];
   lpszArgument[0]=0;
   // WinMain comes only as char* version.
   lmb2tc(lpszArgument, lpszArg, MAX_CMD_LEN);
   lpszArgument[MAX_CMD_LEN-1]=0;
   PROCESS_INFORMATION oProcessInfo;
   STARTUPINFO si;
   _hInst = hThisInstance; // Store instance handle in our global variable

   TCHAR cmd[MAX_CMD_LEN] = TEXT("\"C:\\Program Files\\Notepad++");
   DWORD bWaitForNotepadClose=1;
   //DWORD uWaitTime=0;
   MSG msg;

   MyRegisterClass(_hInst);

   if(0 == ReadRegistry(cmd, bWaitForNotepadClose, /*uWaitTime,*/ bDebug)) 
   {
      return 0; // error case
   }

   if(0 == CreateCommandLine(cmd, arg, bDebug, lpszArgument))
   {
      return 0; // error case
   }
   // préparation des informations de lancement
   memset( &si, 0, sizeof(si) );
   si.cb = sizeof(si);
   memset( &oProcessInfo, 0, sizeof(oProcessInfo) );

   if(bDebug) { 
      if(IDCANCEL == MessageBox(NULL, cmd, TEXT("Command to be called"), MB_OKCANCEL)) 
         return 0; 
   }

   if(FALSE == InitInstance(_hInst, arg)) 
   {
      return 0; // error case
   }

   // lancement de Notepad++
   if(0==CreateProcess( NULL, cmd, NULL, NULL, false, 0, NULL, NULL, &si, &oProcessInfo))
   {
      LPTSTR lpMsgBuf=0;
      LPTSTR lpDisplayBuf;
      DWORD dw = GetLastError(); 

      ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
      lpDisplayBuf = new TCHAR[ lstrlen(lpMsgBuf)+ 20 + lstrlen(MSG_TEXT_PRC_ERR)]; // 20 for err number
      lsprintf(lpDisplayBuf, MSG_TEXT_PRC_ERR, dw, lpMsgBuf); 
      ::MessageBox(NULL, lpDisplayBuf, TEXT("NppLauncher Error"), MB_OK); 
      LocalFree(lpMsgBuf);
      delete[] lpDisplayBuf;
   } else {
      if(bWaitForNotepadClose){
         // Wait until child process exits.
         size_t nameLen = lstrlen(arg)+lstrlen(CAPTION)+1;
         TCHAR* name = new TCHAR[nameLen];
         lsprintf(name, TEXT("%s%s"), CAPTION, arg);
         shortenText(name, MAX_TOOLTIP_LEN, name);
         _TrayIcon.ShowBalloon(TEXT("Double click this icon when editing is finished..."), name, 1);
         delete[] name;
         // message loop for tray icon:
         while (GetMessage(&msg, NULL, 0, 0)) 
         {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
      }
   }

   // on ferme proprement
   CloseHandle( oProcessInfo.hProcess );
   CloseHandle( oProcessInfo.hThread );
   return 0;
}
