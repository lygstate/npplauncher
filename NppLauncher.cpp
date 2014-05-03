//----------------------------------------------------------------------------
// Description: NppLauncher replaces the original wondows notepad 
// additional effort was done to suspend the calling task as long as the notepad++ 
// window with the edited text is open.
// Author  : Mattes H. mattesh(at)gmx.net
// Creation : 2013-01-13
// The source is based on an idea from Stepho (superstepho.free.fr)
// Creation : 2005-08-22
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
#include <wctype.h>

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
#include "NotepadDebug.h"

#define CAPTION_POST TEXT(" ")
#define CAPTION_PRE TEXT("NL ")
#define CAPTION CAPTION_PRE NPPL_VERSION CAPTION_POST // must be 9 chars long to fit to current copyName()
#define WM_ICON_NOTIFY WM_APP+10
#define MAX_TOOLTIP_LEN 64
#define MAX_TITLE_LEN 100

#define MSG_TEXT_PRC_ERR TEXT("CreateProcess() failed with error %d: %s \n\
Please consider checking the registry at HKLM/Software/NppLauncher/.")

static CSystemTray _TrayIcon;
bool bDebug = 0;
static std::wstring filename;
#ifndef length_of
#define length_of(x) (sizeof(x)/sizeof((x)[0]))
#endif

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COPYDATA:
		if (lParam){
			COPYDATASTRUCT * pcds = (COPYDATASTRUCT *)lParam;
			if (pcds->dwData == NPPL_ID_FILE_CLOSED) {
				LPCWSTR lpszString = (LPCWSTR)(pcds->lpData);
				DBGW1("WndProc: got message NPPL_ID_FILE_CLOSED: %s", lpszString);
				if (filename == lpszString) { // TOOD: use more robust filename compare function
					PostQuitMessage(0);
				}
				else {
					if (bDebug) {
						std::wstring msg = filename + L"!=" + lpszString;
						MessageBox(NULL, msg.c_str(), NPPL_APP_TITLE L" Not mine...", MB_OK);
					}
				}
			}
		}
		else  {
			DBG1("WndProc: got message WM_COPYDATA 0x%x", lParam);
		}
		break;
	case WM_ICON_NOTIFY:
		return _TrayIcon.OnTrayNotification(wParam, lParam);

	case WM_COMMAND:
		wmId = LOWORD(wParam);
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
	TCHAR szTitle[MAX_TITLE_LEN] = NPPL_APP_TITLE;

	// Initialize global strings
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = ::LoadIcon(hInstance, (LPCTSTR)IDI_NPP_0);
	wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_CNTXT_MENU);  // TODO: check if this is correct
	wcex.lpszClassName = szTitle;
	wcex.hIconSm = ::LoadIcon(wcex.hInstance, (LPCTSTR)IDI_NPP_0);

	return ::RegisterClassEx(&wcex);
}

// initialize my instance and show the tray icon
BOOL InitInstance(HINSTANCE hInstance, std::wstring arg/*, int nCmdShow*/)
{
	HWND hWnd;
	TCHAR szTitle[MAX_TITLE_LEN];

	::LoadString(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));

	hWnd = CreateWindow(szTitle, szTitle, WS_SYSMENU,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	if (!_TrayIcon.Create(hInstance,
		hWnd,                            // Parent window
		WM_ICON_NOTIFY,                  // Icon notify message to use
		arg.c_str(),                     // tooltip
		::LoadIcon(hInstance, (LPCTSTR)IDI_NPP_0),
		IDR_CNTXT_MENU))
	{
		// Create the icon failed.
		return FALSE;
	}

	_TrayIcon.SetMenuDefaultItem(IDM_EXIT);

	return TRUE;
}

// read the parameters from registry how to behave
bool ReadOptions(std::wstring notepadCmd, bool& bWaitForNotepadClose, /*DWORD& uWaitTime,*/ bool& bDebug)
{
	notepadCmd = L"notepad++.exe"; // The commands path is relative to this executable, test if the executable exist, if doesn't ,then return false
	bWaitForNotepadClose = true; // Flag for waiting or note waiting for notepad++ to exit. "yes" | "no"
	bDebug = false; // default not to debug "yes" | "no"

	return true;
}

