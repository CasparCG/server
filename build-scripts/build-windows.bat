@echo on

:: Fail early if environment not set
if not defined BUILD_VCVARSALL exit /b 1
if not defined BUILD_ARCHIVE_NAME exit /b 1
if not defined BUILD_7ZIP exit /b 1

:: Clean and enter shadow build folder
echo Cleaning...
cd .. || goto :error
if exist bin rmdir bin /s /q || goto :error
if exist ipch rmdir ipch /s /q || goto :error
if exist tmp rmdir tmp /s /q || goto :error

:: Unpack archived dependencies
echo Unpacking archived dependencies...
"%BUILD_7ZIP%" x -y -odependencies dependencies\cef.7z || goto :error

:: Setup VC++ environment
echo Setting up VC++...
call "%BUILD_VCVARSALL%" x86 || goto :error

:: Build with MSBuild
echo Building...
msbuild /t:Clean /p:Configuration=Release || goto :error
msbuild /p:Configuration=Release || goto :error

:: Skip exiting with failure
goto :EOF

:error
exit /b %errorlevel%
