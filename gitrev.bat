@ECHO OFF
for /f "tokens=*" %%a in ('git describe --always') do (
    set TEMPRESPONSE=%%a
)
ECHO #define CASPAR_REV "%TEMPRESPONSE%" > %~dp0\version.h
ECHO gitrev.bat - version.h: %TEMPRESPONSE%
SET TEMPRESPONSE=
exit /b 0