::测试Unicode的支持情况，将这个文件放在Unicode目录下执行
reg delete /f "HKLM\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe"
cd %~dp0
set WD=%CD%
reg add "HKLM\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe" /v "Debugger" /t REG_SZ /d "%WD%\notepadImage.exe"
reg add "HKLM\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe" /v "readme" /t REG_SZ /d "call notepadImage.exe instead of original notepad.exe! To disable this option just remove notepad.exe entry"
pause