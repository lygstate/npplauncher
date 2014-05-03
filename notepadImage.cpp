//----------------------------------------------------------------------------
// Description: notepadImage replaces the original wondows notepad 
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
#include <sstream>

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

#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#define CAPTION_POST TEXT(" ")
#define CAPTION_PRE TEXT("NL ")
#define CAPTION CAPTION_PRE NPPL_VERSION CAPTION_POST // must be 9 chars long to fit to current copyName()
#define WM_ICON_NOTIFY WM_APP+10
#define MAX_TOOLTIP_LEN 64
#define MAX_TITLE_LEN 100

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
			if (pcds->dwData == NPPL_ID_PROG_CLOSED) {
				LPCWSTR lpszString = (LPCWSTR)(pcds->lpData);
				DBGW1("WndProc: got message NPPL_ID_PROG_CLOSED: %s", lpszString);
				PostQuitMessage(0);
			}
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
		//NOTE: not need to exit in NPPL_ID_PROG_CLOSED at the current time: PostQuitMessage(0);
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
	wcex.hIcon = ::LoadIcon(hInstance, (LPCTSTR)IDI_NOTEPAD_IMAGE_ICON);
	wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_CNTXT_MENU);  // TODO: check if this is correct
	wcex.lpszClassName = szTitle;
	wcex.hIconSm = ::LoadIcon(wcex.hInstance, (LPCTSTR)IDI_NOTEPAD_IMAGE_ICON);

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
		::LoadIcon(hInstance, (LPCTSTR)IDI_NOTEPAD_IMAGE_ICON),
		IDR_CNTXT_MENU))
	{
		// Create the icon failed.
		return FALSE;
	}

	_TrayIcon.SetMenuDefaultItem(IDM_EXIT);

	return TRUE;
}

std::wstring QueryErrorString(DWORD dw) {
	// Retrieve the system error message for the last-error code

	LPWSTR lpMsgBuf = 0;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf,
		0, NULL);
	std::wstring ret = lpMsgBuf;
	LocalFree(lpMsgBuf);
	return std::move(ret);
}

bool QueryRegistryString(HKEY hKey, std::wstring key, std::wstring &value) {
	DWORD iType = REG_NONE;
	DWORD iDataSize = 0;
	LSTATUS errorCode = ::RegQueryValueExW(hKey, L"Debugger", NULL, &iType, NULL, &iDataSize);
	if (iType != REG_SZ)
	{
		value.resize(0);
		return false;
	}
	if (errorCode == ERROR_SUCCESS && iDataSize > 0) {
		if (iDataSize & 1) ++iDataSize;
		DWORD sz = iDataSize >> 1;
		value.reserve(sz + 1);
		value.resize(sz);
		value[sz] = 0;
		errorCode = ::RegQueryValueExW(hKey, L"Debugger", NULL, &iType, (LPBYTE)value.data(), &iDataSize);
	}
	if (errorCode != ERROR_SUCCESS) {
		value.resize(0);
		return false;
	}
	return true;
}

wstring FullPath(wstring const &inPath) {
	std::vector<wchar_t> p;
	p.resize(8192);
	return _wfullpath(p.data(), inPath.c_str(), p.size());
}

wstring GetModuleExecutable(HANDLE process, HMODULE module) {
	std::vector<wchar_t> p;
	p.resize(8192);
	while (true) {
		DWORD sz;
		if (process == NULL) {
			sz = GetModuleFileNameW(module, p.data(), p.size() - 1);
		}
		else {
			sz = GetModuleFileNameExW(process, module, p.data(), p.size() - 1);
		}
		p.resize(sz + 1);
		p.data()[sz] = 0;
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER || p.size() <= sz){
			p.resize(p.size() << 1);
			continue;
		}
		break;
	}
	if (p.size() == 1) {
		return L"";
	}
	return std::move(FullPath(p.data()));
}

wstring GetThisExecutable()
{
	return GetModuleExecutable(NULL, NULL);
}


wstring GetParentDir(std::wstring p) {
	wchar_t *ptr = (wchar_t*)p.c_str();
	PathRemoveFileSpecW(ptr);
	return ptr;
}

bool LaunchProcess(STARTUPINFO& si, PROCESS_INFORMATION& oProcessInfo, std::wstring cmd, bool wait);

