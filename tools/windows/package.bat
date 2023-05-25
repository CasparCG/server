if exist "%SERVER_FOLDER%" rmdir "%SERVER_FOLDER%" /s /q

xcopy shell\Release "%SERVER_FOLDER%" /E /I /Y

@REM del "%SERVER_FOLDER%\*_debug.dll"
@REM del "%SERVER_FOLDER%\*-d-2.dll"

copy %1\src\shell\casparcg_auto_restart.bat "%SERVER_FOLDER%\"
xcopy %1\resources\windows\flash-template-host-files "%SERVER_FOLDER%\" /E /I /Y

echo Copying documentation...
copy %1\CHANGELOG.md "%SERVER_FOLDER%"
copy %1\LICENSE.md "%SERVER_FOLDER%"
copy %1\README.md "%SERVER_FOLDER%"