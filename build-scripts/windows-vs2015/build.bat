@echo off

:: Fail early if environment not set
if not defined BUILD_VCVARSALL exit /b 1
if not defined BUILD_ARCHIVE_NAME exit /b 1
if not defined BUILD_PARALLEL_THREADS exit /b 1
if not defined BUILD_7ZIP exit /b 1

:: Clean and enter shadow build folder
echo Cleaning...
cd .. || goto :error
if exist build rmdir build /s /q || goto :error
mkdir build || goto :error

:: Unpack archived dependencies
echo Unpacking archived dependencies...
"%BUILD_7ZIP%" x -y -odependencies64 dependencies64\large_files_win32.7z || goto :error

:: Setup VC++ environment
echo Setting up VC++...
call "%BUILD_VCVARSALL%" amd64 || goto :error

:: Run cmake
cd build || goto :error
cmake -G "Visual Studio 14 2015" -A x64 .. || goto :error

:: Build with MSBuild
echo Building...
msbuild "CasparCG Server.sln" /t:Clean /p:Configuration=RelWithDebInfo || goto :error
msbuild "CasparCG Server.sln" /p:Configuration=RelWithDebInfo /m:%BUILD_PARALLEL_THREADS% || goto :error

:: Create server folder to later zip
set SERVER_FOLDER=CasparCG Server
if exist "%SERVER_FOLDER%" rmdir "%SERVER_FOLDER%" /s /q || goto :error
mkdir "%SERVER_FOLDER%" || goto :error

:: Copy media files
echo Copying media...
xcopy ..\resources\windows\flash-template-host-files\cg20* "%SERVER_FOLDER%\server" /E /I /Y || goto :error

:: Copy binaries
echo Copying binaries...
copy shell\*.dll "%SERVER_FOLDER%\server" || goto :error
copy shell\RelWithDebInfo\casparcg.exe "%SERVER_FOLDER%\server" || goto :error
copy shell\RelWithDebInfo\casparcg.pdb "%SERVER_FOLDER%\server" || goto :error
copy ..\shell\casparcg_auto_restart.bat "%SERVER_FOLDER%\server" || goto :error
copy shell\casparcg.config "%SERVER_FOLDER%\server" || goto :error
copy shell\*.ttf "%SERVER_FOLDER%\server" || goto :error
copy shell\*.pak "%SERVER_FOLDER%\server" || goto :error
copy shell\*.bin "%SERVER_FOLDER%\server" || goto :error
copy shell\*.dat "%SERVER_FOLDER%\server" || goto :error
xcopy shell\locales "%SERVER_FOLDER%\server\locales" /E /I /Y || goto :error
xcopy shell\swiftshader "%SERVER_FOLDER%\server\swiftshader" /E /I /Y || goto :error

:: Copy documentation
echo Copying documentation...
copy ..\CHANGELOG "%SERVER_FOLDER%" || goto :error
copy ..\LICENSE "%SERVER_FOLDER%" || goto :error
copy ..\README "%SERVER_FOLDER%" || goto :error

:: Create zip file
echo Creating zip...
"%BUILD_7ZIP%" a "%BUILD_ARCHIVE_NAME%.zip" "%SERVER_FOLDER%" || goto :error

:: Skip exiting with failure
goto :EOF

:error
exit /b %errorlevel%
