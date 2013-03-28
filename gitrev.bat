@ECHO OFF
for /f "tokens=*" %%a in ('git rev-parse --verify --short HEAD') do (
    set TEMPRESPONSE=%%a
)
COPY "%~dp0\version.tmpl" "%~dp0\version.h" /Y
ECHO #define CASPAR_REV "%TEMPRESPONSE%" >> "%~dp0\version.h"
ECHO gitrev.bat: %TEMPRESPONSE%
SET TEMPRESPONSE=
exit /b 0