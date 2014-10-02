@echo off

:: Fail early if environment not set
if not defined BUILD_VCVARSALL exit /b 1
if not defined BUILD_ARCHIVE_NAME exit /b 1
if not defined BUILD_PARALLEL_THREADS exit /b 1
if not defined BUILD_7ZIP exit /b 1

:: Clean and enter shadow build folder
echo Cleaning...
cd .. || goto :error
if exist bin rmdir bin /s /q || goto :error
if exist ipch rmdir ipch /s /q || goto :error
if exist tmp rmdir tmp /s /q || goto :error
if exist build rmdir build /s /q || goto :error
mkdir build || goto :error

:: Unpack archived dependencies
echo Unpacking archived dependencies...
"%BUILD_7ZIP%" x -y -odependencies dependencies\cef.7z || goto :error

:: Setup VC++ environment
echo Setting up VC++...
call "%BUILD_VCVARSALL%" x86 || goto :error

:: Build with MSBuild
echo Building...
msbuild /t:Clean /p:Configuration=Release || goto :error
msbuild /p:Configuration=Release /m:%BUILD_PARALLEL_THREADS% || goto :error

:: Create server folder to later zip
cd build || goto :error
set SERVER_FOLDER=CasparCG Server
if exist "%SERVER_FOLDER%" rmdir "%SERVER_FOLDER%" /s /q || goto :error
mkdir "%SERVER_FOLDER%" || goto :error

:: Copy media files
echo Copying media...
xcopy ..\deploy\Server "%SERVER_FOLDER%\Server" /E /I /Y || goto :error
xcopy ..\deploy\Wallpapers "%SERVER_FOLDER%\Wallpapers" /E /I /Y || goto :error
copy ..\deploy\CasparCG_Server_2.0-brochure.pdf "%SERVER_FOLDER%" || goto :error

:: Copy binaries
echo Copying binaries...
copy ..\bin\Release\* "%SERVER_FOLDER%\Server" || goto :error
xcopy ..\bin\Release\locales "%SERVER_FOLDER%\Server\locales" /E /I /Y || goto :error

:: Copy documentation
echo Copying documentation...
copy ..\CHANGES.txt "%SERVER_FOLDER%" || goto :error
copy ..\LICENSE.txt "%SERVER_FOLDER%" || goto :error
copy ..\README.txt "%SERVER_FOLDER%" || goto :error

:: Create zip file
echo Creating zip...
"%BUILD_7ZIP%" a "%BUILD_ARCHIVE_NAME%.zip" "%SERVER_FOLDER%" || goto :error

:: Skip exiting with failure
goto :EOF

:error
exit /b %errorlevel%
