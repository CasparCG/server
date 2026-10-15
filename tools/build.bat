@echo off
setlocal

pushd "%~dp0.." || exit /b 1
call tools\windows\build.bat %*
set "BUILD_EXIT_CODE=%ERRORLEVEL%"
popd
exit /b %BUILD_EXIT_CODE%