std::wstring QueryNotepadCommand() {
	HKEY hKey;
	LSTATUS errorCode = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\notepad.exe",
		0,
		KEY_READ,
		&hKey);
	std::wstring parentDir = L"";
	if (errorCode == ERROR_SUCCESS)
	{
		std::wstring notepadImage;
		QueryRegistryString(hKey, L"Debugger", notepadImage);
		notepadImage = FullPath(notepadImage);
		std::wstring notepadImageCurrent = GetThisExecutable();
		RegCloseKey(hKey);
		parentDir = GetParentDir(notepadImageCurrent) + L"\\";
		if (notepadImage != notepadImageCurrent) {
			STARTUPINFO si;
			PROCESS_INFORMATION oProcessInfo;
			LaunchProcess(si, oProcessInfo, parentDir + L"notepadImageInstall.bat", true);
		}
	} else {
		if (bDebug) {
			std::wstring msg = L"Open registry caused error" + QueryErrorString(GetLastError());
			MessageBoxW(NULL, msg.c_str(), L"Error opening registry", MB_OK);
		}
	}
	return parentDir + L"notepad++.exe";
}

// read the parameters from registry how to behave
bool ReadOptions(std::wstring& notepadCmd, bool& bWaitForNotepadClose, /*DWORD& uWaitTime,*/ bool& bDebug)
{
	notepadCmd = QueryNotepadCommand(); // The commands path is relative to this executable, test if the executable exist, if doesn't ,then return false
	bWaitForNotepadClose = true; // Flag for waiting or note waiting for notepad++ to exit. "yes" | "no"
	bDebug = false; // default not to debug "yes" | "no"

	return true;
}

std::wstring StripFilename(std::wstring const& arg) {
	size_t pos = 0;
	size_t end = arg.size() - 1;

	while (pos <= end) {
		if (iswspace(arg[pos]) || arg[pos] == L'\"') {
			++pos;
			continue;
		}
		if (iswspace(arg[end]) || arg[end] == L'\"') {
			--end;
			continue;
		}
		break;
	}
	return std::move(arg.substr(pos, end + 1 - pos));
}

std::wstring GetFilenameParameter(wstring const& arguments) {
	// different calling conventions
	static wstring const notepadNames[] = {
		L"\\NOTEPADIMAGE.EXE",
		L"\\NOTEPADIMAGE",
		L"\\NOTEPAD.EXE",
		L"\\NOTEPAD",
	};

	std::wstring filename = L"";

	size_t i = 0;
	while (i <arguments.size()) {
		if (iswspace(arguments[i])) {
			++i;
			continue;
		}
		size_t j = i;
		if (arguments[i] == L'\"') {
			++j;
			while (j < arguments.size()) {
				++j;
				if (arguments[j - 1] != L'\"') {
					continue;
				}
				if (j >= arguments.size()) {
					break;
				}
				if (iswspace(arguments[j])) {
					break;
				}
				++j;
			}
			++j;
		}
		else
		{
			while (j < arguments.size() && !iswspace(arguments[j])) {
				++j;
			}
		}
		std::wstring arg = StripFilename(arguments.substr(i, j - i));
		std::wstring upperApp = FullPath(arg);
		for (size_t k = 0; k < upperApp.size(); ++k) {
			upperApp[k] = towupper(upperApp[k]);
		}
		size_t idx = std::wstring::npos;
		for (int k = 0; k < length_of(notepadNames); ++k) {
			idx = upperApp.find(notepadNames[k]);
			if (idx != std::wstring::npos) {
				std::wstring notepad = notepadNames[k];
				idx += notepad.size();
				if (idx == upperApp.size())
					break;
				idx = std::wstring::npos;
			}
		}

		if (idx == std::wstring::npos ) {
			break;
		}
		i = j;
	}

	if (i < arguments.size()) {
		filename = FullPath(StripFilename(arguments.substr(i)));
	} else {
		return std::move(filename);
	}

	if (*filename.rbegin() == '\\') filename.resize(filename.size() - 1);

	struct _stat st;
	memset(&st, 0, sizeof(st));
	int ret = _wstat(filename.c_str(), &st);

	if (!ret && (st.st_mode & _S_IFDIR) != 0) { //If this file is a directory, try to append \.txt to it.
		if (bDebug) {
			MessageBoxW(NULL, arguments.c_str(), L"notepadImage is opening the directory as txt.", MB_OK);
		}
		filename += L"\\.txt";
	}

	return std::move(filename);
}