// create from lpszArgument a valid arguments list for program to be called 
// this is especially to remove the notepad.exe name from the line
// cmd: the final running cmd
// filename: the file that notepad will open
bool CreateCommandLine(std::wstring& cmd, std::wstring& filename, boolean const& bDebug, wstring const& arguments)
{
	// different calling conventions
	static wstring const notepadNames[] = {
		L"NOTEPAD.EXE\"",
		L"NOTEPAD.EXE",
		L"NOTEPAD\"",
		L"NOTEPAD",
	};

	if (bDebug) {
		if (IDCANCEL == MessageBoxW(NULL, arguments.c_str(), L"NppLauncher initial command line", MB_OKCANCEL))
			return false;
	}

	std::wstring upperApp = L"";
	for (size_t i = 0; i < arguments.size(); ++i) {
		upperApp += towupper(arguments[i]);
	}
	std::wstring notepad = L"";
	size_t idx = std::wstring::npos;
	for (int i = 0; i < length_of(notepadNames); ++i) {
		idx = upperApp.find(notepadNames[i]);
		if (idx != std::wstring::npos) {
			notepad = notepadNames[i];
			break;
		}
	}
	if (idx != std::wstring::npos)
		idx += notepad.size();
	while (idx < arguments.size() && iswspace(arguments[idx])) ++idx;

	if (idx < arguments.size() && arguments[idx] == L'\"') ++idx;

	size_t end = std::wstring::npos;
	if (idx < arguments.size() && *arguments.rbegin() == L'\"') end = arguments.size() - 1;

	if (idx >= end) {
		if (bDebug) {
			MessageBoxW(NULL, arguments.c_str(), L"NppLauncher initial command line failed, can not found notepad command", MB_OK);
		}
		return true;
	}

	std::wstring tmpPath = arguments.substr(idx, end);
	std::vector<wchar_t> p;
	p.resize(_MAX_PATH + 1);
	wchar_t *fPath = _wfullpath(p.data(), tmpPath.c_str(), p.size());
	filename = fPath;

	if (*filename.rbegin() == '\\') filename.resize(filename.size() - 1);

	struct _stat st;
	memset(&st, 0, sizeof(st));
	int ret = _wstat(filename.c_str(), &st);

	if (!ret && (st.st_mode & _S_IFDIR) != 0) { //If this file is a directory, try to append \.txt to it.
		if (bDebug) {
			MessageBoxW(NULL, arguments.c_str(), L"NppLauncher is opening the directory as txt.", MB_OK);
		}
		filename += L"\\.txt";
	}

	if (bDebug) {
		if (IDCANCEL == MessageBox(NULL, filename.c_str(), TEXT("NppLauncher expanded parameters"), MB_OKCANCEL))
			return 0;
	}
	if (filename.size() > 0) {
		cmd = cmd + L" \"" + filename + L"\"";
	}
	return true;
}

bool LaunchNotepad(STARTUPINFO& si, PROCESS_INFORMATION& oProcessInfo, std::wstring cmd) {
	// preparation the notepad++ process launch information.
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&oProcessInfo, 0, sizeof(oProcessInfo));
	// launch the Notepad++
	std::vector<wchar_t> cmdStr(cmd.c_str(), cmd.c_str() + cmd.size() + 1);
	if (CreateProcessW(NULL, cmdStr.data(), NULL, NULL, false, 0, NULL, NULL, &si, &oProcessInfo) == FALSE) {
		LPTSTR lpMsgBuf = 0;
		LPTSTR lpDisplayBuf;
		DWORD dw = GetLastError();

		::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		lpDisplayBuf = new TCHAR[lstrlen(lpMsgBuf) + 20 + lstrlen(MSG_TEXT_PRC_ERR)]; // 20 for err number
		lsprintf(lpDisplayBuf, MSG_TEXT_PRC_ERR, dw, lpMsgBuf);
		::MessageBox(NULL, lpDisplayBuf, TEXT("NppLauncher Error"), MB_OK);
		LocalFree(lpMsgBuf);
		delete[] lpDisplayBuf;
		return false;
	}
	return true;
}

// main 
int WINAPI wWinMain(
	HINSTANCE hThisInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nShowCmd
	)
{
	std::wstring cmd;
	bool bWaitForNotepadClose = true;

	MyRegisterClass(hThisInstance);

	if (!ReadOptions(cmd, bWaitForNotepadClose, bDebug))
	{
		return -1; // error case
	}

	if (!CreateCommandLine(cmd, filename, bDebug, lpCmdLine))
	{
		return -1; // error case
	}

	if (bDebug) {
		if (IDCANCEL == MessageBox(NULL, cmd.c_str(), TEXT("Command to be called"), MB_OKCANCEL))
			return 0;
	}

	if (FALSE == InitInstance(hThisInstance, filename))
	{
		return -1; // error case
	}

	STARTUPINFO si;
	PROCESS_INFORMATION oProcessInfo;
	if (LaunchNotepad(si, oProcessInfo, cmd) && bWaitForNotepadClose)
	{
		// Wait until child process to exit.

		std::wstring ballonMsg = std::wstring(CAPTION) + L":" + filename;
		_TrayIcon.ShowBalloon(TEXT("Double click this icon when editing is finished..."), ballonMsg.c_str(), 1);

		// message loop for tray icon:
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// on program exit.
	CloseHandle(oProcessInfo.hProcess);
	CloseHandle(oProcessInfo.hThread);
	return 0;
}
