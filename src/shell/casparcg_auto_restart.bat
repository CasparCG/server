@Echo off

IF EXIST scanner.exe (
    start scanner.exe
)

:Start
SET ERRORLEVEL 0

casparcg.exe

if ERRORLEVEL 5 goto :Start
