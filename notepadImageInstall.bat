::����Unicode��֧�������������ļ�����UnicodeĿ¼��ִ��

cd /d %~dp0

if defined ENABLE_ADMIN goto run_as_admin
set ENABLE_ADMIN=""
call request-admin.bat %~dpnx0
goto :finished

:run_as_admin
set WD=%CD%
reg delete "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe" /f 
reg add "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe" /f /v "Debugger" /t REG_SZ /d "%WD%\notepadImage.exe"
reg add "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe" /f /v "readme" /t REG_SZ /d "call notepadImage.exe instead of original notepad.exe! To disable this option just remove notepad.exe entry"

:finished