// create from lpszArgument a valid arguments list for program to be called 
// this is especially to remove the notepad.exe name from the line
// cmd: the final running cmd
// filename: the file that notepad will open
bool CreateCommandLine(std::wstring& cmd, std::wstring& filename, boolean const& bDebug, wstring const& arguments)
{
	std::wstring upperApp;

	if (bDebug) {
		if (IDCANCEL == MessageBoxW(NULL, arguments.c_str(), L"notepadImage initial command line", MB_OKCANCEL))
			return false;
	}

	filename = GetFilenameParameter(GetCommandLineW());

	if (filename.size() > 0) {
		cmd = cmd + L" \"" + filename + L"\"";
	}
	return true;
}

bool LaunchProcess(STARTUPINFO& si, PROCESS_INFORMATION& oProcessInfo, std::wstring cmd, bool wait) {
	// preparation the notepad++ process launch information.
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&oProcessInfo, 0, sizeof(oProcessInfo));
	// launch the Notepad++
	std::vector<wchar_t> cmdStr(cmd.c_str(), cmd.c_str() + cmd.size() + 1);
	if (CreateProcessW(NULL, cmdStr.data(), NULL, NULL, false, 0, NULL, NULL, &si, &oProcessInfo) == FALSE) {
		CloseHandle(oProcessInfo.hProcess);
		CloseHandle(oProcessInfo.hThread);
		return false;
	}
	if (wait) {
		// Wait until child process exits.
		WaitForSingleObject(oProcessInfo.hProcess, INFINITE);
		CloseHandle(oProcessInfo.hProcess);
		CloseHandle(oProcessInfo.hThread);
	}
	return true;
}


HANDLE FoundProcessHandle(std::wstring const & processExecutable)
{
	std::vector<DWORD> processList;
	processList.resize(16);
	DWORD dw = 0;
	while (true) {
		DWORD pBytesReturned;
		if (EnumProcesses(processList.data(), processList.size() * sizeof(DWORD), &pBytesReturned) == FALSE) {
			return 0;
		}
		DWORD count = pBytesReturned / sizeof(DWORD);
		if (count < processList.size()) {
			processList.resize(count);
			break;
		}
		processList.resize(processList.size() << 1);
	}
	for (size_t i = 0; i < processList.size() && dw == 0; ++i) {
		HANDLE hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE,
			processList[i]);
		HMODULE hMod;
		DWORD cbNeeded;
		if (hProcess == NULL) {
			continue;
		}
		if (EnumProcessModules(
			hProcess,
			&hMod,
			sizeof(hMod),
			&cbNeeded))
		{
			std::wstring currentFilepath = GetModuleExecutable(hProcess, hMod);
			if (currentFilepath == processExecutable) {
				return hProcess;
			}
		}
	}
	return NULL;
}


BOOL IsProcessRunning(HANDLE hProcess)
{
	DWORD ret = WaitForSingleObject(hProcess, 0);
	return ret == WAIT_TIMEOUT;
}

// main 
int WINAPI wWinMain(
	HINSTANCE hThisInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nShowCmd
	)
{
	std::wstring commandline;
	std::wstring cmd;
	bool bWaitForNotepadClose = true;

	MyRegisterClass(hThisInstance);

	if (!ReadOptions(commandline, bWaitForNotepadClose, bDebug))
	{
		return -1; // error case
	}

	cmd = commandline;
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
	if (filename.size() == 0) { // No need to wait for newly created files.
		bWaitForNotepadClose = false;
	}
	if (bWaitForNotepadClose) {
		// Wait until child process to exit.

		std::wstring ballonMsg = std::wstring(CAPTION) + L":" + filename;
		_TrayIcon.ShowBalloon(TEXT("Double click this icon when editing is finished..."), ballonMsg.c_str(), 1);
	}
	//MessageBoxW(NULL, cmd.c_str(), L"notepadImage initial command line", MB_OK);
	STARTUPINFO si;
	PROCESS_INFORMATION oProcessInfo;
	if (!LaunchProcess(si, oProcessInfo, cmd, false)) {
		DWORD dw = GetLastError();
		std::wstring errorMsg = QueryErrorString(dw);
		std::wstringstream msg;
		msg << L"CreateProcess() failed with error "
			<< dw << L": " << errorMsg << L"\nPlease consider checking the configurations in notepadImage.ini";
		MessageBoxW(NULL, msg.str().c_str(), TEXT("notepadImage Error"), MB_OK);
	} else if (bWaitForNotepadClose) {
		// message loop for tray icon:
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		CloseHandle(oProcessInfo.hProcess);
		CloseHandle(oProcessInfo.hThread);
	}
	return 0;
}
