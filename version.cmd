cd /d %1
copy /Y "versionInfo.tags" "versionInfo.inc"
if "%SVNDIR%" equ "" (
set SVNDIR=%ProgramW6432%\TortoiseSVN\bin
)
rem "%SVNDIR%\SubWCRev.exe" "." "versionInfo.tags" "versionInfo.inc"