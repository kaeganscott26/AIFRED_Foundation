#define AppName "AIFR3D"
#define AppVersion "1.0.0-beta"
#define AppPublisher "AIFR3D"
#define AppExeName "AIFR3D_Installer_v1.0.0-beta.exe"

[Setup]
AppId={{C6EAB70B-2BD5-4D56-BBB8-AAF3D7B61007}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={commoncf}\VST3
DefaultGroupName={#AppName}
OutputDir=dist\windows\out
OutputBaseFilename=AIFR3D_Installer_v1.0.0-beta
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
DisableDirPage=no
PrivilegesRequired=admin
UninstallDisplayIcon={app}\AIFR3D.vst3

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "dist\windows\staging\AIFR3D.vst3\*"; DestDir: "{app}\AIFR3D.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\Uninstall AIFR3D"; Filename: "{uninstallexe}"

[Run]
Filename: "{cmd}"; Parameters: "/C echo Installed to {app}\AIFR3D.vst3"; Flags: runhidden
