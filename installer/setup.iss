[_ISTool]
EnableISX=true

#define MyAppName "CasparCG"
#define MyAppVersion "0.0.0"
#define MyAppPublisher "Sveriges Television AB"
#define MyAppURL "http://casparcg.com"
#define MyAppExeName "casparcg.exe"

#define BuildOutput "..\bin\Release"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{9EBE28B9-87AE-4FB9-ADE9-3BBC9739C2B2}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={userappdata}\{#MyAppName}
DisableDirPage=yes
DisableReadyPage=no
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=license.rtf
OutputBaseFilename=CasparCGSetup
Compression=lzma
SolidCompression=yes
WizardImageFile=compiler:WIZMODERNIMAGE-IS.BMP 
CreateUninstallRegKey=true

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Dirs]
Name: {app}\data
Name: {app}\log
Name: {app}\media
Name: {app}\templates

[Files]
Source: ..\installer\isxdl.dll; Flags: dontcopy
Source: {#BuildOutput}\casparcg.exe; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\avcodec-54.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\avdevice-54.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\avfilter-2.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\avformat-54.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\avutil-51.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\tbb_preview_debug.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\tbbmalloc.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\tbbmalloc_debug.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\tbbmalloc_proxy.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\tbbmalloc_proxy_debug.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\zlib1.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\FreeImage.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\glew32.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\glew32mx.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\libsndfile-1.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\openal32.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\postproc-52.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\sfml-audio.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\sfml-audio-d.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\sfml-graphics.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\sfml-graphics-d.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\sfml-system.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\sfml-system-d.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\sfml-window.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\sfml-window-d.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\swresample-0.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\swscale-2.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\tbb.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\tbb_debug.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\tbb_preview.dll; DestDir: {app}; Flags: replacesameversion; 
Source: {#BuildOutput}\casparcg.config; DestDir: {app}; 

[Messages]
WinVersionTooLowError=CasparCG requires Windows 2000 or later.

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, "&", "&&")}}"; Flags: nowait postinstall skipifsilent

[InnoIDE_Settings]
LogFileOverwrite=false
