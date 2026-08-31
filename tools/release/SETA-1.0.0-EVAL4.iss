; ─────────────────────────────────────────────────────────────────────────────
; SETA Electronic Target Control 1.0.0-EVAL4 — physical-evaluation installer.
;
; Payload is the FROZEN EVAL4 package, extracted to
;   dist\v1.0.0-eval4\installer-staging
; and audited there. Nothing is read from the repository, so the installer
; cannot pick up source, tests or build artefacts. SETA.exe in the staging
; directory is byte-identical to the one in the frozen zip
; (SHA-256 E310B00A…B5BD59); the only differences from the zip are the two
; internal development documents, which are removed, and one stale provenance
; line in the deployment manifest, which is corrected.
;
; ── INSTALL LOCATION: per-user, and why ─────────────────────────────────────
;
; SETA shares Tech Aim's persistence behaviour, and it was audited before this
; script was written rather than assumed:
;
;   appsettings.cpp  saveMatch()          QFile("Match_<stamp>.tch")  RELATIVE
;   appsettings.cpp  AppSettings("config.ini")  resolved against the working dir
;   tachuswidget.cpp saveNameAndPort()    applicationDirPath()/USER_DETAILS
;
; Under C:\Program Files a standard user cannot write to any of them, and
; saveMatch() does not report the failure - open() fails and the guarded block
; is skipped - so a completed match would be SILENTLY not saved. On an
; evaluation build whose entire purpose is to have matches shot on it, that is
; the worst possible failure.
;
; PrivilegesRequired=lowest therefore resolves {autopf} to
; %LOCALAPPDATA%\Programs\SETA. Consequences, all deliberate: no UAC prompt, no
; administrator rights (range laptops are often locked down), and .tch match
; records, config.ini and the remembered COM port all remain writable.
;
; Session journals, reports and logs are unaffected either way - the reliability
; layer stores them under QStandardPaths::AppLocalDataLocation, which for this
; build is %LOCALAPPDATA%\TechAim\TechAimSETA and is per-user already. That is
; also the root the support collector scopes itself to.
;
; ── AppId ───────────────────────────────────────────────────────────────────
; SETA's own fixed GUID. It is deliberately NOT the Tech Aim 1.0.0 installer's
; and NOT the older SETA-evaluation installer's, so SETA and Tech Aim remain
; separate installable Windows products that can coexist on one machine and
; uninstall independently. It must not change between SETA releases.
; ─────────────────────────────────────────────────────────────────────────────

#ifndef StageDir
  #define StageDir "..\..\dist\v1.0.0-eval4\installer-staging"
#endif
#ifndef OutDir
  #define OutDir "..\..\dist\v1.0.0-eval4"
#endif
#ifndef IconFile
  #define IconFile "..\..\images\logo\seta.ico"
#endif
#ifndef AppVer
  #define AppVer "1.0.0-EVAL4"
#endif

[Setup]
AppId={{34ED89FC-93EA-4E3D-A4AF-FBAC4A5C53AF}
AppName=SETA Electronic Target Control
AppVersion={#AppVer}
AppVerName=SETA Electronic Target Control {#AppVer}
VersionInfoVersion=1.0.0.0
VersionInfoProductName=SETA Electronic Target Control
VersionInfoDescription=SETA Electronic Target Control Setup
VersionInfoCompany=JAC SHOOTING SOLUTIONS (PTY) LTD
AppPublisher=JAC SHOOTING SOLUTIONS (PTY) LTD
AppCopyright=Copyright (C) JAC SHOOTING SOLUTIONS (PTY) LTD
DefaultDirName={autopf}\SETA
DefaultGroupName=SETA
DisableProgramGroupPage=yes
OutputDir={#OutDir}
OutputBaseFilename=SETA-1.0.0-EVAL4-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
SetupIconFile={#IconFile}
UninstallDisplayIcon={app}\SETA.exe
UninstallDisplayName=SETA Electronic Target Control {#AppVer}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; See the header: the application must be able to write beside its own
; executable, so it installs per-user and never needs elevation.
PrivilegesRequired=lowest
; If SETA is running during a reinstall, ask it to close rather than failing on
; a locked Qt DLL. The application takes a single-instance lock, so a stale
; process would otherwise block the install.
CloseApplications=yes
RestartApplications=no
; Stated before anything is installed. This build exists to be evaluated, and a
; tester must not be able to mistake it for a production release.
AppComments=Evaluation Build - Not for Official Competition Results

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
; The whole audited staging payload - executable, Qt runtime, plugins, Qt QML
; modules, deployment manifest and the product-scoped support tooling. Nothing
; is filtered here: the staging directory is audited before this file is
; compiled, and omitting framework files to save space is exactly what breaks
; an installation on a PC that has never had Qt.
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\SETA Electronic Target Control"; Filename: "{app}\SETA.exe"; WorkingDir: "{app}"; IconFilename: "{app}\SETA.exe"; IconIndex: 0
Name: "{group}\Collect support logs"; Filename: "{app}\Collect-Logs.cmd"; WorkingDir: "{app}"
Name: "{group}\Uninstall SETA"; Filename: "{uninstallexe}"
Name: "{autodesktop}\SETA"; Filename: "{app}\SETA.exe"; WorkingDir: "{app}"; IconFilename: "{app}\SETA.exe"; IconIndex: 0; Tasks: desktopicon

[Run]
Filename: "{app}\SETA.exe"; Description: "Launch SETA"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Only the directory Setup created, and only if it is empty. Inno removes just
; the files it installed, so a .tch match record written after installation
; survives an uninstall - as does everything under
; %LOCALAPPDATA%\TechAim\TechAimSETA. A tester's evidence must not be destroyed
; by removing the software that produced it.
Type: dirifempty; Name: "{app}"

[Messages]
WelcomeLabel2=This installs SETA Electronic Target Control {#AppVer} for physical evaluation.%n%nEvaluation Build - Not for Official Competition Results.
