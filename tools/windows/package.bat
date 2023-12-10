if exist "%SERVER_FOLDER%" rmdir "%SERVER_FOLDER%" /s /q

xcopy shell\Release "%SERVER_FOLDER%" /E /I /Y
xcopy flashtemplatehost-prefix\src\flashtemplatehost "%SERVER_FOLDER%\" /E /I /Y

copy %1\src\shell\casparcg_auto_restart.bat "%SERVER_FOLDER%\"

echo Copying documentation...
copy %1\CHANGELOG.md "%SERVER_FOLDER%"
copy %1\LICENSE.md "%SERVER_FOLDER%"
copy %1\README.md "%SERVER_FOLDER%"

if exist "%MEDIA_SCANNER_FOLDER%" xcopy "%MEDIA_SCANNER_FOLDER%" "%SERVER_FOLDER%" /E /I /Y