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
cmake -G "Visual Studio 12 2013" -A x64 .. || goto :error

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
xcopy ..\deploy\general\Server "%SERVER_FOLDER%\Server" /E /I /Y || goto :error
xcopy ..\deploy\general\Wallpapers "%SERVER_FOLDER%\Wallpapers" /E /I /Y || goto :error
copy ..\deploy\general\CasparCG_Server_2.0-brochure.pdf "%SERVER_FOLDER%" || goto :error

:: Copy binaries
echo Copying binaries...
copy shell\*.dll "%SERVER_FOLDER%\Server" || goto :error
copy shell\RelWithDebInfo\casparcg.exe "%SERVER_FOLDER%\Server" || goto :error
copy shell\RelWithDebInfo\casparcg.pdb "%SERVER_FOLDER%\Server" || goto :error
copy shell\casparcg.config "%SERVER_FOLDER%\Server" || goto :error
copy shell\*.ttf "%SERVER_FOLDER%\Server" || goto :error
xcopy shell\locales "%SERVER_FOLDER%\Server\locales" /E /I /Y || goto :error

:: Copy documentation
echo Copying documentation...
copy ..\CHANGES.txt "%SERVER_FOLDER%" || goto :error
copy ..\LICENSE.txt "%SERVER_FOLDER%" || goto :error
copy ..\README.txt "%SERVER_FOLDER%" || goto :error

:: Create zip file
echo Creating zip...
"%BUILD_7ZIP%" a -x!"%SERVER_FOLDER%\Server\casparcg.pdb" "%BUILD_ARCHIVE_NAME%.zip" "%SERVER_FOLDER%" || goto :error
"%BUILD_7ZIP%" a "%BUILD_ARCHIVE_NAME%_debug_symbols.zip" "%SERVER_FOLDER%\Server\casparcg.pdb" || goto :error

:: Skip exiting with failure
goto :EOF

:error
exit /b %errorlevel%
