; Windows installer for Phasemap, built with Inno Setup 6 (https://jrsoftware.org/isinfo.php)
;
;   iscc /DAppVersion=1.0.0 /DVersionLabel="Beta 1.0" /DArtefacts=..\..\build\Phasemap_artefacts\Release packaging\windows\Phasemap.iss
;
; VST3 and CLAP go to the standard Common Files folders, the standalone app to Program Files.

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef VersionLabel
  #define VersionLabel AppVersion
#endif
#ifndef Artefacts
  #define Artefacts "..\..\build\Phasemap_artefacts\Release"
#endif
#ifndef OutputName
  #define OutputName "Phasemap-Windows-Setup"
#endif

[Setup]
AppId={{7B1E6C2A-4F3D-4E8B-9A51-6D2C0F8E3B47}
AppName=Phasemap
AppVersion={#AppVersion}
AppVerName=Phasemap {#VersionLabel}
AppPublisher=Nullzone Audio
AppPublisherURL=https://nullzone-audio.org
AppSupportURL=https://github.com/Nullzone-Audio/phasemap/issues
DefaultDirName={autopf}\Nullzone Audio\Phasemap
DefaultGroupName=Nullzone Audio
DisableProgramGroupPage=yes
DisableDirPage=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..
OutputBaseFilename={#OutputName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Phasemap

[Types]
Name: "full";   Description: "All formats"
Name: "custom"; Description: "Choose formats"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3 plugin (Cubase, Ableton Live, Studio One, FL Studio, ...)"; Types: full custom
Name: "clap";       Description: "CLAP plugin (Bitwig, Reaper, ...)";                             Types: full custom
Name: "standalone"; Description: "Standalone app";                                                Types: full

[Files]
Source: "{#Artefacts}\VST3\Phasemap.vst3\*";       DestDir: "{commoncf64}\VST3\Phasemap.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\CLAP\Phasemap.clap";         DestDir: "{commoncf64}\CLAP";               Components: clap; Flags: ignoreversion
Source: "{#Artefacts}\Standalone\Phasemap.exe";    DestDir: "{app}";                           Components: standalone; Flags: ignoreversion
Source: "..\..\LICENSE";                           DestDir: "{app}";                           Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Phasemap"; Filename: "{app}\Phasemap.exe"; Components: standalone

