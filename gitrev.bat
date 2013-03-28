for /f "tokens=*" %%a in ('git describe --always') do (
    set TEMPRESPONSE=%%a
)
ECHO #define CASPAR_REV "%TEMPRESPONSE%" > %~dp0\version.h
SET TEMPRESPONSE=
ECHO GITREV.bat
exit /b 0