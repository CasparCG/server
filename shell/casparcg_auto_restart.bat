@Echo off
:Start
SET ERRORLEVEL 0

casparcg.exe

if ERRORLEVEL 5 goto :Start
