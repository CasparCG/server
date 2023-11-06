@echo off

set BUILD_ARCHIVE_NAME=casparcg_server
set BUILD_VCVARSALL=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat
set BUILD_7ZIP=C:\Program Files\7-Zip\7z.exe

@REM Github Actions has Enterprise available
if DEFINED CI set BUILD_VCVARSALL=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat

:: Clean and enter shadow build folder
echo Cleaning...
if exist dist rmdir dist /s /q || goto :error
mkdir dist || goto :error

:: Setup VC++ environment
echo Setting up VC++...
call "%BUILD_VCVARSALL%" amd64 || goto :error

:: Run cmake
cd dist || goto :error
cmake -G "Visual Studio 16 2019" -A x64 ..\src || goto :error

:: Build with MSBuild
echo Building...
msbuild "CasparCG Server.sln" /t:Clean /p:Configuration=Release || goto :error
msbuild "CasparCG Server.sln" /p:Configuration=Release /m:%NUMBER_OF_PROCESSORS% || goto :error

:: Create server folder to later zip
set SERVER_FOLDER=casparcg_server
call ..\tools\windows\package.bat ..

:: Create zip file
echo Creating zip...
if exist "%BUILD_ARCHIVE_NAME%.zip" unlink "%BUILD_ARCHIVE_NAME%.zip" || goto :error
"%BUILD_7ZIP%" a "%BUILD_ARCHIVE_NAME%.zip" ".\%SERVER_FOLDER%\*" || goto :error

:: Skip exiting with failure
goto :EOF

:error
exit /b %errorlevel%
