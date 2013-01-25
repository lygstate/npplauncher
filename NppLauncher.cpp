//----------------------------------------------------------------------------
// Description : lanceur pour Notepad++, pour remplacer le notepad.exe de Windows
// First Auteur   : Stepho (http://superstepho.free.fr/)
// Création : 2005-08-22
// Second Author  : Mattes H. mattesh(at)gmx.net
// Creation : 2013-01-13
// Additions done to suspend the task as long as the notepad++ is open or file editing is 
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
#include <shellapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ERRNO.H>

#define CAPTION "NppLauncher 0.9.3 - "

int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)
{     
   PROCESS_INFORMATION oProcessInfo;
   STARTUPINFOA si;
   const int iCmdLen = 1024;
   CHAR cmd[iCmdLen] = "\"C:\\Program Files\\Notepad++";
   DWORD bWaitForNotepadClose=1;
   DWORD uWaitTime=0;
   DWORD bDebug=0;
   // récupération du chemin d'accès de Notepad++ dans le base de registre
   HKEY hKey;
   DWORD i;  
   if( (i = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\NppLauncher", 0, KEY_READ , &hKey )) == ERROR_SUCCESS )
   {
      DWORD   iType;
      unsigned char sData[iCmdLen];
      DWORD   iDataSize = iCmdLen;

      if( RegQueryValueExA( hKey, "cmd", NULL, &iType, sData, &iDataSize )
         == ERROR_SUCCESS && iType == REG_SZ)
      {
         // we let also the name of the program so that user can have different versions
         // of the notepad
         strcpy_s(cmd, iCmdLen, (char*)sData);
      } 
      else if( RegQueryValueExA( hKey, "", NULL, &iType, sData, &iDataSize )
         == ERROR_SUCCESS && iType == REG_SZ)
      {
         // thet old writing becomes converted if found and then we remove it
         if(strlen((char*)sData)>0) 
         {
            strcpy(cmd, "\"");
            strcat(cmd, (char*)sData);
         }
         strcat(cmd, "\\notepad++.exe\"");
      }

      // Flag for waiting or not...
      if( RegQueryValueExA( hKey, "WaitForClose", NULL, &iType, sData, &iDataSize )
         == ERROR_SUCCESS && iType == REG_SZ)
      {
         bWaitForNotepadClose = (0==strcmp((char*)sData, "yes"));
      }

      // timeout for waiting ...
      iDataSize = sizeof(uWaitTime);
      if( RegQueryValueExA( hKey, "WaitTimeMsec", NULL, &iType, (LPBYTE)&uWaitTime, &iDataSize )
         == ERROR_SUCCESS && iType == REG_DWORD)
      {
        // all fine
      } else {
         uWaitTime = 100;
      }
      // Debugging or not...
      if( RegQueryValueExA( hKey, "Debug", NULL, &iType, sData, &iDataSize )
         == ERROR_SUCCESS && iType == REG_SZ)
      {
         bDebug = (0==strcmp((char*)sData, "yes"));
      }

      RegCloseKey(hKey);
   } else { 
      sprintf(cmd, "open registry caused error %d", i);
      MessageBoxA(NULL, cmd, "Error opening registry", MB_OK);
      return 0;
   }
   // different calling conventions
   LPSTR notepad1 = "NOTEPAD.EXE "; 
   LPSTR notepad2 = "NOTEPAD.EXE\" "; 
   LPSTR notepad3 = "NOTEPAD.EXE\""; 
   LPSTR notepad4 = "NOTEPAD.EXE"; 
   LPSTR notepad5 = "NOTEPAD "; 
   LPSTR notepad6 = "NOTEPAD\" "; 
   LPSTR notepad7 = "NOTEPAD\""; 
   LPSTR notepad8 = "NOTEPAD"; 
   LPSTR notepad;

   LPSTR lpszMyArgument;
   CHAR upperApp[iCmdLen];
   if(bDebug) { 
	  LPSTR cp = (*lpszArgument)?lpszArgument:"\"\"";
      if(IDCANCEL == MessageBoxA(NULL, cp, "NppLauncher initial command line", MB_OKCANCEL))
         return 0; 
   }
   
   strncpy(upperApp, lpszArgument, iCmdLen);
   upperApp[iCmdLen-1]=0;

   _strupr(upperApp); // win is agnostic to the case
   notepad = notepad1;
   LPSTR ch = strstr(upperApp, notepad);
   if(ch == NULL) { notepad = notepad2; ch = strstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad3; ch = strstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad4; ch = strstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad5; ch = strstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad6; ch = strstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad7; ch = strstr(upperApp, notepad); }
   if(ch == NULL) { notepad = notepad8; ch = strstr(upperApp, notepad); }

   size_t arglen = strlen(lpszArgument);
   if(ch != NULL) {
      // the first parameter is the notepad.exe we will remove this if some remainder is behind
      ch += strlen(notepad);
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
         (*lpszMyArgument==' ')) { // still space
      lpszMyArgument++;
   }
   // construction de la ligne de commande
   if( strlen(lpszMyArgument) )
   {
      CHAR arg[iCmdLen];
      arg[0] =0;
      // remove quotes to call _fullpath()
      if( lpszMyArgument[0] != '\"' )
      {
         strncat(arg, lpszMyArgument, iCmdLen-2);
         arg[iCmdLen-2] = 0;
      }
      else
      {
         strncat(arg, lpszMyArgument+1, iCmdLen-3);
         arg[iCmdLen-3] = 0;
      }
      _fullpath( arg, arg, iCmdLen);
      struct _stat st;
      memset(&st, 0, sizeof(st));
      if(arg[strlen(arg)-1] == '\\') {
         arg[strlen(arg)-1] = 0;
      }
      int ret = _stat(arg,&st);
      const char* szerr = 0;

      if(!ret && (st.st_mode &_S_IFDIR )!=0){
         size_t len = strlen(arg)+1;
         strncat(arg, "\\.txt", iCmdLen-len);
         arg[iCmdLen-1] = 0;
      } else {
         switch (errno) {
            case ENOENT: szerr = "ENOENT"; break;
            case EINVAL: szerr = "EINVAL"; break;
            default: break;
         }; // switch
      }
      if(bDebug) { 
         if(IDCANCEL == MessageBoxA(NULL, arg, "NppLauncher expanded parameters", MB_OKCANCEL)) 
            return 0; 
      }
      if(strlen(arg)){
         strcat(cmd, " \""); // separate from call and add quotes 
         size_t len = iCmdLen-strlen(cmd);
         strncat(cmd, arg, len);
         cmd[iCmdLen-1] = 0;
         strcat(cmd, "\""); // closing quotes
      }
   }

   // préparation des informations de lancement
   memset( &si, 0, sizeof(si) );
   si.cb = sizeof(si);
   memset( &oProcessInfo, 0, sizeof(oProcessInfo) );

   if(bDebug) { 
	   if(IDCANCEL == MessageBoxA(NULL, cmd, "Command to be called", MB_OKCANCEL)) 
		   return 0; 
   }

   // lancement de Notepad++
   if(0==CreateProcessA( NULL, cmd, NULL, NULL, false, 0, NULL, NULL, &si, &oProcessInfo))
   {
      DWORD d = GetLastError();
      switch(d) {
         case 0:
          default: 
             MessageBoxA( NULL, "Error in creating process. Please check registry HKLM/Software/NppLauncher/", cmd, 0);
      }
   } else {
      if(bWaitForNotepadClose){
         DWORD t = GetTickCount();
         // Wait until child process exits.
         WaitForSingleObject( oProcessInfo.hProcess, INFINITE );
         if ((GetTickCount() - t ) < uWaitTime) {
            // we have been falling through because notepad++ was already open
            // put a message box to wait for user's confirmation to exit
            char* name = new char[strlen(lpszMyArgument)+strlen(CAPTION)+1];
            sprintf(name, "%s%s", CAPTION, lpszMyArgument);
            MessageBoxA( NULL, "press ok when editing of this file is finished...", name, MB_OK);
            delete[] name;
         }
      }
   }
   // on ferme proprement
   CloseHandle( oProcessInfo.hProcess );
   CloseHandle( oProcessInfo.hThread );
   return 0;
}
