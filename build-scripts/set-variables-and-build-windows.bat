@echo off

set BUILD_VCVARSALL=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat
set BUILD_ARCHIVE_NAME=CasparCG Server
set BUILD_7ZIP=C:\Program Files\7-Zip\7z.exe

:: Uncomment if using Visual Studio Express. And install WDK 7.1.0
::set BUILD_ATL_INCLUDE_PATH=BUILD_ATL_INCLUDE_PATH

build-windows.bat

