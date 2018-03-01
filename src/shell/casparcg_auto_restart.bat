@Echo off

IF EXIST scanner.exe (
    IF EXIST leveldown.node (
        start scanner.exe
    )
)

:Start
SET ERRORLEVEL 0

casparcg.exe

if ERRORLEVEL 5 goto :Start
