; ─────────────────────────────────────────────────────────────────────────────
; Tech Aim 0.9.0-RC3 — SETA Evaluation installer.
;
; Driven by tools\release\build-rc3-seta-package.ps1, which stages an
; allow-listed payload and audits it before this file is compiled. The payload
; directory is passed in as StageDir; nothing is read from the repository, so
; the installer cannot pick up source, tests or development data.
;
; AppId is a fixed GUID so later Tech Aim versions upgrade and uninstall
; cleanly over this one. It must not change between releases.
; ─────────────────────────────────────────────────────────────────────────────

#ifndef StageDir
  #define StageDir "..\..\dist\rc3\staging"
#endif
#ifndef OutDir
  #define OutDir "..\..\dist\rc3"
#endif
#ifndef IconFile
  #define IconFile "..\..\images\logo\techaim.ico"
#endif
#ifndef AppVer
  #define AppVer "0.9.0-RC3a"
#endif

[Setup]
AppId={{9F3C7A21-4D8E-4B62-9E5A-2C6D1B0F7A34}
AppName=Tech Aim
AppVersion={#AppVer}
AppVerName=Tech Aim {#AppVer} — SETA Evaluation
VersionInfoVersion=0.9.0.4
VersionInfoProductName=Tech Aim
VersionInfoDescription=Tech Aim Electronic Target Control — SETA Evaluation
AppPublisher=JAC SHOOTING SOLUTIONS (PTY) LTD
AppCopyright=Copyright (C) JAC SHOOTING SOLUTIONS (PTY) LTD
DefaultDirName={autopf}\Tech Aim
DefaultGroupName=Tech Aim
DisableProgramGroupPage=yes
OutputDir={#OutDir}
OutputBaseFilename=TechAim-0.9.0-RC3a-SETA-Evaluation-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; The installer's own icon, and the icon Add/Remove Programs shows.
SetupIconFile={#IconFile}
UninstallDisplayIcon={app}\TechAim.exe
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=admin
UninstallDisplayName=Tech Aim {#AppVer} — SETA Evaluation
; Shown on the welcome page so the evaluation status is stated before install.
AppComments=Evaluation Build — Not for Official Competition Results

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
; The whole staged payload, which has already passed the source-leakage,
; proprietary-QML, legacy-brand and clean-first-run gates.
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Tech Aim {#AppVer} (SETA Evaluation)"; Filename: "{app}\TechAim.exe"; WorkingDir: "{app}"; IconFilename: "{app}\TechAim.exe"; IconIndex: 0
Name: "{group}\Operator manuals"; Filename: "{app}\docs\manuals"
Name: "{group}\Uninstall Tech Aim"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Tech Aim {#AppVer}"; Filename: "{app}\TechAim.exe"; WorkingDir: "{app}"; IconFilename: "{app}\TechAim.exe"; IconIndex: 0; Tasks: desktopicon

[Run]
Filename: "{app}\TechAim.exe"; Description: "Launch Tech Aim"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[Messages]
WelcomeLabel2=This installs Tech Aim {#AppVer} for SETA evaluation.%n%nEvaluation Build — Not for Official Competition Results.
